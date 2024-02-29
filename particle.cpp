// Karl Yerkes
// 2022-01-20

#include "al/app/al_App.hpp"
#include "al/app/al_GUIDomain.hpp"
#include "al/math/al_Random.hpp"

using namespace al;

#include <fstream>
#include <vector>
using namespace std;

Vec3f randomVec3f(float scale) {
  return Vec3f(rnd::uniformS(), rnd::uniformS(), rnd::uniformS()) * scale;
}
string slurp(string fileName);  // forward declaration

struct AlloApp : App {
  Parameter pointSize{"/pointSize", "", 1.0, 0.0, 2.0};
  Parameter timeStep{"/timeStep", "", 0.1, 0.01, 0.6};
  Parameter dragFactor{"/dragFactor", "", 2.0, 0.0, 3.0};
  Parameter sphereRadius{"/sphereRadius", "", 1.5, 0.1, 4.0};
  Parameter springConstant{"/springConstant", "", 20.0, 0.0, 40.0};
  Parameter coulombConstant{"/coulombConstant", "", 0.0015, 0.0005, 0.005};

  ShaderProgram pointShader;

  //  simulation state
  Mesh mesh;  // position *is inside the mesh* mesh.vertices() are the positions
  vector<Vec3f> velocity;
  vector<Vec3f> force;
  vector<float> mass;
  vector<float> hues;
  int mode = 1;

  void onInit() override {
    // set up GUI
    auto GUIdomain = GUIDomain::enableGUI(defaultWindowDomain());
    auto &gui = GUIdomain->newGUI();
    gui.add(pointSize);  // add parameter to GUI
    gui.add(timeStep);   // add parameter to GUI
    gui.add(dragFactor);   // add parameter to GUI
    gui.add(springConstant);
    gui.add(coulombConstant);
    gui.add(sphereRadius);
    //
  }

  void onCreate() override {
    // compile shaders
    pointShader.compile(slurp("../point-vertex.glsl"),
                        slurp("../point-fragment.glsl"),
                        slurp("../point-geometry.glsl"));

    // set initial conditions of the simulation
    //

    // c++11 "lambda" function
    auto randomColor = []() { return HSV(rnd::uniform(), 1.0f, 1.0f); };

    mesh.primitive(Mesh::POINTS);
    // does 1000 work on your system? how many can you make before you get a low
    // frame rate? do you need to use <1000?
    for (int _ = 0; _ < 2000; _++) {
      mesh.vertex(randomVec3f(5));
      HSV color = randomColor();
      mesh.color(color);
      hues.push_back(color.h);

      // float m = rnd::uniform(3.0, 0.5);
      float m = 3 + rnd::normal() / 2;
      if (m < 0.5) m = 0.5;
      mass.push_back(m);

      // using a simplified volume/size relationship
      mesh.texCoord(pow(m, 1.0f / 3), 0);  // s, t

      // separate state arrays
      velocity.push_back(randomVec3f(0.1));
      force.push_back(randomVec3f(1));
    }

    nav().pos(0, 0, 10);
  }

  bool freeze = false;
  void onAnimate(double dt) override {
    if (freeze) return;

    // Calculate forces

    // XXX you put code here that calculates gravitational forces and sets
    // accelerations These are pair-wise. Each unique pairing of two particles
    // These are equal but opposite: A exerts a force on B while B exerts that
    // same amount of force on A (but in the opposite direction!) Use a nested
    // for loop to visit each pair once The time complexity is O(n*n)
    //
    // Vec3f has lots of operations you might use...
    // • +=
    // • -=
    // • +
    // • -
    // • .normalize() ~ Vec3f points in the direction as it did, but has length 1
    // • .normalize(float scale) ~ same but length `scale`
    // • .mag() ~ length of the Vec3f
    // • .magSqr() ~ squared length of the Vec3f
    // • .dot(Vec3f f) 
    // • .cross(Vec3f f)

    // Hooke's law
    for (int i = 0; i < velocity.size(); i++) {
      force[i] += (-mesh.vertices()[i] + (Vec3f(mesh.vertices()[i]).normalize() * sphereRadius)) * springConstant;
    }

    // equilibruim based on Coloumb's law with mass as charges
    if (mode == 1) {
      for (int i = 0; i < velocity.size(); i++) {
        for (int j = i+1; j < velocity.size(); j++) {
          float r2 = pow(((mesh.vertices()[i]-mesh.vertices()[j]).mag()), 2);
          Vec3f f = (mesh.vertices()[i]-mesh.vertices()[j]) * mass[i] * mass[j] * (coulombConstant / r2);
          force[i] += f;
          force[j] -= f;
        }
      }
    }
    // the larger the difference in hue is, the more they are repulsed
    else if (mode == 2) {
      for (int i = 0; i < velocity.size(); i++) {
        for (int j = i+1; j < velocity.size(); j++) {
          float r2 = pow(((mesh.vertices()[i]-mesh.vertices()[j]).mag()), 2);
          Vec3f f = (mesh.vertices()[i]-mesh.vertices()[j]) * (coulombConstant / r2);
          float hueDif = (abs(hues[i]-hues[j]) < abs(hues[i]-hues[j]-1.0)) ? abs(hues[i]-hues[j]) : abs(hues[i]-hues[j]-1.0);
          force[i] += f * hueDif;
          force[j] -= f * hueDif;
        }
      }
    }
    // asymmetrically attracted to similar hues and repulsed by different hues
    else if (mode == 3) {
      for (int i = 0; i < velocity.size(); i++) {
        for (int j = 0; j < velocity.size(); j++) {
          if (i != j) {
            float r2 = pow(((mesh.vertices()[i]-mesh.vertices()[j]).mag()), 2);
            Vec3f f = (mesh.vertices()[i]-mesh.vertices()[j]) * (coulombConstant / r2);
            float hueDif = hues[j] - hues[i];
            if (hueDif < 0) {
              hueDif += 1.0;
            }
            if (hueDif <= 0.5) {
              force[i] -= f;
            } else {
              force[i] += f;
            }
          }
        }
      }
    }

    // drag
    for (int i = 0; i < velocity.size(); i++) {
      force[i] += - velocity[i] * dragFactor;
    }

    // Integration
    //
    vector<Vec3f> &position(mesh.vertices());
    for (int i = 0; i < velocity.size(); i++) {
      // "semi-implicit" Euler integration
      velocity[i] += force[i] / mass[i] * timeStep;
      position[i] += velocity[i] * timeStep;
    }

    // clear all accelerations (IMPORTANT!!)
    for (auto &a : force) a.set(0);
  }

  bool onKeyDown(const Keyboard &k) override {
    if (k.key() == ' ') {
      freeze = !freeze;
    }

    if (k.key() == '1') {
      // introduce some "random" forces
      for (int i = 0; i < velocity.size(); i++) {
        // F = ma
        force[i] += randomVec3f(1);
      }
    }
    else if (k.key() == '2') {
      mode = 1;
    }
    else if (k.key() == '3') {
      mode = 2;
    }
    else if (k.key() == '4') {
      mode = 3;
    }

    return true;
  }

  void onDraw(Graphics &g) override {
    g.clear(0.3);
    g.shader(pointShader);
    g.shader().uniform("pointSize", pointSize / 100);
    g.blending(true);
    g.blendTrans();
    g.depthTesting(true);
    g.draw(mesh);
  }
};

int main() {
  AlloApp app;
  app.configureAudio(48000, 512, 2, 0);
  app.start();
}

string slurp(string fileName) {
  fstream file(fileName);
  string returnValue = "";
  while (file.good()) {
    string line;
    getline(file, line);
    returnValue += line + "\n";
  }
  return returnValue;
}
