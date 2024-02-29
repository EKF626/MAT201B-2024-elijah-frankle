// Karl Yerkes
// 2022-01-20

#include "al/app/al_App.hpp"
#include "al/app/al_GUIDomain.hpp"
#include "al/math/al_Random.hpp"
#include "al/graphics/al_Image.hpp"
#include "al/io/al_File.hpp"

using namespace al;

#include <fstream>
#include <vector>
using namespace std;

string slurp(string fileName);  // forward declaration

struct AlloApp : App {
  Parameter pointSize{"/pointSize", "", 1.0, 0.1, 3.0};
  Parameter timeStep{"/timeStep", "", 0.1, 0.01, 0.6};
  //

  ShaderProgram pointShader;

  void onInit() override {
    // set up GUI
    auto GUIdomain = GUIDomain::enableGUI(defaultWindowDomain());
    auto &gui = GUIdomain->newGUI();
    gui.add(pointSize);  // add parameter to GUI
    gui.add(timeStep);   // add parameter to GUI
    //
  }

  // a mesh for every style
  Mesh current;
  Mesh original;
  Mesh rgb;
  Mesh hsv;
  Mesh your_style;

  vector<Vec3f> target;

  void onCreate() override {
    pointShader.compile(slurp("../point-vertex.glsl"),
                        slurp("../point-fragment.glsl"),
                        slurp("../point-geometry.glsl"));

    current.primitive(Mesh::POINTS);

    auto file = File::currentPath() + "../sunrise1.jpeg";
    auto image = Image(file);
    if (image.width() == 0) {
      cout << "did not load image" << endl;
      exit(1);
    }
    auto aspect_ratio = 1.0f * image.width() / image.height();
    for (int j = 0; j < image.height(); j++) {
      for (int i = 0; i < image.width(); i++) {
        auto pixel = image.at(i, j); // 0-255 (unsigned char / uint8)
        current.vertex(1.0 * i / image.width() * aspect_ratio, 1.0 * j / image.height(), 0);
        current.color(pixel.r / 255.0, pixel.g / 255.0, pixel.b / 255.0);
        current.texCoord(0.05, 0);  // s, t

        original.vertex(1.0 * i / image.width() * aspect_ratio, 1.0 * j / image.height(), 0);
        original.color(pixel.r / 255.0, pixel.g / 255.0, pixel.b / 255.0);
        original.texCoord(0.05, 0);  // s, t
      
        rgb.vertex(pixel.r / 255.0 - 0.5, pixel.g / 255.0 - 0.5, pixel.b / 255.0 - 0.5);
        rgb.color(pixel.r / 255.0, pixel.g / 255.0, pixel.b / 255.0);
        rgb.texCoord(0.05, 0);

        HSV hsvC(RGB(pixel.r / 255.0, pixel.g / 255.0, pixel.b / 255.0));
        float x = hsvC.s * cos(hsvC.h*2*M_PI);
        float y = hsvC.v - 0.5;
        float z = hsvC.s * sin(hsvC.h*2*M_PI);

        hsv.vertex(x, y, z);
        hsv.color(pixel.r / 255.0, pixel.g / 255.0, pixel.b / 255.0);
        hsv.texCoord(0.05, 0);

        x = hsvC.v * sin(hsvC.h*2*M_PI) * cos(hsvC.s*2*M_PI);
        y = hsvC.v * cos(hsvC.h*2*M_PI);
        z = hsvC.v * sin(hsvC.h*2*M_PI) * sin(hsvC.s*2*M_PI);

        your_style.vertex(x, y, z);
        your_style.color(pixel.r / 255.0, pixel.g / 255.0, pixel.b / 255.0);
        your_style.texCoord(0.05, 0);
      }
    }
    target = current.vertices();

    nav().pos(0, 0, 5);
  }

  void onAnimate(double dt) override {
    for (int i = 0; i < current.vertices().size(); i++) {
      Vec3f &v = current.vertices()[i];
      Vec3f &t = target[i];
      v.lerp(t, dt);
    }
  }

  bool onKeyDown(const Keyboard &k) override {
    if (k.key() == '1') {
      target = original.vertices();
    }
    if (k.key() == '2') {
      target = rgb.vertices();
    }
    if (k.key() == '3') {
      target = hsv.vertices();
    }
    if (k.key() == '4') {
      target = your_style.vertices();
    }
    return true;
  }

  void onDraw(Graphics &g) override {
    // XXX no need to change anything in this function
    g.clear(0.3);
    g.shader(pointShader);
    g.shader().uniform("pointSize", pointSize / 100);
    g.blending(true);
    g.blendTrans();
    g.depthTesting(true);
    g.draw(current);
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
