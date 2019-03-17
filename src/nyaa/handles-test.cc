#include "mai-lang/nyaa.h"
#include "test/nyaa-test.h"

namespace mai {
    
namespace nyaa {
    
class HandlesTest : public test::NyaaTest {
public:
    void SetUp() override {
        NyaaTest::SetUp();
        N_ = new Nyaa(isolate_);
    }
    
    void TearDown() override {
        delete N_;
        NyaaTest::TearDown();
    }
    
    Nyaa *N_ = nullptr;
};

TEST_F(HandlesTest, Sanity) {
    HandleScope scope(isolate_);
    
    Local<Value> h;
    ASSERT_TRUE(h.is_empty());
    
    Local<Value> m(h);
    ASSERT_TRUE(m.is_empty());
    
    Value *v = nullptr;
    Local<Value> n(v);
    ASSERT_FALSE(n.is_empty());
    ASSERT_TRUE(n.is_null());
}
    
} // namespace nyaa
    
} // namespace mai
