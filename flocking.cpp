#include "al/app/al_App.hpp"

// how to include things ~ how do we know what to include?
// fewer includes == faster compile == only include what you need
#include "al/math/al_Random.hpp"
#include "al/graphics/al_Shapes.hpp" // addCone

struct MyApp : public al::App {
    al::Mesh mesh;

    static const int numPrey = 20;
    static const int numPredator = 2;
    static const int numFood = 3;

    const float radius = 2.5;
    const float vision = 1;

    // const float targetSperation = 0.02;
    const float cohesion = 0.05;
    const float alignment = 0.03;
    const float hunger = 0.05;
    const float fear = 0.5;
    const float correction = 0.5;

    const float prey_move_rate = 0.025;
    const float predator_move_rate = 0.02;

    al::Nav prey[numPrey];
    al::Nav predator[numPredator];
    al::Vec3d food[numFood];

    // int preyColors[numPrey][3];

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

        nav().pos(0, 0, 8);
        nav().faceToward(0,0,0);
    }

    void onAnimate(double dt) {
        for (int i = 0; i < numFood; i++) {
            for (int j = 0; j < numPrey; j++) {
                if (al::dist(food[i], prey[j].pos()) < 0.04) {
                    food[i] = al::rnd::ball<al::Vec3d>() * radius * 0.9;
                    break;
                }
            }
        }

        
        for (int i = 0; i < numPrey; i++) {
            std::vector<al::Nav> predatorInRange;
            std::vector<al::Nav> preyInRange;
            for (int j = 0; j < numPredator; j++) {
                if (al::dist(prey[i].pos(), predator[j].pos()) <= vision) {
                    predatorInRange.push_back(predator[j]);
                }
            }
            for (int j = 0; j < numPrey; j++) {
                if (i != j) {
                    if (al::dist(prey[i].pos(), prey[j].pos()) <= vision) {
                        preyInRange.push_back(prey[j]);
                    }
                }
            }

            al::Vec3d neighborhoodPos = al::Vec3d(0);
            al::Vec3d predatorPos = al::Vec3d(0);
            al::Vec3d avgUF = al::Vec3d(0);
            if (preyInRange.size() > 0) {
                for (int j = 0; j < preyInRange.size(); j++) {
                    neighborhoodPos += preyInRange[j].pos();
                    avgUF += preyInRange[j].uf();
                }
                neighborhoodPos /= (float)preyInRange.size();
                avgUF /= (float)preyInRange.size();
            }
            if (predatorInRange.size() > 0) {
                for (int j = 0; j < predatorInRange.size(); j++) {
                    predatorPos += predatorInRange[j].pos();
                }
                predatorPos /= (float)predatorInRange.size();
            }

            al::Vec3d closestFood = food[0];
            for (int j = 1; j < numFood; j++) {
                if (al::dist(prey[i].pos(), food[j]) < al::dist(prey[i].pos(), closestFood)) {
                    closestFood = food[j];
                }
            }

            // cohesion and seperation
            if (preyInRange.size() > 0) {
                float d = dist(prey[i].pos(), neighborhoodPos);
                if (d > 0.2) {
                    prey[i].faceToward(neighborhoodPos, cohesion);
                }
                else if (d < 0.1) {
                    faceAway(prey[i], neighborhoodPos, cohesion);
                }
            }
            // alignment
            prey[i].faceToward(prey[i].pos()+avgUF.normalized(), alignment);
            // hunger
            prey[i].faceToward(closestFood, hunger);
            //fear
            // if (predatorInRange.size() > 0) {
            //     faceAway(prey[i], predatorPos, fear);
            // }
            for (int j = 0; j < predatorInRange.size(); j++) {
                float d = al::dist(prey[i].pos(), predatorInRange[j].pos());
                faceAway(prey[i], predatorInRange[j], fear/d);
            }
            // correction
            if (al::dist(prey[i].pos(), al::Vec3d(0)) > radius) {
                prey[i].faceToward(al::Vec3d(0), correction);
            }
        }

        // al::Vec3d preyPos = al::Vec3d(0);
        // for (int i = 0; i < numPrey; i++) {
        //     preyPos += prey[i].pos();
        // }
        // preyPos /= (float)numPrey;
        // for (int i = 0; i < numPredator; i++) {
        //     predator[i].faceToward(preyPos, hunger);
        // }
        for (int i = 0; i < numPredator; i++) {
            al::Vec3d closestPrey = prey[0];
            for (int j = 1; j < numPrey; j++) {
                if (al::dist(predator[i].pos(), prey[j].pos()) < al::dist(predator[i].pos(), closestPrey)) {
                    closestPrey = prey[j].pos();
                }
            }
            predator[i].faceToward(closestPrey, hunger);
        }

        for (int i = 0; i < numPrey; i++) {
            prey[i].moveF(prey_move_rate);
        }
        for (int i = 0; i < numPredator; i++) {
            predator[i].moveF(predator_move_rate);
        }

        for (int i = 0; i < numPrey; i++) {
            prey[i].step();
        }
        for (int i = 0; i < numPredator; i++) {
            predator[i].step();
        }

        al::Vec3d avgPosition;
        for (int i = 0; i < numPrey; i++) {
            avgPosition += prey[i].pos()/numPrey;
        }
        for (int i = 0; i < numPredator; i++) {
            avgPosition += predator[i].pos()/numPredator;
        }
        avgPosition /= 2.0;
        nav().faceToward(avgPosition, 0.08);
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
            g.scale(0.07);
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
            g.color(0, 0, 0);
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
