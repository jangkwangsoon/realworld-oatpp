# 합의 계획: AI 안전망 구축

## 메타데이터

| 항목 | 값 |
|------|-----|
| 생성일 | 2026-03-11 |
| 최종 수정 | 2026-03-11 (R3 - 합의 승인) |
| 상태 | 승인됨 — 실행 준비 완료 |
| 복잡도 | 높음 (3단계, 약 25개 신규 파일, CMakeLists.txt 수정) |
| 승인자 | Architect + Critic (R2에서 합의 도달) |

---

## 1. 요구사항 요약

AI 도구(Claude 등)가 이 C++ oatpp RealWorld 코드베이스를 안전하게 수정할 수 있도록 안전망을 구축합니다:

1. **CI 파이프라인** (GitHub Actions) — PostgreSQL을 포함한 자동화 빌드 검증
2. **테스트** (oatpp::test::UnitTest) — 컨트롤러 통합 테스트 + 인증 단위 테스트
3. **문서화** — CLAUDE.md, 현대화된 README, API 문서, 아키텍처 문서

### 엄격한 제약

- 기존 `src/` 소스 코드 리팩터링 **금지**
- C++11 표준, 기존 CMake 구조 유지
- oatpp 1.3.0 모듈은 소스에서 빌드 (apt 패키지 아님)
- 테스트 프레임워크: oatpp-test (CMakeLists.txt 48행에서 이미 링크됨)
- 단계별 실행: CI → 테스트 → 문서 순서
- CMakeLists.txt 테스트 타겟에 .hpp 파일 **금지** (프로젝트 규칙, 36행)

---

## 2. RALPLAN-DR 요약

### 원칙 (5개)

1. **안전 최우선**: 모든 변경은 추가적이어야 함; 기존 `src/` 파일 절대 수정 금지
2. **CI 우선**: 작동하는 CI 파이프라인이 테스트가 깨끗한 환경에서 통과하는지 검증
3. **기존 패턴 따르기**: `test/`에 이미 확립된 테스트 패턴 사용 (가상 네트워킹, ClientServerTestRunner, ApiClient)
4. **최소 결합**: 테스트는 자체 완결적; 각 테스트 클래스가 고유한 테스트 데이터 사용
5. **실행 가능한 문서**: CLAUDE.md와 README의 명령어를 복사-붙여넣기로 실행 가능해야 함

### 결정 동인 (3개)

1. **oatpp 모듈 빌드 시간** — 소스에서 빌드하는 데 상당한 시간 소요; CI에서 캐싱이 중요
2. **테스트 아키텍처** — 기존 TestComponent.hpp는 가상 네트워킹 사용하지만 DatabaseComponent나 Config가 없음; 통합 테스트에는 둘 다 필요
3. **컨트롤러 의존성** — 모든 컨트롤러가 Config, DatabaseComponent, 전체 DI 그래프에 의존; 테스트가 모든 OATPP_COMPONENT를 제공해야 함

### 검토된 옵션

#### 옵션 A: 모든 테스트 클래스를 포함한 단일 테스트 바이너리 (채택)
- CMake 테스트 타겟 하나 `realworld-oatpp-test`
- 컨트롤러별 + 인증용 별도 테스트 `.cpp`
- 단일 `test/tests.cpp` 진입점이 `OATPP_RUN_TEST()`로 모든 테스트 클래스 실행
- **장점**: 단순한 CMake 설정, 기존 패턴과 일치, CI에서 단일 테스트 실행
- **단점**: 하나의 바이너리에 모든 테스트 → 테스트 변경 시 재컴파일 시간 증가

#### 옵션 B: 도메인별 다중 테스트 바이너리 (기각)
- 별도 CMake 타겟: `test-user`, `test-article`, `test-comment`, `test-auth`
- 각각 독립적인 `main()`
- **장점**: 빠른 증분 빌드, CI에서 병렬 테스트 실행
- **기각 근거**: 기존 테스트 인프라(66-85행)가 단일 바이너리를 기대함. 다중 바이너리는 상당한 CMake 재구성이 필요하고 20개 엔드포인트 프로젝트에 비례하지 않는 복잡성을 추가

---

## 3. 아키텍처 결정 기록 (ADR)

- **결정**: 옵션 A — 단일 테스트 바이너리
- **동인**: 기존 CMake 패턴과 일치, 단순한 CI 통합, 프로젝트 규모에 적합
- **대안**: 다중 테스트 바이너리 (옵션 B) — CMake 복잡성 증가와 기존 패턴 이탈로 기각
- **채택 이유**: 20개 엔드포인트는 단일 바이너리로 빠르게 컴파일 가능; 확립된 `test/tests.cpp` 패턴 따름
- **결과**: 모든 테스트가 함께 통과해야 함; 하나의 테스트 실패가 전체 스위트 차단. TestComponent는 CORS 인터셉터나 gzip 없이 생성 — 알려진 충실도 격차
- **후속 작업**: 테스트 클래스가 약 50개를 초과하거나 다른 DI 설정이 필요하면 바이너리 분할 고려

---

## 4. 구현 단계

### Phase 1: CI 파이프라인

| 단계 | 설명 | 수용 기준 |
|------|------|-----------|
| 1.1 | `sql/schema.sql` 생성 (멱등성: DROP IF EXISTS CASCADE) | `psql -f sql/schema.sql` 두 번 실행 시 오류 없음 |
| 1.2 | `.github/workflows/ci.yml` 생성 | PR/main push 시 자동 트리거, PostgreSQL 서비스 컨테이너, oatpp 모듈 캐싱 |

### Phase 2: 테스트

| 단계 | 설명 | 수용 기준 |
|------|------|-----------|
| 2.1 | 구 테스트 파일 삭제, CMakeLists.txt 업데이트 | `.cpp` 파일만 `add_executable`에 포함 |
| 2.2 | 테스트 인프라 파일 생성 | TestComponent에 Config 추가, TestDatabaseComponent 생성, MyApiTestClient 20개 API_CALL |
| 2.3 | 컨트롤러 통합 테스트 + 인증 단위 테스트 작성 | 4개 테스트 클래스 전부 통과, 객체 누수 없음 |

**테스트 데이터 격리 전략:**

| 테스트 클래스 | 이메일 | 사용자 이름 |
|---------------|--------|-------------|
| UserControllerTest | `testuser_uc@test.com` | `testuser_uc` |
| ArticleControllerTest | `testuser_ac@test.com` | `testuser_ac` |
| CommentControllerTest | `testuser_cc@test.com` | `testuser_cc` |

### Phase 3: 문서화

| 단계 | 설명 | 수용 기준 |
|------|------|-----------|
| 3.1 | CLAUDE.md 생성 | 정확한 빌드/테스트 명령어 포함 |
| 3.2 | README.md 현대화 | CI 배지, API 개요 표, 테스트 안내 |
| 3.3 | docs/API.md, docs/ARCHITECTURE.md 생성 | 20개 엔드포인트 전체 커버, 계층 아키텍처 설명 |

---

## 5. 위험과 완화

| 위험 | 영향 | 가능성 | 완화 |
|------|------|--------|------|
| 가상 네트워킹이 DB 연동 컨트롤러에서 동작 안 함 | 테스트 런타임 실패 | 중간 | TestDatabaseComponent가 실제 DB 연결 제공; 가상 네트워킹은 TCP 계층만 대체 |
| Config가 config.json 없으면 `exit(1)` 호출 | 어서션 전 테스트 크래시 | 높음 | CI에서 config.json 생성 + python3로 유효성 검증. 프로젝트 루트에서 실행 |
| oatpp 소스 빌드 실패 | CI 차단 | 낮음 | 1.3.0 고정; install-oatpp-modules.sh로 검증됨 |
| 테스트 클래스 간 데이터 충돌 | 유니크 제약 위반으로 불안정 테스트 | 중간 | 클래스별 전용 이메일/사용자 이름. CI에서 TRUNCATE CASCADE 실행 |
| CORS/gzip 충실도 격차 | 테스트 통과하나 프로덕션 동작 다름 | 낮음 | 알려진 격차로 문서화. 테스트는 비즈니스 로직 검증 목적 |

---

## 6. 검증 단계

### Phase 1 검증
1. 워크플로우를 브랜치에 push, PR 열어 GitHub Actions 잡 실행 확인
2. `psql -f sql/schema.sql` 두 번 실행 시 오류 없음 확인 (멱등성)

### Phase 2 검증
```bash
# 4개 테스트 클래스 실행 확인
cd build && ./realworld-oatpp-test 2>&1 | grep 'TEST\[' | wc -l
# 기대 출력: 4

# 종료 코드 0 확인
cd build && ./realworld-oatpp-test
echo $?
# 기대 출력: 0

# 객체 누수 없음 확인
cd build && ./realworld-oatpp-test 2>&1 | grep 'objectsCount'
# 기대 출력: objectsCount = 0

# CMakeLists.txt에 .hpp 파일 없음 확인
grep -A 10 'add_executable.*test' CMakeLists.txt | grep '\.hpp' | wc -l
# 기대 출력: 0
```

### Phase 3 검증
1. CLAUDE.md 빌드 안내를 새 환경에서 따라 실행
2. README 배지 URL 정확성 확인
3. API.md가 20개 엔드포인트 전부 커버하는지 컨트롤러와 교차 확인

---

## 7. 파일 목록

### 신규 생성 파일
| 파일 | 설명 |
|------|------|
| `.github/workflows/ci.yml` | CI 파이프라인 |
| `sql/schema.sql` | 데이터베이스 스키마 (멱등성 보장) |
| `test/app/TestDatabaseComponent.hpp` | src/DatabaseComponent.hpp의 구조적 복제 |
| `test/UserControllerTest.hpp` + `.cpp` | 사용자 컨트롤러 13개 테스트 |
| `test/ArticleControllerTest.hpp` + `.cpp` | 게시글 컨트롤러 10개 테스트 |
| `test/CommentControllerTest.hpp` + `.cpp` | 댓글 컨트롤러 3개 테스트 |
| `test/UserAuthTest.hpp` + `.cpp` | JWT 인증 4개 테스트 |
| `CLAUDE.md` | AI 어시스턴트 가이드 |
| `docs/API.md` | API 레퍼런스 |
| `docs/ARCHITECTURE.md` | 아키텍처 문서 |

### 수정 파일
| 파일 | 변경 내용 |
|------|-----------|
| `CMakeLists.txt` | 주석 처리된 테스트 타겟(66-85행)을 새 블록으로 교체 |
| `test/tests.cpp` | 4개 테스트 클래스 실행하는 새 테스트 러너로 교체 |
| `test/app/TestComponent.hpp` | Config 컴포넌트를 첫 번째 OATPP_CREATE_COMPONENT로 추가 |
| `test/app/MyApiTestClient.hpp` | 20개 API_CALL 매크로로 전면 재작성 |
| `README.md` | CI 배지, 빠른 시작, API 개요 추가 |

### 삭제 파일
| 파일 | 사유 |
|------|------|
| `test/MyControllerTest.cpp` | 존재하지 않는 `/hello` 엔드포인트와 BaseController 참조 |
| `test/MyControllerTest.hpp` | 파일명과 클래스명 불일치 (MyControllerTest ↔ BaseControllerTest) |

### 변경 없는 파일
- `src/` 하위 모든 파일
- `3rd/` 하위 모든 파일
- `utility/install-oatpp-modules.sh`

---

## 8. 실행 순서

```
Phase 1: CI 파이프라인
  1.1 sql/schema.sql 생성 (DROP IF EXISTS + TRUNCATE)
  1.2 .github/workflows/ci.yml 생성 (캐시 YAML 수정됨)
  1.3 CI 실행 확인 (빌드만, 테스트는 Phase 2까지 실패)

Phase 2: 테스트
  2.1  test/MyControllerTest.cpp + .hpp 삭제
  2.2  CMakeLists.txt 업데이트 (.cpp 파일만 add_executable에)
  2.3  test/app/TestComponent.hpp 업데이트 (Config 추가)
  2.4  test/app/TestDatabaseComponent.hpp 생성
  2.5  test/app/MyApiTestClient.hpp 재작성 (20개 API_CALL)
  2.6  test/UserAuthTest.hpp + .cpp 생성
  2.7  test/UserControllerTest.hpp + .cpp 생성
  2.8  test/ArticleControllerTest.hpp + .cpp 생성
  2.9  test/CommentControllerTest.hpp + .cpp 생성
  2.10 test/tests.cpp 재작성 (4개 테스트 클래스 등록)
  2.11 로컬 + CI에서 모든 테스트 통과 확인

Phase 3: 문서화
  3.1 CLAUDE.md 생성
  3.2 README.md 업데이트
  3.3 docs/API.md 생성
  3.4 docs/ARCHITECTURE.md 생성
```

---

## 9. 변경 이력

합의 과정에서 2회 반복(R1 → R2)을 거쳐 승인되었습니다.

| # | 변경 | 출처 |
|---|------|------|
| 1 | CI 캐시 YAML: 파이프(`\|`) 다중행 경로 사용, `/usr/local/share/oatpp-*` 추가 | Critic: 치명적 |
| 2 | CMakeLists.txt `add_executable`에 .cpp 파일만 나열 (프로젝트 규칙 준수) | Critic: 치명적 |
| 3 | 테스트 데이터 격리 전략: 클래스별 고유 이메일/사용자 이름 | Architect: 중대 |
| 4 | TestDatabaseComponent를 동일한 멤버 순서의 구조적 복제로 문서화 | Architect: 중대 |
| 5 | 기존 테스트 파일 명명 버그 명확화 (MyControllerTest vs BaseControllerTest) | Architect: 중대 |
| 6 | TestComponent.hpp에 Config 컴포넌트를 첫 번째 OATPP_CREATE_COMPONENT로 추가 | Architect: 중대 |
| 7 | CORS/gzip 충실도 격차를 알려진 제한으로 ADR에 문서화 | Architect: 중대 |
| 8 | 서비스 레이어 테스트가 컨트롤러 테스트를 통해 간접 커버됨을 명시 | Architect: 중대 |
| 9 | sql/schema.sql에 DROP TABLE IF EXISTS CASCADE 사용 (멱등성) | Architect: 중대 |
| 10 | 검증 단계에 정확한 명령어와 예상 출력 추가 | Critic: 중대 |
| 11 | BaseController를 명시적으로 테스트에서 제외 (근거 포함) | Architect: 개선 |
| 12 | CI에 config.json 유효성 검증 단계 추가 (exit(1) 완화) | Architect: 개선 |
| 13 | 테스트 클래스 약 50개 초과 시 바이너리 분할 후속 작업 문서화 | Architect: 개선 |
| 14 | CI 데이터베이스 설정에 TRUNCATE CASCADE 추가 | Architect: 중대 |
