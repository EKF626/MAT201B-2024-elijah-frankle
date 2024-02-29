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
    const float tightness = 0.2;

    const float prey_food_rate = 0.03;
    const float prey_attract_rate = 0.02;
    const float prey_avoid_rate = 0.15;
    const float prey_move_rate = 0.02;
    const float predator_turn_rate = 0.03;
    const float predator_move_rate = 0.02;
    const float correction_rate = 0.2;

    al::Nav prey[numPrey];
    al::Nav predator[numPredator];
    al::Vec3d food[numFood];

    int preyColors[numPrey][3];

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
            std::vector<al::Vec3d> predatorInRange;
            std::vector<al::Vec3d> preyInRange;

            for (int j = 0; j < numPredator; j++) {
                if (al::dist(prey[i].pos(), predator[j].pos()) <= vision) {
                    predatorInRange.push_back(predator[j].pos());
                }
            }
            for (int j = 0; j < numPrey; j++) {
                if (i != j) {
                    if (al::dist(prey[i].pos(), prey[j].pos()) <= vision) {
                        preyInRange.push_back(prey[j].pos());
                    }
                }
            }

            for (int j = 0; j < numFood; j++) {
                float d = al::dist(prey[i].pos(), food[j]);
                prey[i].faceToward(food[j], (2-pow(d/radius, 2))*prey_food_rate/(float)numFood);
            }
            for (int j = 0; j < preyInRange.size(); j++) {
                float d = al::dist(prey[i].pos(), preyInRange[j]);
                if (d > tightness) {
                    prey[i].faceToward(preyInRange[j], ((d-tightness)/(float)(vision-tightness))*prey_attract_rate/(float)preyInRange.size());
                } else {
                    faceAway(prey[i], preyInRange[j], (d/tightness)*prey_attract_rate/(float)preyInRange.size()/2.0);
                }
            }
            for (int j = 0; j < predatorInRange.size(); j++) {
                float d = al::dist(prey[i].pos(), predatorInRange[j]);
                faceAway(prey[i], predatorInRange[j], (1-pow(d/radius, 2))*prey_avoid_rate/(float)predatorInRange.size());
            }
            if (dist(prey[i].pos(), al::Vec3f(0)) >= radius) {
                prey[i].faceToward(al::Vec3f(0), correction_rate);
            }
        }

        for (int i = 0; i < numPredator; i++) {
            for (int j = 0; j < numPrey; j++) {
                float d = al::dist(predator[i].pos(), prey[j].pos());
                predator[i].faceToward(prey[j], (2-pow(d/radius, 2))*predator_turn_rate/(float)numPrey);
            }
            if (dist(predator[i].pos(), al::Vec3f(0)) >= radius) {
                predator[i].faceToward(al::Vec3f(0), correction_rate);
            }
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
