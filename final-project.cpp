#include <iostream>
#include "al/app/al_App.hpp"
#include "al/math/al_Random.hpp"
#include "al/app/al_GUIDomain.hpp"
#include "al/types/al_Buffer.hpp"

#include "al/app/al_DistributedApp.hpp"
#include "al_ext/statedistribution/al_CuttleboneDomain.hpp"
#include "al_ext/statedistribution/al_CuttleboneStateSimulationDomain.hpp"

#define STB_PERLIN_IMPLEMENTATION
#include "allolib/external/stb/stb/stb_perlin.h"

using namespace al;
using namespace std;

static const int numParticles = 1500;
static const int trailLength = 100;
static const float baseSpeed = 0.1;
static const float speedBoost = 0.9;
static const float rotationConst = 0.02;
static const float noiseSize = 0.1;
static const float radius = 1.0;
static const float chaosOffset = 0.015;
static const float chaosMaxOffset = 0.1;

Vec3f sphereToCar(float t, float p) {
    float x = sin(t) * cos(p);
    float y = sin(t) * sin(p);
    float z = cos(t);
    return Vec3f(x, y, z).normalized();
}

Vec3f rotatePoint(Vec3f point, float t, float p, float amt) {
    Vec3f axis = sphereToCar(t, p);

    float r1 = cos(amt) + pow(axis.x, 2) * (1 - cos(amt));
    float r2 = axis.x * axis.y * (1 - cos(amt)) - axis.z * sin(amt);
    float r3 = axis.x * axis.z * (1 - cos(amt)) + axis.y * sin(amt);
    float r4 = axis.y * axis.x * (1 - cos(amt)) + axis.z * sin(amt);
    float r5 = cos(amt) + pow(axis.y, 2) * (1 - cos(amt));
    float r6 = axis.y * axis.z * (1 - cos(amt)) - axis.x * sin(amt);
    float r7 = axis.z * axis.x * (1 - cos(amt)) - axis.y * sin(amt);
    float r8 = axis.z * axis.y * (1 - cos(amt)) + axis.x * sin(amt);
    float r9 = cos(amt) + pow(axis.z, 2) * (1 - cos(amt));

    Vec3f newPoint = Vec3f(r1 * point.x + r2 * point.y + r3 * point.z,
                           r4 * point.x + r5 * point.y + r6 * point.z,
                           r7 * point.x + r8 * point.y + r9 * point.z);

    return newPoint;
}

struct CommonState {
    Nav primaryNav;
    Vec3f currentParticles[numParticles];
    float pointSize;
};

struct MyApp : DistributedAppWithState<CommonState> {
    Parameter theta{"theta", "", 0.0, -M_PI, M_PI};
    Parameter phi{"phi", "", 0.0, -M_PI/2.0, M_PI/2.0};
    Parameter pointSize{"pointSize", "", 4.0, 1.0, 8.0};
    Parameter radius{"radius", "", 1.0, 0.1, 2.0};
    Parameter chaos{"chaos", "", 0.0, 0.0, 1.0};
    Parameter flickerSpeed{"flickerSpeed", "", 0.03, 0.0, 0.1};
    Parameter flickerIntens{"flickerIntens", "", 0.0, 0.0, 1.0};
    ParameterBool radiusByNoise{"radiusByNoise", "", 0.0};
    Parameter radiusSpeed{"radiusSpeed", "", 0.004, 0.0, 0.015};
    Parameter radiusIntens{"radiusIntens", "", 0.0, 0.0, 1.0};
    Parameter cameraWander{"cameraWander", "", 0.0, 0.0, 10.0};
    Parameter camMoveSpeed{"camMoveSpeed", "", 0.003, 0.0, 0.007};
    ParameterBool orbitCircle{"orbitCircle", "", 0.0};
    Parameter orbitRadius{"orbitRadius", "", 8.0, 5.0, 12.0};
    Parameter orbitSpeed{"orbitSpeed", "", 0.005, -0.01, 0.01};
    ParameterBool lookAtCenter{"lookAtCenter", "", 0.0};

    RingBuffer<Vec3f> particlePositions[numParticles];

    float frameFlicker = 0;
    float frameRadius = 0;
    float frameCam = 0;

    float orbitTrans = 0;
    float orbitOffset = 0;

    bool frozen = false;
    
    void onInit() override {
        auto cuttleboneDomain =
        CuttleboneStateSimulationDomain<CommonState>::enableCuttlebone(this);
        if (!cuttleboneDomain) {
            std::cerr << "ERROR: Could not start Cuttlebone. Quitting." << std::endl;
            quit();
        }
        if (isPrimary()) {
            auto guiDomain = GUIDomain::enableGUI(defaultWindowDomain());
            auto &gui = guiDomain->newGUI();
            gui.add(theta);
            gui.add(phi);
            gui.add(pointSize);
            gui.add(radius);
            gui.add(chaos);
            gui.add(flickerSpeed);
            gui.add(flickerIntens);
            gui.add(radiusByNoise);
            gui.add(radiusSpeed);
            gui.add(radiusIntens);
            gui.add(cameraWander);
            gui.add(camMoveSpeed);
            gui.add(orbitCircle);
            gui.add(orbitRadius);
            gui.add(orbitSpeed);
            gui.add(lookAtCenter);
        }
    }

    void onCreate() override {
        if (isPrimary()) {
            for (int i = 0; i < numParticles; i++) {
                Vec3f pos = rnd::ball<Vec3f>();
                state().currentParticles[i] =  pos;
            }

            state().primaryNav.pos(0, 0, 4);
            state().primaryNav.faceToward(0,0,0);
        }

        for (int i = 0; i < numParticles; i++) {
            particlePositions[i].resize(trailLength);
        }
    }

    void onAnimate(double dt) override {
        if (isPrimary()) {
            if (!frozen) { 
                float adjustedChaos = chaos*0.8+0.01;
                theta = theta + adjustedChaos*rotationConst;
                if (theta > M_PI) {theta = theta - 2.0*M_PI;}
                phi = phi + adjustedChaos*rotationConst*0.25;
                if (phi > M_PI/2.0) {phi = phi - M_PI;}

                float amount = (baseSpeed + speedBoost*chaos) * dt;
                float noiseVal = radiusIntens*stb_perlin_noise3(0, 0, frameRadius, 0, 0, 0);
                float newRadius = radius+noiseVal;

                for (int i = 0; i < numParticles; i++) {
                    Vec3f currentPoint = state().currentParticles[i];
                    Vec3f newPoint = rotatePoint(currentPoint, theta, phi, amount);
                    if (radiusByNoise) {
                        newRadius = radius + radiusIntens*stb_perlin_noise3(newPoint.x, newPoint.y, newPoint.z+frameRadius, 0, 0, 0);
                    }
                    newPoint += rnd::ball<Vec3f>() * chaos * chaosOffset;
                    float radiusUpper = newRadius*(1+chaos*chaosMaxOffset);
                    float radiusLower = newRadius*(1-chaos*chaosMaxOffset);
                    if (newPoint.mag() > radiusUpper) {
                        newPoint.mag(radiusUpper);
                    } else if (newPoint.mag() < radiusLower) {
                        newPoint.mag(radiusLower);
                    }

                    state().currentParticles[i] = newPoint;
                }
            }

            if (orbitCircle || orbitTrans > 0 ) {
                nav().pos().lerp(Vec3f(cos(orbitOffset), 0, sin(orbitOffset))*orbitRadius*orbitTrans, 0.3);
                orbitOffset += orbitSpeed*orbitTrans;
                if (orbitOffset > 2*M_PI) {
                    orbitOffset -= 2*M_PI;
                }
                if (lookAtCenter) {
                    nav().faceToward(Vec3f(0), 1);
                }
                if (orbitCircle && orbitTrans < 1) {
                    orbitTrans += 0.01;
                } else if (!orbitCircle) {
                    orbitTrans -= 0.01;
                }
            }

            if (cameraWander > 0) {
                Vec3f camTarget = Vec3f(stb_perlin_noise3(frameCam, 0, 0, 0, 0, 0), stb_perlin_noise3(0, frameCam+0.2, 0, 0, 0, 0), stb_perlin_noise3(0, 0, frameCam+0.4, 0, 0, 0));
                nav().pos() = Vec3f(0);
                nav().pos().lerp(camTarget, cameraWander);
                if (lookAtCenter) {
                    nav().faceToward(Vec3f(0), 0.05);
                }
            }

            state().primaryNav = nav();
            state().pointSize = pointSize;

            if (!frozen) {
                frameFlicker += flickerSpeed;
                frameRadius += radiusSpeed;
                frameCam += camMoveSpeed;
            }
        }
        
        if (!frozen) {
            for (int i = 0; i < numParticles; i++) {
                particlePositions[i].write(state().currentParticles[i]);
            }
        }

        if (!isPrimary()) {
            nav() = state().primaryNav;
        }
    }

    void onDraw(Graphics& g) override {
            g.clear(0.1);
            g.blending(true);
            g.blendTrans();
            g.depthTesting(true);
            g.pointSize(state().pointSize);
            g.meshColor();

            Mesh newMesh;
            newMesh.primitive(Mesh::POINTS);
            for (int i = 0; i < numParticles; i++) {
                for (int j = 0; j < trailLength; j++) {
                    int index = (particlePositions[i].pos()+j)%trailLength;
                    Vec3f pos = particlePositions[i][index];
                    newMesh.vertex(pos);
                    float noiseVal = 1-flickerIntens*(0.5+stb_perlin_noise3(pos.x, pos.y, pos.z+frameFlicker, 0, 0, 0));
                    Color c = Color(noiseVal*(0.8+chaos*0.2), noiseVal*(0.8-chaos*0.8), noiseVal*(1-chaos), j/(float)trailLength);
                    newMesh.color(c);
                }
            }

            g.draw(newMesh);
    }

    bool onKeyDown(Keyboard const& k) override {
        if (k.key() == ' ') {
            nav().pos() = Vec3f(0);
        } 
        else if (k.key() == 'f') {
            frozen = !frozen;
        }
        return true;
    }
};

int main() {
    MyApp app;
    app.start();
}