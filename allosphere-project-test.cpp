#include <iostream>
#include "al/app/al_App.hpp"
#include "al/math/al_Random.hpp"
#include "al/app/al_GUIDomain.hpp"

#include "al/app/al_DistributedApp.hpp"
#include "al_ext/statedistribution/al_CuttleboneDomain.hpp"
#include "al_ext/statedistribution/al_CuttleboneStateSimulationDomain.hpp"

using namespace al;

static const int numParticles = 1500;

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

void setMeshColor(Mesh &m, Color c) {
    for (int i = 0; i < m.colors().size(); i++) {
        m.colors()[i] = c;
    }
}

std::string slurp(std::string fileName) {
  std::fstream file(fileName);
  std::string returnValue = "";
  while (file.good()) {
    std::string line;
    getline(file, line);
    returnValue += line + "\n";
  }
  return returnValue;
}

struct MeshNode {
    Mesh mesh;
    MeshNode* next;

    MeshNode() {
        mesh.reset();
        next = NULL;
    }

    MeshNode(Mesh m) {
        mesh = m;
        next = NULL;
    }
};

struct LinkedList {
    MeshNode* head;
    int length;

    LinkedList() {
        head = NULL;
        length = 0;
    }

    void insertNode(Mesh m, bool first) {
        MeshNode* newNode = new MeshNode(m);
        if (!head) {
            head = newNode;
        }
        else {
            MeshNode* temp = head;
            if (first) {
                head = newNode;
                head->next = temp;
            }
            else {
                while (temp->next) {
                    temp = temp->next;
                }
                temp->next = newNode;
            }
        }
        length++;
    }

    void deleteNode(bool first) {
        if (!head) {
            return;
        }
        if (first) {
            MeshNode* temp = head;
            head = temp->next;
            delete temp;
        }
        else {
            MeshNode* temp1 = head;
            MeshNode* temp2 = NULL;
            while (temp1->next) {
                temp2 = temp1;
                temp1 = temp1->next;
            }
            delete temp1;
            if (temp2) {
                temp2->next = NULL;
            }
        }
        length--;
    }
};

struct CommonState {
    float particles[numParticles][3];
};

struct MyApp : DistributedAppWithState<CommonState> {
    Parameter theta{"theta", "", 0, -M_PI, M_PI};
    Parameter phi{"phi", "", 0.0, -M_PI, M_PI};
    Parameter trailLength{"trailLength", "", 30, 0, 100};
    Parameter speed{"speed", "", 0.5, 0.0, 2.0};
    Parameter pointSize{"pointSize", "", 4.0, 1.0, 10.0};

    LinkedList meshList;

    ShaderProgram starShader;

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
        gui.add(trailLength);
        gui.add(speed);
        gui.add(pointSize);
    }
  }

    void onCreate() override {
        starShader.compile(slurp("../star-vertex.glsl"),
                        slurp("../star-fragment.glsl"),
                        slurp("../star-geometry.glsl"));


        if (isPrimary()) {
            for (int i = 0; i < numParticles; i++) {
                Vec3f pos = rnd::ball<Vec3f>();
                state().particles[i][0] = pos[0];
                state().particles[i][1] = pos[1];
                state().particles[i][2] = pos[2];
            }
        }

        nav().pos(0, 0, 4);
        nav().faceToward(0,0,0);
    }

    void onAnimate(double dt) override {
        if (isPrimary()) {
            while (meshList.length >= trailLength) {
                meshList.deleteNode(false);
            }

            Mesh newMesh;
            newMesh.primitive(Mesh::POINTS);

            float amount = speed * dt;

            for (int i = 0; i < numParticles; i++) {
                Vec3f newPoint = rotatePoint(state().particles[i], theta, phi, amount);
                state().particles[i][0] = newPoint[0];
                state().particles[i][1] = newPoint[1];
                state().particles[i][2] = newPoint[2];

                newMesh.vertex(state().particles[i]);
                newMesh.color(Color(1.0, 1.0));
            }

            meshList.insertNode(newMesh, true);
        }
    }

    void onDraw(Graphics& g) override {
        g.clear();
        g.shader(starShader);
        g.depthTesting(true);
        g.pointSize(pointSize);
        g.meshColor();
        Mesh finalMesh;
        finalMesh.primitive(Mesh::POINTS);
        // MeshNode* node = meshList.head;
        // for (int i = 0; i < meshList.length; i++) {
        //     setMeshColor(node->mesh, Color(1.0f, 1.0f, 1.0f, 1.0f-i/(float)(meshList.length)));
        //     finalMesh.merge(node->mesh);
        //     node = node->next;
        // }
        for (int i = 0; i < numParticles; i++) {
            finalMesh.vertex(state().particles[i]);
            finalMesh.color(1, 1, 1);
        }
        g.draw(finalMesh);
    }
};

int main() {
    MyApp app;
    app.start();
}