#include "mai/env.h"
#include "gtest/gtest.h"

namespace mai {
    
TEST(TlsTest, EnvTls) {
    auto env = Env::Default();
    
    std::unique_ptr<ThreadLocalSlot> slot;
    
    Error rs = env->NewThreadLocalSlot("test", ::free, &slot);
    ASSERT_TRUE(rs.ok()) << rs.ToString();
    
    ASSERT_EQ(nullptr, slot->Get());
    
    int *n = static_cast<int *>(::malloc(sizeof(int)));
    *n = 996;
    slot->Set(n);
    
    ASSERT_EQ(n, slot->Get());
}
    
} // namespace mai
