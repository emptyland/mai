#include "lang/scavenger.h"
#include "lang/heap.h"
#include "test/isolate-initializer.h"
#include "base/slice.h"
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
    Persistent<String> v2 = Persistent<String>::New(String::NewUtf8("World"));
    String *p1 = *v1, *p2 = *v2;

    Scavenger collector(isolate_, isolate_->heap());
    base::StringBuildingPrinter logger;
    collector.Run(&logger);

    ASSERT_STREQ("Hello", v1->data());
    ASSERT_STREQ("World", v2->data());
    ASSERT_NE(p1, *v1);
    ASSERT_NE(p2, *v2);
    
    v2.Dispose();
}

TEST_F(ScavengerTest, Promote) {
    HandleScope handle_scope(HandleScope::INITIALIZER);
    Local<String> v1(String::NewUtf8("Hello"));
    Persistent<String> v2 = Persistent<String>::New(String::NewUtf8("World"));

    Scavenger collector(isolate_, isolate_->heap());
    base::StringBuildingPrinter logger;
    collector.set_force_promote(true);
    collector.Run(&logger);

    ASSERT_TRUE(STATE->heap()->InNewArea(*v1));
    ASSERT_TRUE(STATE->heap()->InNewArea(*v2));
    collector.Run(&logger);
    
    ASSERT_TRUE(STATE->heap()->InOldArea(*v1));
    ASSERT_TRUE(STATE->heap()->InOldArea(*v2));
    
    v2.Dispose();
}

TEST_F(ScavengerTest, InternalObjectsMovable) {
    HandleScope handle_scope(HandleScope::INITIALIZER);
    String *empty = STATE->factory()->empty_string();
    String *oom = STATE->factory()->oom_text();
    
    Scavenger collector(isolate_, isolate_->heap());
    base::StringBuildingPrinter logger;
    collector.Run(&logger);

    ASSERT_EQ(empty, STATE->factory()->empty_string());
    ASSERT_EQ(oom, STATE->factory()->oom_text());
}

} // namespace lang

} // namespace mai
