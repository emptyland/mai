#include "mai-lang/nyaa.h"
#include "test/nyaa-test.h"

namespace mai {
    
namespace nyaa {
    
class HandlesTest : public test::NyaaTest {
public:
    void SetUp() override {
        NyaaTest::SetUp();
        N_ = new Nyaa(Nyaa::Options(), isolate_);
    }
    
    void TearDown() override {
        delete N_;
        NyaaTest::TearDown();
    }
    
    Nyaa *N_ = nullptr;
};

TEST_F(HandlesTest, Sanity) {
    HandleScope scope(N_);

    Local<Value> h;
    ASSERT_TRUE(h.is_empty());

    Local<Value> m(h);
    ASSERT_TRUE(m.is_empty());

    Value *v = nullptr;
    Local<Value> n = Local<Value>::New(v);
    ASSERT_FALSE(n.is_empty());
    ASSERT_TRUE(n.is_null());
    
    Local<String> s = Local<String>::Cast(n);
    ASSERT_FALSE(s.is_empty());
    ASSERT_TRUE(s.is_null());
}

int firstUniqChar(std::string s) {
    int lookup[256] = {0};
    memset(lookup, 0, sizeof(lookup));
    int index[256] = {0};
    memset(index, -1, sizeof(lookup));

    if (s.size() == 0) {
        return -1;
    }
    if (s.size() == 1) {
        return 0;
    }

    for (int i = 0; i < s.size(); i++) {
        int c = s[i];
        int n = lookup[c];
        if (n == 0 && index[c] == -1) {
            index[c] = i;
        }
        lookup[c]++;
    }
    for (int i = 0; i < s.size(); i++) {
        int c = s[i];
        if (lookup[c] == 1) {
            return index[c];
        }
    }
    
    return -1;
}

TEST_F(HandlesTest, Test1) {
    ASSERT_EQ(2, firstUniqChar("loveleetcode"));
}
    
} // namespace nyaa
    
} // namespace mai
