#ifndef UserAuthTest_hpp
#define UserAuthTest_hpp

#include "oatpp-test/UnitTest.hpp"

class UserAuthTest : public oatpp::test::UnitTest {
public:
  UserAuthTest() : UnitTest("TEST[UserAuthTest]") {}
  void onRun() override;
};

#endif // UserAuthTest_hpp
