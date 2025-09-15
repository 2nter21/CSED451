#include <GL/glew.h>
#include <GL/freeglut.h>
#include <iostream>

// 사각형 위치
float playerX = 0.0f;
float playerY = 0.0f;

// 이동 속도
float moveSpeed = 0.05f;

// VBO와 VAO
GLuint vao, vbo;

// 사각형 정점 (2D)
float squareVertices[] = {
    -0.05f, -0.05f,
     0.05f, -0.05f,
     0.05f,  0.05f,
    -0.05f,  0.05f
};

// VBO 초기화
void initVBO() {
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);

    glBindVertexArray(vao);

    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(squareVertices), squareVertices, GL_STATIC_DRAW);

    // 위치 속성
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

// 사각형 그리기 (VBO 활용)
void drawSquare(float x, float y) {
    glBindVertexArray(vao);

    glPushMatrix();
    glTranslatef(x, y, 0.0f);

    glColor3f(0.0f, 1.0f, 0.0f);
    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

    glPopMatrix();
    glBindVertexArray(0);
}

// 화면 출력
void display() {
    glClear(GL_COLOR_BUFFER_BIT);

    drawSquare(playerX, playerY);

    glutSwapBuffers();
}

// 키 입력 처리
void handleKey(unsigned char key, int x, int y) {
    switch (key) {
    case 'w': playerY += moveSpeed; break;
    case 's': playerY -= moveSpeed; break;
    case 'a': playerX -= moveSpeed; break;
    case 'd': playerX += moveSpeed; break;
    }
    glutPostRedisplay();
}

int main(int argc, char** argv) {
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);
    glutInitWindowSize(600, 600);
    glutCreateWindow("VBO Demo");

    glewInit();

    // 초기화
    initVBO();

    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(-1, 1, -1, 1);

    glutDisplayFunc(display);
    glutKeyboardFunc(handleKey);

    glutMainLoop();
    return 0;
}
