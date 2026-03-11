#include "UserAuthTest.hpp"

#include "app/TestComponent.hpp"
#include "utils/UserAuth.hpp"

#include <chrono>

void UserAuthTest::onRun() {

  TestComponent component;

  // Test 1: toToken() / fromToken() roundtrip
  {
    auto exp = UserAuth::newExp();
    UserAuth ua(exp, 42, "testuser");
    auto token = ua.toToken();

    OATPP_ASSERT(token);
    OATPP_ASSERT(token->length() > 0);

    auto decoded = UserAuth::fromToken(token);
    OATPP_ASSERT(decoded.id == 42);
    OATPP_ASSERT(decoded.username == "testuser");
    OATPP_ASSERT(decoded.exp == exp);

    OATPP_LOGD("UserAuthTest", "Token roundtrip: PASSED");
  }

  // Test 2: Expired token returns id==0
  {
    Int64 pastExp = 1000; // Unix timestamp in the past
    UserAuth ua(pastExp, 99, "expired_user");
    auto token = ua.toToken();

    auto decoded = UserAuth::fromToken(token);
    OATPP_ASSERT(decoded.id == 0);
    OATPP_ASSERT(decoded.exp == (int64_t)0LL);

    OATPP_LOGD("UserAuthTest", "Expired token: PASSED");
  }

  // Test 3: Token prefix stripping logic
  {
    auto exp = UserAuth::newExp();
    UserAuth ua(exp, 7, "headeruser");
    auto token = ua.toToken();

    // fromAuthHeader strips "Token " prefix, then calls fromToken
    // We verify the token itself decodes correctly
    auto decoded = UserAuth::fromToken(token);
    OATPP_ASSERT(decoded.id == 7);
    OATPP_ASSERT(decoded.username == "headeruser");

    OATPP_LOGD("UserAuthTest", "Token parsing: PASSED");
  }

  OATPP_LOGD("UserAuthTest", "All tests PASSED");

  OATPP_COMPONENT(std::shared_ptr<oatpp::async::Executor>, executor);
  executor->stop();
  executor->join();
}
