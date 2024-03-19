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

static const int numPoints = 10;
static const float bounds = 1.0;
static const float speed = 0.1;

struct CommonState {

};

struct MyApp : DistributedAppWithState<CommonState> {

    vector<Vec3f> points;
    Vec3f point;
    int frame = 0;

    float max = -99999;
    float min = 99999;

    void onInit() override {
        
    }

    void onCreate() override {
        if (isPrimary()) {
           point = Vec3f();
        }
    }

    void onAnimate(double dt) override {
        if (isPrimary()) {
           point = Vec3f(stb_perlin_noise3(frame*speed, 0, 0, 0, 0, 0), 0, 0);
        }
        frame++;
    }

    void onDraw(Graphics& g) override {
        g.clear(0.0);
        g.blending(true);
        g.blendTrans();
        g.depthTesting(true);
        g.pointSize(1);
        g.meshColor();

        Mesh newMesh;
        newMesh.vertex(point);
        newMesh.color(Color(1));

        g.draw(newMesh);

        float noiseVal = stb_perlin_noise3(frame*speed, 0, 0, 0, 0, 0);
        if (noiseVal > max) {max = noiseVal;}
        if (noiseVal < min) {min = noiseVal;}
        cout << "min: " << min << "      max : " << max << endl;
    }
};

int main() {
    MyApp app;
    app.start();
}