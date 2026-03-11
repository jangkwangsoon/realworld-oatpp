#ifndef MyApiTestClient_hpp
#define MyApiTestClient_hpp

#include "dto/UserDto.hpp"
#include "dto/ArticleDto.hpp"
#include "dto/CommentDto.hpp"

#include "oatpp/web/client/ApiClient.hpp"
#include "oatpp/core/macro/codegen.hpp"

#include OATPP_CODEGEN_BEGIN(ApiClient)

class MyApiTestClient : public oatpp::web::client::ApiClient {

  API_CLIENT_INIT(MyApiTestClient)

  // User endpoints (8)
  API_CALL("POST", "/api/users/login", login, BODY_DTO(Object<LoginUserDto>, dto))
  API_CALL("POST", "/api/users", createUser, BODY_DTO(Object<NewUserDto>, dto))
  API_CALL("GET", "/api/user", getUser, HEADER(String, auth, "Authorization"))
  API_CALL("PUT", "/api/user", updateUser, HEADER(String, auth, "Authorization"), BODY_DTO(Object<UpdateUserDto>, dto))
  API_CALL("GET", "/api/users/all/{offset}/{limit}", getAllUsers, PATH(String, offset), PATH(String, limit))
  API_CALL("GET", "/api/profiles/{username}", getProfile, HEADER(String, auth, "Authorization"), PATH(String, username))
  API_CALL("POST", "/api/profiles/{username}/follow", follow, HEADER(String, auth, "Authorization"), PATH(String, username))
  API_CALL("DELETE", "/api/profiles/{username}/follow", unfollow, HEADER(String, auth, "Authorization"), PATH(String, username))

  // Article endpoints (9)
  API_CALL("POST", "/api/articles", createArticle, HEADER(String, auth, "Authorization"), BODY_DTO(Object<NewArticleDto>, dto))
  API_CALL("GET", "/api/articles", getArticles, HEADER(String, auth, "Authorization"))
  API_CALL("GET", "/api/articles/feed", getArticlesFeed, HEADER(String, auth, "Authorization"))
  API_CALL("GET", "/api/articles/{slug}", getArticle, HEADER(String, auth, "Authorization"), PATH(String, slug))
  API_CALL("PUT", "/api/articles/{slug}", updateArticle, HEADER(String, auth, "Authorization"), PATH(String, slug), BODY_DTO(Object<UpdateArticleDto>, dto))
  API_CALL("DELETE", "/api/articles/{slug}", deleteArticle, HEADER(String, auth, "Authorization"), PATH(String, slug))
  API_CALL("POST", "/api/articles/{slug}/favorite", favorite, HEADER(String, auth, "Authorization"), PATH(String, slug))
  API_CALL("DELETE", "/api/articles/{slug}/favorite", unfavorite, HEADER(String, auth, "Authorization"), PATH(String, slug))
  API_CALL("GET", "/api/tags", getTags)

  // Comment endpoints (3)
  API_CALL("GET", "/api/articles/{slug}/comments", getComments, PATH(String, slug))
  API_CALL("POST", "/api/articles/{slug}/comments", createComment, HEADER(String, auth, "Authorization"), PATH(String, slug), BODY_DTO(Object<CommentInPakDto>, dto))
  API_CALL("DELETE", "/api/articles/{slug}/comments/{cid}", deleteComment, HEADER(String, auth, "Authorization"), PATH(String, slug), PATH(String, cid))

};

#include OATPP_CODEGEN_END(ApiClient)

#endif // MyApiTestClient_hpp
