#ifndef CommentControllerTest_hpp
#define CommentControllerTest_hpp

#include "oatpp-test/UnitTest.hpp"

class CommentControllerTest : public oatpp::test::UnitTest {
public:
  CommentControllerTest() : UnitTest("TEST[CommentControllerTest]") {}
  void onRun() override;
};

#endif // CommentControllerTest_hpp
