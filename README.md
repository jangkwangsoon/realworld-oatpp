# realworld-oatpp

![CI](https://github.com/formoon/realworld-oatpp/actions/workflows/ci.yml/badge.svg)

C++과 oatpp 웹 프레임워크를 사용한 RealWorld (Conduit) 백엔드 API 구현체. PostgreSQL 사용.

## 빠른 시작

1. **사전 요구사항**: 시스템 의존성 설치
   ```bash
   # Ubuntu
   sudo apt install build-essential cmake git zlib1g-dev libssl-dev nlohmann-json3-dev libpq-dev postgresql-client
   # macOS
   brew install libpq nlohmann-json
   ```

2. **oatpp 모듈 설치** (1.3.0):
   ```bash
   ./utility/install-oatpp-modules.sh
   ```

3. **PostgreSQL 설정**:
   ```bash
   psql -f sql/schema.sql
   cp config.json.txt config.json  # DB 자격증명을 편집하세요
   ```

4. **빌드 및 실행**:
   ```bash
   mkdir build && cd build && cmake .. && make -j$(nproc)
   cd .. && ./build/realworld-oatpp
   ```

## 테스트 실행

스키마가 로드된 PostgreSQL이 실행 중이고 유효한 `config.json`이 필요합니다.

```bash
# 단위/통합 테스트 (38개 + UserAuth)
cd build && ./realworld-oatpp-test

# E2E 시나리오 테스트 (서버 실행 중이어야 함, jq 필요)
bash test/e2e/run-e2e.sh http://localhost:8002

# 정적 분석
find src/ -name '*.hpp' -o -name '*.cpp' | xargs clang-tidy -p build/ --quiet
```

CI 파이프라인 (`.github/workflows/ci.yml`)이 자동으로 lint → build → UT/IT → E2E를 실행합니다.

## 프로젝트 구조

```
src/
├── App.cpp              # 진입점
├── AppComponent.hpp     # HTTP 서버 DI 설정
├── DatabaseComponent.hpp # PostgreSQL DI 설정
├── controller/          # API 컨트롤러 (User, Article, Comment, Base)
├── service/             # 비즈니스 로직
├── db/                  # 데이터베이스 쿼리 (oatpp-postgresql ORM)
├── dto/                 # 데이터 전송 객체
└── utils/               # 설정, JWT 인증, HTTP 도우미
test/                    # 통합 + 단위 테스트
3rd/                     # 벤더 라이브러리: cpp-jwt, libbcrypt
sql/                     # 데이터베이스 스키마
docs/                    # API 및 아키텍처 문서
```

## API 개요

| 메서드 | 경로 | 인증 | 설명 |
|--------|------|------|------|
| POST | /api/users/login | 불필요 | 로그인 |
| POST | /api/users | 불필요 | 회원가입 |
| GET | /api/user | 필수 | 현재 사용자 조회 |
| PUT | /api/user | 필수 | 사용자 수정 |
| GET | /api/users/all/{offset}/{limit} | 불필요 | 사용자 목록 |
| GET | /api/profiles/{username} | 필수 | 프로필 조회 |
| POST | /api/profiles/{username}/follow | 필수 | 팔로우 |
| DELETE | /api/profiles/{username}/follow | 필수 | 팔로우 취소 |
| POST | /api/articles | 필수 | 게시글 작성 |
| GET | /api/articles | 필수 | 게시글 목록 |
| GET | /api/articles/feed | 필수 | 피드 |
| GET | /api/articles/{slug} | * | 게시글 조회 |
| PUT | /api/articles/{slug} | 필수 | 게시글 수정 |
| DELETE | /api/articles/{slug} | 필수 | 게시글 삭제 |
| POST | /api/articles/{slug}/favorite | 필수 | 즐겨찾기 |
| DELETE | /api/articles/{slug}/favorite | 필수 | 즐겨찾기 취소 |
| GET | /api/tags | 불필요 | 태그 목록 |
| GET | /api/articles/{slug}/comments | 불필요 | 댓글 목록 |
| POST | /api/articles/{slug}/comments | * | 댓글 작성 |
| DELETE | /api/articles/{slug}/comments/{cid} | * | 댓글 삭제 |

\* = 설정에서 `publicMode` 활성화 시 인증 선택사항

## 문서

- [API 레퍼런스](docs/API.md)
- [아키텍처](docs/ARCHITECTURE.md)
- [아키텍처 결정 기록 (ADR)](docs/adr/)
- [심층 인터뷰](docs/DEEP-INTERVIEW.md) — 바이브 코딩 안전망 요구사항 도출 과정
- [합의 계획](docs/CONSENSUS-PLAN.md) — Planner/Architect/Critic 합의 기반 구현 계획
- [Claude Code 스킬](docs/SKILLS.md) — 바이브 코딩용 커스텀 스킬 가이드

---

# 배경

Realworld는 ["모든 데모 앱의 어머니"](https://codebase.show/projects/realworld)입니다.
이 프로젝트는 정밀한 API 명세를 정의합니다. 따라서 다수의 독립적인 프론트엔드 구현과 백엔드 구현이 존재합니다. 어떤 프론트엔드든 다른 백엔드와 함께 동작할 수 있습니다.
한편, Realworld는 완전한 기능의 포럼 시스템을 정의합니다. 이 정교하고 작은 목표를 달성하기 위해 많은 실용적인 기술이 적용되어야 합니다.

# C++ - OAT++

이 백엔드 구현은 C++과 OAT++ 웹 프레임워크를 사용합니다. 데이터베이스 시스템은 oatpp-postgres ORM을 통해 PostgreSQL을 사용합니다.
다른 개발 언어와 비교하면 웹 개발은 C++에게 최선의 선택이 아닙니다.
OAT++를 사용하면 이 과정이 훨씬 단순해지지만, 여전히 번거롭고 어렵습니다.
하지만 저는 C++를 좋아하고, 그것으로 충분합니다.

추신: 2021/10/27 oat++ 1.3.0 적용

# 사전 요구사항

macOS:
```bash
brew install libpq nlohmann-json
```
Ubuntu:
```bash
sudo apt install zlib1g-dev libssl-dev nlohmann-json3-dev libpq postgresql-server-dev-12 -y
```

Centos:
```bash
sudo yum install -y json-devel *atomic* zlib-devel libpq-devel postgresql-server-devel
```

# PostgreSQL 설정

PSQL 클라이언트로 데이터베이스와 사용자를 설정합니다:
```sql
CREATE USER realworld WITH PASSWORD 'realworld';
CREATE DATABASE realworld;
GRANT ALL PRIVILEGES ON DATABASE realworld TO realworld;
```
그리고 모든 테이블:
```sql
CREATE TABLE users (
  id SERIAL PRIMARY KEY,
  username TEXT NOT NULL UNIQUE,
  email TEXT NOT NULL UNIQUE,
  bio TEXT,
  image TEXT,
  hash TEXT NOT NULL
);

CREATE TABLE articles (
  id SERIAL PRIMARY KEY,
  slug TEXT NOT NULL UNIQUE,
  title TEXT NOT NULL,
  description TEXT NOT NULL,
  body TEXT NOT NULL,
  author INTEGER NOT NULL REFERENCES users ON DELETE CASCADE,
  tag_list TEXT[] NOT NULL,
  created_at TIMESTAMP WITH TIME ZONE NOT NULL DEFAULT NOW(),
  updated_at TIMESTAMP WITH TIME ZONE NOT NULL DEFAULT NOW(),
  favorites_count INTEGER NOT NULL DEFAULT 0
);

CREATE TABLE favorites (
       "user" INTEGER REFERENCES users ON DELETE CASCADE,
       article INTEGER REFERENCES articles ON DELETE CASCADE,
       PRIMARY KEY ("user", article)
);

CREATE TABLE follows (
       follower INTEGER REFERENCES users ON DELETE CASCADE,
       followed INTEGER REFERENCES users ON DELETE CASCADE,
       CHECK (follower != followed),
       PRIMARY KEY(follower, followed)
);

CREATE TABLE comments (
       id SERIAL PRIMARY KEY,
       body TEXT NOT NULL,
       article INTEGER NOT NULL REFERENCES articles ON DELETE CASCADE,
       author INTEGER NOT NULL REFERENCES users ON DELETE CASCADE,
       created_at TIMESTAMP WITH TIME ZONE NOT NULL DEFAULT NOW(),
       updated_at TIMESTAMP WITH TIME ZONE NOT NULL DEFAULT NOW()
);
```

## oatpp 프레임워크 컴파일 및 설치
* oatpp
* oatpp-zlib
* oatpp-postgres

`\utility\install-oatpp-modules.sh` 스크립트가 Ubuntu에서 이 작업을 도와줍니다. 다른 시스템에서도 참고할 수 있습니다.

cpp-jwt 모듈 코드는 std-c++-14가 필요하므로 g++ 버전이 8 이상이어야 하며, macOS clang은 정상 동작합니다.

## 코드 복제
```bash
git clone https://github.com/formoon/realworld-oatpp
```

## 설정
프로젝트 루트에 `config.json` 파일을 생성합니다:
```js
{
    "thread": {
        "dataThread": 4,
        "ioThread": 1,
        "timerThread": 1
    },
    "server":{
        "address": "0.0.0.0",
        "port": 8002,
        "secretKey": "8Xui8SN4mI+7egV/9dlfYYLGQJeEx4+DwmSQLwDVXJg=",
        "publicMode": true,
        "upAndDownPath": "./download/",
        "staticFilePath": "./static/"
    },
    "postgres":{
        "url": "postgres://realworld:realworld@127.0.0.1:6551/realworld",
        "pool": 10,
        "ttl": 5
    }
}
```

## 컴파일 및 실행
```bash
cd realworld-oatpp
mkdir build
cd build
cmake ..
make -j4
cd ..
# cp config.json.txt config.json
./build/realworld-oatpp
```

## 데모 주소
<http://39.105.37.153:8002>
(상시 운영 서버가 아니므로 오래 유지되지 않을 수 있습니다)


# 기타
업로드 기능과 정적 파일 제공은 프론트엔드 협업이 필요합니다. [Vue 버전](https://github.com/formoon/vue-realworld-example-app) 사용을 권장합니다. [Vue2 버전 RealWorld Frontend](https://github.com/gothinkster/vue-realworld-example-app)를 수정한 것입니다.
