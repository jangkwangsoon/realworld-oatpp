#include "ArticleControllerTest.hpp"

#include "controller/UserController.hpp"
#include "controller/ArticleController.hpp"

#include "app/MyApiTestClient.hpp"
#include "app/TestComponent.hpp"
#include "app/TestDatabaseComponent.hpp"

#include "oatpp/web/client/HttpRequestExecutor.hpp"
#include "oatpp-test/web/ClientServerTestRunner.hpp"

void ArticleControllerTest::onRun() {

  TestComponent component;
  TestDatabaseComponent dbComponent;

  oatpp::test::web::ClientServerTestRunner runner;

  runner.addController(std::make_shared<UserController>());
  runner.addController(std::make_shared<ArticleController>());

  runner.run([this, &runner] {

    OATPP_COMPONENT(std::shared_ptr<oatpp::network::ClientConnectionProvider>, clientConnectionProvider);
    OATPP_COMPONENT(std::shared_ptr<oatpp::data::mapping::ObjectMapper>, objectMapper);

    auto requestExecutor = oatpp::web::client::HttpRequestExecutor::createShared(clientConnectionProvider);
    auto client = MyApiTestClient::createShared(requestExecutor, objectMapper);

    oatpp::String authToken;
    oatpp::String articleSlug;

    // Setup: Create user for article tests
    {
      auto nud = NewUserDto::createShared();
      nud->user = NewUserDataDto::createShared();
      nud->user->username = "testuser_ac";
      nud->user->email = "testuser_ac@test.com";
      nud->user->password = "password123";

      auto response = client->createUser(nud);
      OATPP_ASSERT(response->getStatusCode() == 200);

      auto body = response->readBodyToDto<oatpp::Object<UserAuthResultDto>>(objectMapper.get());
      authToken = "Token " + body->user->token;
    }

    // Test 1: Create article -> 200
    {
      auto nad = NewArticleDto::createShared();
      nad->article = NewArticleDataDto::createShared();
      nad->article->title = "Test Article Title";
      nad->article->description = "Test article description";
      nad->article->body = "Test article body content";
      nad->article->tagList = oatpp::Vector<oatpp::String>::createShared();
      nad->article->tagList->push_back("test");
      nad->article->tagList->push_back("oatpp");

      auto response = client->createArticle(authToken, nad);
      OATPP_ASSERT(response->getStatusCode() == 200);

      auto body = response->readBodyToDto<oatpp::Object<ArticleResultDto>>(objectMapper.get());
      OATPP_ASSERT(body);
      OATPP_ASSERT(body->article);
      OATPP_ASSERT(body->article->title == "Test Article Title");
      articleSlug = body->article->slug;

      OATPP_LOGD("ArticleControllerTest", "Create article: PASSED");
    }

    // Test 2: Create article with short fields -> 400
    {
      auto nad = NewArticleDto::createShared();
      nad->article = NewArticleDataDto::createShared();
      nad->article->title = "X";
      nad->article->description = "Y";
      nad->article->body = "Z";
      nad->article->tagList = oatpp::Vector<oatpp::String>::createShared();

      auto response = client->createArticle(authToken, nad);
      OATPP_ASSERT(response->getStatusCode() == 400);

      OATPP_LOGD("ArticleControllerTest", "Create article short fields: PASSED");
    }

    // Test 3: Get articles list -> 200
    {
      auto response = client->getArticles(authToken);
      OATPP_ASSERT(response->getStatusCode() == 200);

      OATPP_LOGD("ArticleControllerTest", "Get articles: PASSED");
    }

    // Test 4: Get single article -> 200
    {
      auto response = client->getArticle(authToken, articleSlug);
      OATPP_ASSERT(response->getStatusCode() == 200);

      auto body = response->readBodyToDto<oatpp::Object<ArticleResultDto>>(objectMapper.get());
      OATPP_ASSERT(body);
      OATPP_ASSERT(body->article);
      OATPP_ASSERT(body->article->slug == articleSlug);

      OATPP_LOGD("ArticleControllerTest", "Get single article: PASSED");
    }

    // Test 5: Update article -> 200
    {
      auto uad = UpdateArticleDto::createShared();
      uad->article = UpdateArticleDataDto::createShared();
      uad->article->title = "Updated Article Title";
      uad->article->description = "Updated description";
      uad->article->body = "Updated body content";
      uad->article->slug = articleSlug;
      uad->article->tagList = oatpp::Vector<oatpp::String>::createShared();
      uad->article->tagList->push_back("updated");

      auto response = client->updateArticle(authToken, articleSlug, uad);
      OATPP_ASSERT(response->getStatusCode() == 200);

      // Update generates a new slug from the new title
      auto body = response->readBodyToDto<oatpp::Object<ArticleResultDto>>(objectMapper.get());
      if(body && body->article && body->article->slug) {
        articleSlug = body->article->slug;
      }

      OATPP_LOGD("ArticleControllerTest", "Update article: PASSED");
    }

    // Test 6: Get feed -> 200
    {
      auto response = client->getArticlesFeed(authToken);
      OATPP_ASSERT(response->getStatusCode() == 200);

      OATPP_LOGD("ArticleControllerTest", "Get feed: PASSED");
    }

    // Test 7: Favorite article -> 200
    {
      auto response = client->favorite(authToken, articleSlug);
      OATPP_ASSERT(response->getStatusCode() == 200);

      OATPP_LOGD("ArticleControllerTest", "Favorite: PASSED");
    }

    // Test 8: Unfavorite article -> 200
    {
      auto response = client->unfavorite(authToken, articleSlug);
      OATPP_ASSERT(response->getStatusCode() == 200);

      OATPP_LOGD("ArticleControllerTest", "Unfavorite: PASSED");
    }

    // Test 9: Get tags -> 200
    {
      auto response = client->getTags();
      OATPP_ASSERT(response->getStatusCode() == 200);

      OATPP_LOGD("ArticleControllerTest", "Get tags: PASSED");
    }

    // Test 10: Create article without auth -> 500
    {
      auto nad = NewArticleDto::createShared();
      nad->article = NewArticleDataDto::createShared();
      nad->article->title = "No Auth Article";
      nad->article->description = "Should fail";
      nad->article->body = "Should fail body";
      nad->article->tagList = oatpp::Vector<oatpp::String>::createShared();

      auto response = client->createArticle("", nad);
      OATPP_ASSERT(response->getStatusCode() == 500);

      OATPP_LOGD("ArticleControllerTest", "Create article no auth: PASSED");
    }

    // Test 11: Get articles without auth -> 500
    {
      auto response = client->getArticles("");
      OATPP_ASSERT(response->getStatusCode() == 500);

      OATPP_LOGD("ArticleControllerTest", "Get articles no auth: PASSED");
    }

    // Test 12: Get feed without auth -> 500
    {
      auto response = client->getArticlesFeed("");
      OATPP_ASSERT(response->getStatusCode() == 500);

      OATPP_LOGD("ArticleControllerTest", "Get feed no auth: PASSED");
    }

    // Test 13: Favorite without auth -> 500
    {
      auto response = client->favorite("", articleSlug);
      OATPP_ASSERT(response->getStatusCode() == 500);

      OATPP_LOGD("ArticleControllerTest", "Favorite no auth: PASSED");
    }

    // Test 14: Delete article -> 200
    {
      auto response = client->deleteArticle(authToken, articleSlug);
      OATPP_ASSERT(response->getStatusCode() == 200);

      OATPP_LOGD("ArticleControllerTest", "Delete article: PASSED");
    }

    OATPP_LOGD("ArticleControllerTest", "All tests PASSED");

  }, std::chrono::minutes(10));

  OATPP_COMPONENT(std::shared_ptr<oatpp::async::Executor>, executor);
  executor->waitTasksFinished();
  executor->stop();
  executor->join();
}
