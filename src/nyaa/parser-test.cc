#include "nyaa/parser.h"
#include "nyaa/ast.h"
#include "test/nyaa-test.h"
#include "base/arenas.h"
#include "mai-lang/nyaa.h"

namespace mai {
    
namespace nyaa {

class NyaaParserTest : public test::NyaaTest {
public:
    NyaaParserTest()
        : arena_(isolate_->env()->GetLowLevelAllocator()) {
    }
    
    base::StandaloneArena arena_;
};

TEST_F(NyaaParserTest, Sanity) {
    static const char z[] = {
        "var a, b, c = 1, 2, 3\n"
        "return a + b / c\n"
    };
    
    auto result = Parser::Parse(z, &arena_);
    ASSERT_EQ(nullptr, result.error) << result.ToString();
    ASSERT_NE(nullptr, result.block);
}
    
} // namespace nyaa
    
} // namespace mai
