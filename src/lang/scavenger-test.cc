#include "lang/scavenger.h"
#include "test/isolate-initializer.h"
#include "gtest/gtest.h"

namespace mai {

namespace lang {

class ScavengerTest : public test::IsolateInitializer {
public:
    ScavengerTest() {}
    
    void SetUp() override {
        IsolateInitializer::SetUp();
    }
    void TearDown() override {
        IsolateInitializer::TearDown();
    }
}; // class ScavengerTest

TEST_F(ScavengerTest, Sanity) {
    HandleScope handle_scope(HandleScope::INITIALIZER);
    Local<String> v1(String::NewUtf8("Hello"));
    
    Scavenger collector(isolate_, isolate_->heap());
    collector.Run();

    ASSERT_STREQ("Hello", v1->data());
}

} // namespace lang

} // namespace mai
