#include <GL/glew.h>
#include <GL/freeglut.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <vector>
#include <cmath>
#include <algorithm>
#include <map>
#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h"
#define STB_IMAGE_IMPLEMENTATION
#include "std_image.h"

// Texture loader. returns LG texture id
unsigned int loadTexture(const char* path) {
    int width, height, nrChannels;
    stbi_set_flip_vertically_on_load(true);
    unsigned char *data = stbi_load(path, &width, &height, &nrChannels, 0);
    if (!data) {
        std::cerr << "Failed to load texture: " << path << std::endl;
        return 0;
    }

    GLenum format = GL_RGB;
    if (nrChannels == 1) format = GL_RED;
    else if (nrChannels == 3) format = GL_RGB;
    else if (nrChannels == 4) format = GL_RGBA;

    unsigned int tex;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);

    glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    stbi_image_free(data);
    return tex;
}

class Shader {
public:
    unsigned int ID; // shader program ID

    // constrictor : make shader
    Shader(const char* vertexPath, const char* fragmentPath) {
        // 1. read file
        std::string vertexCode;
        std::string fragmentCode;
        std::ifstream vShaderFile;
        std::ifstream fShaderFile;

        // error exception
        vShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
        fShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);

        try {
            // open file
            vShaderFile.open(vertexPath);
            fShaderFile.open(fragmentPath);
            std::stringstream vShaderStream, fShaderStream;

            // read buffer info as stream
            vShaderStream << vShaderFile.rdbuf();
            fShaderStream << fShaderFile.rdbuf();

            // close file
            vShaderFile.close();
            fShaderFile.close();

            // stream -> string
            vertexCode = vShaderStream.str();
            fragmentCode = fShaderStream.str();
        }
        catch (std::ifstream::failure& e) {
            std::cout << "ERROR::SHADER::FILE_NOT_SUCCESFULLY_READ: " << e.what() << std::endl;
        }

        const char* vShaderCode = vertexCode.c_str();
        const char* fShaderCode = fragmentCode.c_str();

        // 2. shader complie
        unsigned int vertex, fragment;

        // Vertex Shader
        vertex = glCreateShader(GL_VERTEX_SHADER);
        glShaderSource(vertex, 1, &vShaderCode, NULL);
        glCompileShader(vertex);
        checkCompileErrors(vertex, "VERTEX");

        // Fragment Shader
        fragment = glCreateShader(GL_FRAGMENT_SHADER);
        glShaderSource(fragment, 1, &fShaderCode, NULL);
        glCompileShader(fragment);
        checkCompileErrors(fragment, "FRAGMENT");

        // 3. shader program link (merge)
        ID = glCreateProgram();
        glAttachShader(ID, vertex);
        glAttachShader(ID, fragment);
        glLinkProgram(ID);
        checkCompileErrors(ID, "PROGRAM");

        // delete
        glDeleteShader(vertex);
        glDeleteShader(fragment);
    }

    // activate shader
    void use() {
        glUseProgram(ID);
    }

    // utility: find uniform variable and assign value
    void setMat4(const std::string& name, const GLfloat* value) const {
        glUniformMatrix4fv(glGetUniformLocation(ID, name.c_str()), 1, GL_FALSE, value);
    }

    void setVec3(const std::string& name, float x, float y, float z) const {
        glUniform3f(glGetUniformLocation(ID, name.c_str()), x, y, z);
    }

private:
    // error check
    void checkCompileErrors(unsigned int shader, std::string type) {
        int success;
        char infoLog[1024];
        if (type != "PROGRAM") {
            glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
            if (!success) {
                glGetShaderInfoLog(shader, 1024, NULL, infoLog);
                std::cout << "ERROR::SHADER_COMPILATION_ERROR of type: " << type << "\n" << infoLog << "\n -- --------------------------------------------------- -- " << std::endl;
            }
        }
        else {
            glGetProgramiv(shader, GL_LINK_STATUS, &success);
            if (!success) {
                glGetProgramInfoLog(shader, 1024, NULL, infoLog);
                std::cout << "ERROR::PROGRAM_LINKING_ERROR of type: " << type << "\n" << infoLog << "\n -- --------------------------------------------------- -- " << std::endl;
            }
        }
    }
};


class Model {
public:
    unsigned int VAO = 0;
    unsigned int VBO = 0;
    int vertexCount = 0;
    std::vector<float> vertices;

    Model() : vertexCount(0), VAO(0), VBO(0) {}

    // destructor
    ~Model() {
        if (VAO != 0) glDeleteVertexArrays(1, &VAO);
        if (VBO != 0) glDeleteBuffers(1, &VBO);
    }

    void load(const char* filename) {
        tinyobj::attrib_t attrib;
        std::vector<tinyobj::shape_t> shapes;
        std::vector<tinyobj::material_t> materials;
        std::string warn;
        std::string err;

        // 1. read file
        if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, filename)) {
            std::cerr << "Error: " << warn << err << std::endl;
            return;
        }

        vertices.clear();
        vertexCount = 0;

        for (const auto& shape : shapes) {
            for (const auto& index : shape.mesh.indices) {
                vertices.push_back(attrib.vertices[3 * index.vertex_index + 0]);
                vertices.push_back(attrib.vertices[3 * index.vertex_index + 1]);
                vertices.push_back(attrib.vertices[3 * index.vertex_index + 2]);
                vertexCount++;
            }
        }

        // 1. generate VAO, VBO
        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);

        // 2. VAO binding
        glBindVertexArray(VAO);

        // 3. VBO binding, copy data
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);

        // 4. attribute setting
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);

        // 5. unbinding
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);

        // vertices.clear();
    }

    void draw() {
        if (vertexCount == 0 || VAO == 0) return;

        glBindVertexArray(VAO); // load setting
        glDrawArrays(GL_TRIANGLES, 0, vertexCount); // draw
        glBindVertexArray(0);   // unbind
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
    void drawRecursive(glm::mat4 parentTransform, Shader* shader) {
        if (!isVisible) return;

        // 1. make Local Matrix (T * R * S)
        glm::mat4 localTransform = glm::mat4(1.0f);

        // translation
        localTransform = glm::translate(localTransform, glm::vec3(pos.x, pos.y, pos.z));

        // rotation
        localTransform = glm::rotate(localTransform, glm::radians(rot.x), glm::vec3(1.0f, 0.0f, 0.0f));
        localTransform = glm::rotate(localTransform, glm::radians(rot.y), glm::vec3(0.0f, 1.0f, 0.0f));
        localTransform = glm::rotate(localTransform, glm::radians(rot.z), glm::vec3(0.0f, 0.0f, 1.0f));

        // scaling
        localTransform = glm::scale(localTransform, glm::vec3(scale.x, scale.y, scale.z));

        // 2. final Global Matrix
        glm::mat4 globalTransform = parentTransform * localTransform;

        // 3. draw
        if (model != nullptr && shader != nullptr) {
            // send model matrix to shader
            shader->setMat4("model", glm::value_ptr(globalTransform));

            // send objectColor color to shader
            shader->setVec3("objectColor", color.x, color.y, color.z);

            // draw model (VAO)
            model->draw();
        }

        // for drawing bounding box
        if (customDrawFunc != nullptr) {
            // 1. send current node's matrix(globalTransform) to shader
            shader->setMat4("model", glm::value_ptr(globalTransform));

            // 2. send color
            shader->setVec3("objectColor", color.x, color.y, color.z);

            // 3. draw
            customDrawFunc();
        }

        // 4. recursive draw (maintain global matrix)
        for (Node* child : children) {
            child->drawRecursive(globalTransform, shader);
        }
    }

};

const float PI = 3.14159265358979323846f;

// Player related variables
float playerX = 0.0f;
float playerY = 0.0f;
int currentCameraView = 0; // 0: top view(perspective), 1: top view(orthographic), 2: third-person view
int currentGraphicStyle = 0;// 0: opaque polygon style, 1: wireframe style, 2: hidden line removal
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

// 3D models (Assuming these files exist in 'assets/' directory)
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

// Scene Graph Nodes
Node rootNode;
Node bboxNode;
Node playerNode;
Node playerModelNode;
Node orbitGroupNode;
Node enemiesGroupNode;
Node bulletsGroupNode;
Node particlesGroupNode;

const int MAX_ORBIENTITIES = 5;
const int MAX_ENEMIES = 3;
const int MAX_BULLETS = 500;
const int MAX_ENEMY_ORBITS = 4;
const int MAX_PARTICLES = 100;
std::vector<Node> orbitEntityNodePool;
std::vector<Node> enemyNodePool;
std::vector<Node> bulletNodePool;
std::vector<Node> enemyOrbitEntityNodePool;
std::vector<Node> particleNodePool;

Shader* myShader = nullptr;
glm::mat4 g_viewMatrix = glm::mat4(1.0f); // 1.0f : identity matrix
glm::mat4 g_projMatrix = glm::mat4(1.0f);
unsigned int bboxVAO = 0;
unsigned int bboxVBO = 0;

static inline float lerp(float a, float b, float t) {
    return a + (b - a) * t;
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
};

std::vector<Enemy> enemies;

void spawnEnemy(float x, float y, float size = 0.2f, int health = 5) {
    enemies.emplace_back(x, y, size, health);
}

// initialize bounding box data (only one call in main)
void initBoundingBox() {
    // 상자를 이루는 8개 점의 좌표 (Min: -1, Max: 1)

    float vertices[] = {
        // floor
        -1.0f, -1.0f, -1.0f,  1.0f, -1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,  1.0f,  1.0f, -1.0f,
         1.0f,  1.0f, -1.0f, -1.0f,  1.0f, -1.0f,
        -1.0f,  1.0f, -1.0f, -1.0f, -1.0f, -1.0f,

        // ceiling
        -1.0f, -1.0f,  1.0f,  1.0f, -1.0f,  1.0f,
         1.0f, -1.0f,  1.0f,  1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f, -1.0f,  1.0f,  1.0f,
        -1.0f,  1.0f,  1.0f, -1.0f, -1.0f,  1.0f,

        // pillars
        -1.0f, -1.0f, -1.0f, -1.0f, -1.0f,  1.0f,
         1.0f, -1.0f, -1.0f,  1.0f, -1.0f,  1.0f,
         1.0f,  1.0f, -1.0f,  1.0f,  1.0f,  1.0f,
        -1.0f,  1.0f, -1.0f, -1.0f,  1.0f,  1.0f
    };

    glGenVertexArrays(1, &bboxVAO);
    glGenBuffers(1, &bboxVBO);

    glBindVertexArray(bboxVAO);
    glBindBuffer(GL_ARRAY_BUFFER, bboxVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

void drawBoundingBox() {
    if (bboxVAO == 0) return;

    glBindVertexArray(bboxVAO);
    glDrawArrays(GL_LINES, 0, 24);
    glBindVertexArray(0);
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

        p.lifetime = PARTICLE_LIFETIME + ((rand() % 10) / 100.0f);
        p.color = Vec3(1.0f, 1.0f, 0.0f);

        particles.push_back(p);
    }
}

void boostParticleEffect(float playerX, float playerY) {
    int particlesNum = 1 + (rand() % 2);

    for(int i=0 ; i<particlesNum ; i++) {
        Particle p;

        p.pos.x = playerX + ((rand() % 100 / 50.0f) - 1.0f) * 0.03f;
        p.pos.y = playerY - 0.05f;
        p.pos.z = ((rand() % 100 / 50.0f) - 1.0f) * 0.01f;

        p.velocity.x = ((rand() % 100 / 50.0f) - 1.0f) * 0.1f;
        p.velocity.y = -0.5f - (rand() % 100 / 100.0f) * 0.3f;
        p.velocity.z = ((rand() % 100 / 50.0f) - 1.0f) * 0.1f;

        p.lifetime = 0.2f + (rand() % 100 / 100.0f) * 0.2f;

        p.color.x = 1.0f;
        p.color.y = 0.5f + (rand() % 100 / 100.0f) * 0.5f;
        p.color.z = 0.0f;

        particles.push_back(p);
    }
}

// Fuction for collision detection
bool rectCollision(float x1, float y1, float s1, float x2, float y2, float s2) {
    return std::abs(x1 - x2) < (s1 + s2) / 2 && std::abs(y1 - y2) < (s1 + s2) / 2;
}


void setCameraViews(int viewType, float pX, float pY) {
    float aspectRatio = 800.0f / 600.0f;
    // 1. Initialze
    g_projMatrix = glm::mat4(1.0f);
    g_viewMatrix = glm::mat4(1.0f);

    // 2. Projection Matrix calculate
    if (viewType == 1) { // Top View (Orthographic)
        g_projMatrix = glm::ortho(-1.2f * aspectRatio, 1.2f * aspectRatio, -1.2f, 1.2f, -10.0f, 10.0f);
    }
    else { // Perspective (Top or Third-person)
        g_projMatrix = glm::perspective(glm::radians(60.0f), aspectRatio, 0.1f, 100.0f);
    }

    // 3. View Matrix calculate 
    if (viewType == 0) { // Top View (Perspective)
        g_viewMatrix = glm::lookAt(glm::vec3(0.0f, 0.0f, 2.5f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    }
    else if (viewType == 1) { // Top View (Orthographic)
        g_viewMatrix = glm::lookAt(glm::vec3(0.0f, 0.0f, 2.5f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    }
    else if (viewType == 2) { // Third-person View
        g_viewMatrix = glm::lookAt(glm::vec3(pX, pY - 1.0f, 1.0f), glm::vec3(pX, pY, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    }
}

void setGraphicStyleThenRender() {
    glm::mat4 identityMatrix = glm::mat4(1.0f);

    // 0 = opaque polygon style
    if (currentGraphicStyle == 0) {
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        glDisable(GL_CULL_FACE); // Bbox is lines, models are 3D
        rootNode.drawRecursive(identityMatrix, myShader);
    }
    // 1 = wireframe style
    else if (currentGraphicStyle == 1) {
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        glDisable(GL_CULL_FACE);
        rootNode.drawRecursive(identityMatrix, myShader);
    }
    // 2 = wireframe style with hidden line removal
    else if (currentGraphicStyle == 2) {
        // 1. fill depth buffer
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        glEnable(GL_POLYGON_OFFSET_FILL);
        glPolygonOffset(1.0f, 1.0f);

        // ban color (transparency)
        glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);

        // draw (not in screen, only in depth buffer)
        glEnable(GL_CULL_FACE); // Use culling for better performance
        glCullFace(GL_BACK);
        rootNode.drawRecursive(identityMatrix, myShader);

        // recovering
        glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE); // allow color
        glDisable(GL_POLYGON_OFFSET_FILL);

        // 2. draw wireframe
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        glDisable(GL_CULL_FACE); // Disable culling for lines/bounding box
        rootNode.drawRecursive(identityMatrix, myShader);
    }

    // recover for next frame
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    glDisable(GL_CULL_FACE); // Default back to disabled
}

void updateSceneGraph() {

    // 1. player node update
    playerNode.pos.x = playerX;
    playerNode.pos.y = playerY;
    playerNode.isVisible = isPlayerAlive;

    // 2. plyer orbiting node pool update
    orbitGroupNode.children.clear();
    if (isPlayerAlive) {
        const float orbitRadius = 0.4f;
        float timeAngle = glutGet(GLUT_ELAPSED_TIME) * orbitSpeed;

        for (int i = 0; i < playerLives; ++i) {
            Node* orbitEntityNode = &orbitEntityNodePool[i % MAX_ORBIENTITIES]; // Use modulo for safety

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

    const Vec3 healthyColor(0.8f, 0.5f, 1.0f);
    const Vec3 damagedColor(1.0f, 0.2f, 0.2f);

    float time = glutGet(GLUT_ELAPSED_TIME) / 1000.0f;
    const float ENEMY_ORBIT_RADIUS = 0.5f;
    const float ENEMY_ORBIT_SPEED = 3.0f;

    for (const auto& enemy : enemies) {
        if (enemyNodeIndex >= MAX_ENEMIES) break;

        Node* enemyNode = &enemyNodePool[enemyNodeIndex];

        enemyNode->children.clear();

        enemyNode->isVisible = enemy.isAlive;
        enemyNode->pos.x = enemy.x;
        enemyNode->pos.y = enemy.y;
        enemyNode->rot.y = enemy.targetAngle * 180.0f / PI;

        float hpRatio = std::max(0.0f, (float)enemy.health / (float)enemy.maxHealth);
        enemyNode->color.x = lerp(damagedColor.x, healthyColor.x, hpRatio);
        enemyNode->color.y = lerp(damagedColor.y, healthyColor.y, hpRatio);
        enemyNode->color.z = lerp(damagedColor.z, healthyColor.z, hpRatio);

        // Enemy orbiting entities
        for(int i=0 ; i<MAX_ENEMY_ORBITS ; i++) {
            int sphereNodeIndex = enemyNodeIndex * MAX_ENEMY_ORBITS + i;
            if (sphereNodeIndex < enemyOrbitEntityNodePool.size()) {
                Node* sphereNode = &enemyOrbitEntityNodePool[sphereNodeIndex];

                float angle = time * ENEMY_ORBIT_SPEED + (i * (2.0f * PI / MAX_ENEMY_ORBITS));

                sphereNode->pos.x = cos(angle) * ENEMY_ORBIT_RADIUS;
                sphereNode->pos.y = sin(angle) * ENEMY_ORBIT_RADIUS;
                sphereNode->pos.z = 0.0f;

                enemyNode->children.push_back(sphereNode);
            }
        }

        enemiesGroupNode.children.push_back(enemyNode); // set child node
        enemyNodeIndex++;
    }


    // 4. bullets node pool update
    bulletsGroupNode.children.clear();
    int bulletNodeIndex = 0;

    for (const auto& bullet : bullets) {
        if (bulletNodeIndex >= MAX_BULLETS) break;

        if (bullet.isFromPlayer) {
            if (bulletNodeIndex + 1 >= MAX_BULLETS) break;

            // left bullet (Rice model)
            Node* bulletNodeL = &bulletNodePool[bulletNodeIndex++];
            bulletNodeL->isVisible = true;
            bulletNodeL->model = &riceModel;
            bulletNodeL->color = Vec3(1.0f, 1.0f, 0.8f);
            bulletNodeL->pos.x = bullet.x - 0.02f;
            bulletNodeL->pos.y = bullet.y;
            bulletNodeL->rot = Vec3(90.0f, 0.0f, -15.0f);
            bulletsGroupNode.children.push_back(bulletNodeL);

            // right bullet (Rice model)
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

    // 5. particles node pool update
    particlesGroupNode.children.clear();
    int particleNodeIndex = 0;

    for (const auto& p : particles) {
        if (particleNodeIndex >= MAX_PARTICLES) break;

        Node* particleNode = &particleNodePool[particleNodeIndex++];

        particleNode->isVisible = true;
        particleNode->model = &triangleModel;

        // apply particle color
        particleNode->color = p.color;

        particleNode->pos = p.pos;
        particleNode->scale = Vec3(0.02f * p.lifetime/0.4f, 0.02f * p.lifetime/0.4f, 0.02f * p.lifetime/0.4f); // Scale down as it fades

        // little rotation (visual)
        particleNode->rot.z += 10.0f;

        particlesGroupNode.children.push_back(particleNode);
    }
}


void display() {
    // 1. clear screen (color & depth buffer)
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // 2. calculate camera matrix (GLM)
    setCameraViews(currentCameraView, playerX, playerY);

    // 3. camera shake effect
    if (shakeTimer > 0) {
        float offsetX = ((rand() % 100) / 100.0f - 0.5f) * 2 * shakeManitude;
        float offsetY = ((rand() % 100) / 100.0f - 0.5f) * 2 * shakeManitude;
        g_viewMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(offsetX, offsetY, 0.0f)) * g_viewMatrix;

        shakeTimer--;
    }

    // 4. activate shader & send camera info
    if (myShader != nullptr) {
        myShader->use();

        // send View, Projection matrix to shader
        myShader->setMat4("view", glm::value_ptr(g_viewMatrix));
        myShader->setMat4("projection", glm::value_ptr(g_projMatrix));

        // 5. Scene Graph update & draw
        updateSceneGraph();
        setGraphicStyleThenRender();
    }

    // 6. Update Window Title (Replaces 2D UI Drawing)
    std::stringstream title;
    title << "Bullet Hell Shooter | Lives: " << playerLives;

    if (isGameOver) {
        title << " | GAME OVER (R: Restart)";
    } else if (isGameClear) {
        title << " | GAME CLEAR! (R: Restart)";
    } else {
        title << " | C: Camera (";
        if (currentCameraView == 0) title << "Top Persp";
        else if (currentCameraView == 1) title << "Top Ortho";
        else if (currentCameraView == 2) title << "Third-person";
        title << ")";

        title << " | Q: Style (";
        if (currentGraphicStyle == 0) title << "Opaque";
        else if (currentGraphicStyle == 1) title << "Wireframe";
        else if (currentGraphicStyle == 2) title << "Hidden Line";
        title << ")";
    }

    glutSetWindowTitle(title.str().c_str());

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
                        shakeTimer = 15; // Shake for 15 frames
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
    if (keyState['w']) {
        dy += 1.0f;
        boostParticleEffect(playerX, playerY);
    }
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

    if (!isGameOver && !isGameClear) {
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

    // Check for game clear condition after collisions/updates
    if (!isGameOver && enemies.empty()) {
        isGameClear = true;
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
        currentGraphicStyle = (currentGraphicStyle + 1) % 3;
    }

    // Reset condition
    if (key == 'r' || key == 'R') {
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
    
    initBoundingBox();
    // shader load
    myShader = new Shader("shader.vs", "shader.fs");

    // Assuming assets directory and files exist
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

    // load texture files
    unsigned int diffuseSonic1 = loadTexture("new_assets/diffuse_sonic_1.png");
    unsigned int diffuseSonic2 = loadTexture("new_assets/diffuse_sonic_2.png");
    unsigned int diffuseStarship = loadTexture("new_assets/diffuse_starship.png");
    unsigned int normalCobble = loadTexture("new_assets/normal_cobble.png");


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

    enemyOrbitEntityNodePool.resize(MAX_ENEMIES * MAX_ENEMY_ORBITS);
    for(int i=0 ; i<MAX_ENEMIES * MAX_ENEMY_ORBITS ; i++) {
        enemyOrbitEntityNodePool[i].model = &sphereModel;
        enemyOrbitEntityNodePool[i].scale = Vec3(0.05f, 0.05f, 0.05f);
        enemyOrbitEntityNodePool[i].color = Vec3(1.0f, 0.5f, 0.0f);
    }

    particleNodePool.resize(MAX_PARTICLES);
    for (int i = 0; i < MAX_PARTICLES; i++) {
        particleNodePool[i].model = &triangleModel;
    }

    // initialize enemies
    spawnEnemy( 0.0f,  0.6f, 0.12f, 10);
    spawnEnemy(-0.5f,  0.4f, 0.08f, 4);
    spawnEnemy( 0.6f,  0.45f,0.07f, 3);

    rootNode.children.push_back(&bboxNode);
    bboxNode.customDrawFunc = &drawBoundingBox;
    bboxNode.color = Vec3(1.0f, 1.0f, 0.0f);
    rootNode.children.push_back(&playerNode);
    playerNode.children.push_back(&playerModelNode);
    playerModelNode.model = &jetModel;
    playerModelNode.color = Vec3(0, 1, 0);
    playerModelNode.scale = Vec3(0.02f, 0.02f, 0.02f);
    playerModelNode.rot = Vec3(-90.0f, 0.0f, 0.0f);
    playerNode.children.push_back(&orbitGroupNode);
    rootNode.children.push_back(&enemiesGroupNode);
    rootNode.children.push_back(&bulletsGroupNode);
    rootNode.children.push_back(&particlesGroupNode);

    glutDisplayFunc(display);
    glutKeyboardFunc(handleKeyDown);
    glutKeyboardUpFunc(handleKeyUp);
    glutTimerFunc(0, timer, 0);

    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);

    glutMainLoop();
    return 0;
}