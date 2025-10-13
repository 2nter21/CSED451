#include <GL/glew.h>
#include <GL/freeglut.h>
#include <vector>
#include <cmath>
#include <algorithm>
#include <map>
#include <string>
#include <sstream>

const float PI = 3.14159265358979323846f;

// Player related variables
float playerX = 0.0f;
float playerY = 0.0f;
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

// Store all bullets
std::vector<Bullet> bullets;

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

// ------------------
// Vertex arrays
// ------------------
GLfloat playerVertices[30];
GLfloat squareVertices[8];
GLfloat circleVertices[2 * (1 + 36 + 1) /*center + 36 segments + back to first point*/];
GLfloat bossVertices[2*(1+36+10)];

// Initialize each centered at origin
void initializeVA() {

    // Main body
    playerVertices[0] = -0.05f; playerVertices[1] = -0.10f;
    playerVertices[2] =  0.05f; playerVertices[3] = -0.10f;
    playerVertices[4] =  0.05f; playerVertices[5] =  0.10f;
    playerVertices[6] = -0.05f; playerVertices[7] =  0.10f;

    // Top triangle
    playerVertices[8]  = -0.05f; playerVertices[9]  = 0.10f;
    playerVertices[10] =  0.05f; playerVertices[11] = 0.10f;
    playerVertices[12] =  0.0f;  playerVertices[13] = 0.20f;

    // Left engine
    playerVertices[14] = -0.10f; playerVertices[15] = -0.05f;
    playerVertices[16] = -0.05f; playerVertices[17] = -0.05f;
    playerVertices[18] = -0.05f; playerVertices[19] =  0.05f;
    playerVertices[20] = -0.10f; playerVertices[21] =  0.05f;

    // Right engine
    playerVertices[22] = 0.05f; playerVertices[23] = -0.05f;
    playerVertices[24] = 0.10f; playerVertices[25] = -0.05f;
    playerVertices[26] = 0.10f; playerVertices[27] =  0.05f;
    playerVertices[28] = 0.05f; playerVertices[29] =  0.05f;

    // ------------------

    // Square
    squareVertices[0] = -0.5f; squareVertices[1] = -0.5f;
    squareVertices[2] = 0.5f;  squareVertices[3] = -0.5f;
    squareVertices[4] = 0.5f;  squareVertices[5] = 0.5f;
    squareVertices[6] = -0.5f; squareVertices[7] = 0.5f;

    // Circle
    circleVertices[0] = 0.0f; circleVertices[1] = 0.0f;
    for (int i = 0; i < 36; ++i) {
        float angle = 2.0f * PI * i / 36;
        circleVertices[2 * (i + 1)] = cos(angle);
        circleVertices[2 * (i + 1) + 1] = sin(angle);
    }
    circleVertices[2 * (36 + 1)] = circleVertices[2];
    circleVertices[2 * (36 + 1) + 1] = circleVertices[3];

    // --- Boss: circle + star ---
    bossVertices[0] = 0.0f; bossVertices[1] = 0.0f;
    for(int i=0;i<36;i++){
        float angle = 2*PI*i/36;
        bossVertices[2*(i+1)] = cos(angle)*0.5f;
        bossVertices[2*(i+1)+1] = sin(angle)*0.5f;
    }
    float starOuter=1.0f, starInner=0.35f;
    for(int i=0;i<5;i++){
        float angleOuter = 2*PI*i/5 - PI/2;
        float angleInner = angleOuter + PI/5;
        bossVertices[2*(36+1 + i*2)]   = cos(angleOuter)*starOuter;
        bossVertices[2*(36+1 + i*2)+1] = sin(angleOuter)*starOuter;
        bossVertices[2*(36+1 + i*2+1)]   = cos(angleInner)*starInner;
        bossVertices[2*(36+1 + i*2+1)+1] = sin(angleInner)*starInner;
    }
}

// ------------------
// Basic drawing functions
// ------------------
void drawPlayer_(float size) {
    glEnableClientState(GL_VERTEX_ARRAY);
    glVertexPointer(2,GL_FLOAT,0,playerVertices);
    glPushMatrix();
    glScalef(size,size,1.0f);
    glDrawArrays(GL_QUADS,0,4);
    glDrawArrays(GL_TRIANGLES,4,3);
    glDrawArrays(GL_QUADS,7,4);
    glDrawArrays(GL_QUADS,11,4);
    glPopMatrix();
    glDisableClientState(GL_VERTEX_ARRAY);
}

void drawSquare(float width, float height = 0.0f) {
    if (height == 0.0f) height = width; // If height not specified, make it a square
    glEnableClientState(GL_VERTEX_ARRAY);
    glVertexPointer(2, GL_FLOAT, 0, squareVertices);
    glPushMatrix();
    glScalef(width, height, 1.0f);
    glDrawArrays(GL_QUADS, 0, 4);
    glPopMatrix();
    glDisableClientState(GL_VERTEX_ARRAY);
}

void drawCircle(float radius) {
    glEnableClientState(GL_VERTEX_ARRAY);
    glVertexPointer(2, GL_FLOAT, 0, circleVertices);
    glPushMatrix();
    glScalef(radius, radius, 1.0f);
    glDrawArrays(GL_TRIANGLE_FAN, 0, 36 + 2);
    glPopMatrix();
    glDisableClientState(GL_VERTEX_ARRAY);
}

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

void drawBoss(float baseSize, int health, int maxHealth, float tailPhase) {
    float hpRatio = std::max(0.0f, std::min(1.0f, (float)health / (float)maxHealth));
    float inv = 1.0f - hpRatio;

    float bodyScale = baseSize * lerp(0.85f, 1.05f, hpRatio);

    // body color changing by health decreasing
    float healthyR = 0.6f, healthyG = 0.2f, healthyB = 0.8f;
    float damagedR = 1.0f, damagedG = 0.15f, damagedB = 0.15f;
    float r,g,b;
    lerpColor(damagedR, damagedG, damagedB, healthyR, healthyG, healthyB, hpRatio, r, g, b);

    // body pulsing effect
    float pulse = 0.9f + 0.03f * sinf(tailPhase * 3.0f + inv * 6.0f);

    glColor3f(r, g, b);
    drawCircle(bodyScale * pulse);
    

    // inner spike parameters
    int spikeCount = 5;
    float outerBase = 1.0f + 0.20f * inv;
    float innerBase = 0.45f - 0.05f * inv;
    float spikeJitter = 0.08f * inv;
    float outerR = outerBase * bodyScale;
    float innerR = innerBase * bodyScale;

    // draw spikes
    for (int i = 0; i < spikeCount; ++i) {
        float a0 = (2.0f * PI * i) / spikeCount;
        float aMid = a0 + (PI / spikeCount);
        float a1 = a0 + (2.0f * PI / spikeCount);
        float jitter = spikeJitter * (sinf(tailPhase * 5.0f + i) * 0.5f + 0.5f);

        float oR = outerR * (1.0f + jitter);
        float iR = innerR * (1.0f - jitter * 0.5f);

        // outer vertex
        float tx = oR * cosf(aMid);
        float ty = oR * sinf(aMid);
        // two inner vertices
        float bx1 = iR * cosf(a0);
        float by1 = iR * sinf(a0);
        float bx2 = iR * cosf(a1);
        float by2 = iR * sinf(a1);
        
        glBegin(GL_TRIANGLES);
            glVertex2f(tx, ty);
            glVertex2f(bx1, by1);
            glVertex2f(bx2, by2);
        glEnd();
    }
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

        glPopMatrix();
        glPushMatrix();
        glTranslatef(x, y, 0.0f);
        glRotatef(targetAngle * 180.0f / PI, 0, 0, 1);

        // Body
        glColor3f(0.6f, 0.2f, 0.8f);
        drawBoss(size, health, maxHealth, tailPhase);

        // Cannon
        glPushMatrix();
            glTranslatef(0.0f, size * 1.3, 0.0f);
            float cannonW = size * 0.3f;
            float cannonH = size * 0.6f;
            glColor3f(1.0f, 0.05f, 0.05f);
            drawSquare(cannonW, cannonH);
        glPopMatrix();

        // Tail
        float tailBaseAngle = -0.5f  * PI;
        float baseRadius = size * 1.5f;
        float tailAngleOffset = tailAmplitude * sin(tailPhase);
        float tailAngle = tailBaseAngle + tailAngleOffset;

        glPushMatrix();
            glTranslatef(cosf(tailAngle) * baseRadius, sinf(tailAngle) * baseRadius, 0.0f);
            glRotatef((tailAngle + PI / 2) * 180.0f / PI, 0, 0, 1);
            glColor3f(0.8f, 0.5f, 0.2f);
            drawSquare(size * 0.3f, size);
            glPushMatrix();
                tailAngle = tailAngle * 3.0f;
                glTranslatef(0.0f, -0.5f * size, 0.0f);
                glRotatef((tailAngle + PI / 2) * 180.0f / PI, 0, 0, 1);
                glColor3f(0.8f, 0.5f, 0.2f);
                drawSquare(size * 0.3f, size);
            glPopMatrix();
        glPopMatrix();

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
    drawPlayer_(playerSize);
    glPopMatrix();
}



void drawBullets() {
    for (auto& b : bullets) {

        // Player bullet : two yellow rectangles
        if (b.isFromPlayer)
        {
            glColor3f(1.0f, 1.0f, 0.0f);
            glPushMatrix();
            glTranslatef(b.x - 0.75f * BULLET_SIZE, b.y, 0.0f);
            drawSquare(BULLET_SIZE, 2 * BULLET_SIZE);
            glPopMatrix();

            glPushMatrix();
            glTranslatef(b.x + 0.75f * BULLET_SIZE, b.y, 0.0f);
            drawSquare(BULLET_SIZE, 2 * BULLET_SIZE);
            glPopMatrix();
        }

		// Enemy bullet : red circle
        else
        {
            glPushMatrix();
            glTranslatef(b.x, b.y, 0.0f);            
            glColor3f(1.0f, 0.0f, 0.0f);
            drawCircle(BULLET_SIZE);
            glPopMatrix();
        }        
    }
}

void drawPlayerOrbitingEntities() {
    if (!isPlayerAlive) return;

    const float orbitRadius = 0.1f;
    const float entityRadius = 0.02f;

    for (int i = 0; i < playerLives; ++i) {
        float angle = orbitAngle + i * (2.0f * PI / playerLives) + glutGet(GLUT_ELAPSED_TIME) * orbitSpeed;

        float ex = playerX + orbitRadius * cos(angle);
        float ey = playerY + orbitRadius * sin(angle);

        glPushMatrix();
        glTranslatef(ex, ey, 0.0f);
        glColor3f(0.0f, 1.0f, 1.0f);
        drawCircle(entityRadius);
        glPopMatrix();
    }
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

void display() {
    glClear(GL_COLOR_BUFFER_BIT);

    // Camera shake effect
    glPushMatrix();
    if (shakeTimer > 0) {
        float offsetX = ((rand() % 100) / 100.0f - 0.5f) * 2 * shakeManitude;
        float offsetY = ((rand() % 100) / 100.0f - 0.5f) * 2 * shakeManitude;
        glTranslatef(offsetX, offsetY, 0.0f);
        shakeTimer--;
    }

    drawPlayer();
    drawPlayerOrbitingEntities();

    for(auto& e : enemies) e.draw();
    drawBullets();
    glPopMatrix();

    std::stringstream ss;
    ss << "Lives: " << playerLives;
    drawText(-0.98f, 0.95f, ss.str());

    if (isGameOver) {
        drawText(-0.1f, 0.0f, "GAME OVER");
    }
    else if (isGameClear) {
        drawText(-0.1f, 0.0f, "GAME CLEAR!");
    }


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

void handleCollisions() {
    // Player bullet collision with enemy
    for (auto it = bullets.begin(); it != bullets.end();) {
        if (!it->isFromPlayer) { ++it; continue; }
        bool erased = false;
        for (auto &e : enemies) {
            if (!e.isAlive) continue;
            if (e.hitTest(it->x, it->y, 0.01f)) {
                e.onHit(1);
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
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);
    glutInitWindowSize(800, 600);
    glutCreateWindow("Bullet Hell Shooter");

    glewInit();

    initializeVA(); // Initialize vertex arrays

    // initial enemies
    spawnEnemy( 0.0f,  0.6f, 0.12f, 10);
    spawnEnemy(-0.5f,  0.4f, 0.08f, 4);
    spawnEnemy( 0.6f,  0.45f,0.07f, 3);

    glutDisplayFunc(display);
    glutKeyboardFunc(handleKeyDown);
    glutKeyboardUpFunc(handleKeyUp);
    glutTimerFunc(0, timer, 0);

    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(-1, 1, -1, 1);

    glutMainLoop();
    return 0;
}
