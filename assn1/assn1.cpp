#include <GL/glew.h>
#include <GL/freeglut.h>
#include <vector>
#include <cmath>
#include <algorithm>
#include <map>
#include <string>
#include <sstream>

// Player related variables
float playerX = 0.0f;
float playerY = 0.0f;
const float playerSize = 0.05f;
float moveSpeed = 0.05f;
int playerLives = 3;
bool isPlayerAlive = true;
int respawnTimer = 0; // 프레임 카운트
bool gameOver = false;

// Bullet structure
struct Bullet {
    float x, y;
    // x, y direction vector (normalized)
    float vx = 0.0f;
    float vy = 0.0f;
    float speed = 0.07f;
	bool fromPlayer = true; // Distinguish player, enemy bullet
};

// Enemy structure
struct Enemy {
    float x, y;
    float size;
    int health;
    int shootCooldown;
    bool alive;
};

// Store all bullets
std::vector<Bullet> bullets;

Enemy enemy;

// Handle key states
std::map<unsigned char, bool> keyState;

// 플레이어 발사 쿨다운 (프레임)
int playerFireCooldownMax = 10;
int playerFireCooldown = 0;

// Game constants
const int RESPAWN_FRAMES = 60; // Respawn after 60 frames
const int ENEMY_SHOOT_COOLDOWN = 50; // Enemy shoots every 50 frames
const float BULLET_SIZE = 0.01f;

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

// Drawing functions
void drawPlayer() {
    if (!isPlayerAlive) return;
    glColor3f(0.0f, 1.0f, 0.0f);

    glBegin(GL_TRIANGLES);
    glVertex2f(playerX - 0.5f * playerSize, playerY - std::sqrt(3.0f) / 6.0f * playerSize);
    glVertex2f(playerX + 0.5f * playerSize, playerY - std::sqrt(3.0f) / 6.0f * playerSize);
    glVertex2f(playerX, playerY + std::sqrt(3.0f) / 3.0f * playerSize);
    glEnd();
}

void drawEnemy() {
    if (!enemy.alive) return;
    glColor3f(0.6f, 0.2f, 0.8f);

    int segments = 36;
    glBegin(GL_TRIANGLE_FAN);
    glVertex2f(enemy.x, enemy.y);
    for (int i = 0; i <= segments; i++)
    {
        float angle = 2.0f * 3.14 * i / segments;
        float x = enemy.x + enemy.size * cos(angle);
        float y = enemy.y + enemy.size * sin(angle);
        glVertex2f(x, y);
    }
    glEnd();

    // 체력 바(간단)
    float barW = 0.2f;
    float barH = 0.02f;
    float hpRatio = std::max(0.0f, (float)enemy.health / 5.0f); // max hp 가 5라 가정
    // 백그라운드
    glColor3f(0.3f, 0.3f, 0.3f);
    glBegin(GL_QUADS);
    glVertex2f(enemy.x - barW / 2, enemy.y + enemy.size + 0.03f);
    glVertex2f(enemy.x + barW / 2, enemy.y + enemy.size + 0.03f);
    glVertex2f(enemy.x + barW / 2, enemy.y + enemy.size + 0.03f + barH);
    glVertex2f(enemy.x - barW / 2, enemy.y + enemy.size + 0.03f + barH);
    glEnd();
    // HP
    glColor3f(1.0f - (1.0f - hpRatio), hpRatio, 0.0f); // 빨강->초록 (단순)
    glBegin(GL_QUADS);
    glVertex2f(enemy.x - barW / 2, enemy.y + enemy.size + 0.03f);
    glVertex2f(enemy.x - barW / 2 + barW * hpRatio, enemy.y + enemy.size + 0.03f);
    glVertex2f(enemy.x - barW / 2 + barW * hpRatio, enemy.y + enemy.size + 0.03f + barH);
    glVertex2f(enemy.x - barW / 2, enemy.y + enemy.size + 0.03f + barH);
    glEnd();
}

void drawBullets() {
    for (auto& b : bullets) {
        // Player bullet : two yello rectangles
        if (b.fromPlayer)
        {
            glColor3f(1.0f, 1.0f, 0.0f);
            glBegin(GL_QUADS);
            glVertex2f(b.x - 1.25f * BULLET_SIZE, b.y - BULLET_SIZE);
            glVertex2f(b.x - 0.25f * BULLET_SIZE, b.y - BULLET_SIZE);
            glVertex2f(b.x - 0.25f * BULLET_SIZE, b.y + BULLET_SIZE);
            glVertex2f(b.x - 1.25f * BULLET_SIZE, b.y + BULLET_SIZE);
            glEnd();
            glBegin(GL_QUADS);
            glVertex2f(b.x + 1.25f * BULLET_SIZE, b.y - BULLET_SIZE);
            glVertex2f(b.x + 0.25f * BULLET_SIZE, b.y - BULLET_SIZE);
            glVertex2f(b.x + 0.25f * BULLET_SIZE, b.y + BULLET_SIZE);
            glVertex2f(b.x + 1.25f * BULLET_SIZE, b.y + BULLET_SIZE);
            glEnd();
        }

		// Enemy bullet : red circle
        else
        {
            glColor3f(1.0f, 0.0f, 0.0f);
            glBegin(GL_TRIANGLE_FAN);
            glVertex2f(b.x, b.y);
            for (int i = 0; i <= 36; i++)
            {
                float angle = 2.0f * 3.14 * i / 36;
                float x = b.x + BULLET_SIZE * 2 * cos(angle);
                float y = b.y + BULLET_SIZE * 2 * sin(angle);
                glVertex2f(x, y);
            }
            glEnd();
        }        
    }
}

void display() {
    glClear(GL_COLOR_BUFFER_BIT);

    drawPlayer();
    drawEnemy();
    drawBullets();

    std::stringstream ss;
    ss << "Lives: " << playerLives << "   Enemy HP: " << (enemy.alive ? enemy.health : 0);
    drawText(-0.98f, 0.95f, ss.str());

    if (gameOver) {
        drawText(-0.1f, 0.0f, "GAME OVER");
    }
    else if (!enemy.alive) {
        drawText(-0.12f, 0.0f, "ENEMY DESTROYED!");
    }

    glutSwapBuffers();
}

void spawnEnemyBullet() {
    if (!enemy.alive) return;

    // Calculate direction vector towards player
    float dx = playerX - enemy.x;
    float dy = playerY - enemy.y;
    float len = std::sqrt(dx * dx + dy * dy);

    Bullet b;
    b.x = enemy.x;
    b.y = enemy.y - (enemy.size + 0.02f);
    b.fromPlayer = false;
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
    if (enemy.alive) {
        for (auto it = bullets.begin(); it != bullets.end();) {
            if (it->fromPlayer) {
                if (rectCollision(it->x, it->y, BULLET_SIZE, enemy.x, enemy.y, enemy.size)) {
                    enemy.health -= 1;
                    it = bullets.erase(it);
                    if (enemy.health <= 0) {
                        enemy.alive = false;
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
            if (!it->fromPlayer) {
                if (rectCollision(it->x, it->y, BULLET_SIZE, playerX, playerY, playerSize)) {
                    playerLives--;
                    isPlayerAlive = false;
                    respawnTimer = RESPAWN_FRAMES;
                    it = bullets.erase(it);
                    if (playerLives <= 0) {
                        gameOver = true;
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
    if (gameOver) return;
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
        b.fromPlayer = true;
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
    if (!gameOver) {
        processInput();

        // Enemy bullet shooting considering cooldown
        if (enemy.alive) {
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
        if (!isPlayerAlive && !gameOver) {
            respawnTimer--;
            if (respawnTimer <= 0) {
                if (playerLives > 0) {
                    isPlayerAlive = true;
                    playerX = 0.0f;
                    playerY = -0.6f;
                }
                else {
                    gameOver = true;
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
        gameOver = false;
        playerX = 0.0f; playerY = -0.6f;
        enemy.alive = true;
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
    gameOver = false;

    enemy.x = 0.0f;
    enemy.y = 0.6f;
    enemy.size = 0.09f;
    enemy.health = 5;
    enemy.shootCooldown = 30;
    enemy.alive = true;

    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);
    glutInitWindowSize(800, 600);
    glutCreateWindow("Simple Game Assignment");

    glewInit();

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
