# RealWorld API 문서

이 문서는 realworld-oatpp Conduit API의 20개 엔드포인트를 다룹니다.

---

## 인증

- **방식**: JWT (JSON Web Token), HS256 알고리즘
- **헤더 형식**: `Authorization: Token <jwt>`
- **토큰 페이로드**:
  - `exp`: 만료 타임스탬프 (생성 시점 기준 60일 후)
  - `id`: 사용자 ID (정수)
  - `username`: 사용자 이름 (문자열)
- **비밀번호 해싱**: BCrypt, cost factor 12

---

## 오류 형식

모든 오류 응답은 다음 구조를 따릅니다:

```json
{
  "errors": {
    "message": ["오류 설명"]
  }
}
```

## 상태 코드

| 코드 | 의미 |
|------|------|
| 200  | 성공 |
| 400  | 유효성 검사 오류 |
| 500  | 인증 필요 / 서버 오류 |

---

## 사용자 엔드포인트 (8개)

### POST /api/users/login

이메일과 비밀번호로 로그인합니다. 성공 시 JWT 토큰을 반환합니다.

- **인증**: 불필요
- **요청 본문**:
```json
{
  "user": {
    "email": "jake@example.com",
    "password": "password123"
  }
}
```
- **응답** (200):
```json
{
  "user": {
    "username": "jake",
    "email": "jake@example.com",
    "bio": "소개 텍스트",
    "image": "https://example.com/photo.jpg",
    "token": "<jwt>"
  }
}
```

---

### POST /api/users

새로운 사용자 계정을 등록합니다.

- **인증**: 불필요
- **요청 본문**:
```json
{
  "user": {
    "username": "jake",
    "email": "jake@example.com",
    "password": "password123"
  }
}
```
- **유효성 검사**:
  - `username`: 필수
  - `email`: 유효한 이메일 형식
  - `password`: 최소 8자
- **응답** (200): 로그인 응답과 동일한 구조

---

### GET /api/user

현재 인증된 사용자 정보를 조회합니다.

- **인증**: 필수
- **응답** (200):
```json
{
  "user": {
    "username": "jake",
    "email": "jake@example.com",
    "bio": "소개 텍스트",
    "image": "https://example.com/photo.jpg",
    "token": "<jwt>"
  }
}
```

---

### PUT /api/user

현재 인증된 사용자를 수정합니다. 모든 필드는 선택 사항입니다.

- **인증**: 필수
- **요청 본문**:
```json
{
  "user": {
    "username": "jake_updated",
    "email": "newemail@example.com",
    "bio": "수정된 소개",
    "image": "https://example.com/new-photo.jpg",
    "password": "newpassword123"
  }
}
```
- **응답** (200): 로그인 응답과 동일한 구조

---

### GET /api/users/all/{offset}/{limit}

모든 사용자를 페이지네이션으로 조회합니다.

- **인증**: 불필요
- **경로 매개변수**:
  - `offset`: 건너뛸 사용자 수
  - `limit`: 반환할 최대 사용자 수

---

### GET /api/profiles/{username}

사용자 이름으로 프로필을 조회합니다.

- **인증**: 필수
- **경로 매개변수**:
  - `username`: 조회할 사용자 이름
- **응답** (200):
```json
{
  "profile": {
    "id": 1,
    "username": "jake",
    "bio": "소개 텍스트",
    "image": "https://example.com/photo.jpg",
    "following": false
  }
}
```

---

### POST /api/profiles/{username}/follow

사용자를 팔로우합니다.

- **인증**: 필수
- **경로 매개변수**:
  - `username`: 팔로우할 사용자 이름
- **응답** (200): `"following": true`가 포함된 프로필 객체

---

### DELETE /api/profiles/{username}/follow

사용자 팔로우를 취소합니다.

- **인증**: 필수
- **경로 매개변수**:
  - `username`: 팔로우 취소할 사용자 이름
- **응답** (200): `"following": false`가 포함된 프로필 객체

---

## 게시글 엔드포인트 (9개)

### POST /api/articles

새 게시글을 작성합니다.

- **인증**: 필수
- **요청 본문**:
```json
{
  "article": {
    "title": "드래곤 훈련법",
    "description": "어떻게 하는지 궁금하셨죠?",
    "body": "믿어야 합니다...",
    "tagList": ["dragons", "training"]
  }
}
```
- **유효성 검사**:
  - `title`: 최소 2자
  - `description`: 최소 2자
  - `body`: 최소 2자

---

### PUT /api/articles/{slug}

기존 게시글을 수정합니다.

- **인증**: 필수
- **경로 매개변수**:
  - `slug`: 게시글 슬러그 식별자

---

### DELETE /api/articles/{slug}

게시글을 삭제합니다.

- **인증**: 필수
- **경로 매개변수**:
  - `slug`: 게시글 슬러그 식별자

---

### GET /api/articles

선택적 필터링으로 게시글 목록을 조회합니다.

- **인증**: 필수
- **쿼리 매개변수**:
  - `tag`: 태그로 필터링
  - `author`: 작성자 이름으로 필터링
  - `favorited`: 즐겨찾기한 사용자로 필터링
  - `offset`: 페이지네이션 오프셋
  - `limit`: 페이지네이션 제한

---

### GET /api/articles/feed

팔로우한 사용자의 게시글을 조회합니다.

- **인증**: 필수
- **쿼리 매개변수**:
  - `offset`: 페이지네이션 오프셋
  - `limit`: 페이지네이션 제한

---

### GET /api/articles/{slug}

슬러그로 단일 게시글을 조회합니다.

- **인증**: 선택 (publicMode 활성화 시 비인증 접근 허용)
- **경로 매개변수**:
  - `slug`: 게시글 슬러그 식별자

---

### POST /api/articles/{slug}/favorite

게시글을 즐겨찾기에 추가합니다.

- **인증**: 필수
- **경로 매개변수**:
  - `slug`: 게시글 슬러그 식별자

---

### DELETE /api/articles/{slug}/favorite

게시글 즐겨찾기를 취소합니다.

- **인증**: 필수
- **경로 매개변수**:
  - `slug`: 게시글 슬러그 식별자

---

### GET /api/tags

모든 태그를 조회합니다.

- **인증**: 불필요
- **응답** (200):
```json
{
  "tags": ["dragons", "training", "reactjs"]
}
```

---

## 댓글 엔드포인트 (3개)

### GET /api/articles/{slug}/comments

게시글의 모든 댓글을 조회합니다.

- **인증**: 불필요
- **경로 매개변수**:
  - `slug`: 게시글 슬러그 식별자

---

### POST /api/articles/{slug}/comments

게시글에 댓글을 작성합니다.

- **인증**: 선택 (publicMode 활성화 시 비인증 접근 허용)
- **경로 매개변수**:
  - `slug`: 게시글 슬러그 식별자
- **요청 본문**:
```json
{
  "comment": {
    "body": "좋은 글이네요!"
  }
}
```

---

### DELETE /api/articles/{slug}/comments/{cid}

댓글을 삭제합니다.

- **인증**: 선택 (publicMode 활성화 시 비인증 접근 허용)
- **경로 매개변수**:
  - `slug`: 게시글 슬러그 식별자
  - `cid`: 댓글 ID
