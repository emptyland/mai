#include "sql/ast-factory.h"
#include "base/arena-utils.h"
#include "base/standalone-arena.h"
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
    base::StandaloneArena arena_;
    AstFactory factory_;
};
    
TEST_F(AstFactoryTest, Sanity) {

}

} // namespace sql
    
} // namespace mai
