#include "al/app/al_App.hpp"

#include "al/math/al_Random.hpp"
#include "al/graphics/al_Shapes.hpp"

struct MyApp : public al::App {
    al::Mesh mesh;

    const int numX = 20;
    const int numY = 20;
    const int numZ = 20;
    const float spacing = 1.2;
    const float size = 0.3;

    std::vector<al::Vec3f> anchors;


    void onCreate() {
        addOctahedron(mesh);
        mesh.generateNormals();

        int count = 0;
        for (int i = 0; i < numX; i++) {
            for (int j = 0; j < numY; j++) {
                for (int k = 0; k < numZ; k++) {
                    float x = (i-numX/2.0)*spacing;
                    float y = (j-numY/2.0)*spacing;
                    float z = (k-numZ/2.0)*spacing;
                    anchors.push_back(al::Vec3f(x, y, z));
                    count++;
                }
            }
        }

        nav().pos(0, 0, 8);
        nav().faceToward(0,0,0);
    }

    void onAnimate(double dt) {

    }

    void onDraw(al::Graphics& g) {
        g.depthTesting(true);
        g.lighting(true);
        g.clear(1);

        for (int i = 0; i < anchors.size(); i++) {
            g.color(0.5, 0, 1);
            g.pushMatrix();
            g.translate(anchors[i]);
            g.scale(size);
            g.draw(mesh);
            g.popMatrix();
        }
    }
};

int main() {
    MyApp app;
    app.configureAudio(48000, 512, 2, 0);
    app.start();
}

// int main() {  MyApp().start(); }
