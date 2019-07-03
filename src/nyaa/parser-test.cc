#include "nyaa/parser.h"
#include "nyaa/ast.h"
#include "nyaa/code-gen-utils.h"
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
        "var a = t.name\n"
        "var b = t.t.a\n"
        "var c = t.t['name']\n"
        "var d = t().name\n"
        "var e = t:f()['name']\n"
    };
    
    auto result = Parser::Parse(z, &arena_);
    ASSERT_EQ(nullptr, result.error) << result.ToString();
    ASSERT_NE(nullptr, result.block);
}

TEST_F(NyaaParserTest, Expression) {
    static const char z[] = {
        "var a, b, c = 1, 2, 3\n"
        "return a + b / c\n"
    };
    
    auto result = Parser::Parse(z, &arena_);
    ASSERT_EQ(nullptr, result.error) << result.ToString();
    ASSERT_NE(nullptr, result.block);
}
    
TEST_F(NyaaParserTest, LambdaLiteral) {
    static const char z[] = {
        "var f = lambda(a) { return a } \n"
        "return f(0)\n"
    };
    
    auto result = Parser::Parse(z, &arena_);
    ASSERT_EQ(nullptr, result.error) << result.ToString();
    ASSERT_NE(nullptr, result.block);
}
    
TEST_F(NyaaParserTest, Serialize) {
    static const char z[] = {
        "var f = lambda(a) { return a } \n"
        "return f(0)\n"
    };
    
    auto result = Parser::Parse(z, &arena_);
    ASSERT_EQ(nullptr, result.error) << result.ToString();
    ASSERT_NE(nullptr, result.block);
    
    std::string buf;
    SerializeCompactedAST(result.block, &buf);
    ASSERT_EQ(41, buf.size());
    
    ast::Factory factory(&arena_);
    ast::AstNode *ast = nullptr;
    LoadCompactedAST(buf, &factory, &ast);
    ASSERT_NE(nullptr, ast);
    
    ast::Block *block = ast->ToBlock();
    ASSERT_NE(nullptr, block);
    ASSERT_EQ(2, block->stmts()->size());
    ASSERT_EQ(1, block->line());
    ASSERT_EQ(2, block->end_line());
    
    ast::VarDeclaration *vd = block->stmts()->at(0)->ToVarDeclaration();
    ASSERT_NE(nullptr, vd);
    ASSERT_EQ(1, vd->names()->size());
    ASSERT_EQ(1, vd->inits()->size());
    
    ast::Return *rt = block->stmts()->at(1)->ToReturn();
    ASSERT_NE(nullptr, rt);
    ASSERT_EQ(1, rt->rets()->size());
}

} // namespace nyaa
    
} // namespace mai
