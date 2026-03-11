# Claude Code 스킬 가이드

이 문서는 realworld-oatpp 프로젝트에서 사용할 수 있는 Claude Code 커스텀 스킬을 설명합니다.

스킬은 `.claude/skills/` 디렉토리에 정의되어 있으며, Claude Code 세션에서 `/스킬명`으로 호출하거나 자동으로 참조됩니다.

---

## 스킬 개요

| 스킬 | 호출 방식 | 용도 |
|------|-----------|------|
| `/build-test` | 수동 | 빌드 및 테스트 실행, 오류 진단 |
| `/add-endpoint` | 수동 | 새 API 엔드포인트 전체 스캐폴딩 |
| `/sync-docs` | 수동 | 코드 변경 후 문서 동기화 |
| `/db-schema` | 수동 | DB 스키마 변경 시 관련 파일 동기화 |
| `oatpp-conventions` | 자동 | oatpp 1.3.0 코딩 규칙 참조 (백그라운드) |

---

## /build-test — 빌드 및 테스트

코드 변경 후 빌드와 테스트를 실행하여 변경 사항을 검증합니다.

### 사용법

```
/build-test           # 전체 (빌드 + 테스트)
/build-test build     # 빌드만
/build-test test      # 테스트만
/build-test all       # 전체 (명시적)
```

### 사전 조건

- 프로젝트 루트에 `config.json` 존재 (없으면 `exit(1)` 발생)
- PostgreSQL 실행 중
- oatpp 1.3.0 모듈 설치됨

### 오류 대응

| 오류 | 원인 | 해결 |
|------|------|------|
| config.json 크래시 | 설정 파일 누락 | `cp config.json.txt config.json` 후 DB 자격증명 편집 |
| OATPP_ASSERT 실패 | 테스트 어서션 | 실패한 테스트와 관련 엔드포인트 코드 확인 |
| 링커 오류 | .cpp 미등록 | CMakeLists.txt의 `add_executable` 확인 |
| DB 연결 오류 | PostgreSQL 미실행 | `config.json`의 `postgres.url` 확인 |

### 사용 시나리오

```
사용자: 새 엔드포인트를 추가했어. 확인해줘.
Claude: /build-test all 실행 → 빌드 성공, 테스트 30개 중 30개 통과
```

---

## /add-endpoint — 엔드포인트 스캐폴딩

새 API 엔드포인트 추가 시 필요한 모든 파일을 oatpp 패턴에 맞게 일관되게 생성합니다.

### 사용법

```
/add-endpoint METHOD /api/path 설명
```

### 예시

```
/add-endpoint POST /api/articles/{slug}/bookmark 게시글 북마크
/add-endpoint GET /api/users/{id}/stats 사용자 통계 조회
/add-endpoint DELETE /api/articles/{slug}/bookmark 북마크 취소
```

### 생성/수정되는 파일

실행 시 다음 파일들이 순서대로 처리됩니다:

```
1. src/dto/DTOs.hpp              ← DTO 추가 (필요시)
2. src/db/{Domain}Db.hpp         ← DB 쿼리 추가 (필요시)
3. src/service/{Domain}Service.hpp ← 서비스 메서드 추가
4. src/controller/{Domain}Controller.hpp ← ENDPOINT_ASYNC 추가
5. test/app/MyApiTestClient.hpp  ← API_CALL 매크로 추가
6. test/{Domain}ControllerTest.cpp ← 테스트 케이스 추가
```

### 도메인 자동 판별

| 경로 패턴 | 도메인 | Controller |
|-----------|--------|------------|
| `/api/users*`, `/api/profiles*` | User | UserController |
| `/api/articles*` (comments 제외) | Article | ArticleController |
| `/api/articles/*/comments*` | Comment | CommentController |

### 테스트 데이터 격리

각 테스트 클래스는 고유한 테스트 데이터를 사용합니다:

| 테스트 클래스 | 이메일 | 사용자 이름 |
|---|---|---|
| UserControllerTest | `testuser_uc@test.com` | `testuser_uc` |
| ArticleControllerTest | `testuser_ac@test.com` | `testuser_ac` |
| CommentControllerTest | `testuser_cc@test.com` | `testuser_cc` |

### 완료 체크리스트

스캐폴딩 후 자동으로 다음을 확인합니다:

- DTO 추가됨 (필요시)
- DB 쿼리 추가됨 (필요시)
- Service 메서드 추가됨
- ENDPOINT_ASYNC 추가됨
- 인증 요구사항 결정됨
- API_CALL 매크로 추가됨
- 테스트 케이스 추가됨
- 빌드 성공
- 테스트 통과

---

## /sync-docs — 문서 동기화

코드 변경 후 관련 문서를 현재 코드 상태와 동기화합니다.

### 사용법

```
/sync-docs            # 모든 문서 동기화
/sync-docs api        # docs/API.md만
/sync-docs architecture  # docs/ARCHITECTURE.md만
/sync-docs all        # 모든 문서 (명시적)
```

### 동기화 대상

| 문서 | 동기화 내용 |
|------|-------------|
| `docs/API.md` | 엔드포인트 추가/삭제/수정 반영, 인증 요구사항 |
| `docs/ARCHITECTURE.md` | DI 컴포넌트 변경, 새 서비스/컨트롤러 추가 |
| `CLAUDE.md` | 엔드포인트 수, 아키텍처 변경 |
| `README.md` | API 개요 표 업데이트 |
| `openspec/specs/*/spec.md` | OpenSpec 스펙 업데이트 (해당 도메인) |

### 동기화 프로세스

1. 소스 코드에서 현재 상태 추출 (ENDPOINT_ASYNC, QUERY, DTO 등)
2. 각 문서의 기존 내용과 비교
3. 차이점을 반영하여 문서 업데이트
4. 변경 결과 요약 출력

### 사용 시나리오

```
사용자: /add-endpoint POST /api/articles/{slug}/bookmark 게시글 북마크
Claude: (엔드포인트 스캐폴딩 완료)
사용자: /sync-docs
Claude: API.md에 북마크 엔드포인트 추가, CLAUDE.md 엔드포인트 수 20→21로 업데이트...
```

---

## /db-schema — DB 스키마 관리

데이터베이스 스키마 변경 시 관련 파일(schema.sql, DTO, DbClient, DatabaseComponent)을 동기화합니다.

### 사용법

```
/db-schema add-table 테이블명       # 새 테이블 추가
/db-schema modify-table 테이블명    # 기존 테이블 수정
/db-schema sync                     # 코드-스키마 불일치 확인
```

### 예시

```
/db-schema add-table bookmarks
/db-schema modify-table articles
/db-schema sync
```

### add-table 시 생성/수정되는 파일

```
1. sql/schema.sql                  ← DROP IF EXISTS + CREATE TABLE 추가
2. src/dto/DTOs.hpp                ← 새 DTO 클래스 추가
3. src/db/{Name}Db.hpp             ← 새 DbClient 생성
4. src/DatabaseComponent.hpp       ← OATPP_CREATE_COMPONENT 추가
5. test/app/TestDatabaseComponent.hpp ← 동일한 순서로 컴포넌트 추가
```

### 중요 규칙

- `sql/schema.sql`은 항상 **멱등**해야 합니다 (`DROP TABLE IF EXISTS CASCADE`)
- `DatabaseComponent.hpp`와 `TestDatabaseComponent.hpp`의 **멤버 선언 순서가 반드시 일치**해야 합니다 (C++ 멤버 초기화 순서 의존성)
- 새 DbClient는 헤더 전용(`.hpp`)이므로 CMakeLists.txt 변경 불필요

### 현재 스키마

| 테이블 | DbClient | 비고 |
|--------|----------|------|
| users | UserDb | 주 사용자 테이블 |
| articles | ArticleDb | 게시글 |
| favorites | ArticleDb | 연결 테이블 (User↔Article) |
| follows | UserDb | 연결 테이블 (User↔User) |
| comments | CommentDb | 댓글 |

---

## oatpp-conventions — 코딩 규칙 (자동 참조)

이 스킬은 `/` 메뉴에 표시되지 않으며, Claude가 `src/` 또는 `test/` 디렉토리의 C++ 코드를 작성할 때 **자동으로 참조**됩니다.

### 포함 내용

- **ENDPOINT_ASYNC 패턴**: GET (본문 없음)과 POST/PUT (본문 있음)의 코루틴 패턴
- **인증 패턴**: 필수 인증과 publicMode 조건부 인증
- **DI 패턴**: `OATPP_CREATE_COMPONENT`와 `OATPP_COMPONENT` 사용법
- **DB 쿼리 패턴**: `QUERY` 매크로와 `PREPARE(true)` 사용법
- **DTO 패턴**: `DTO_INIT`, `DTO_FIELD` 매크로
- **오류 응답 패턴**: `mkErrMsg`, `OATPP_ASSERT_HTTP`
- **금지 사항**: .hpp를 CMakeLists에 추가, oatpp 업그레이드, C++11 초과 기능 등

### 동작 방식

Claude가 이 프로젝트에서 C++ 코드를 작성하거나 수정할 때, 스킬의 description이 컨텍스트에 자동 포함됩니다. Claude는 이 규칙을 참고하여 프로젝트 패턴에 맞는 코드를 생성합니다.

---

## 일반적인 바이브 코딩 워크플로우

새 기능을 추가하는 전형적인 흐름:

```
1. /add-endpoint POST /api/articles/{slug}/bookmark 게시글 북마크
   → 전체 파일 스캐폴딩 (Controller, Service, DB, DTO, Test)

2. /build-test all
   → 빌드 및 테스트 검증

3. /sync-docs
   → API.md, README.md 등 문서 자동 업데이트

4. (선택) /opsx:propose add-bookmark-feature
   → OpenSpec으로 변경사항 추적
```

DB 스키마 변경이 필요한 경우:

```
1. /db-schema add-table bookmarks
   → schema.sql, DTO, DbClient, DatabaseComponent 동기화

2. /add-endpoint POST /api/articles/{slug}/bookmark 게시글 북마크
   → 엔드포인트 스캐폴딩

3. /build-test all
   → 검증

4. /sync-docs
   → 문서 동기화
```

---

## 스킬 파일 위치

모든 스킬은 프로젝트 루트의 `.claude/skills/` 디렉토리에 있습니다:

```
.claude/skills/
├── build-test/SKILL.md          # 빌드 및 테스트
├── add-endpoint/SKILL.md        # 엔드포인트 스캐폴딩
├── sync-docs/SKILL.md           # 문서 동기화
├── db-schema/SKILL.md           # DB 스키마 관리
├── oatpp-conventions/SKILL.md   # 코딩 규칙 (자동)
├── openspec-propose/SKILL.md    # OpenSpec 변경 제안
├── openspec-apply-change/SKILL.md  # OpenSpec 변경 적용
├── openspec-archive-change/SKILL.md # OpenSpec 변경 아카이브
└── openspec-explore/SKILL.md    # OpenSpec 탐색
```
