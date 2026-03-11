#!/bin/bash
# E2E 시나리오 테스트 — Bash + curl + jq
# 사용법: ./test/e2e/run-e2e.sh [BASE_URL]
# 기본값: http://localhost:8002

set -euo pipefail

BASE_URL="${1:-http://localhost:8002}"
PASSED=0
FAILED=0
TOTAL=0

# 색상
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m'

assert_status() {
  local test_name="$1"
  local expected="$2"
  local actual="$3"
  TOTAL=$((TOTAL + 1))
  if [ "$actual" -eq "$expected" ]; then
    echo -e "  ${GREEN}PASS${NC} $test_name (HTTP $actual)"
    PASSED=$((PASSED + 1))
  else
    echo -e "  ${RED}FAIL${NC} $test_name (expected $expected, got $actual)"
    FAILED=$((FAILED + 1))
  fi
}

assert_json_field() {
  local test_name="$1"
  local json="$2"
  local field="$3"
  TOTAL=$((TOTAL + 1))
  local value
  value=$(echo "$json" | jq -r "$field" 2>/dev/null || echo "null")
  if [ "$value" != "null" ] && [ "$value" != "" ]; then
    echo -e "  ${GREEN}PASS${NC} $test_name ($field = $value)"
    PASSED=$((PASSED + 1))
  else
    echo -e "  ${RED}FAIL${NC} $test_name ($field is null or missing)"
    FAILED=$((FAILED + 1))
  fi
}

echo "========================================"
echo "E2E 시나리오 테스트"
echo "Base URL: $BASE_URL"
echo "========================================"

# 고유 테스트 데이터 (타임스탬프 기반)
TS=$(date +%s)
USERNAME="e2euser_${TS}"
EMAIL="e2e_${TS}@test.com"
PASSWORD="e2epassword123"

# ========================================
echo ""
echo -e "${YELLOW}시나리오 1: 회원가입 → 로그인 → 프로필${NC}"
echo "----------------------------------------"

# 1.1 회원가입
RESPONSE=$(curl -s -w "\n%{http_code}" -X POST "$BASE_URL/api/users" \
  -H "Content-Type: application/json" \
  -d "{\"user\":{\"username\":\"$USERNAME\",\"email\":\"$EMAIL\",\"password\":\"$PASSWORD\"}}")
BODY=$(echo "$RESPONSE" | sed '$d')
STATUS=$(echo "$RESPONSE" | tail -1)
assert_status "1.1 회원가입" 200 "$STATUS"
assert_json_field "1.1 토큰 반환" "$BODY" ".user.token"
assert_json_field "1.1 사용자명 확인" "$BODY" ".user.username"

# 토큰 추출
TOKEN=$(echo "$BODY" | jq -r '.user.token')

# 1.2 로그인
RESPONSE=$(curl -s -w "\n%{http_code}" -X POST "$BASE_URL/api/users/login" \
  -H "Content-Type: application/json" \
  -d "{\"user\":{\"email\":\"$EMAIL\",\"password\":\"$PASSWORD\"}}")
BODY=$(echo "$RESPONSE" | sed '$d')
STATUS=$(echo "$RESPONSE" | tail -1)
assert_status "1.2 로그인" 200 "$STATUS"
assert_json_field "1.2 로그인 토큰" "$BODY" ".user.token"

# 로그인 토큰으로 갱신
TOKEN=$(echo "$BODY" | jq -r '.user.token')

# 1.3 현재 사용자 조회
RESPONSE=$(curl -s -w "\n%{http_code}" -X GET "$BASE_URL/api/user" \
  -H "Authorization: Token $TOKEN")
BODY=$(echo "$RESPONSE" | sed '$d')
STATUS=$(echo "$RESPONSE" | tail -1)
assert_status "1.3 현재 사용자 조회" 200 "$STATUS"
assert_json_field "1.3 이메일 확인" "$BODY" ".user.email"

# 1.4 사용자 정보 수정
RESPONSE=$(curl -s -w "\n%{http_code}" -X PUT "$BASE_URL/api/user" \
  -H "Content-Type: application/json" \
  -H "Authorization: Token $TOKEN" \
  -d "{\"user\":{\"bio\":\"E2E test bio\"}}")
BODY=$(echo "$RESPONSE" | sed '$d')
STATUS=$(echo "$RESPONSE" | tail -1)
assert_status "1.4 사용자 수정" 200 "$STATUS"

# 1.5 프로필 조회
RESPONSE=$(curl -s -w "\n%{http_code}" -X GET "$BASE_URL/api/profiles/$USERNAME" \
  -H "Authorization: Token $TOKEN")
BODY=$(echo "$RESPONSE" | sed '$d')
STATUS=$(echo "$RESPONSE" | tail -1)
assert_status "1.5 프로필 조회" 200 "$STATUS"
assert_json_field "1.5 프로필 사용자명" "$BODY" ".profile.username"

# 1.6 인증 없이 접근 → 500
RESPONSE=$(curl -s -w "\n%{http_code}" -X GET "$BASE_URL/api/user")
STATUS=$(echo "$RESPONSE" | tail -1)
assert_status "1.6 비인증 접근 거부" 500 "$STATUS"

# 1.7 잘못된 비밀번호 로그인
RESPONSE=$(curl -s -w "\n%{http_code}" -X POST "$BASE_URL/api/users/login" \
  -H "Content-Type: application/json" \
  -d "{\"user\":{\"email\":\"$EMAIL\",\"password\":\"wrongpassword\"}}")
STATUS=$(echo "$RESPONSE" | tail -1)
assert_status "1.7 잘못된 비밀번호" 500 "$STATUS"

# ========================================
echo ""
echo -e "${YELLOW}시나리오 2: 글쓰기 → 수정 → 즐겨찾기 → 댓글 → 삭제${NC}"
echo "----------------------------------------"

# 2.1 게시글 작성
ARTICLE_TITLE="E2E Test Article ${TS}"
RESPONSE=$(curl -s -w "\n%{http_code}" -X POST "$BASE_URL/api/articles" \
  -H "Content-Type: application/json" \
  -H "Authorization: Token $TOKEN" \
  -d "{\"article\":{\"title\":\"$ARTICLE_TITLE\",\"description\":\"E2E test desc\",\"body\":\"E2E test body content\",\"tagList\":[\"e2e\",\"test\"]}}")
BODY=$(echo "$RESPONSE" | sed '$d')
STATUS=$(echo "$RESPONSE" | tail -1)
assert_status "2.1 게시글 작성" 200 "$STATUS"
assert_json_field "2.1 제목 확인" "$BODY" ".article.title"

SLUG=$(echo "$BODY" | jq -r '.article.slug')

# 2.2 게시글 조회
RESPONSE=$(curl -s -w "\n%{http_code}" -X GET "$BASE_URL/api/articles/$SLUG" \
  -H "Authorization: Token $TOKEN")
BODY=$(echo "$RESPONSE" | sed '$d')
STATUS=$(echo "$RESPONSE" | tail -1)
assert_status "2.2 게시글 조회" 200 "$STATUS"
assert_json_field "2.2 슬러그 확인" "$BODY" ".article.slug"

# 2.3 게시글 수정
RESPONSE=$(curl -s -w "\n%{http_code}" -X PUT "$BASE_URL/api/articles/$SLUG" \
  -H "Content-Type: application/json" \
  -H "Authorization: Token $TOKEN" \
  -d "{\"article\":{\"title\":\"Updated $ARTICLE_TITLE\",\"description\":\"Updated desc\",\"body\":\"Updated body\",\"slug\":\"$SLUG\",\"tagList\":[\"e2e\",\"updated\"]}}")
BODY=$(echo "$RESPONSE" | sed '$d')
STATUS=$(echo "$RESPONSE" | tail -1)
assert_status "2.3 게시글 수정" 200 "$STATUS"

# 2.4 게시글 목록 조회
RESPONSE=$(curl -s -w "\n%{http_code}" -X GET "$BASE_URL/api/articles" \
  -H "Authorization: Token $TOKEN")
BODY=$(echo "$RESPONSE" | sed '$d')
STATUS=$(echo "$RESPONSE" | tail -1)
assert_status "2.4 게시글 목록" 200 "$STATUS"

# 2.5 즐겨찾기
RESPONSE=$(curl -s -w "\n%{http_code}" -X POST "$BASE_URL/api/articles/$SLUG/favorite" \
  -H "Authorization: Token $TOKEN")
STATUS=$(echo "$RESPONSE" | tail -1)
assert_status "2.5 즐겨찾기" 200 "$STATUS"

# 2.6 즐겨찾기 취소
RESPONSE=$(curl -s -w "\n%{http_code}" -X DELETE "$BASE_URL/api/articles/$SLUG/favorite" \
  -H "Authorization: Token $TOKEN")
STATUS=$(echo "$RESPONSE" | tail -1)
assert_status "2.6 즐겨찾기 취소" 200 "$STATUS"

# 2.7 태그 목록
RESPONSE=$(curl -s -w "\n%{http_code}" -X GET "$BASE_URL/api/tags")
STATUS=$(echo "$RESPONSE" | tail -1)
assert_status "2.7 태그 목록" 200 "$STATUS"

# 2.8 댓글 작성
RESPONSE=$(curl -s -w "\n%{http_code}" -X POST "$BASE_URL/api/articles/$SLUG/comments" \
  -H "Content-Type: application/json" \
  -H "Authorization: Token $TOKEN" \
  -d "{\"comment\":{\"body\":\"E2E test comment\"}}")
BODY=$(echo "$RESPONSE" | sed '$d')
STATUS=$(echo "$RESPONSE" | tail -1)
assert_status "2.8 댓글 작성" 200 "$STATUS"

# 2.9 댓글 목록
RESPONSE=$(curl -s -w "\n%{http_code}" -X GET "$BASE_URL/api/articles/$SLUG/comments")
BODY=$(echo "$RESPONSE" | sed '$d')
STATUS=$(echo "$RESPONSE" | tail -1)
assert_status "2.9 댓글 목록" 200 "$STATUS"

COMMENT_ID=$(echo "$BODY" | jq -r '.comments[0].id // empty')
if [ -n "$COMMENT_ID" ]; then
  # 2.10 댓글 삭제
  RESPONSE=$(curl -s -w "\n%{http_code}" -X DELETE "$BASE_URL/api/articles/$SLUG/comments/$COMMENT_ID" \
    -H "Authorization: Token $TOKEN")
  STATUS=$(echo "$RESPONSE" | tail -1)
  assert_status "2.10 댓글 삭제" 200 "$STATUS"
else
  echo -e "  ${RED}FAIL${NC} 2.10 댓글 삭제 (댓글 ID를 찾을 수 없음)"
  FAILED=$((FAILED + 1))
  TOTAL=$((TOTAL + 1))
fi

# 2.11 게시글 삭제
RESPONSE=$(curl -s -w "\n%{http_code}" -X DELETE "$BASE_URL/api/articles/$SLUG" \
  -H "Authorization: Token $TOKEN")
STATUS=$(echo "$RESPONSE" | tail -1)
assert_status "2.11 게시글 삭제" 200 "$STATUS"

# 2.12 짧은 필드로 게시글 작성 → 400
RESPONSE=$(curl -s -w "\n%{http_code}" -X POST "$BASE_URL/api/articles" \
  -H "Content-Type: application/json" \
  -H "Authorization: Token $TOKEN" \
  -d "{\"article\":{\"title\":\"X\",\"description\":\"Y\",\"body\":\"Z\",\"tagList\":[]}}")
STATUS=$(echo "$RESPONSE" | tail -1)
assert_status "2.12 유효성 검사 실패" 400 "$STATUS"

# ========================================
echo ""
echo "========================================"
echo -e "결과: ${GREEN}${PASSED} 통과${NC} / ${RED}${FAILED} 실패${NC} / 총 ${TOTAL}"
echo "========================================"

if [ "$FAILED" -gt 0 ]; then
  exit 1
fi
exit 0
