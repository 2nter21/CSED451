// final_game.cpp
// 컴파일 예: g++ final_game.cpp -lGL -lGLU -lglut -lGLEW -std=c++11

#include <GL/glew.h>
#include <GL/freeglut.h>
#include <vector>
#include <cmath>
#include <algorithm>
#include <map>
#include <string>
#include <sstream>

// ----- 플레이어 상태 -----
float playerX = 0.0f;
float playerY = 0.0f;
const float playerSize = 0.05f;
float moveSpeed = 0.05f;
int playerLives = 3;
bool isPlayerAlive = true;
int respawnTimer = 0; // 프레임 카운트
bool gameOver = false;

// ----- 총알 구조체 -----
struct Bullet {
    float x, y;
    float vx = 0.0f;
    float vy = 0.0f;
    float speed = 0.07f;
    bool fromPlayer = true; // true = 플레이어 총알, false = 적 총알
};

// ----- 적 구조체 -----
struct Enemy {
    float x, y;
    float size;
    int health;
    int shootCooldown; // 프레임 카운트(쿨다운)
    bool alive;
};

std::vector<Bullet> bullets;
Enemy enemy;

// 키 상태 저장
std::map<unsigned char, bool> keyState;

// 플레이어 발사 쿨다운 (프레임)
int playerFireCooldownMax = 10;
int playerFireCooldown = 0;

// 게임 상수
const int RESPAWN_FRAMES = 60; // 죽으면 60프레임 이후 리스폰
const int ENEMY_SHOOT_COOLDOWN = 50; // 적이 50프레임마다 발사
const float BULLET_SIZE = 0.01f;

// ----- 유틸 함수 -----
bool rectCollision(float x1, float y1, float s1, float x2, float y2, float s2) {
    return std::abs(x1 - x2) < (s1 + s2) && std::abs(y1 - y2) < (s1 + s2);
}

void drawText(float x, float y, const std::string& text) {
    glRasterPos2f(x, y);
    for (char c : text) {
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, c);
    }
}

// ----- 그리기 -----
void drawPlayer() {
    if (!isPlayerAlive) return;
    glColor3f(0.0f, 1.0f, 0.0f);
    glBegin(GL_QUADS);
    glVertex2f(playerX - playerSize, playerY - playerSize);
    glVertex2f(playerX + playerSize, playerY - playerSize);
    glVertex2f(playerX + playerSize, playerY + playerSize);
    glVertex2f(playerX - playerSize, playerY + playerSize);
    glEnd();
}

void drawEnemy() {
    if (!enemy.alive) return;
    glColor3f(0.6f, 0.2f, 0.8f);
    glBegin(GL_QUADS);
    glVertex2f(enemy.x - enemy.size, enemy.y - enemy.size);
    glVertex2f(enemy.x + enemy.size, enemy.y - enemy.size);
    glVertex2f(enemy.x + enemy.size, enemy.y + enemy.size);
    glVertex2f(enemy.x - enemy.size, enemy.y + enemy.size);
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
        if (b.fromPlayer) glColor3f(1.0f, 1.0f, 0.0f);
        else glColor3f(1.0f, 0.0f, 0.0f);
        glBegin(GL_QUADS);
        glVertex2f(b.x - BULLET_SIZE, b.y - BULLET_SIZE);
        glVertex2f(b.x + BULLET_SIZE, b.y - BULLET_SIZE);
        glVertex2f(b.x + BULLET_SIZE, b.y + BULLET_SIZE);
        glVertex2f(b.x - BULLET_SIZE, b.y + BULLET_SIZE);
        glEnd();
    }
}

void display() {
    glClear(GL_COLOR_BUFFER_BIT);

    // 오브젝트
    drawPlayer();
    drawEnemy();
    drawBullets();

    // UI 텍스트
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

// ----- 게임 로직 업데이트 -----
void spawnEnemyBullet() {
    if (!enemy.alive) return;
    // 단순: 적에서 플레이어 방향으로 발사
    float dx = playerX - enemy.x;
    float dy = playerY - enemy.y;
    float len = std::sqrt(dx * dx + dy * dy);
    Bullet b;
    b.x = enemy.x;
    b.y = enemy.y - (enemy.size + 0.02f); // 적 앞부분에서 발사
    b.fromPlayer = false;
    b.speed = 0.04f;
    if (len == 0) { b.vx = 0; b.vy = -1; }
    else { b.vx = dx / len; b.vy = dy / len; }
    bullets.push_back(b);
}

void updateBullets() {
    // 위치 업데이트 (벡터 기반)
    for (auto& b : bullets) {
        b.x += b.vx * b.speed;
        b.y += b.vy * b.speed;
    }

    // 화면 밖 총알 제거
    bullets.erase(
        std::remove_if(bullets.begin(), bullets.end(), [](const Bullet& b) {
            return b.x < -1.1f || b.x > 1.1f || b.y < -1.1f || b.y > 1.1f;
            }),
        bullets.end()
    );
}

void handleCollisions() {
    // 플레이어 총알 -> 적 충돌
    if (enemy.alive) {
        for (auto it = bullets.begin(); it != bullets.end();) {
            if (it->fromPlayer) {
                if (rectCollision(it->x, it->y, BULLET_SIZE, enemy.x, enemy.y, enemy.size)) {
                    // 적이 맞음
                    enemy.health -= 1; // 데미지 1
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

    // 적 총알 -> 플레이어 충돌
    if (isPlayerAlive) {
        for (auto it = bullets.begin(); it != bullets.end();) {
            if (!it->fromPlayer) {
                if (rectCollision(it->x, it->y, BULLET_SIZE, playerX, playerY, playerSize)) {
                    // 플레이어 히트
                    playerLives--;
                    isPlayerAlive = false;
                    respawnTimer = RESPAWN_FRAMES;
                    it = bullets.erase(it);
                    if (playerLives <= 0) {
                        gameOver = true;
                    }
                    break; // 한 번 맞으면 처리 끝
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
        // 경계 체크 (플레이어가 완전히 화면 밖으로 안 나가게)
        if (newX - playerSize > -1.0f && newX + playerSize < 1.0f) playerX = newX;
        if (newY - playerSize > -1.0f && newY + playerSize < 1.0f) playerY = newY;
    }

    // 발사 (스페이스)
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
        // 입력 처리
        processInput();

        // 적 행동: 쿨다운 감소 및 발사
        if (enemy.alive) {
            if (enemy.shootCooldown > 0) enemy.shootCooldown--;
            else {
                spawnEnemyBullet();
                enemy.shootCooldown = ENEMY_SHOOT_COOLDOWN;
            }
        }

        // 총알 업데이트 및 충돌
        updateBullets();
        handleCollisions();

        // 플레이어 리스폰 처리
        if (!isPlayerAlive && !gameOver) {
            respawnTimer--;
            if (respawnTimer <= 0) {
                if (playerLives > 0) {
                    isPlayerAlive = true;
                    // 리스폰 위치 안전하게 중앙(원하면 변경)
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
    glutTimerFunc(16, timer, 0); // 약 60FPS
}

// ----- 입력 -----
void handleKeyDown(unsigned char key, int x, int y) {
    keyState[key] = true;

    // 게임 오버 상태에서 R 누르면 재시작 (간단 기능)
    if (gameOver && (key == 'r' || key == 'R')) {
        // 리셋
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
    // 초기화: 플레이어, 적
    playerX = 0.0f; playerY = -0.6f;
    playerLives = 3;
    isPlayerAlive = true;
    gameOver = false;

    enemy.x = 0.0f;
    enemy.y = 0.6f;
    enemy.size = 0.09f;
    enemy.health = 5; // 예: 5HP
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
