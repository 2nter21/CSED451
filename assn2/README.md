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

* `spawnEnemy` : `enemies`에 새로운 Enemy 추가
* `bulletParticleEffect` : 플레이어 bullet이 적에게 충돌했을 때, `particles`에 새로운 파티클 추가
* `boostParticleEffect` : 플레이어가 w키 입력으로 앞으로 전진할 때, `particles`에 새로운 파티클 추가

* `processInput` : `ketState` 확인 및 이동 처리
* `handleKeyDown` : 키다운 핸들링
* `handleKeyUp` : 키업 핸들링
* `updateBullets` : `bullets` 업데이트, 화면 밖의 `bullet` 삭제
* `updateParticles` : `particles` 업데이트, 일정 시간 이상 유지된 `particle` 삭제

* `drawPlayer` : 플레이어 오브젝트(jetModel) draw
* `drawBullets` : Bullet 오브젝트들(sphereModel) draw
* `drawPlayerOrbitingEntities` : 플레이어 주변을 playerLives개의 회전하는 엔티티들(starModel) draw
* `drawBoundingBox` : 노란색 선으로 구분되는 정육면체 구역(플레이어 이동 가능 구역) draw
* `drawParticleEffect` : 파티클 draw
* `drawText` : 텍스트 draw

* `reactCollision` : 충돌 감지
* `handleCollisions` : 충돌 발생 시 내부 처리

* `setGraphicStyle` : 그래픽 스타일 변경
* `setCameraView` : 카메라 시점 변경
* `updateSceneGraph` : 씬 그래프 변경사항 업데이트

* `display` : 카메라 진동 효과, rootNode 호출하여 엔티티들 draw, 2D UI draw

* `timer` : 충돌 처리 총괄, `bullets`와 `particles` 업데이트, 플레이어 리스폰

* `main` : 변수 초기화 및 환경설정, glutinit, glewinit, 3D 모델 로드, Node Pool 초기화, 씬 그래프 설정, 각종 함수를 게임 플레이 도중 반복해서 실행되도록 설정
# Author
- Team Name: openGameLab
- Team Member #1: 안재영 / 20220019 / enter21
- Team Member #2: 정윤혁 / 20240385 / jyhyeok

