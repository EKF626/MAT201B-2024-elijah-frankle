#include <iostream>
#include "al/app/al_App.hpp"
#include "al/math/al_Random.hpp"
#include "al/app/al_GUIDomain.hpp"

using namespace al;

static const int numParticles = 1500;
static const int trailLength = 30;

static float pointSize = 4;

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

Vec3f sphereToCar(float t, float p) {
    float x = sin(t) * cos(p);
    float y = sin(t) * sin(p);
    float z = cos(t);
    return Vec3f(x, y, z);
}

struct Particle {
    Vec3f trail[trailLength];
    Nav nav;

    void updateTrail() { 
        for (int i = trailLength-1; i > 0; i--) {
            trail[i] = Vec3f(trail[i-1]);
        }
        trail[0] = nav.pos();
    }

    void set(Vec3f initial) {
        for (int i = 0; i < trailLength; i++) {
            trail[i] = initial;
        }
        nav.pos() = initial;
    }

    void moveTo(Vec3f coor) {
        nav.pos() = coor;
    }
};

struct MyApp : public App {
    Parameter theta{"/theta", "", 0, -M_PI, M_PI};
    Parameter phi{"/phi", "", 0.0, -M_PI, M_PI};

    Particle particles[numParticles];

    Mesh particleMesh[trailLength+1];

    void onInit() override {
        auto GUIdomain = GUIDomain::enableGUI(defaultWindowDomain());
        auto &gui = GUIdomain->newGUI();
        gui.add(theta);
        gui.add(phi);
    }

    void onCreate() override {
        for (auto& p : particles) {
            p.set(rnd::ball<Vec3f>());
        }

        nav().pos(0, 0, 4);
        nav().faceToward(0,0,0);

    }

    void onAnimate(double dt) override {
        for (Mesh& m : particleMesh) {
            m.reset();
            m.primitive(Mesh::POINTS);
        }


        float amount = 0.5 * dt;

        for (auto& p : particles) {
            p.updateTrail();

            Vec3f newPoint = rotatePoint(p.nav.pos(), theta, phi, amount);
            p.moveTo(newPoint);

            particleMesh[0].vertex(p.nav.pos());
            particleMesh[0].color(RGB(1));
            for (int i = 0; i < trailLength; i++) {
                particleMesh[i+1].vertex(p.trail[i]);
                particleMesh[i+1].color(RGB(1));
            }
        }
    }

    void onDraw(Graphics& g) override {
        g.clear();
        g.depthTesting(true);
        g.pointSize(pointSize);
        g.meshColor();
        for (int i = 0; i < trailLength; i++) {
            g.tint(1-i/(float)(trailLength-1));
            g.draw(particleMesh[i]);
        }
    }
};

int main() {
    MyApp app;
    app.start();
}
