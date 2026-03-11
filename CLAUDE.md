# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## 빌드 및 테스트

```bash
# 빌드 (프로젝트 루트에서)
mkdir build && cd build && cmake .. && make -j$(nproc)

# 단위/통합 테스트 실행 (PostgreSQL 실행 중 + 프로젝트 루트에 config.json 필요)
cd build && ./realworld-oatpp-test

# E2E 시나리오 테스트 (서버 실행 중이어야 함, jq 필요)
bash test/e2e/run-e2e.sh http://localhost:8002

# 정적 분석 (clang-tidy, 경고 보고만)
mkdir -p build && cd build && cmake -DCMAKE_EXPORT_COMPILE_COMMANDS=ON ..
find src/ -name '*.hpp' -o -name '*.cpp' | xargs clang-tidy -p build/ --quiet

# 변경 후 재빌드
cd build && make -j$(nproc)

# 데이터베이스 초기 설정
psql -f sql/schema.sql
cp config.json.txt config.json  # DB 자격증명 편집
```

oatpp 1.3.0 모듈 소스 설치: `./utility/install-oatpp-modules.sh`

시스템 의존성 (Ubuntu): `sudo apt install build-essential cmake zlib1g-dev libssl-dev nlohmann-json3-dev libpq-dev`

## 아키텍처

**계층 구조:** Controller (비동기 HTTP) → Service (비즈니스 로직) → DbClient (ORM) → PostgreSQL

**헤더 전용 패턴:** 모든 비즈니스 로직은 `.hpp` 파일에 있습니다. 컴파일되는 `.cpp` 파일은 `App.cpp`, `Statics.cpp`, `deunicode.cpp`, `MimeTypes.cpp`, 그리고 테스트 파일뿐입니다.

**의존성 주입:** oatpp는 매크로 기반 DI 시스템을 사용합니다:
- `OATPP_CREATE_COMPONENT(type, name)` 컴포넌트 등록 (`AppComponent.hpp`, `DatabaseComponent.hpp`에서)
- `OATPP_COMPONENT(type, name)` 컴포넌트 주입/해결
- 컴포넌트 초기화 순서가 중요함 — 클래스 본문에서 위에서 아래로 선언
- `DatabaseComponent` 멤버는 선언 순서에 의존하는 비람다 멤버 초기화를 사용 (config → ConnectionProvider → ConnectionPool → Executor → DbClients)

**비동기 엔드포인트:** Controller는 코루틴 패턴의 `ENDPOINT_ASYNC`를 사용합니다. 본문 읽기 엔드포인트는 `readBodyToDtoAsync().callbackTo()`를 통해 `act()` → `act1()`으로 체이닝합니다.

**인증 흐름:** `UserAuth::fromAuthHeader(request)`가 `Authorization` 헤더에서 "Token " 접두사를 제거하고, JWT(HS256, cpp-jwt)를 디코딩하여 `{exp, id, username}`을 반환합니다. `id == 0`이면 비인증 상태입니다. 각 엔드포인트가 수동으로 인증을 확인합니다 — 미들웨어 없음.

**설정:** `Config.hpp`가 시작 시 `config.json`을 읽습니다. **파일이 없거나 필수 섹션(thread/server/postgres)이 없으면 `exit(1)`을 호출합니다.** 실행 전 항상 config.json이 존재하는지 확인하세요.

## 핵심 DI 컴포넌트 그래프

```
AppComponent: Config → Executor → ServerConnectionProvider → HttpRouter → ConnectionHandler → ObjectMapper
DatabaseComponent: Config → ConnectionProvider → ConnectionPool → DbExecutor → UserDb, ArticleDb, CommentDb
Controller 주입: ObjectMapper (생성자), Config (멤버), Services (const 멤버)
Service 주입: Db 클라이언트 (멤버 OATPP_COMPONENT)
```

## 코드 규칙

- `CMakeLists.txt`의 `add_executable`에는 `.cpp` 파일만 — `.hpp` 절대 안 됨 (프로젝트 규칙, 36번째 줄)
- 인증 확인: `auto ua = UserAuth::fromAuthHeader(request); if (ua.id == 0) { /* 500 */ }`
- 오류 응답: `httpTools.hpp`의 `mkErrMsg("message")` 헬퍼
- `publicMode` 설정 플래그가 일부 엔드포인트의 인증을 완화 (GetArticle, NewComment, DelComments)
- `3rd/`에 벤더 라이브러리: cpp-jwt (JWT HS256), libbcrypt (비밀번호 해싱, cost 12)

## API (20개 엔드포인트)

**사용자 (8개):** POST 로그인, POST 회원가입, GET/PUT 현재 사용자, GET 전체 사용자, GET 프로필, POST/DELETE 팔로우

**게시글 (9개):** POST/PUT/DELETE CRUD, GET 목록 (쿼리: tag/author/favorited/offset/limit), GET 피드, GET 단일, POST/DELETE 즐겨찾기, GET 태그

**댓글 (3개):** GET 목록, POST 작성, DELETE ID로 삭제

## 데이터베이스

5개 PostgreSQL 테이블: `users`, `articles`, `favorites` (사용자↔게시글), `follows` (사용자↔사용자), `comments`. 스키마: `sql/schema.sql`.

## 테스트

테스트는 3단계로 구성됩니다:

**단위/통합 테스트 (UT/IT):** oatpp 가상 네트워킹(실제 TCP 없음) + 실제 PostgreSQL
- `UserControllerTest` (19개 테스트) — 회원가입, 로그인, 프로필, 팔로우 + 에러 경로 6개
- `ArticleControllerTest` (14개 테스트) — CRUD, 즐겨찾기, 태그 + 에러 경로 4개
- `CommentControllerTest` (5개 테스트) — CRUD + 엣지 케이스 2개
- `UserAuthTest` — JWT 인증 단위 테스트

**E2E 시나리오 테스트:** `test/e2e/run-e2e.sh` (Bash + curl + jq)
- 시나리오 1: 회원가입 → 로그인 → 프로필 (7개 assertions)
- 시나리오 2: 글쓰기 → 수정 → 즐겨찾기 → 댓글 → 삭제 (12개 assertions + 유효성 검사)

**정적 분석:** `.clang-tidy` 설정, CI에서 경고 보고만 (빌드 실패 아님)

테스트 인프라:
- `test/app/TestComponent.hpp` — 가상 네트워크 + Config용 DI
- `test/app/TestDatabaseComponent.hpp` — `DatabaseComponent.hpp`의 구조적 복제 (멤버 순서 반드시 일치)
- `test/app/MyApiTestClient.hpp` — 20개 엔드포인트 전체의 API_CALL 매크로
- 각 테스트 클래스는 고유한 테스트 데이터 사용 (예: `testuser_uc@test.com`) — 교차 간섭 방지
- 테스트 패턴: `TestComponent` + `TestDatabaseComponent` → `ClientServerTestRunner` → `addController` → `runner.run([]{...})` → 실행기 종료

## 제약 사항

- oatpp는 1.3.0으로 고정 — 업그레이드 금지
- CMake 최소 3.1, C++11 표준
- 명시적 요청 없이 기존 `src/` 코드 리팩토링 금지
- BaseController (정적 파일 서빙)는 테스트에서 제외 — API 컨트롤러만 테스트
