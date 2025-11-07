# CSED451
CSED451 Computer Graphics Assignment1 & 2

# Feature
- **Shooting game: Bullet Hell Shooter**
- Enemy : 일정 속도로 하강 / 플레이어 추적 회전 / HP 비례 외형 변화 / 계층적 꼬리 애니메이션
- Player : orbiting object를 통해 남은 목숨 표시

# How To Play
* 이동 : W, A, S, D
* 공격 : Space
* 재시작 : R
* 그래픽 스타일 변경 : Q
* 카메라 시점 변경 : C

# How To Build
1. Powershell을 사용, 저장소 루트로 이동
```
cd <project-directory>
```
2. Powershell 빌드 스크립트 실행
```
.\build.ps1
```
3. 실행 정책 제한 관련 오류 발생 시
```
Set-ExecutionPolicy RemoteSigned -Scope CurrentUser
```

# Project Structure
```
/assets
    -.obj
/bin
    freeglut.dll
    glew32.dll
/include
    /GL
    /glm
/lib
    freeglut.lib
    glew32.lib
code.cpp
build.ps1
README.md
```

# Code Overview
* `Modle` class : .obj 파일에서 데이터 load(), draw()로 glVertexPointer 방식으로 렌더링
* `Vec3` struct : x, y, z를 가지는 3D 벡터 구조체
* `Node` struct : 씬 그래프 기본 단위. 속성 및 model, children을 가지고 drawRecursive로 재귀적 draw
* `Bullet` struct : 게임 탄환 데이터. 위치, 속도, 적/플레이어 여부 저장
* `Particle` struct : 파티클 이펙트 데이터. 위치, 속도, 생존 시간, 색 저장
* `Enemy` struct : 적 데이터. 체력, 생존 여부 저장 및 update 함수로 관리.

* entity containers : `enemies`, `bullets`, `particles`
* Node Pools : `orbitEntityNodePool`, `enemyNodePool`, `bulletNodePool`, `enemyOrbitEntityNodePool`

* `processInput` : `ketState` 확인 및 이동 처리
* `updateBullets` : 

# Author
- Team Name: openGameLab
- Team Member #1: 안재영 / 20220019 / enter21
- Team Member #2: 정윤혁 / 20240385 / jyhyeok

