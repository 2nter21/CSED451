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
const float playerSize = 0.05f;
float moveSpeed = 0.05f;
int playerLives = 3;
bool isPlayerAlive = true;
bool isGameOver = false;

// Bullet structure
struct Bullet {
    float x, y;
    // x, y direction vector (normalized)
    float vx = 0.0f;
    float vy = 0.0f;
    float speed = 0.07f;
	bool isFromPlayer = true; // Distinguish player, enemy bullet
};

// Enemy structure
struct Enemy {
    float x, y;
    float size;
    int health;
    int shootCooldown;
    bool isAlive;
};

// Store all bullets
std::vector<Bullet> bullets;

Enemy enemy;

// Handle key states
std::map<unsigned char, bool> keyState;

// Timer for player actions
int playerFireCooldownMax = 10;
int playerFireCooldown = 0;
int respawnTimer = 0;

// Game constants
const int RESPAWN_FRAMES = 60; // Respawn after 60 frames
const int ENEMY_SHOOT_COOLDOWN = 50; // Enemy shoots every 50 frames
const float BULLET_SIZE = 0.01f;

// ------------------
// Vertex arrays
// ------------------
GLfloat triangleVertices[6];
GLfloat squareVertices[8];
GLfloat circleVertices[2 * (1 + 36 + 1) /*center + 36 segments + back to first point*/];

// Initialize each centered at origin
void initializeVA() {

    // Triangle
    triangleVertices[0] = -0.5f; triangleVertices[1] = -std::sqrt(3.0f) / 6.0f;
    triangleVertices[2] = 0.5f;  triangleVertices[3] = -std::sqrt(3.0f) / 6.0f;
    triangleVertices[4] = 0.0f;  triangleVertices[5] = std::sqrt(3.0f) / 3.0f;

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
}

// ------------------
// Basic drawing functions
// ------------------
void drawTriangle(float size) {
    glEnableClientState(GL_VERTEX_ARRAY);
    glVertexPointer(2, GL_FLOAT, 0, triangleVertices);
    glPushMatrix();
    glScalef(size, size, 1.0f);
    glDrawArrays(GL_TRIANGLES, 0, 3);
    glPopMatrix();
    glDisableClientState(GL_VERTEX_ARRAY);
}

void drawSquare(float size) {
    glEnableClientState(GL_VERTEX_ARRAY);
    glVertexPointer(2, GL_FLOAT, 0, squareVertices);
    glPushMatrix();
    glScalef(size, size, 1.0f);
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

// ------------------
// Objects drawing functions

void drawPlayer() {
    if (!isPlayerAlive) return;

    glPushMatrix();
    glTranslatef(playerX, playerY, 0.0f);
    glColor3f(0.0f, 1.0f, 0.0f);
    drawTriangle(playerSize);
    glPopMatrix();
}

void drawEnemy() {
    if (!enemy.isAlive) return;

    glPushMatrix();
    glTranslatef(enemy.x, enemy.y, 0.0f);
    glColor3f(0.6f, 0.2f, 0.8f);
    drawCircle(enemy.size);
    glPopMatrix();    
    
    // HP bar
    float barW = 0.2f;
    float barH = 0.02f;
    float hpRatio = std::max(0.0f, (float)enemy.health / 5.0f);

    // Background
    glColor3f(0.3f, 0.3f, 0.3f);
    glBegin(GL_QUADS);
    glVertex2f(enemy.x - barW / 2, enemy.y + enemy.size + 0.03f);
    glVertex2f(enemy.x + barW / 2, enemy.y + enemy.size + 0.03f);
    glVertex2f(enemy.x + barW / 2, enemy.y + enemy.size + 0.03f + barH);
    glVertex2f(enemy.x - barW / 2, enemy.y + enemy.size + 0.03f + barH);
    glEnd();
    
    // Bar
    glColor3f(1.0f - (1.0f - hpRatio), hpRatio, 0.0f);
    glBegin(GL_QUADS);
    glVertex2f(enemy.x - barW / 2, enemy.y + enemy.size + 0.03f);
    glVertex2f(enemy.x - barW / 2 + barW * hpRatio, enemy.y + enemy.size + 0.03f);
    glVertex2f(enemy.x - barW / 2 + barW * hpRatio, enemy.y + enemy.size + 0.03f + barH);
    glVertex2f(enemy.x - barW / 2, enemy.y + enemy.size + 0.03f + barH);
    glEnd();
}

void drawBullets() {
    for (auto& b : bullets) {

        // Player bullet : two yellow rectangles
        if (b.isFromPlayer)
        {
            glColor3f(1.0f, 1.0f, 0.0f);
            glPushMatrix();
            glTranslatef(b.x - 0.75f * BULLET_SIZE, b.y, 0.0f);
            drawSquare(BULLET_SIZE);
            glPopMatrix();

            glPushMatrix();
            glTranslatef(b.x + 0.75f * BULLET_SIZE, b.y, 0.0f);
            drawSquare(BULLET_SIZE);
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
// ------------------

// Fuction for collision detection
bool rectCollision(float x1, float y1, float s1, float x2, float y2, float s2) {
    return std::abs(x1 - x2) < (s1 + s2) && std::abs(y1 - y2) < (s1 + s2);
}

void drawText(float x, float y, const std::string& text) {
    glRasterPos2f(x, y);
    for (char c : text) {
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, c);
    }
}

void display() {
    glClear(GL_COLOR_BUFFER_BIT);

    drawPlayer();
    drawEnemy();
    drawBullets();

    std::stringstream ss;
    ss << "Lives: " << playerLives << "   Enemy HP: " << (enemy.isAlive ? enemy.health : 0);
    drawText(-0.98f, 0.95f, ss.str());

    if (isGameOver) {
        drawText(-0.1f, 0.0f, "GAME OVER");
    }
    else if (!enemy.isAlive) {
        drawText(-0.12f, 0.0f, "ENEMY DESTROYED!");
    }

    glutSwapBuffers();
}

void spawnEnemyBullet() {
    if (!enemy.isAlive) return;

    // Calculate direction vector towards player
    float dx = playerX - enemy.x;
    float dy = playerY - enemy.y;
    float len = std::sqrt(dx * dx + dy * dy);

    Bullet b;
    b.x = enemy.x;
    b.y = enemy.y - (enemy.size + 0.02f);
    b.isFromPlayer = false;
    b.speed = 0.04f;
    if (len == 0) { b.vx = 0; b.vy = -1; }
    else { b.vx = dx / len; b.vy = dy / len; }
    bullets.push_back(b);
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
    if (enemy.isAlive) {
        for (auto it = bullets.begin(); it != bullets.end();) {
            if (it->isFromPlayer) {
                if (rectCollision(it->x, it->y, BULLET_SIZE, enemy.x, enemy.y, enemy.size)) {
                    enemy.health -= 1;
                    it = bullets.erase(it);
                    if (enemy.health <= 0) {
                        enemy.isAlive = false;
                    }
                }
                else ++it;
            }
            else ++it;
        }
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
                    break;
                }
                else ++it;
            }
            else ++it;
        }
    }
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
        if (newX - playerSize > -1.0f && newX + playerSize < 1.0f) playerX = newX;
        if (newY - playerSize > -1.0f && newY + playerSize < 1.0f) playerY = newY;
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
    if (!isGameOver) {
        processInput();

        // Enemy bullet shooting considering cooldown
        if (enemy.isAlive) {
            if (enemy.shootCooldown > 0) enemy.shootCooldown--;
            else {
                spawnEnemyBullet();
                enemy.shootCooldown = ENEMY_SHOOT_COOLDOWN;
            }
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
        playerLives = 3;
        isPlayerAlive = true;
        isGameOver = false;
        playerX = 0.0f; playerY = -0.6f;
        enemy.isAlive = true;
        enemy.health = 5;
        bullets.clear();
    }
}

void handleKeyUp(unsigned char key, int x, int y) {
    keyState[key] = false;
}

int main(int argc, char** argv) {
    // Player start position
    playerX = 0.0f; playerY = -0.6f;
    playerLives = 3;
    isPlayerAlive = true;
    isGameOver = false;

    enemy.x = 0.0f;
    enemy.y = 0.6f;
    enemy.size = 0.09f;
    enemy.health = 5;
    enemy.shootCooldown = 30;
    enemy.isAlive = true;

    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);
    glutInitWindowSize(800, 600);
    glutCreateWindow("ASSN 1");

    glewInit();

    initializeVA(); // Initialize vertex arrays

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
