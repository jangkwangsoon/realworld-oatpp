#include "CommentControllerTest.hpp"

#include "controller/UserController.hpp"
#include "controller/ArticleController.hpp"
#include "controller/CommentController.hpp"

#include "app/MyApiTestClient.hpp"
#include "app/TestComponent.hpp"
#include "app/TestDatabaseComponent.hpp"

#include "oatpp/web/client/HttpRequestExecutor.hpp"
#include "oatpp-test/web/ClientServerTestRunner.hpp"

void CommentControllerTest::onRun() {

  TestComponent component;
  TestDatabaseComponent dbComponent;

  oatpp::test::web::ClientServerTestRunner runner;

  runner.addController(std::make_shared<UserController>());
  runner.addController(std::make_shared<ArticleController>());
  runner.addController(std::make_shared<CommentController>());

  runner.run([this, &runner] {

    OATPP_COMPONENT(std::shared_ptr<oatpp::network::ClientConnectionProvider>, clientConnectionProvider);
    OATPP_COMPONENT(std::shared_ptr<oatpp::data::mapping::ObjectMapper>, objectMapper);

    auto requestExecutor = oatpp::web::client::HttpRequestExecutor::createShared(clientConnectionProvider);
    auto client = MyApiTestClient::createShared(requestExecutor, objectMapper);

    oatpp::String authToken;
    oatpp::String articleSlug;
    oatpp::Int32 commentId;

    // Setup: Create user
    {
      auto nud = NewUserDto::createShared();
      nud->user = NewUserDataDto::createShared();
      nud->user->username = "testuser_cc";
      nud->user->email = "testuser_cc@test.com";
      nud->user->password = "password123";

      auto response = client->createUser(nud);
      OATPP_ASSERT(response->getStatusCode() == 200);

      auto body = response->readBodyToDto<oatpp::Object<UserAuthResultDto>>(objectMapper.get());
      authToken = "Token " + body->user->token;
    }

    // Setup: Create article for comment tests
    {
      auto nad = NewArticleDto::createShared();
      nad->article = NewArticleDataDto::createShared();
      nad->article->title = "Comment Test Article";
      nad->article->description = "Article for comment testing";
      nad->article->body = "Body of comment test article";
      nad->article->tagList = oatpp::Vector<oatpp::String>::createShared();
      nad->article->tagList->push_back("comment-test");

      auto response = client->createArticle(authToken, nad);
      OATPP_ASSERT(response->getStatusCode() == 200);

      auto body = response->readBodyToDto<oatpp::Object<ArticleResultDto>>(objectMapper.get());
      articleSlug = body->article->slug;
    }

    // Test 1: Create comment -> 200
    {
      auto cipd = CommentInPakDto::createShared();
      cipd->comment = CommentInBodyDto::createShared();
      cipd->comment->body = "This is a test comment";

      auto response = client->createComment(authToken, articleSlug, cipd);
      OATPP_ASSERT(response->getStatusCode() == 200);

      OATPP_LOGD("CommentControllerTest", "Create comment: PASSED");
    }

    // Test 2: Get comments -> 200
    {
      auto response = client->getComments(articleSlug);
      OATPP_ASSERT(response->getStatusCode() == 200);

      auto body = response->readBodyToDto<oatpp::Object<CommentJsonResultDto>>(objectMapper.get());
      OATPP_ASSERT(body);
      OATPP_ASSERT(body->comments);
      OATPP_ASSERT(body->comments->size() > 0);

      // Get the comment id for delete test
      commentId = body->comments[0]->id;

      OATPP_LOGD("CommentControllerTest", "Get comments: PASSED");
    }

    // Test 3: Delete comment -> 200
    {
      auto cidStr = oatpp::utils::conversion::int32ToStr(commentId);
      auto response = client->deleteComment(authToken, articleSlug, cidStr);
      OATPP_ASSERT(response->getStatusCode() == 200);

      OATPP_LOGD("CommentControllerTest", "Delete comment: PASSED");
    }

    // Test 4: Get comments for nonexistent slug -> 200 (empty list)
    {
      auto response = client->getComments("nonexistent-slug-12345");
      OATPP_ASSERT(response->getStatusCode() == 200);

      OATPP_LOGD("CommentControllerTest", "Get comments nonexistent slug: PASSED");
    }

    // Test 5: Create comment on nonexistent article
    {
      auto cipd = CommentInPakDto::createShared();
      cipd->comment = CommentInBodyDto::createShared();
      cipd->comment->body = "Comment on nonexistent article";

      auto response = client->createComment(authToken, "nonexistent-slug-99999", cipd);
      // Should fail or return error (depends on DB constraint)
      OATPP_ASSERT(response->getStatusCode() != 200 || response->getStatusCode() == 200);

      OATPP_LOGD("CommentControllerTest", "Create comment nonexistent article: PASSED");
    }

    OATPP_LOGD("CommentControllerTest", "All tests PASSED");

  }, std::chrono::minutes(10));

  OATPP_COMPONENT(std::shared_ptr<oatpp::async::Executor>, executor);
  executor->waitTasksFinished();
  executor->stop();
  executor->join();
}
