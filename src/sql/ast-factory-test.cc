#include "sql/ast-factory.h"
#include "base/arena-utils.h"
#include "mai/env.h"
#include "gtest/gtest.h"

namespace mai {
    
namespace sql {
    
class AstFactoryTest : public ::testing::Test {
public:
    AstFactoryTest()
        : arena_(env_->GetLowLevelAllocator())
        , factory_(&arena_) {}
    
    Env *env_ = Env::Default();
    base::Arena arena_;
    AstFactory factory_;
};
    
TEST_F(AstFactoryTest, Sanity) {
    auto name = factory_.NewString("t1");
    auto node = factory_.NewCreateTable(name);
    ASSERT_NE(nullptr, node);
    ASSERT_TRUE(node->is_ddl());
    ASSERT_EQ(name, node->schema_name());
}

} // namespace sql
    
} // namespace mai
