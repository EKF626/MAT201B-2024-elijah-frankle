#include <iostream>
#include "al/app/al_App.hpp"
#include "al/math/al_Random.hpp"
#include "al/app/al_GUIDomain.hpp"
#include "al/types/al_Buffer.hpp"

#include "al/app/al_DistributedApp.hpp"
#include "al_ext/statedistribution/al_CuttleboneDomain.hpp"
#include "al_ext/statedistribution/al_CuttleboneStateSimulationDomain.hpp"

using namespace al;

static const int numParticles = 1500;
static const int trailLength = 30;
static const float baseSpeed = 0.3;
static const float speedBoost = 0.7;

Vec3f sphereToCar(float t, float p) {
    float x = sin(t) * cos(p);
    float y = sin(t) * sin(p);
    float z = cos(t);
    return Vec3f(x, y, z);
}

Vec3f rotatePoint(Vec3f point, float t, float p, float amt) {
    Vec3f axis = sphereToCar(t, p).normalized();

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
    Vec3f currentParticles[numParticles];
    Nav primaryNav;
};

struct MyApp : DistributedAppWithState<CommonState> {
    Parameter theta{"theta", "", 0, -M_PI, M_PI};
    Parameter phi{"phi", "", 0.0, -M_PI, M_PI};
    Parameter pointSize{"pointSize", "", 4.0, 1.0, 10.0};
    Parameter chaos{"chaos", "", 0.0, 0.0, 1.0};

    RingBuffer<Vec3f> particlePositions[numParticles];

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
            gui.add(chaos);
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
            float amount = (baseSpeed + speedBoost * chaos) * dt;
            for (int i = 0; i < numParticles; i++) {
                Vec3f newPoint = rotatePoint(state().currentParticles[i], theta, phi, amount);
                state().currentParticles[i] = newPoint;
            }
        }
    }

    void onDraw(Graphics& g) override {
        for (int i = 0; i < numParticles; i++) {
            particlePositions[i].write(state().currentParticles[i]);
        }

        g.clear();
        g.blending(true);
        g.blendTrans();
        g.depthTesting(true);
        g.pointSize(pointSize);
        g.meshColor();

        Mesh newMesh;
        newMesh.primitive(Mesh::POINTS);
        for (int i = 0; i < numParticles; i++) {
            for (int j = 0; j < trailLength; j++) {
                newMesh.vertex(particlePositions[i][(particlePositions[i].pos()+j)%trailLength]);
                newMesh.color(Color(1.0, j/(float)trailLength));
            }
        }

        g.draw(newMesh);
    }
};

int main() {
    MyApp app;
    app.start();
}