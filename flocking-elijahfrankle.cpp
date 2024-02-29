#include "al/app/al_App.hpp"

// how to include things ~ how do we know what to include?
// fewer includes == faster compile == only include what you need
#include "al/math/al_Random.hpp"
#include "al/graphics/al_Shapes.hpp" // addCone

struct MyApp : public al::App {
    al::Mesh mesh;

    const bool rotateCamera = false;
    float phase = 0.0;
    const float rotSpeed = 0.007;
    const float cameraRadius = 13.0;

    static const int numPrey = 200;
    static const int numPredator = 3;
    static const int numFood = 4;

    const float radius = 3.0;

    const float bounding = 0.1;
    
    const float preySpeed = 0.025;
    const float preyHunger = 0.05;
    const float preyCohesion = 0.02;
    const float preySeperation = 0.015;
    const float preyAlignment = 0.01;
    const float preyFear = 0.1;
    const float tightness = 0.1;
    const float hysteresis = 0.05;
    const float neighborhood = 0.2;
    const float vision = 1.0;

    const float predatorSpeed = 0.03;
    const float predatorHunger = 0.02;
    const float predatorSeperation = 0.04;
    const float predatorTightness = 0.4;

    al::Nav prey[numPrey];
    al::Nav predator[numPredator];
    al::Vec3d food[numFood];

    void faceAway(al::Nav &object, al::Vec3d point, double amt=1) {
        al::Vec3d oppositePoint = object.pos() * 2 - point;
        object.faceToward(oppositePoint, amt);
    }

    void onCreate() {
        addCone(mesh);
        mesh.generateNormals();

        for (int i = 0; i < numPrey; i++) {
            prey[i].pos(al::rnd::ball<al::Vec3d>() * radius);
        }
        for (int i = 0; i < numPredator; i++) {
            predator[i].pos(al::rnd::ball<al::Vec3d>() * radius);
        }
        for (int i = 0; i < numFood; i++) {
            food[i] = al::rnd::ball<al::Vec3d>() * radius * 0.9;
        }

        nav().pos(0, 0, cameraRadius);
        nav().faceToward(0,0,0);
    }

    void onAnimate(double dt) {
        for (int i = 0; i < numFood; i++) {
            for (int j = 0; j < numPrey; j++) {
                if (al::dist(food[i], prey[j].pos()) <= 0.07) {
                    food[i] = al::rnd::ball<al::Vec3d>() * radius * 0.9;
                    break;
                }
            }
        }

        for (int i = 0; i < numPrey; i++) {
            std::vector<al::Nav> preyInRange;
            for (int j = 0; j < numPrey; j++) {
                if (i != j) {
                    if (al::dist(prey[i].pos(), prey[j].pos()) <= neighborhood) {
                        preyInRange.push_back(prey[j]);
                    }
                }
            }

            std::vector<al::Nav> predatorInRange;
            for (int j = 0; j < numPredator; j++) {
                if (al::dist(prey[i].pos(), predator[j].pos()) <= vision) {
                    predatorInRange.push_back(predator[j]);
                }
            }

            al::Vec3d avgPreyPos = al::Vec3d(0);
            al::Vec3d avgUF = al::Vec3d(0);
            for (int j = 0; j < preyInRange.size(); j++) {
                avgPreyPos += preyInRange[j].pos()/(float)preyInRange.size();
                avgUF += preyInRange[j].uf()/(float)preyInRange.size();
            }

            al::Vec3d avgPredatorPos = al::Vec3d(0);
            for (int j = 0; j < predatorInRange.size(); j++) {
                avgPredatorPos += predatorInRange[j].pos()/(float)predatorInRange.size();
            }

            al::Vec3d closestFood = food[0];
            for (int j = 1; j < numFood; j++) {
                if (al::dist(prey[i].pos(), food[j]) < al::dist(prey[i].pos(), closestFood)) {
                    closestFood = food[j];
                }
            }

            // cohesion + seperation
            if (preyInRange.size() > 0) {
                if (al::dist(prey[i].pos(), avgPreyPos) < tightness) {
                    faceAway(prey[i], avgPreyPos, preySeperation);
                } else if (al::dist(prey[i].pos(), avgPreyPos) > tightness+hysteresis) {
                    prey[i].faceToward(avgPreyPos, preyCohesion);
                }
            }

            // alignment
            if (preyInRange.size() > 0) {
                prey[i].faceToward(prey[i].pos()+avgUF.normalized(), preyAlignment);
            }

            // hunger
            prey[i].faceToward(closestFood, preyHunger);

            //fear
            if (predatorInRange.size() > 0) {
                faceAway(prey[i], avgPredatorPos, preyFear);
            }

            // stay in radius
            if (al::dist(prey[i].pos(), al::Vec3d(0)) > radius) {
                prey[i].faceToward(al::Vec3d(0), bounding);
            }
        }

        for (int i = 0; i < numPredator; i++) {
            al::Vec3d preyPos = al::Vec3d(0);
            for (int j = 0; j < numPrey; j++) {
                preyPos += prey[j].pos();
            }
            preyPos /= (float)numPrey;

            // hunger
            predator[i].faceToward(preyPos, predatorHunger);

            // seperation
            for (int j = 0; j < numPredator; j++) {
                if (i != j) {
                    if (al::dist(predator[i].pos(), predator[j].pos()) < predatorTightness) {
                        faceAway(predator[i], predator[j].pos(), predatorSeperation);
                    }
                }
            }

            // stay in radius
            if (al::dist(predator[i].pos(), al::Vec3d(0)) > radius) {
                predator[i].faceToward(al::Vec3d(0), bounding);
            }
        }

        for (int i = 0; i < numPrey; i++) {
            prey[i].moveF(preySpeed);
        }
        for (int i = 0; i < numPredator; i++) {
            predator[i].moveF(predatorSpeed);
        }

        for (int i = 0; i < numPrey; i++) {
            prey[i].step();
        }
        for (int i = 0; i < numPredator; i++) {
            predator[i].step();
        }

        al::Vec3d avgPosition;
        for (int i = 0; i < numPrey; i++) {
            avgPosition += prey[i].pos()/(float)numPrey;
        }
        for (int i = 0; i < numPredator; i++) {
            avgPosition += predator[i].pos()/(float)numPredator;
        }
        avgPosition /= 2.0;

        if (rotateCamera) {
            phase += rotSpeed;
            if (phase > 2*M_PI) {
                phase -= 2*M_PI;
            }
            nav().pos(cameraRadius*sin(phase), 0, cameraRadius*cos(phase));
        }

        nav().faceToward(avgPosition, 1);
    }

    void onDraw(al::Graphics& g) {
        g.depthTesting(true);
        g.lighting(true);
        g.clear(1);
        
        for (int i = 0; i < numPrey; i++) {
            g.color(0, 1, 0);
            g.pushMatrix();
            g.translate(prey[i].pos());
            g.rotate(prey[i].quat());
            g.scale(0.06);
            g.draw(mesh);
            g.popMatrix();
        }

        for (int i = 0; i < numPredator; i++) {
            g.color(1, 0, 0);
            g.pushMatrix();
            g.translate(predator[i].pos());
            g.rotate(predator[i].quat());
            g.scale(0.2);
            g.draw(mesh);
            g.popMatrix();
        }

        for (int i = 0; i < numFood; i++) {
            g.color(0, 0, 1);
            g.pushMatrix();
            g.translate(food[i]);
            g.scale(0.04);
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
