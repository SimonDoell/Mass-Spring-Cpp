#include <SFML/Graphics.hpp>
#include <list>
#include <vector>
#include <iostream>
#include <cmath>
#include <random>
#include <cstdlib>
#include <ctime>

// Window Management
const int WIDTH = 1920;
const int HEIGHT = 1010;
int MAX_FRAMES = 240;
float dt = 1.0f/float(MAX_FRAMES);



// Functions
float len(const sf::Vector2f& v) {
    return sqrt(v.x*v.x + v.y*v.y);
}

float lenSqr(const sf::Vector2f& v) {
    return (v.x*v.x + v.y*v.y);
}

float dist(const sf::Vector2f& v1, const sf::Vector2f& v2) {
    sf::Vector2f dV = v2 - v1;
    return len(dV);
}

sf::Vector2f normalize(const sf::Vector2f& v) {
    float l = len(v); if (l == 0) return {0,0}; return v / l;
}

float dot(const sf::Vector2f& v1, const sf::Vector2f& v2) {
    return float(v1.x*v2.x + v1.y*v2.y);
}


sf::Vector2f getRandomInitPos() {
    return sf::Vector2f(rand() % int(WIDTH), rand() % int(HEIGHT));
}




// Structs
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


struct Mass {
    public:
        float mass = 1.0f;
        float r;
        sf::Vector2f pos;
        sf::Vector2f lastPos;
        sf::Vector2f acc;

        Mass(sf::Vector2f _startPos, float _r = 15.0f, sf::Vector2f _initVel = sf::Vector2f(0, 0), float _mass = 1.0f) : r(_r), mass(_mass) {
            pos = _startPos;
            lastPos = _startPos - _initVel*dt;
            acc = sf::Vector2f(0, 0);
        }

        void changeVel(const sf::Vector2f& velMatrix) {
            lastPos.x = pos.x + (lastPos.x - pos.x) * velMatrix.x;
            lastPos.y = pos.y + (lastPos.y - pos.y) * velMatrix.y;
        }

        sf::Vector2f getVelV() {
            return (pos - lastPos);
        }

        void move() {
            sf::Vector2f tempPos = pos;
            pos += pos - lastPos + acc*dt*dt;
            lastPos = tempPos;
        }
};


struct Spring {
    public:
        Mass& m1;
        Mass& m2;
        float restLen;
        float springConstant;
        float dampingConstant;

        Spring(Mass& _m1, Mass& _m2, float _restLen, float _springConstant = 30000.0f, float _dampingConstant = 5000.0f) : m1(_m1), m2(_m2), restLen(_restLen), springConstant(_springConstant), dampingConstant(_dampingConstant) {}

        void calculateMassAcc() {
            float seperation = restLen - dist(m1.pos, m2.pos);
            seperation /= restLen;
            sf::Vector2f springDir = normalize(m1.pos - m2.pos);
            sf::Vector2f force = springConstant * springDir * seperation;
            m1.acc += force / m1.mass;
            m2.acc -= force / m2.mass;

            // Damping
            sf::Vector2f vel1 = (m1.pos - m1.lastPos);
            sf::Vector2f vel2 = (m2.pos - m2.lastPos);
            sf::Vector2f deltaVel = vel2 - vel1;
            float alignment = dot(deltaVel, springDir);
            sf::Vector2f dampingForce = springDir * dampingConstant * alignment;
            m1.acc += dampingForce / m1.mass;
            m2.acc -= dampingForce / m2.mass;
        }
};




struct SpringMassMesh {
    public:
        std::vector<Mass> masses;
        std::vector<Spring> springs;
};



struct Bound {
    sf::Vector2f pos;
    sf::Vector2f size;
    Bound(sf::Vector2f _pos, sf::Vector2f _size) : pos(_pos), size(_size) {}
};





namespace Make {
    SpringMassMesh Rectangle(sf::Vector2f startPos, int width, int height, float spacing, float r = 15.0f, float nodeMass = 1.0f) {
        SpringMassMesh mesh;

        for (int x = std::floor(-(float)width/2.0f); x < std::floor((float)width/2.0f); ++x) {
            for (int y = std::floor(-(float)height/2.0f); y < std::floor((float)height/2.0f); ++y) {
                mesh.masses.emplace_back(Mass(
                    startPos + sf::Vector2f(float(x)*spacing, float(y)*spacing),
                    r,
                    {0, 0},
                    nodeMass
                ));
            }
        }

        for (int i = 0; i < mesh.masses.size(); ++i) {
            for (int x = 0; x < mesh.masses.size(); ++x) {
                if (x != i) {
                    float distance = dist(mesh.masses[i].pos, mesh.masses[x].pos);
                    if (distance <= len({spacing, spacing})*1.5f) {
                        mesh.springs.emplace_back(Spring(mesh.masses[i], mesh.masses[x], distance, 50000, 4000));
                    }
                }
            }
        }

        return mesh;
    }

    SpringMassMesh UVSphere(sf::Vector2f startPos, float circleR, float massR, int sideIterations = 25) {
        SpringMassMesh mesh;

        mesh.masses.emplace_back(Mass(startPos, massR));
        float angleIncrement = 2.0f * M_PI / (float)sideIterations;
        for (float angle = 0.0f; angle < 2.0f*M_PI; angle+=angleIncrement) {
            mesh.masses.emplace_back(Mass(
                startPos + sf::Vector2f(sin(angle), cos(angle))*circleR, massR
            ));
        }

        for (int i = 1; i < mesh.masses.size(); ++i) {
            mesh.springs.emplace_back(Spring(mesh.masses[0], mesh.masses[i], dist(mesh.masses[0].pos, mesh.masses[i].pos), 300000.0f, 7500.0f));

            Mass& nextMass = mesh.masses[(i == mesh.masses.size() - 1) ? 1 : i + 1];
            mesh.springs.emplace_back(Spring(mesh.masses[i], nextMass, dist(nextMass.pos, mesh.masses[i].pos), 3000000.0f/sideIterations, 750.0f*sideIterations));
        }

        return mesh;
    }

    SpringMassMesh Triangle(sf::Vector2f startPos, float r, float massR) {
        SpringMassMesh mesh;
        mesh = UVSphere(startPos, r, massR, 3);
        return mesh;
    }

    SpringMassMesh Virus(sf::Vector2f startPos, float r, float massR) {
        SpringMassMesh mesh;
        mesh = Make::UVSphere(startPos, r, massR, 5);    
        return mesh;
    }
}



struct World {
    public:
        std::vector<SpringMassMesh> meshes;
        sf::RenderWindow& window;
        Bound& activeBound;
        float bounceEnergy = 1.0f;

        World(sf::RenderWindow& _activeRenderWindow, Bound& _activeBound) : window(_activeRenderWindow), activeBound(_activeBound) {}

        void renderMeshes() {
            for (SpringMassMesh& s : meshes) {
                for (Mass& m : s.masses) {
                    sf::CircleShape obj(m.r);
                    obj.setOrigin(m.r, m.r);
                    obj.setPosition(m.pos);

                    float accCol = len(m.acc);
                    accCol = abs(accCol) / 100.0f;
                    accCol = std::min(255.0f, accCol);
                    obj.setFillColor(sf::Color(accCol, 0, 255.0f-accCol, 255.0f));

                    window.draw(obj);
                }
            }
        }

        void renderMeshesSprings() {
            for (SpringMassMesh& m : meshes) {
                for (Spring& s : m.springs) {
                    float deltaLen = s.restLen - dist(s.m1.pos, s.m2.pos);
                    deltaLen = abs(deltaLen) * 50.0f;

                    deltaLen = std::min(deltaLen, 255.0f);
                    deltaLen = std::max(deltaLen, 30.0f);

                   Line line(s.m1.pos, s.m2.pos, sf::Color(255.0f-deltaLen, deltaLen, 0, deltaLen), 8.0f);
                   line.render(window);
                }
            }
        }

        void calculateSpringAcc() {
            for (SpringMassMesh& m : meshes)
                for (Spring& s : m.springs)
                    s.calculateMassAcc();
        }

        void applyExternalForce(sf::Vector2f force) {
            for (SpringMassMesh& s : meshes)
                for (Mass& m : s.masses)
                    m.acc += force;
        }

        void moveMasses() {
            for (SpringMassMesh& s : meshes)
                for (Mass& m : s.masses)
                    m.move();
        }

        void checkBounds() {
            for (SpringMassMesh& s : meshes) {
                for (Mass& m : s.masses) {
                    if (m.pos.x < activeBound.pos.x) {
                    m.pos.x = activeBound.pos.x;
                    m.changeVel({-bounceEnergy, 1});
                } else if (m.pos.y < activeBound.pos.y) {
                    m.pos.y = activeBound.pos.y;
                    m.changeVel({1, -bounceEnergy});
                }
                if (m.pos.x > activeBound.pos.x + activeBound.size.x) {
                    m.pos.x = activeBound.pos.x + activeBound.size.x;
                    m.changeVel({-bounceEnergy, 1});
                } else if (m.pos.y > activeBound.pos.y + activeBound.size.y) {
                    m.pos.y = activeBound.pos.y + activeBound.size.y;
                    m.changeVel({1, -bounceEnergy});
                }
                }
            }
        }

        void resetMassAcc() {
            for (SpringMassMesh& s : meshes)
                for (Mass& m : s.masses)
                    m.acc = sf::Vector2f(0, 0);
        }

        void doMouseInteraction(const sf::Vector2f& mousePos, const sf::Vector2f& moveV, float _mouseInteractionDistance) {
            for (SpringMassMesh& s : meshes) {
                for (Mass& m : s.masses) {
                    float distance = dist(m.pos, mousePos);
                    if (distance < _mouseInteractionDistance) {
                        m.acc += moveV;
                    }
                }
            }
        }

        void checkExternalCollision() {
            std::vector<Mass*> masses = {};
        
            for (SpringMassMesh& s : meshes) {
                for (Mass& m : s.masses) {
                    Mass* targetMass = &m;
                    masses.push_back(targetMass);
                }
            }
            
            for (int x = 0; x < masses.size(); ++x) {
                for (int i = x+1; i < masses.size(); ++i) {
                    if (i != x) {
                        sf::Vector2f deltaPos = masses[i]->pos - masses[x]->pos;
                        float distanceSqr = lenSqr(deltaPos);
                        float combR = (masses[i]->r + masses[x]->r);
                        if ((distanceSqr - combR*combR) > 0) continue;


                        float distance = sqrt(distanceSqr);
                        distance -= combR;
        
                        if (distance <= 0) {
                            sf::Vector2f dPosNorm = deltaPos / (distance + combR);
                            distance = abs(distance);

                            masses[i]->pos += dPosNorm * distance/2.0f;
                            masses[x]->pos -= dPosNorm * distance/2.0f;

                            masses[i]->acc += dPosNorm*distance*dt * 100000.0f;
                            masses[x]->acc -= dPosNorm*distance*dt * 100000.0f;
                        }
                    }
                }
            }
        }


        template<typename Function>
        void addMesh(Function createMesh) {
            meshes.emplace_back(createMesh());
        }
};




// Variables
bool leftDown = false;
bool gDwon = false;
bool gToggle = false;
bool rightDown = false;
sf::Vector2f mousePos;
sf::Vector2f lastMousePos(0, 0);




int main() {
    sf::RenderWindow window(sf::VideoMode(WIDTH, HEIGHT), "Mass-Spring System", sf::Style::Titlebar | sf::Style::Close);
    window.setFramerateLimit(MAX_FRAMES);
    sf::Event ev;
    sf::ContextSettings settings;
    sf::View view(sf::FloatRect(0, 0, WIDTH, HEIGHT));
    view.zoom(1.02f);
    window.setView(view);
    srand(time(0));


    Bound bound({0, 0}, {WIDTH, HEIGHT});
    World world(window, bound);


    world.addMesh([&](){return Make::Rectangle({WIDTH/5.0f*1.0f, HEIGHT/2.0f}, 8, 8, 50.0f, 20.0f);});
    world.addMesh([&](){return Make::Rectangle({WIDTH/5.0f*2.0f, HEIGHT/2.0f}, 5, 9, 40.0f, 15.0f);});
    world.addMesh([&](){return Make::Rectangle({WIDTH/5.0f*3.0f, HEIGHT/2.0f}, 3, 3, 70.0f, 30.0f);});
    world.addMesh([&](){return Make::UVSphere({WIDTH/5.0f*3.8f, HEIGHT/2.0f}, 150.0f, 20.0f, 20);});
    world.addMesh([&](){return Make::UVSphere({WIDTH/5.0f*4.7f, HEIGHT/2.0f}, 80.0f, 20.0f, 10);});
    world.addMesh([&](){return Make::Triangle({WIDTH/2.0f, 150}, 150.0f, 25.0f);});
    world.addMesh([&](){return Make::Virus({WIDTH/2.0f, HEIGHT-150}, 150.0f, 20.0f);});
    
    


    while (window.isOpen()) {
        while (window.pollEvent(ev)) {
            if (ev.type == sf::Event::Closed) {window.close(); break;}
        }
        lastMousePos = mousePos;
        mousePos = window.mapPixelToCoords(sf::Mouse::getPosition(window));

        // Interactions
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Escape)) window.close();

        if (sf::Keyboard::isKeyPressed(sf::Keyboard::G)) {
            if (!gDwon) {
                gToggle = !gToggle;
            }
            gDwon = true;
        } else {
            gDwon = false;
        }



        // Updates
        world.resetMassAcc();
        world.calculateSpringAcc();
        if (sf::Mouse::isButtonPressed(sf::Mouse::Left)) world.doMouseInteraction(mousePos, (mousePos - lastMousePos)/dt*5.0f, 100.0f);
        if (gToggle) world.applyExternalForce({0.0f, 500.0f});
        world.moveMasses();

        if (sf::Mouse::isButtonPressed(sf::Mouse::Right)) {
            rightDown = true;
        } else {
            rightDown = false;
        }

        if (rightDown) {
            SpringMassMesh mesh;
            mesh.masses.emplace_back(Mass(mousePos, 25.0f, {0, 0}, 1.0f));
            world.meshes.emplace_back(mesh);
        }

        for (int i = 0; i < 3; ++i) {
            world.checkExternalCollision();
            world.checkBounds();
        }


        // Rendering
        window.clear(sf::Color::Black);
        world.renderMeshesSprings();
        world.renderMeshes();
        window.display();

        if (rightDown) {
            world.meshes.pop_back();
        }
    }
    return 0;
}



// TODO:
// -shape matching
