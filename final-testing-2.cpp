#include <iostream>
#include "al/app/al_App.hpp"
#include "al/math/al_Random.hpp"

using namespace al;

struct Particle {
    Vec3f pos, vel;

    void update(float dt) { 
        pos += vel*dt; 
    }
};

struct MyApp : public App {

    static const int numParticles = 50;

    Particle particles[numParticles];

    Mesh particleMesh;
    Texture texBlur;

    void onCreate() {
        for (auto& p : particles) {
            p.pos = rnd::ball<Vec3f>();
            p.vel = rnd::ball<Vec3f>() * 0.2;
        }

        // nav().pos(0, 0, 4);
        // nav().faceToward(0,0,0);

        texBlur.filter(Texture::LINEAR);
    }

    void onAnimate(double dt) {
        particleMesh.reset();
        particleMesh.primitive(Mesh::POINTS);

        for (auto& p : particles) {
            p.update(dt);

            particleMesh.vertex(p.pos);
            particleMesh.color(RGB(1));
        }
    }

    void onDraw(Graphics& g) {
        // texBlur.resize(fbWidth(), fbHeight());
        // g.tint(0.98);
        // g.quadViewport(texBlur, -1.005, -1.005, 2.01, 2.01);
        // g.tint(1);
        
        g.clear();
        g.depthTesting(true);
        g.pointSize(8);
        g.meshColor();
        g.camera(Viewpoint::UNIT_ORTHO);  // ortho camera that fits [-1:1] x [-1:1]
        g.draw(particleMesh);

        texBlur.copyFrameBuffer();
    }
};

int main() {
    MyApp app;
    app.start();
}
