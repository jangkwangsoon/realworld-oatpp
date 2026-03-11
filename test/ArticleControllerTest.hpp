#ifndef ArticleControllerTest_hpp
#define ArticleControllerTest_hpp

#include "oatpp-test/UnitTest.hpp"

class ArticleControllerTest : public oatpp::test::UnitTest {
public:
  ArticleControllerTest() : UnitTest("TEST[ArticleControllerTest]") {}
  void onRun() override;
};

#endif // ArticleControllerTest_hpp
