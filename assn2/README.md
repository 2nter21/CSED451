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
/bin
    freeglut.dll
    glew32.dll
/include
    /GL
    /glm
/lib
    freeglut.lib
    glew32.lib
assn1.cpp
build.ps1
README.md
```

# Code Overview
* `initializeVA` : 정점 배열 initialize

* `Bullet struct` : 위치 저장 변수, 이동 방향 벡터 저장 변수, 탄막 속도 변수 등 다양한 변수

* `updateBullets` : bullet들 업데이트, 화면 밖의 bullet 삭제

* `Enemy` struct
    * `update` : 각종 초기화, 적을 점점 화면 아래쪽으로 이동시킴
    * `hitTest` : 충돌했는지 판별
    * `onHit` : 충돌 시 내부 처리
    * `draw` : 적 렌더링 (hierarchical animation)

* `enemies` : Vector<Enemy> 타입의 Enemy 컨테이너
* `spawnEnemy` : Vector 컨테이너에 새로운 Enemy 추가

* `draw` functions
    * `drawPlayer_` : 플레이어 모양 draw
    * `drawSquare` : 사각형 draw
    * `drawCircle` : 원 draw
    * `drawBoss` : 적 모양 draw
    * `drawPlayer` : 플레이어 오브젝트 draw
    * `drawBullets` : Bullet 오브젝트들 draw
    * `drawText` : 텍스트 draw
    * `drawPlayerOrbitingEntities` : 플레이어 주변 회전하는 엔티티들(남은 목숨 수) draw

* `display` : 모든 draw 함수 총괄

* `Collision` functions
    * rectCollision : 충돌 감지
    * handleCollisions : 충돌 시 내부 처리

* `Input` functions
    * processInput : 키 입력 총괄, 키 입력에 따라 플레이어 위치 update, 플레이어 bullet 발사 관리
    * handleKeyDown : 키다운 핸들링, 재시작 시 초기화
    * handleKeyUp : 키업 핸들링

* `timer` : Bullet, 리스폰 핸들링

* `main` : 초기값 초기화 및 설정, glutinit, glewinit, 각종 함수를 게임 플레이 도중 반복해서 실행되도록 설정

# Author
- Team Name: openGameLab
- Team Member #1: 안재영 / 20220019 / enter21
- Team Member #2: 정윤혁 / 20240385 / jyhyeok

