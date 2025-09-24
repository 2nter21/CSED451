# CSED451
CSED451 Computer Graphics Assignment1

# Feature
Shooting game: Bullet Hell Shooters

# How To Play
* 이동 : W, A, S, D
* 공격 : Space
* 재시작 : R

# How To Build
Window Powershell에서 프로그램이 저장된 디렉토리로 이동(cd 명령어 이용)

.\build.ps1 명령어 입력

실행 정책 제한 관련 오류 발생 시, Set-ExecutionPolicy RemoteSigned -Scope CurrentUser 명령어 입력

# Code Composition
* initializeVA() : 정점 배열 initialize

* draw 함수
    * drawPlayer_ : 플레이어 모양 draw
    * drawSquare : 사각형 draw
    * drawCircle : 원 draw
    * drawBoss : 적 모양 draw

    * drawPlayer : 플레이어 오브젝트 draw
    * drawEnemy : 적 오브젝트 draw
    * drawBullets : Bullet 오브젝트들 draw
    * drawText : 텍스트 draw

    * display : 모든 draw 함수 총괄

* 충돌 관련 함수
    * rectCollision : 충돌 감지
    * handleCollisions : 충돌 시 내부 처리

* 키 입력 관련 함수
    * processInput : 키 입력 총괄
    * handleKeyDown : 키다운 핸들링
    * handleKeyUp : 키업 핸들링

* timer : Bullet, 리스폰 핸들링

* main : 초기값 초기화 및 설정, glutinit, glewinit, 각종 함수를 게임 플레이 도중 반복해서 실행되도록 설정
