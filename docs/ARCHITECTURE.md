# RealWorld oatpp 아키텍처

이 문서는 realworld-oatpp 프로젝트의 내부 아키텍처, 설계 패턴 및 데이터 흐름을 설명합니다.

---

## 계층 아키텍처

```
HTTP 요청
    |
Controller (ENDPOINT_ASYNC) - src/controller/
    |
Service (비즈니스 로직) - src/service/
    |
DbClient (ORM 쿼리) - src/db/
    |
PostgreSQL
```

각 계층은 단일 책임을 갖습니다:

- **Controller**: HTTP 라우팅, 요청 파싱, 응답 직렬화. `ENDPOINT_ASYNC` 매크로로 정의됩니다.
- **Service**: 비즈니스 로직, 유효성 검사, 인증 확인, 데이터 변환.
- **DbClient**: oatpp-postgresql ORM을 통한 데이터베이스 쿼리. 각 도메인마다 독립된 클라이언트(UserDb, ArticleDb, CommentDb)가 있습니다.
- **PostgreSQL**: 커넥션 풀링을 통해 접근하는 영구 저장소.

---

## 의존성 주입 (DI)

프로젝트는 oatpp 내장 DI 컨테이너를 두 가지 핵심 매크로와 함께 사용합니다:

- `OATPP_CREATE_COMPONENT`: DI 컨테이너에 명명된 컴포넌트를 등록합니다.
- `OATPP_COMPONENT`: DI 컨테이너에서 컴포넌트를 주입(해결)합니다.

### 컴포넌트 등록

**AppComponent.hpp**가 핵심 인프라를 등록합니다:

| 컴포넌트 | 용도 |
|----------|------|
| Config | config.json에서 로드된 애플리케이션 설정 |
| Executor | 코루틴 기반 엔드포인트용 비동기 실행기 |
| ServerConnectionProvider | TCP 리스너 (Config에서 주소 + 포트) |
| HttpRouter | URL 라우팅 |
| ConnectionHandler | HTTP 연결 처리 |
| ObjectMapper | JSON 직렬화/역직렬화 |

**DatabaseComponent.hpp**가 데이터베이스 클라이언트를 등록합니다:

| 컴포넌트 | 용도 |
|----------|------|
| UserDb | 사용자 및 프로필 쿼리 |
| ArticleDb | 게시글 및 태그 쿼리 |
| CommentDb | 댓글 쿼리 |

각 데이터베이스 클라이언트는 다음 체인을 통해 생성됩니다: ConnectionPool → ConnectionProvider → Config (연결 문자열).

### 주입 패턴

- **Controller**는 생성자 매개변수를 통해 `ObjectMapper`를 받습니다. `Config`는 `OATPP_COMPONENT`를 사용하여 멤버 필드로 주입됩니다.
- **Service**는 데이터베이스 클라이언트(`UserDb`, `ArticleDb`, `CommentDb`)를 `OATPP_COMPONENT`를 사용하여 멤버 필드로 주입받습니다.

---

## 비동기 모델

모든 엔드포인트는 코루틴 기반 요청 핸들러를 정의하는 `ENDPOINT_ASYNC` 매크로를 사용합니다.

### 코루틴 패턴

엔드포인트는 2단계 코루틴 패턴을 따릅니다:

1. `act()` -- 진입점. 요청 본문이 있는 엔드포인트의 경우 비동기 본문 읽기를 시작하고 `act1()`으로 체이닝합니다.
2. `act1()` -- 파싱된 DTO를 받아 비즈니스 로직을 실행합니다.

요청 본문이 없는 엔드포인트(예: GET 요청)의 경우 모든 로직이 `act()`에서 실행됩니다.

### 실행기 생명주기

비동기 실행기는 특정 순서로 종료해야 합니다:

1. `waitTasksFinished()` -- 진행 중인 모든 코루틴이 완료될 때까지 대기합니다.
2. `stop()` -- 실행기가 새 작업 수락을 중단하도록 신호를 보냅니다.
3. `join()` -- 실행기 스레드가 종료될 때까지 대기합니다.

### 테스트 러너

`ClientServerTestRunner`는 테스트 중 비동기 생명주기를 관리하여 서버 시작, 테스트 실행, 정상 종료를 보장합니다.

---

## 설정 흐름

```
config.json
    |
Config 클래스 (src/utils/Config.hpp)
    |
OATPP_CREATE_COMPONENT (AppComponent.hpp에서 등록)
    |
OATPP_COMPONENT (필요한 곳에서 주입)
```

`Config` 클래스는 `config.json`을 파싱하고 모든 설정 값(데이터베이스 연결 문자열, JWT 시크릿, 서버 호스트/포트 등)에 대한 타입 안전 접근자를 제공합니다. DI 컴포넌트로 한 번 등록되어 애플리케이션 전체에서 주입됩니다.

**주의**: 설정 파일이 없거나 필수 섹션(thread/server/postgres)이 누락되면 `exit(1)`을 호출합니다.

---

## 인증 흐름

```
HTTP 요청
    |
Authorization 헤더 ("Token <jwt>")
    |
UserAuth::fromAuthHeader()
    |  "Token " 접두사를 제거하고 원시 JWT 문자열을 추출
    v
UserAuth::fromToken()
    |  Config의 시크릿 키로 HS256을 사용하여 JWT 디코딩
    v
UserAuth { exp, id, username }
```

인증 미들웨어는 없습니다. 인증이 필요한 각 엔드포인트가 `UserAuth::fromAuthHeader()`를 직접 호출하고 `Authorization` 헤더 값을 전달합니다. 토큰이 없거나 유효하지 않으면 엔드포인트가 오류를 반환합니다.

---

## 데이터 흐름 예시: 로그인

모든 계층을 거치는 로그인 흐름의 전체 추적:

```
POST /api/users/login
    |
UserController::Login (ENDPOINT_ASYNC)
    |
act() -> readBodyToDtoAsync<LoginUserDto>()
    |
act1()
    |  DTO에서 이메일과 비밀번호 추출
    v
UserService::login(email, password)
    |
UserDb::getByEmail(email)
    |  준비된 SQL 쿼리, 사용자 행 반환
    v
bcrypt_checkpw(password, storedHash)
    |  BCrypt 해시와 비밀번호 검증
    v
UserAuth::toToken(id, username, secret)
    |  60일 만료의 JWT 생성
    v
응답: { "user": { username, email, bio, image, token } }
```

---

## 데이터베이스

- **ORM**: oatpp-postgresql
- **쿼리 방식**: `PREPARE(true)`를 사용한 `QUERY` 매크로의 준비된 문(Prepared Statement)
- **커넥션 풀링**: `config.json`으로 구성된 풀 크기, `oatpp::postgresql::ConnectionPool`이 관리
- **스키마**: 세 개의 도메인별 DbClient 클래스(UserDb, ArticleDb, CommentDb)가 각자의 쿼리를 정의

---

## 주요 설계 결정

자세한 내용은 [ADR 문서](adr/)를 참조하세요.

### 헤더 전용 비즈니스 로직

모든 서비스 및 유틸리티 클래스가 `.hpp` 헤더 파일에만 구현되어 있습니다. 비즈니스 로직에 대응하는 `.cpp` 파일은 없습니다. 이는 빌드를 단순화하지만 대규모 변경 시 컴파일 시간이 증가합니다.

### 인증 미들웨어 없음

인증이 중앙 미들웨어 계층에서 처리되지 않습니다. 대신 인증이 필요한 각 엔드포인트가 명시적으로 `UserAuth::fromAuthHeader()`를 호출합니다. 이는 엔드포인트 수준에서 인증 요구사항을 가시적으로 만들지만 반복을 초래합니다.

### 서비스를 const 멤버로 선언

서비스 인스턴스가 컨트롤러 내부에서 `const` 멤버 변수로 생성됩니다. 컨트롤러 생성 시 한 번 만들어지고 컨트롤러 수명 동안 불변입니다.

### 테스트에 가상 네트워킹 사용

테스트 인프라(`TestComponent.hpp`에 정의)는 실제 TCP 소켓 대신 oatpp의 가상 네트워크 인터페이스를 사용합니다. 이를 통해 실제 포트 바인딩 없이 테스트를 실행하고 CI 환경에서 포트 충돌을 방지합니다.
