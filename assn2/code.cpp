#include <GL/glew.h>
#include <GL/freeglut.h>
#include <vector>
#include <cmath>
#include <algorithm>
#include <map>
#include <string>
#include <sstream>
#include <iostream>
#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h"

class Model {
public:
    std::vector<float> vertices;
    int vertexCount;

    Model() : vertexCount(0) {}

    void load(const char* filename) {
        tinyobj::attrib_t attrib;
        std::vector<tinyobj::shape_t> shapes;
        std::vector<tinyobj::material_t> materials;
        std::string warn;
        std::string err;

        if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, filename)) {
            std::cerr << "Error: " << warn << err << std::endl;
            return;
        }

        vertices.clear();
        vertexCount = 0;

        for (const auto& shape : shapes) {
            for (const auto& index : shape.mesh.indices) {
                vertices.push_back(
                    attrib.vertices[3 * index.vertex_index + 0]
                );
                vertices.push_back(
                    attrib.vertices[3 * index.vertex_index + 1]
                );
                vertices.push_back(
                    attrib.vertices[3 * index.vertex_index + 2]
                );

                vertexCount++;
            }
        }
    }

    void draw() {
        if (vertexCount == 0) return;

        glEnableClientState(GL_VERTEX_ARRAY);
        glVertexPointer(3, GL_FLOAT, 0, vertices.data());
        glDrawArrays(GL_TRIANGLES, 0, vertexCount);
        glDisableClientState(GL_VERTEX_ARRAY);
    }
};

struct Vec3 {
    float x = 0.0f, y = 0.0f, z = 0.0f;
    Vec3() {}
    Vec3(float x, float y, float z) : x(x), y(y), z(z) {}
};

struct Node {
    // vectors
    Vec3 pos;    // glTranslatef
    Vec3 rot;    // glRotatef
    Vec3 scale;  // glScalef
    Vec3 color;  // glColor3f

    // model to draw
    Model* model = nullptr;

    // child node
    std::vector<Node*> children;

    // another way to draw
    void (*customDrawFunc)() = nullptr;

    bool isVisible = true;

    // constructor
    Node() {
        pos = Vec3(0, 0, 0);
        rot = Vec3(0, 0, 0);
        scale = Vec3(1, 1, 1); // default scale
        color = Vec3(1, 1, 1); // default color: white
        model = nullptr;
        customDrawFunc = nullptr;
        isVisible = true;
    }

    // recursively draw
    void drawRecursive() {
        if (!isVisible) {
            return;
        }
        glPushMatrix();
        glTranslatef(pos.x, pos.y, pos.z);
        glRotatef(rot.x, 1.0f, 0.0f, 0.0f);
        glRotatef(rot.y, 0.0f, 1.0f, 0.0f);
        glRotatef(rot.z, 0.0f, 0.0f, 1.0f);
        glScalef(scale.x, scale.y, scale.z);

        if (model != nullptr) {
            glColor3f(color.x, color.y, color.z);
            model->draw();
        }

        if (customDrawFunc != nullptr) {
            customDrawFunc();
        }

        for (Node* child : children) {
            child->drawRecursive();
        }
        glPopMatrix();
    }
};

const float PI = 3.14159265358979323846f;

// Player related variables
float playerX = 0.0f;
float playerY = 0.0f;
int currentCameraView = 0; // 0: top view(perspective), 1: top view(orthographic), 2: third-person view
int currentGraphicStyle = 0;// 0: opaque polygon style, 1: wireframe style
const float playerSize = 0.3f;
float orbitAngle = 0.0f; // Angle for entities around player
float orbitSpeed = 0.005f;
float moveSpeed = 0.05f;
int playerLives = 3;
bool isPlayerAlive = true;
bool isGameOver = false;
bool isGameClear = false;

// Bullet structure
struct Bullet {
    float x, y;
    // x, y direction vector (normalized)
    float vx = 0.0f;
    float vy = 0.0f;
    float speed = 0.07f;
	bool isFromPlayer = true; // Distinguish player, enemy bullet
};

std::vector<Bullet> bullets;

// Bullet particle
struct Particle {
    Vec3 pos;
    Vec3 velocity;
    float lifetime;
    Vec3 color;
};

std::vector<Particle> particles;

// Handle key states
std::map<unsigned char, bool> keyState;

// Timer for player actions
int playerFireCooldownMax = 10;
int playerFireCooldown = 0;
int respawnTimer = 0;

// Game constants
const int RESPAWN_FRAMES = 60; // Respawn after 60 frames
const int ENEMY_SHOOT_COOLDOWN = 50; // Enemy shoots every 50 frames
const float BULLET_SIZE = 0.015f;

// Camera shake
int shakeTimer = 0;
float shakeManitude = 0.02f;


// 3D models
Model donutModel;
Model droneModel;
Model jetModel;
Model paperplaneModel;
Model riceModel;
Model sonicModel;
Model sphereModel;
Model squareModel;
Model starModel;
Model triangleModel;

Node rootNode;
Node bboxNode;
Node playerNode;
Node playerModelNode;
Node orbitGroupNode;
Node enemiesGroupNode;
Node bulletsGroupNode;



const int MAX_ORBIENTITIES = 5;
const int MAX_ENEMIES = 3;
const int MAX_BULLETS = 500;
std::vector<Node> orbitEntityNodePool;
std::vector<Node> enemyNodePool;
std::vector<Node> bulletNodePool;



static inline float lerp(float a, float b, float t) {
    return a + (b - a) * t;
}
static inline void lerpColor(float aR, float aG, float aB,
                             float bR, float bG, float bB,
                             float t, float &oR, float &oG, float &oB) {
    oR = lerp(aR, bR, t);
    oG = lerp(aG, bG, t);
    oB = lerp(aB, bB, t);
}

// Enemy structure
struct Enemy {
    // variables
    float x = 0, y = 0;
    float size = 0.2f;
    int health = 5;
    int maxHealth = 5;
    int shootCooldown = 30;
    bool isAlive = true;

    // animation
    float targetAngle = 0.0f;
    float tailPhase = 0.0f;
    float tailAmplitude = 0.4f; // in radians
    float tailSpeed = 2.0f;

    Enemy() {}
    Enemy(float x, float y, float size, int health)
        : x(x), y(y), size(size), health(health), maxHealth(health), shootCooldown(30), isAlive(true) {}

    void update(float dt, float playerX, float playerY, std::vector<Bullet>& bullets) {
        if(!isAlive) return;
        tailPhase += tailSpeed * dt;
        y -= 0.02f * dt; // Move down slowly

        float dx = playerX - x;
        float dy = playerY - y;
        targetAngle = atan2(dy, dx) - PI / 2; // Face towards player

        if (shootCooldown > 0) shootCooldown--;
        else {
            // Shoot a bullet towards player
            Bullet b;
            b.x = x;
            b.y = y - (size * 0.6f);
            b.isFromPlayer = false;
            float len = std::sqrt(dx * dx + dy * dy);
            b.vx = (dx / len);
            b.vy = (dy / len);
            b.speed = 0.03f;
            bullets.push_back(b);

            shootCooldown = ENEMY_SHOOT_COOLDOWN;
        }
    }

    bool hitTest(float bulletx, float bullety, float bulletSize) {
        float dx = bulletx - x;
        float dy = bullety - y;
        float distSq = dx * dx + dy * dy;
        float r = (size + bulletSize);
        return distSq < r * r;
    }

    void onHit(int damage) {
        health -= damage;
        if(health <= 0) {
            isAlive = false;
        } else {
            shakeTimer += 12;
        }
    }

    void draw() {
        if(!isAlive) return;
        glPushMatrix();
        glTranslatef(x, y, 0.0f);
        glRotatef(90.0f, 1.0f, 0.0f, 0.0f);
        glScalef(0.3f, 0.3f, 0.3f);
        glColor3f(0.8f, 0.5f, 1.0f);
        droneModel.draw();
        glPopMatrix();
    }
};

std::vector<Enemy> enemies;

void spawnEnemy(float x, float y, float size = 0.2f, int health = 5) {
    enemies.emplace_back(x, y, size, health);
}

// ------------------
// Objects drawing functions
// ------------------
void drawPlayer() {
    if (!isPlayerAlive) return;

    glPushMatrix();
    glTranslatef(playerX, playerY, 0.0f);
    glColor3f(0.0f, 1.0f, 0.0f);
    glScalef(0.02f, 0.02f, 0.02f);
    glRotatef(-90.0f, 1.0f, 0.0f, 0.0f);
    jetModel.draw();
    glPopMatrix();
}



void drawBullets() {
    for (auto& b : bullets) {

        // Player bullet : two yellow rectangles
        if (b.isFromPlayer)
        {

            glPushMatrix();
            glTranslatef(b.x - 0.75f * BULLET_SIZE, b.y, 0.0f);
            glColor3f(0.5f, 0.8f, 1.0f);
            glScalef(0.02f, 0.02f, 0.02f);
            sphereModel.draw();

            glPopMatrix();

            glPushMatrix();
            glTranslatef(b.x + 0.75f * BULLET_SIZE, b.y, 0.0f);
            glColor3f(0.5f, 0.8f, 1.0f);
            glScalef(0.02f, 0.02f, 0.02f);
            sphereModel.draw();

            glPopMatrix();
        }

		// Enemy bullet : red circle
        else
        {
            glPushMatrix();
            glTranslatef(b.x, b.y, 0.0f);            
            glColor3f(1.0f, 0.0f, 0.0f);
            glScalef(0.02f, 0.02f, 0.02f);
            sphereModel.draw();
            glPopMatrix();
        }        
    }
}

void drawPlayerOrbitingEntities() {
    if (!isPlayerAlive) return;

    const float orbitRadius = 0.4f;
    const float entityRadius = 0.04f;

    for (int i = 0; i < playerLives; ++i) {
        float angle = orbitAngle + i * (2.0f * PI / playerLives) + glutGet(GLUT_ELAPSED_TIME) * orbitSpeed;

        float ex = playerX + orbitRadius * cos(angle);
        float ey = playerY + orbitRadius * sin(angle);

        glPushMatrix();
        glTranslatef(ex, ey, 0.0f);
        glScalef(0.02f, 0.02f, 0.02f);
        glRotatef(glutGet(GLUT_ELAPSED_TIME) * 0.1f, 0.5f, 1.0f, 0.0f);
        glColor3f(0.0f, 1.0f, 1.0f);
        starModel.draw();
        glPopMatrix();
    }
}

void drawBoundingBox() {
    // bounding box scale
    float minX = -1.0f, maxX = 1.0f;
    float minY = -1.0f, maxY = 1.0f;
    float minZ = -1.0f, maxZ = 1.0f;

    glColor3f(1.0f, 1.0f, 0.0f); // color: yellow

    // line drawing
    glBegin(GL_LINES);

    // floor
    glVertex3f(minX, minY, minZ); glVertex3f(maxX, minY, minZ);
    glVertex3f(maxX, minY, minZ); glVertex3f(maxX, maxY, minZ);
    glVertex3f(maxX, maxY, minZ); glVertex3f(minX, maxY, minZ);
    glVertex3f(minX, maxY, minZ); glVertex3f(minX, minY, minZ);

    // ceiling
    glVertex3f(minX, minY, maxZ); glVertex3f(maxX, minY, maxZ);
    glVertex3f(maxX, minY, maxZ); glVertex3f(maxX, maxY, maxZ);
    glVertex3f(maxX, maxY, maxZ); glVertex3f(minX, maxY, maxZ);
    glVertex3f(minX, maxY, maxZ); glVertex3f(minX, minY, maxZ);

    // column
    glVertex3f(minX, minY, minZ); glVertex3f(minX, minY, maxZ);
    glVertex3f(maxX, minY, minZ); glVertex3f(maxX, minY, maxZ);
    glVertex3f(maxX, maxY, minZ); glVertex3f(maxX, maxY, maxZ);
    glVertex3f(minX, maxY, minZ); glVertex3f(minX, maxY, maxZ);

    glEnd();
}

void bulletParticleEffect(float x, float y) {
    const int NUM_PARTICLES = 8;
    const float SPREAD_SPEED = 0.3f;
    const float PARTICLE_LIFETIME = 0.2f;

    for (int i=0 ; i<NUM_PARTICLES ; i++) {
        float angle = (2.0f * PI * i) / NUM_PARTICLES;

        Particle p;
        p.pos = Vec3(x, y, 0.0f);

        p.velocity.x = cos(angle) * SPREAD_SPEED;
        p.velocity.y = sin(angle) * SPREAD_SPEED;
        p.velocity.z = 0.0f;

        p.lifetime = PARTICLE_LIFETIME;
        p.color = Vec3(1.0f, 1.0f, 0.0f);

        particles.push_back(p);
    }
}

void drawParticleEffect() {
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_POINT_SMOOTH);
    glPointSize(5.0f);
    glBegin(GL_POINTS);
    for(const auto& p : particles) {
        float lifeRatio = std::max(0.0f, p.lifetime / 0.2f);

        glColor4f(p.color.x, p.color.y, p.color.z, lifeRatio);
        glVertex3f(p.pos.x, p.pos.y, p.pos.z);
    }
    glEnd();
    glEnable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);
    glDisable(GL_POINT_SMOOTH);
}
// ------------------

// Fuction for collision detection
bool rectCollision(float x1, float y1, float s1, float x2, float y2, float s2) {
    return std::abs(x1 - x2) < (s1 + s2) / 2 && std::abs(y1 - y2) < (s1 + s2) / 2;
}

void drawText(float x, float y, const std::string& text) {
    glColor3f(1.0f, 1.0f, 1.0f);
    glRasterPos2f(x, y);
    for (char c : text) {
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, c);
    }
}

void setGraphicStyle(int style) {
    if (style == 0) {
        // 0 = opaque polygon style
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    }
    else {
        // 1 = wireframe style
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    }
}

void setCameraViews(int viewType, float pX, float pY) {
    // lens setting
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();

    float aspectRatio = 800.0f / 600.0f;

    switch (viewType) {
    case 0: // 0. top view(perspective)
        gluPerspective(60.0f, aspectRatio, 0.1f, 100.0f);
        break;
    case 1: // 1. top view(orthographic)
        glOrtho(-1.2f * aspectRatio, 1.2f * aspectRatio, -1.2f, 1.2f, -10.0f, 10.0f);
        break;
    case 2: // 2. third-person view
        gluPerspective(60.0f, aspectRatio, 0.1f, 100.0f);
        break;
    }

    // setting camera position
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    switch (viewType) {
    case 0: // 0. top view(perspective)
        gluLookAt(0.0f, 0.0f, 2.5f,
            0.0f, 0.0f, 0.0f,
            0.0f, 1.0f, 0.0f);
        break;
    case 1: // 1. top view(orthographic)
        gluLookAt(0.0f, 0.0f, 2.5f,
            0.0f, 0.0f, 0.0f,
            0.0f, 1.0f, 0.0f);
        break;
    case 2: // 2. third-person view
        gluLookAt(pX, pY - 2.5f, 2.0f,
            pX, pY, 0.0f,
            0.0f, 1.0f, 0.0f);
        break;
    }
}

void updateSceneGraph() {

    // 1. player node update
    playerNode.pos.x = playerX;
    playerNode.pos.y = playerY;
    playerNode.isVisible = isPlayerAlive;

    // 2. plyer orbiting node pool update
    orbitGroupNode.children.clear(); // ('자식'... ...'싹'... ...'비우기'!)
    if (isPlayerAlive) {
        const float orbitRadius = 0.4f;
        float timeAngle = glutGet(GLUT_ELAPSED_TIME) * orbitSpeed;

        for (int i = 0; i < playerLives; ++i) {
            Node* orbitEntityNode = &orbitEntityNodePool[i];

            float angle = orbitAngle + i * (2.0f * PI / playerLives) + timeAngle;
            orbitEntityNode->pos.x = orbitRadius * cos(angle);
            orbitEntityNode->pos.y = orbitRadius * sin(angle);
            orbitEntityNode->pos.z = 0.0f;
            orbitEntityNode->rot.z = glutGet(GLUT_ELAPSED_TIME) * 0.1f;

            orbitGroupNode.children.push_back(orbitEntityNode); // set child node
        }
    }

    // 3. enemies node pool update
    enemiesGroupNode.children.clear();
    int enemyNodeIndex = 0;

    for (const auto& enemy : enemies) {
        if (enemyNodeIndex >= MAX_ENEMIES) break;

        Node* enemyNode = &enemyNodePool[enemyNodeIndex++];

        enemyNode->isVisible = enemy.isAlive;
        enemyNode->pos.x = enemy.x;
        enemyNode->pos.y = enemy.y;
        enemyNode->rot.y = enemy.targetAngle * 180.0f / PI;

        enemiesGroupNode.children.push_back(enemyNode); // set child node
    }

    // 4. bullets node pool update
    bulletsGroupNode.children.clear();
    int bulletNodeIndex = 0;

    for (const auto& bullet : bullets) {
        if (bullet.isFromPlayer) {
            if (bulletNodeIndex + 1 >= MAX_BULLETS) break;

            // left bullet
            Node* bulletNodeL = &bulletNodePool[bulletNodeIndex++];
            bulletNodeL->isVisible = true;
            bulletNodeL->model = &riceModel;
            bulletNodeL->color = Vec3(1.0f, 1.0f, 0.8f);
            bulletNodeL->pos.x = bullet.x - 0.02f;
            bulletNodeL->pos.y = bullet.y;
            bulletNodeL->rot = Vec3(90.0f, 0.0f, -15.0f);
            bulletsGroupNode.children.push_back(bulletNodeL);

            // right bullet
            Node* bulletNodeR = &bulletNodePool[bulletNodeIndex++];
            bulletNodeR->isVisible = true;
            bulletNodeR->model = &riceModel;
            bulletNodeR->color = Vec3(1.0f, 1.0f, 0.8f);
            bulletNodeR->pos.x = bullet.x + 0.02f;
            bulletNodeR->pos.y = bullet.y;
            bulletNodeR->rot = Vec3(90.0f, 0.0f, 15.0f);
            bulletsGroupNode.children.push_back(bulletNodeR);

        }
        else {
            // enemy bullet: red sphere
            if (bulletNodeIndex >= MAX_BULLETS) break;

            Node* bulletNode = &bulletNodePool[bulletNodeIndex++];
            bulletNode->isVisible = true;
            bulletNode->model = &sphereModel;
            bulletNode->color = Vec3(1.0f, 0.3f, 0.3f);
            bulletNode->pos.x = bullet.x;
            bulletNode->pos.y = bullet.y;
            bulletNode->rot = Vec3(0, 0, 0);
            bulletsGroupNode.children.push_back(bulletNode);
        }
    }
}
void display() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    setCameraViews(currentCameraView, playerX, playerY);
    setGraphicStyle(currentGraphicStyle);

    // Camera shake effect
    glPushMatrix();
    if (shakeTimer > 0) {
        float offsetX = ((rand() % 100) / 100.0f - 0.5f) * 2 * shakeManitude;
        float offsetY = ((rand() % 100) / 100.0f - 0.5f) * 2 * shakeManitude;
        glTranslatef(offsetX, offsetY, 0.0f);
        shakeTimer--;
    }

    updateSceneGraph();
    rootNode.drawRecursive();
    drawParticleEffect();
    glPopMatrix();


    // drawing 2D UI
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    gluOrtho2D(-1, 1, -1, 1);
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();
    glDisable(GL_DEPTH_TEST);

    setGraphicStyle(0); // text: opaque polygon style

    std::stringstream ss;
    ss << "Lives: " << playerLives;
    drawText(-0.98f, 0.95f, ss.str());

    if (isGameOver) {
        drawText(-0.1f, 0.0f, "GAME OVER");
    }
    else if (isGameClear) {
        drawText(-0.1f, 0.0f, "GAME CLEAR!");
    }
    if (currentCameraView == 0) drawText(-0.98f, 0.90f, "Top View(Perspective)");
    if (currentCameraView == 1) drawText(-0.98f, 0.90f, "Top View(Orthographic)");
    if (currentCameraView == 2) drawText(-0.98f, 0.90f, "Third-person View");
    if (currentGraphicStyle == 0) drawText(-0.98f, 0.85f, "Opaque Polygon Style");
    if (currentGraphicStyle == 1) drawText(-0.98f, 0.85f, "Wireframe style");

    // rollback to 3D drawing
    glEnable(GL_DEPTH_TEST);
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();

    glutSwapBuffers();
}


void updateBullets() {
    // Update bullet positions
    for (auto& b : bullets) {
        b.x += b.vx * b.speed;
        b.y += b.vy * b.speed;
    }

    // Erase bullets out of window
    bullets.erase(
        std::remove_if(bullets.begin(), bullets.end(), [](const Bullet& b) {
            return b.x < -1.1f || b.x > 1.1f || b.y < -1.1f || b.y > 1.1f;
            }),
        bullets.end()
    );
}

void updateParticles(float dt) {
    for(auto p = particles.begin() ; p != particles.end() ; ) {
        p->pos.x += p->velocity.x * dt;
        p->pos.y += p->velocity.y * dt;
        p->pos.z += p->velocity.z * dt;
        p->lifetime -= dt;
        if(p->lifetime <= 0.0f)
            p = particles.erase(p);
        else
            ++p;
    }
}

void handleCollisions() {
    // Player bullet collision with enemy
    for (auto it = bullets.begin(); it != bullets.end();) {
        if (!it->isFromPlayer) { ++it; continue; }
        bool erased = false;
        for (auto &e : enemies) {
            if (!e.isAlive) continue;
            if (e.hitTest(it->x, it->y, 0.01f)) {
                e.onHit(1);
                bulletParticleEffect(it->x, it->y);
                it = bullets.erase(it);
                erased = true;
                break;
            }
        }
        if (!erased) ++it;
    }

    // Enemy bullet collision with player
    if (isPlayerAlive) {
        for (auto it = bullets.begin(); it != bullets.end();) {
            if (!it->isFromPlayer) {
                if (rectCollision(it->x, it->y, BULLET_SIZE, playerX, playerY, playerSize)) {
                    playerLives--;
                    isPlayerAlive = false;
                    respawnTimer = RESPAWN_FRAMES;
                    it = bullets.erase(it);
                    if (playerLives <= 0) {
                        isGameOver = true;
                    }
                    else
                    {
                        shakeTimer = 15; // Shake for 5 frames
                    }
                    break;
                }
                else ++it;
            }
            else ++it;
        }
    }

    // remove dead enemies
    enemies.erase(std::remove_if(enemies.begin(), enemies.end(), [](const Enemy &e) {
        return !e.isAlive;
    }), enemies.end());
}

void processInput() {
    if (isGameOver) return;
    if (!isPlayerAlive) return;

    float dx = 0.0f, dy = 0.0f;
    if (keyState['w']) dy += 1.0f;
    if (keyState['s']) dy -= 1.0f;
    if (keyState['a']) dx -= 1.0f;
    if (keyState['d']) dx += 1.0f;

    if (dx != 0.0f || dy != 0.0f) {
        float len = std::sqrt(dx * dx + dy * dy);
        dx /= len; dy /= len;
        float newX = playerX + dx * moveSpeed;
        float newY = playerY + dy * moveSpeed;
        // Limit player within widndow boundary
        if (newX - 0.1f * playerSize > -1.0f && newX + 0.1f * playerSize < 1.0f) playerX = newX;
        if (newY - 0.2f * playerSize > -1.0f && newY + 0.2f * playerSize < 1.0f) playerY = newY;
    }

    // Player bullet shooting
    if (playerFireCooldown > 0) playerFireCooldown--;
    if (keyState[' '] && playerFireCooldown == 0) {
        Bullet b;
        b.isFromPlayer = true;
        b.x = playerX;
        b.y = playerY + (playerSize + 0.01f);
        b.vx = 0.0f;
        b.vy = 1.0f;
        b.speed = 0.08f;
        bullets.push_back(b);
        playerFireCooldown = playerFireCooldownMax;
    }
}

void timer(int value) {
    const float dt = 1.0f / 60.0f; // 60 FPS

    if (!isGameOver) {
        processInput();

        // update enemies
        for (auto &e : enemies) {
            e.update(dt, playerX, playerY, bullets);
        }

        // Bullet handling
        updateBullets();
        updateParticles(dt);
        handleCollisions();

        // Player respawn
        if (!isPlayerAlive && !isGameOver) {
            respawnTimer--;
            if (respawnTimer <= 0) {
                if (playerLives > 0) {
                    isPlayerAlive = true;
                    playerX = 0.0f;
                    playerY = -0.6f;
                }
                else {
                    isGameOver = true;
                }
            }
        }
    }

    glutPostRedisplay();
    glutTimerFunc(16, timer, 0); // 60 FPS
}

// Handle key input
void handleKeyDown(unsigned char key, int x, int y) {
    keyState[key] = true;

    if (key == 'c' || key == 'C') {
        currentCameraView = (currentCameraView + 1) % 3; 
    }

    if (key == 'q' || key == 'Q') {
        currentGraphicStyle = (currentGraphicStyle + 1) % 2;
    }

    // Reset condition
    if ((key == 'r' || key == 'R')) {
        playerLives = 5;
        isPlayerAlive = true;
        isGameOver = false;
        isGameClear = false;
        playerX = 0.0f; playerY = -0.6f;
        enemies.clear();
        bullets.clear();
        spawnEnemy( 0.0f,  0.6f, 0.12f, 10);
        spawnEnemy(-0.5f,  0.4f, 0.08f, 4);
        spawnEnemy( 0.6f,  0.45f,0.07f, 3);
    }
    else if (enemies.size() == 0 && !isGameOver) { isGameClear = true; }
}

void handleKeyUp(unsigned char key, int x, int y) {
    keyState[key] = false;
}

int main(int argc, char** argv) {
    // Player start position
    playerX = 0.0f; playerY = -0.6f;
    playerLives = 5;
    isPlayerAlive = true;
    isGameOver = false;
    isGameClear = false;

    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    glutInitWindowSize(800, 600);
    glutCreateWindow("Bullet Hell Shooter");

    glewInit();

    donutModel.load("assets/donut.obj");
    droneModel.load("assets/drone.obj");
    jetModel.load("assets/jet.obj");
    paperplaneModel.load("assets/paperplane.obj");
    riceModel.load("assets/rice.obj");
    sonicModel.load("assets/sonic.obj");
    sphereModel.load("assets/sphere.obj");
    squareModel.load("assets/square.obj");
    starModel.load("assets/star.obj");
    triangleModel.load("assets/triangle.obj");

    /*initializeVA();*/ // Initialize vertex arrays 

    // intialize node pool
    orbitEntityNodePool.resize(MAX_ORBIENTITIES);
    for(int i = 0; i < MAX_ORBIENTITIES; i++) {
        orbitEntityNodePool[i].model = &starModel;
        orbitEntityNodePool[i].scale = Vec3(0.04f, 0.04f, 0.04f);
        orbitEntityNodePool[i].color = Vec3(1.0f, 0.0f, 1.0f);
    }
    
    enemyNodePool.resize(MAX_ENEMIES);
    for(int i = 0; i < MAX_ENEMIES; i++) {
        enemyNodePool[i].model = &droneModel; 
        enemyNodePool[i].scale = Vec3(0.3f, 0.3f, 0.3f); 
        enemyNodePool[i].color = Vec3(0.8f, 0.5f, 1.0f); 
        enemyNodePool[i].rot.x = 90.0f; 
    }

    bulletNodePool.resize(MAX_BULLETS);
    for(int i = 0; i < MAX_BULLETS; i++) {
        bulletNodePool[i].scale = Vec3(0.02f, 0.02f, 0.02f); 
    }
    // initial enemies
    spawnEnemy( 0.0f,  0.6f, 0.12f, 10);
    spawnEnemy(-0.5f,  0.4f, 0.08f, 4);
    spawnEnemy( 0.6f,  0.45f,0.07f, 3);

    rootNode.children.push_back(&bboxNode);
    bboxNode.customDrawFunc = &drawBoundingBox;
    rootNode.children.push_back(&playerNode);
    playerNode.children.push_back(&playerModelNode);
    playerModelNode.model = &jetModel;
    playerModelNode.color = Vec3(0, 1, 0);
    playerModelNode.scale = Vec3(0.02f, 0.02f, 0.02f);
    playerModelNode.rot = Vec3(-90.0f, 0.0f, 0.0f);
    playerNode.children.push_back(&orbitGroupNode);
    rootNode.children.push_back(&enemiesGroupNode);
    rootNode.children.push_back(&bulletsGroupNode);

    glutDisplayFunc(display);
    glutKeyboardFunc(handleKeyDown);
    glutKeyboardUpFunc(handleKeyUp);
    glutTimerFunc(0, timer, 0);

    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    /*
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(-1, 1, -1, 1);
    */
    glEnable(GL_DEPTH_TEST);

    glutMainLoop();
    return 0;
}
