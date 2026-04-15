#include <Graphics.hpp>
#include <vector>
#include <iostream>
#include <cmath>
#include <random>
#include <ctime>

// Window Management
const int WIDTH = 1920;
const int HEIGHT = 1011;
int MAX_FRAMES = 60;


// Math
float len(const sf::Vector2f& v) {
    return sqrt(v.x*v.x + v.y*v.y);
}

sf::Vector2f normalize(const sf::Vector2f& v) {
    float l = len(v); if (l == 0) return {0,0}; return v / l;
}

float dot(const sf::Vector2f& v1, const sf::Vector2f& v2) {
    return float(v1.x*v2.x + v1.y*v2.y);
}


struct Line {
    public:
        sf::Vector2f posA, posB;
        sf::Color color;
        float lineWidth;

        Line(sf::Vector2f _posA, sf::Vector2f _posB, sf::Color _color = sf::Color::Red, float _lineWidth = 2.0f) : posA(_posA), posB(_posB), color(_color), lineWidth(_lineWidth) {}

        void render(sf::RenderWindow& window) {
            float dx = posA.x - posB.x;
            float dy = posA.y - posB.y;
            float length = len({dx, dy});
            float rotation = atan2(dy, dx);

            sf::RectangleShape obj;
            obj.setSize({length, lineWidth});
            obj.setOrigin(length, lineWidth/2.0f);
            obj.setRotation(rotation*180.0f/M_PI);
            obj.setPosition(posA);
            obj.setFillColor(color);

            window.draw(obj);
        }
};



struct MouseHandler {
    public:
        static sf::Vector2f mousePos;
        static sf::Vector2f lastMousePos;
        static sf::Vector2f delta;

        static void update(sf::RenderWindow& window) {
            lastMousePos = mousePos;
            mousePos = window.mapPixelToCoords(sf::Mouse::getPosition(window));
            delta = mousePos - lastMousePos;
        }
};

sf::Vector2f MouseHandler::delta        = sf::Vector2f(0, 0);
sf::Vector2f MouseHandler::mousePos     = sf::Vector2f(0, 0);
sf::Vector2f MouseHandler::lastMousePos = sf::Vector2f(0, 0);


// Simulation classes
struct Mass {
    public:
        sf::Vector2f pos;
        sf::Vector2f lastPos;
        sf::Vector2f acc = sf::Vector2f(0, 0);
        float radius;
        float mass = 1.0f;

        Mass(const sf::Vector2f& _pos, float _radius) : pos(_pos), lastPos(_pos), radius(_radius) {}

        void move(float dt) {
            sf::Vector2f tempPos = pos;
            pos = 2.0f * pos - lastPos + acc * dt*dt;
            acc = sf::Vector2f(0, 0);  // Resets for easy accumalation of forces
            lastPos = tempPos;
        }

        void changeVelocity(float factorX, float factorY) {
            sf::Vector2f velocity = sf::Vector2f(getVelX(), getVelY());
            velocity.x *= factorX;
            velocity.y *= factorY;
            lastPos = pos - velocity;
        }

        float getVelX() {return (pos.x - lastPos.x);}
        float getVelY() {return (pos.y - lastPos.y);}
};

struct Spring {
    public:
        unsigned int indexA;
        unsigned int indexB;
        float restLen;
        float springConstant  = 300000.0f;
        float dampingConstant = 10000.0f;

        Spring(unsigned int _indexA, unsigned int _indexB, float _restLen) : indexA(_indexA), indexB(_indexB), restLen(_restLen) {}

        void accelerateMasses(Mass* m1, Mass* m2) {
            // Spring force
            float currentLen = len(m1->pos - m2->pos);
            sf::Vector2f springDir = normalize(m2->pos - m1->pos);
            float extension = currentLen - restLen;
            sf::Vector2f force = springConstant * springDir * extension / restLen;
            m1->acc += force / m1->mass;
            m2->acc -= force / m2->mass;

            // Damping
            sf::Vector2f vel1 = (m1->pos - m1->lastPos);
            sf::Vector2f vel2 = (m2->pos - m2->lastPos);
            sf::Vector2f deltaVel = vel2 - vel1;
            float alignment = dot(deltaVel, springDir);
            sf::Vector2f dampingForce = springDir * dampingConstant * alignment;
            m1->acc += dampingForce / m1->mass;
            m2->acc -= dampingForce / m2->mass;
        }
};

struct Mesh {
    public:
        std::vector<Mass>   masses;
        std::vector<Spring> springs;

        Mesh() {}

        void updateAcceleration(float dt) {
            for (Spring& spring : springs) {
                spring.accelerateMasses(&masses[spring.indexA], &masses[spring.indexB]);
            }

            for (Mass& mass : masses) {
                mass.move(dt);
            }
        }

        void render(sf::RenderWindow& window) {
            // Springs
            Line line({0, 0}, {0, 0}, sf::Color::Red, 4.0f);

            for (Spring& spring : springs) {
                line.posA = masses[spring.indexA].pos;
                line.posB = masses[spring.indexB].pos;

                line.render(window);
            }


            // Masses
            sf::CircleShape obj;
            obj.setFillColor(sf::Color::Blue);

            for (Mass mass : masses) {
                obj.setPosition(mass.pos);
                obj.setRadius(mass.radius);
                obj.setOrigin(mass.radius, mass.radius);

                window.draw(obj);
            }
        }
};


namespace Make {
    Mesh rectangleMesh(sf::Vector2f centerPos, int width, int height, int amountMassesX, int amountMassesY, float massRadius) {
        Mesh mesh;

        float spacingX = (float)width  / (float)amountMassesX;
        float spacingY = (float)height / (float)amountMassesY;

        centerPos -= sf::Vector2f(width, height) / 2.0f;

        for (int x = 0; x < amountMassesX; ++x) {
            for (int y = 0; y < amountMassesY; ++y) {
                mesh.masses.emplace_back(Mass(centerPos + sf::Vector2f((float)x * spacingX, (float)y * spacingY), massRadius));
            }
        }

        float maxDist = sqrt(spacingX*spacingX + spacingY*spacingY);

        for (int i = 0; i < mesh.masses.size(); ++i) {
            for (int k = i + 1; k < mesh.masses.size(); ++k) {
                float distance = len(mesh.masses[i].pos - mesh.masses[k].pos);
                if (distance <= maxDist) {
                    mesh.springs.emplace_back(Spring(i, k, distance));
                }
            }
        }


        return mesh;
    }
};

struct Simulation {
    public:
        float dt = 1.0f / 240.0f;
        int iterationsPerUpdate = 4;
        float boundBounceEnergyLoss = 0.95f;
        std::vector<Mesh> meshes;

        Simulation() {}

        void update() {
            for (int i = 0; i < iterationsPerUpdate; ++i) {
                for (Mesh& mesh : meshes) {
                    mesh.updateAcceleration(dt);
                    checkBoundCollision();
                }
            }
        }

        void render(sf::RenderWindow& window) {
            for (Mesh& mesh : meshes) {
                mesh.render(window);
            }
        }

        void checkBoundCollision() {
            const float eps = 1.0f;

            for (Mesh& mesh : meshes) {
                for (Mass& mass : mesh.masses) {
                    float r = mass.radius;
                    sf::Vector2f& pos = mass.pos;

                    if (pos.x - r < 0)      {pos.x = r;          if (mass.getVelX() < 0) mass.changeVelocity(-1.0f, 1.0f);}
                    if (pos.y - r < 0)      {pos.y = r;          if (mass.getVelY() < 0) mass.changeVelocity(1.0f, -1.0f);}
                    if (pos.x + r > WIDTH)  {pos.x = WIDTH  - r; if (mass.getVelX() > 0) mass.changeVelocity(-1.0f, 1.0f);}
                    if (pos.y + r > HEIGHT) {pos.y = HEIGHT - r; if (mass.getVelY() > 0) mass.changeVelocity(1.0f, -1.0f);}
                }
            }
        }

        void doMouseInteraction() {
            const float maxDist = 250.0f;
            const float springConstant = 300.0f;

            for (Mesh& mesh : meshes) {
                for (Mass& mass : mesh.masses) {
                    float distance = len(mass.pos - MouseHandler::mousePos);
                    if (distance < maxDist) {
                        sf::Vector2f dir = MouseHandler::mousePos - mass.pos;
                        float strength = (1.0f - (distance / maxDist));
                        sf::Vector2f force = dir * springConstant * strength;
                        mass.acc += force / mass.mass;
                    }
                }
            }
        }
};



int main() {
    sf::RenderWindow window(sf::VideoMode(WIDTH, HEIGHT), "Mass Spring System", sf::Style::Titlebar | sf::Style::Close);
    window.setFramerateLimit(MAX_FRAMES);
    sf::Event ev;
    sf::View view(sf::FloatRect(0, 0, WIDTH, HEIGHT));
    window.setView(view);


    Simulation sim;
    sim.meshes.emplace_back(Make::rectangleMesh({WIDTH/2.0f, HEIGHT/2.0f}, 350, 350, 8, 8, 15.0f));


    while (window.isOpen()) {
        while (window.pollEvent(ev)) {if (ev.type == sf::Event::Closed) {window.close();}}
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Escape)) {window.close();}

        MouseHandler::update(window);
        if (sf::Mouse::isButtonPressed(sf::Mouse::Left)) sim.doMouseInteraction();
        sim.update();
        

        // Rendering
        window.clear(sf::Color(31, 31, 31, 255));
        sim.render(window);
        window.display();

    }
    return 0;
}