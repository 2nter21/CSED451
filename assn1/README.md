# CSED451
CSED451 Computer Graphics Assignment1 & 2

# Feature
Shooting game: Bullet Hell Shooter

# How To Play
* 이동 : W, A, S, D
* 공격 : Space
* 재시작 : R

# How To Build
Window Powershell에서 프로그램이 저장된 디렉토리로 이동(cd 명령어 이용)

.\build.ps1 명령어 입력

실행 정책 제한 관련 오류 발생 시, Set-ExecutionPolicy RemoteSigned -Scope CurrentUser 명령어 입력

# Code Composition
* initializeVA : 정점 배열 initialize

* Bullet struct
    * 위치 저장 변수, 이동 방향 벡터 저장 변수, 탄막 속도 변수 등 다양한 변수
* updateBullets : bullet들 업데이트, 화면 밖의 bullet 삭제

* Enemy struct
    * 생성자
    * update : 각종 초기화, 적을 점점 화면 아래쪽으로 이동시킴
    * hitTest : 충돌했는지 판별
    * onHit : 충돌 시 내부 처리
    * draw : enemy draw (hierarchical animation)
* spawnEnemy : 적 세 기 스폰

* draw 함수
    * drawPlayer_ : 플레이어 모양 draw
    * drawSquare : 사각형 draw
    * drawCircle : 원 draw
    * drawBoss : 적 모양 draw
    * drawEnemyBodyParametric : 적 몸통 parametric하게 draw

    * drawPlayer : 플레이어 오브젝트 draw
    * drawBullets : Bullet 오브젝트들 draw
    * drawText : 텍스트 draw
    * drawPlayerOrbitingEntities : 플레이어 주변 회전하는 엔티티들(플레이어 life와 개수 동일) draw

    * display : 모든 draw 함수 총괄

* 충돌 관련 함수
    * rectCollision : 충돌 감지
    * handleCollisions : 충돌 시 내부 처리

* 키 입력 관련 함수
    * processInput : 키 입력 총괄, 키 입력에 따라 플레이어 위치 update, 플레이어 bullet 발사 관리
    * handleKeyDown : 키다운 핸들링, 재시작 시 초기화
    * handleKeyUp : 키업 핸들링

* timer : Bullet, 리스폰 핸들링

* main : 초기값 초기화 및 설정, glutinit, glewinit, 각종 함수를 게임 플레이 도중 반복해서 실행되도록 설정
