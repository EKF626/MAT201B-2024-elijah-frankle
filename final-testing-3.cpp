#include <iostream>
#include "al/app/al_App.hpp"
#include "al/math/al_Random.hpp"

using namespace al;

static const int numParticles = 50;
static const int trailLength = 30;

struct Particle {
    Vec3f pos[trailLength];
    Nav nav;

    void update(float dt) { 
        for (int i = trailLength-1; i > 0; i--) {
            pos[i] = Vec3f(pos[i-1]);
        }
        pos[0] = nav.pos();
    }

    void set(Vec3f initial) {
        for (int i = 0; i < trailLength; i++) {
            pos[i] = initial;
        }
        nav.pos() = initial;
    }
};

struct MyApp : public App {

    Particle particles[numParticles];

    Mesh particleMesh[trailLength+1];

    float phase = 0;

    void onCreate() {
        for (auto& p : particles) {
            p.set(rnd::ball<Vec3f>());
            p.nav.faceToward(rnd::ball<Vec3f>(), 1);
        }

        nav().pos(0, 0, 4);
        nav().faceToward(0,0,0);

    }

    void onAnimate(double dt) {
        phase += 0.1*dt;

        for (Mesh& m : particleMesh) {
            m.reset();
            m.primitive(Mesh::POINTS);
        }

        for (auto& p : particles) {
            p.update(dt);
            if (dist(p.nav.pos(), Vec3f(0)) > 1) {
                p.nav.faceToward(Vec3f(0), 1);
            }

            p.nav.moveF(0.01);
            p.nav.step();

            for (int i = 0; i < trailLength; i++) {
                // particleMesh.vertex(p.pos[i]);
                // particleMesh.color(RGB(1));
                particleMesh[i+1].vertex(p.pos[i]);
                particleMesh[i+1].color(RGB(1));
            }
            particleMesh[0].vertex(p.nav.pos());
            particleMesh[0].color(RGB(1));
        }
    }

    void onDraw(Graphics& g) {
        g.clear();
        g.depthTesting(true);
        g.pointSize(8);
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
