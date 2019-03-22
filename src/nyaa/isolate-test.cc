#include "mai-lang/isolate.h"
#include "mai-lang/nyaa.h"
#include "gtest/gtest.h"

namespace mai {

namespace nyaa {
    
class IsolateTest : public ::testing::Test {
public:
    IsolateTest() : isolate_(Isolate::New()) {}
    
    ~IsolateTest() override { isolate_->Dispose(); }
    
    void SetUp() override { isolate_->Enter(); }
    
    void TearDown() override { isolate_->Exit(); }
    
    Isolate *isolate_;
};
    
TEST_F(IsolateTest, Sanity) {
    ASSERT_EQ(Isolate::Current(), isolate_);
    
    auto inner = Isolate::New();
    inner->Enter();
    ASSERT_EQ(Isolate::Current(), inner);
    inner->Exit();
    inner->Dispose();
}
    
TEST_F(IsolateTest, NyaaScope) {
    Nyaa nyaa(Nyaa::Options(), Isolate::Current());
    
    ASSERT_EQ(Isolate::Current()->GetNyaa(), &nyaa);
    {
        Nyaa inner(Nyaa::Options(), Isolate::Current());
        ASSERT_EQ(Isolate::Current()->GetNyaa(), &inner);
    }
    ASSERT_EQ(Isolate::Current()->GetNyaa(), &nyaa);
    
    //Handle<String> s = String::New(&nyaa, "ok");
}
    
} // namespace nyaa

} // namespace mai
