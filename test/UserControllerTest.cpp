#include "UserControllerTest.hpp"

#include "controller/UserController.hpp"

#include "app/MyApiTestClient.hpp"
#include "app/TestComponent.hpp"
#include "app/TestDatabaseComponent.hpp"

#include "oatpp/web/client/HttpRequestExecutor.hpp"
#include "oatpp-test/web/ClientServerTestRunner.hpp"

void UserControllerTest::onRun() {

  TestComponent component;
  TestDatabaseComponent dbComponent;

  oatpp::test::web::ClientServerTestRunner runner;

  runner.addController(std::make_shared<UserController>());

  runner.run([this, &runner] {

    OATPP_COMPONENT(std::shared_ptr<oatpp::network::ClientConnectionProvider>, clientConnectionProvider);
    OATPP_COMPONENT(std::shared_ptr<oatpp::data::mapping::ObjectMapper>, objectMapper);

    auto requestExecutor = oatpp::web::client::HttpRequestExecutor::createShared(clientConnectionProvider);
    auto client = MyApiTestClient::createShared(requestExecutor, objectMapper);

    oatpp::String authToken;

    // Test 1: Create user with valid data -> 200
    {
      auto nud = NewUserDto::createShared();
      nud->user = NewUserDataDto::createShared();
      nud->user->username = "testuser_uc";
      nud->user->email = "testuser_uc@test.com";
      nud->user->password = "password123";

      auto response = client->createUser(nud);
      OATPP_ASSERT(response->getStatusCode() == 200);

      auto body = response->readBodyToDto<oatpp::Object<UserAuthResultDto>>(objectMapper.get());
      OATPP_ASSERT(body);
      OATPP_ASSERT(body->user);
      OATPP_ASSERT(body->user->token);
      OATPP_ASSERT(body->user->username == "testuser_uc");
      OATPP_ASSERT(body->user->email == "testuser_uc@test.com");

      authToken = "Token " + body->user->token;

      OATPP_LOGD("UserControllerTest", "Create user: PASSED");
    }

    // Test 2: Create user with missing fields -> 400
    {
      auto nud = NewUserDto::createShared();
      nud->user = NewUserDataDto::createShared();
      // missing username, email, password

      auto response = client->createUser(nud);
      OATPP_ASSERT(response->getStatusCode() == 400);

      OATPP_LOGD("UserControllerTest", "Create user missing fields: PASSED");
    }

    // Test 3: Create user with short password -> 400
    {
      auto nud = NewUserDto::createShared();
      nud->user = NewUserDataDto::createShared();
      nud->user->username = "shortpw";
      nud->user->email = "shortpw@test.com";
      nud->user->password = "short";

      auto response = client->createUser(nud);
      OATPP_ASSERT(response->getStatusCode() == 400);

      OATPP_LOGD("UserControllerTest", "Create user short password: PASSED");
    }

    // Test 4: Login with valid credentials -> 200
    {
      auto lud = LoginUserDto::createShared();
      lud->user = LoginUserDataDto::createShared();
      lud->user->email = "testuser_uc@test.com";
      lud->user->password = "password123";

      auto response = client->login(lud);
      OATPP_ASSERT(response->getStatusCode() == 200);

      auto body = response->readBodyToDto<oatpp::Object<UserAuthResultDto>>(objectMapper.get());
      OATPP_ASSERT(body);
      OATPP_ASSERT(body->user);
      OATPP_ASSERT(body->user->token);

      OATPP_LOGD("UserControllerTest", "Login valid: PASSED");
    }

    // Test 5: Login with wrong password -> error
    {
      auto lud = LoginUserDto::createShared();
      lud->user = LoginUserDataDto::createShared();
      lud->user->email = "testuser_uc@test.com";
      lud->user->password = "wrongpassword";

      auto response = client->login(lud);
      OATPP_ASSERT(response->getStatusCode() != 200);

      OATPP_LOGD("UserControllerTest", "Login wrong password: PASSED");
    }

    // Test 6: Get current user with auth -> 200
    {
      auto response = client->getUser(authToken);
      OATPP_ASSERT(response->getStatusCode() == 200);

      auto body = response->readBodyToDto<oatpp::Object<UserAuthResultDto>>(objectMapper.get());
      OATPP_ASSERT(body);
      OATPP_ASSERT(body->user);
      OATPP_ASSERT(body->user->username == "testuser_uc");

      OATPP_LOGD("UserControllerTest", "Get user with auth: PASSED");
    }

    // Test 7: Get current user without auth -> 500
    {
      auto response = client->getUser("");
      OATPP_ASSERT(response->getStatusCode() == 500);

      OATPP_LOGD("UserControllerTest", "Get user no auth: PASSED");
    }

    // Test 8: Update user -> 200
    {
      auto uud = UpdateUserDto::createShared();
      uud->user = UpdateUserDataDto::createShared();
      uud->user->username = "testuser_uc";
      uud->user->email = "testuser_uc@test.com";
      uud->user->bio = "Updated bio for test";
      uud->user->image = "";

      auto response = client->updateUser(authToken, uud);
      OATPP_ASSERT(response->getStatusCode() == 200);

      OATPP_LOGD("UserControllerTest", "Update user: PASSED");
    }

    // Test 9: Get profile -> 200
    {
      auto response = client->getProfile(authToken, "testuser_uc");
      OATPP_ASSERT(response->getStatusCode() == 200);

      auto body = response->readBodyToDto<oatpp::Object<UserProfileResultDto>>(objectMapper.get());
      OATPP_ASSERT(body);
      OATPP_ASSERT(body->profile);
      OATPP_ASSERT(body->profile->username == "testuser_uc");

      OATPP_LOGD("UserControllerTest", "Get profile: PASSED");
    }

    // Test 10: Create second user for follow tests
    {
      auto nud = NewUserDto::createShared();
      nud->user = NewUserDataDto::createShared();
      nud->user->username = "testuser_uc2";
      nud->user->email = "testuser_uc2@test.com";
      nud->user->password = "password123";

      auto response = client->createUser(nud);
      OATPP_ASSERT(response->getStatusCode() == 200);
    }

    // Test 11: Follow user -> 200
    {
      auto response = client->follow(authToken, "testuser_uc2");
      OATPP_ASSERT(response->getStatusCode() == 200);

      OATPP_LOGD("UserControllerTest", "Follow: PASSED");
    }

    // Test 12: Unfollow user -> 200
    {
      auto response = client->unfollow(authToken, "testuser_uc2");
      OATPP_ASSERT(response->getStatusCode() == 200);

      OATPP_LOGD("UserControllerTest", "Unfollow: PASSED");
    }

    // Test 13: Get all users -> 200
    {
      auto response = client->getAllUsers("0", "10");
      OATPP_ASSERT(response->getStatusCode() == 200);

      OATPP_LOGD("UserControllerTest", "Get all users: PASSED");
    }

    // Test 14: Duplicate email registration -> error
    {
      auto nud = NewUserDto::createShared();
      nud->user = NewUserDataDto::createShared();
      nud->user->username = "testuser_uc_dup";
      nud->user->email = "testuser_uc@test.com"; // already exists
      nud->user->password = "password123";

      auto response = client->createUser(nud);
      OATPP_ASSERT(response->getStatusCode() != 200);

      OATPP_LOGD("UserControllerTest", "Duplicate email: PASSED");
    }

    // Test 15: Login with missing fields -> 400
    {
      auto lud = LoginUserDto::createShared();
      lud->user = LoginUserDataDto::createShared();
      // missing email and password

      auto response = client->login(lud);
      OATPP_ASSERT(response->getStatusCode() == 400);

      OATPP_LOGD("UserControllerTest", "Login missing fields: PASSED");
    }

    // Test 16: Invalid email format -> 400
    {
      auto nud = NewUserDto::createShared();
      nud->user = NewUserDataDto::createShared();
      nud->user->username = "invalidemail";
      nud->user->email = "not-an-email";
      nud->user->password = "password123";

      auto response = client->createUser(nud);
      OATPP_ASSERT(response->getStatusCode() == 400);

      OATPP_LOGD("UserControllerTest", "Invalid email format: PASSED");
    }

    // Test 17: Update user without auth -> 500
    {
      auto uud = UpdateUserDto::createShared();
      uud->user = UpdateUserDataDto::createShared();
      uud->user->bio = "Should fail";

      auto response = client->updateUser("", uud);
      OATPP_ASSERT(response->getStatusCode() == 500);

      OATPP_LOGD("UserControllerTest", "Update user no auth: PASSED");
    }

    // Test 18: Get profile without auth -> 500
    {
      auto response = client->getProfile("", "testuser_uc");
      OATPP_ASSERT(response->getStatusCode() == 500);

      OATPP_LOGD("UserControllerTest", "Get profile no auth: PASSED");
    }

    // Test 19: Follow without auth -> 500
    {
      auto response = client->follow("", "testuser_uc2");
      OATPP_ASSERT(response->getStatusCode() == 500);

      OATPP_LOGD("UserControllerTest", "Follow no auth: PASSED");
    }

    OATPP_LOGD("UserControllerTest", "All tests PASSED");

  }, std::chrono::minutes(10));

  OATPP_COMPONENT(std::shared_ptr<oatpp::async::Executor>, executor);
  executor->waitTasksFinished();
  executor->stop();
  executor->join();
}
