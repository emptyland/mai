#include "nyaa/code-gen.h"
#include "nyaa/ast.h"
#include "nyaa/bytecode.h"
#include "nyaa/bytecode-builder.h"
#include "nyaa/nyaa-core.h"
#include "nyaa/nyaa-values.h"
#include "nyaa/object-factory.h"
#include "test/nyaa-test.h"
#include "base/arenas.h"
#include "mai-lang/nyaa.h"

namespace mai {
    
namespace nyaa {
    
class NyaaCodeGenTest : public test::NyaaTest {
public:
    NyaaCodeGenTest()
        : arena_(isolate_->env()->GetLowLevelAllocator())
        , af_(&arena_)
        , scope_(Nyaa::Options{}, isolate_)
        , N_(&scope_)
        , core_(scope_.core())
        , factory_(core_->factory()) {
    }
    
    base::StandaloneArena arena_;
    ast::Factory af_;
    Nyaa scope_;
    Nyaa *N_ = nullptr;
    NyaaCore *core_ = nullptr;
    ObjectFactory *factory_ = nullptr;
};

TEST_F(NyaaCodeGenTest, Sanity) {
    auto names = af_.NewList<const ast::String *>(ast::String::New(&arena_, "a"));
    names->push_back(ast::String::New(&arena_, "b"));
    names->push_back(ast::String::New(&arena_, "c"));
    
    auto stmts = af_.NewList<ast::Statement*>(af_.NewVarDeclaration(names, nullptr));
    stmts->push_back(af_.NewReturn(nullptr));

    auto block = af_.NewBlock(stmts);
    
    HandleScope handle_scope(isolate_);
    Handle<NyFunction> script(CodeGen::Generate(Handle<NyString>::Null(), block, core_));
    
    std::string buf;
    BytecodeArrayDisassembler::Disassembly(script->bcbuf(), script->file_info(), &buf, 512);

    static const char *z =
        "[000] LoadNil 0 3 ; line: 0\n"
        "[003] Ret 0 0 ; line: 0\n";
    ASSERT_EQ(z, buf);
}
    
TEST_F(NyaaCodeGenTest, VarInitializer) {
    auto names = af_.NewList<const ast::String *>(ast::String::New(&arena_, "a"));
    names->push_back(ast::String::New(&arena_, "b"));
    names->push_back(ast::String::New(&arena_, "c"));
    
    auto inits = af_.NewList<ast::Expression*>(af_.NewApproxLiteral(1.2));
    inits->push_back(af_.NewVariable(ast::String::New(&arena_, "name")));
    
    auto stmts = af_.NewList<ast::Statement*>(af_.NewVarDeclaration(names, inits));
    stmts->push_back(af_.NewReturn(nullptr));
    
    auto block = af_.NewBlock(stmts);
    
    HandleScope handle_scope(isolate_);
    Handle<NyFunction> script(CodeGen::Generate(Handle<NyString>::Null(), block, core_));

    std::string buf;
    BytecodeArrayDisassembler::Disassembly(script->bcbuf(), script->file_info(), &buf, 512);
    static const char z[] = {
        "[000] LoadConst 0 0 ; line: 0\n"
        "[003] LoadGlobal 1 1 ; line: 0\n"
        "[006] LoadNil 2 1 ; line: 0\n"
        "[009] Ret 0 0 ; line: 0\n"
    };
    ASSERT_EQ(z, buf);
}

TEST_F(NyaaCodeGenTest, VarInitializerCalling) {
    auto names = af_.NewList<const ast::String *>(ast::String::New(&arena_, "a"));
    names->push_back(ast::String::New(&arena_, "b"));
    names->push_back(ast::String::New(&arena_, "c"));
    
    auto inits = af_.NewList<ast::Expression *>(af_.NewCall(af_.NewVariable(ast::String::New(&arena_, "foo")), nullptr));
    auto stmts = af_.NewList<ast::Statement*>(af_.NewVarDeclaration(names, inits));
    stmts->push_back(af_.NewReturn(nullptr));
    
    auto block = af_.NewBlock(stmts);
    
    HandleScope handle_scope(isolate_);
    Handle<NyFunction> script(CodeGen::Generate(Handle<NyString>::Null(), block, core_));
    
    static const char z[] = {
        "[000] LoadGlobal 0 0 \n"
        "[003] Call 0 0 3 \n"
        "[007] Ret 0 0 \n"
    };
    std::string buf;
    BytecodeArrayDisassembler::Disassembly(script->bcbuf(), Handle<NyInt32Array>::Null(), &buf,
                                           1024);
    ASSERT_EQ(z, buf);
}
    
TEST_F(NyaaCodeGenTest, ReturnCallingInCall) {
    auto args = af_.NewList<ast::Expression *>(nullptr);
    auto bar = af_.NewVariable(ast::String::New(&arena_, "bar"));
    auto foo = af_.NewVariable(ast::String::New(&arena_, "foo"));
    auto call = af_.NewCall(bar, nullptr);
    args->push_back(call);
    call = af_.NewCall(foo, args);
    auto ret = af_.NewReturn(af_.NewList<ast::Expression *>(call));
    auto stmts = af_.NewList<ast::Statement*>(ret);
    auto block = af_.NewBlock(stmts);
    
    HandleScope handle_scope(isolate_);
    Handle<NyFunction> script(CodeGen::Generate(Handle<NyString>::Null(), block, core_));
    
    static const char z[] = {
        "[000] LoadGlobal 0 0 ; line: 0\n"
        "[003] LoadGlobal 1 1 ; line: 0\n"
        "[006] Call 1 0 -1 ; line: 0\n"
        "[010] Call 0 -1 -1 ; line: 0\n"
        "[014] Ret 0 -1 ; line: 0\n"
    };
    std::string buf;
    BytecodeArrayDisassembler::Disassembly(script->bcbuf(), script->file_info(), &buf, 512);
    ASSERT_EQ(z, buf);
}
    
TEST_F(NyaaCodeGenTest, ReturnExpression) {
    // var a, b
    // return 1.2 * a + 1.3 / b
    auto name_a = ast::String::New(&arena_, "a");
    auto name_b = ast::String::New(&arena_, "b");
    auto lhs = af_.NewBinary(Operator::kMul, af_.NewApproxLiteral(1.2), af_.NewVariable(name_a));
    auto rhs = af_.NewBinary(Operator::kDiv, af_.NewApproxLiteral(1.3), af_.NewVariable(name_b));
    auto expr = af_.NewBinary(Operator::kAdd, lhs, rhs);
    auto names = af_.NewList<const ast::String *>(name_a);
    names->push_back(name_b);
    auto stmts = af_.NewList<ast::Statement*>(af_.NewVarDeclaration(names, nullptr));
    stmts->push_back(af_.NewReturn(af_.NewList<ast::Expression *>(expr)));
    auto block = af_.NewBlock(stmts);
    
    HandleScope handle_scope(isolate_);
    Handle<NyFunction> script(CodeGen::Generate(Handle<NyString>::Null(), block, core_));
    
    BytecodeArrayDisassembler::Disassembly(core_, script, stdout);
}
    
TEST_F(NyaaCodeGenTest, CallExpression) {
    auto expr = af_.NewBinary(Operator::kAdd, af_.NewApproxLiteral(1.2), af_.NewVariable(ast::String::New(&arena_, "a")));
    expr = af_.NewBinary(Operator::kMul, expr, af_.NewApproxLiteral(2.2));
    expr = af_.NewBinary(Operator::kSub, expr, af_.NewStringLiteral(ast::String::New(&arena_, "c")));
    
    auto args = af_.NewList<ast::Expression *>(expr);
    args->push_back(af_.NewApproxLiteral(100));
    args->push_back(af_.NewVariable(ast::String::New(&arena_, "d")));
    
    auto stmts = af_.NewList<ast::Statement*>(af_.NewCall(af_.NewVariable(ast::String::New(&arena_, "foo")), args));
    auto block = af_.NewBlock(stmts);
    
    HandleScope handle_scope(isolate_);
    Handle<NyFunction> script(CodeGen::Generate(Handle<NyString>::Null(), block, core_));
    
    BytecodeArrayDisassembler::Disassembly(core_, script, stdout);
}
    
TEST_F(NyaaCodeGenTest, Compile) {
    static const char s[] = {
        "var a, b, c = 2\n"
        "\n"
        "return a + b / c - 2\n"
    };
    
    HandleScope handle_scope(isolate_);
    Handle<NyFunction> script(NyFunction::Compile(s, core_));
    ASSERT_TRUE(script.is_valid());
    ASSERT_EQ(6, script->max_stack());
    
    BytecodeArrayDisassembler::Disassembly(core_, script, stdout);
}
    
TEST_F(NyaaCodeGenTest, MutilReceive) {
    static const char s[] = {
        "var a, b, c = foo()\n"
        "return a * b + c / b, c\n"
    };
    
    HandleScope handle_scope(isolate_);
    Handle<NyFunction> script(NyFunction::Compile(s, core_));
    ASSERT_TRUE(script.is_valid());
    ASSERT_EQ(4, script->max_stack());
    
    BytecodeArrayDisassembler::Disassembly(core_, script, stdout);
}
    
TEST_F(NyaaCodeGenTest, CallRaise) {
    static const char s[] = {
        "raise(\"error!\")"
    };
    static const char z[] = {
        "[000] LoadGlobal 0 0 ; line: 1\n"
        "[003] LoadConst 1 1 ; line: 1\n"
        "[006] Call 0 1 0 ; line: 1\n"
    };
    
    HandleScope handle_scope(isolate_);
    Handle<NyFunction> script(NyFunction::Compile(s, core_));
    ASSERT_TRUE(script.is_valid());
    ASSERT_EQ(2, script->max_stack());
    
    std::string buf;
    BytecodeArrayDisassembler::Disassembly(script->bcbuf(), script->file_info(), &buf, 1024);
    //puts(buf.c_str());
    ASSERT_EQ(z, buf);
}
    
TEST_F(NyaaCodeGenTest, LambdaLiteral) {
    static const char s[] = {
        "var b = 100\n"
        "var foo = lambda (a) { return a + b }\n"
    };
    
    HandleScope handle_scope(isolate_);
    Handle<NyFunction> script(NyFunction::Compile(s, core_));
    ASSERT_TRUE(script.is_valid());
    ASSERT_EQ(2, script->max_stack());
    
    BytecodeArrayDisassembler::Disassembly(core_, script, stdout);
}
    
TEST_F(NyaaCodeGenTest, CallExpressions) {
    static const char s[] = {
        "var a,b,c = 100\n"
        "var f = a/c + foo(a/b + a*c, 1 + a, 2 + c)\n"
        "return f\n"
    };
    static const char z[] = {
        "function: [unnamed], params: 0, vargs: 0, max-stack: 9, upvals: 0\n"
        "file name: :memory:\n"
        "const pool: 4\n"
        " [0] (-1) 100\n"
        " [1] (-2) foo\n"
        " [2] (-3) 1\n"
        " [3] (-4) 2\n"
        ".............................\n"
        "[000] LoadConst 0 0 ; line: 1\n"
        "[003] LoadNil 1 1 ; line: 1\n"
        "[006] LoadNil 2 1 ; line: 1\n"
        "[009] Div 4 0 2 ; line: 2\n"
        "[013] LoadGlobal 5 1 ; line: 2\n"
        "[016] Div 7 0 1 ; line: 2\n"
        "[020] Mul 8 0 2 ; line: 2\n"
        "[024] Add 6 7 8 ; line: 2\n"
        "[028] Add 7 -3 0 ; line: 2\n"
        "[032] Add 8 -4 2 ; line: 2\n"
        "[036] Call 5 3 1 ; line: 2\n"
        "[040] Add 3 4 5 ; line: 2\n"
        "[044] Ret 3 1 ; line: 3\n"
    };
    
    HandleScope handle_scope(isolate_);
    Handle<NyFunction> script(NyFunction::Compile(s, core_));
    ASSERT_TRUE(script.is_valid());
    ASSERT_EQ(9, script->max_stack());
    
    std::string buf;
    BytecodeArrayDisassembler::Disassembly(core_, script, &buf, 1024);
    ASSERT_EQ(z, buf);
}
    
TEST_F(NyaaCodeGenTest, Index) {
    static const char s[] = {
        "var a, b = t[1], t.field\n"
        "return a, b\n"
    };
    static const char z[] = {
        "function: [unnamed], params: 0, vargs: 0, max-stack: 2, upvals: 0\n"
        "file name: :memory:\n"
        "const pool: 3\n"
        " [0] (-1) t\n"
        " [1] (-2) 1\n"
        " [2] (-3) field\n"
        ".............................\n"
        "[000] LoadGlobal 0 0 ; line: 1\n"
        "[003] GetField 0 -2 ; line: 1\n"
        "[006] LoadGlobal 1 0 ; line: 1\n"
        "[009] GetField 1 -3 ; line: 1\n"
        "[012] Ret 0 2 ; line: 2\n"
    };
    HandleScope handle_scope(isolate_);
    Handle<NyFunction> script(NyFunction::Compile(s, core_));
    ASSERT_TRUE(script.is_valid());
    ASSERT_EQ(2, script->max_stack());
    
    std::string buf;
    BytecodeArrayDisassembler::Disassembly(core_, script, &buf, 1024);
    ASSERT_EQ(z, buf);
}
    
TEST_F(NyaaCodeGenTest, MapLiteral) {
    static const char s[] = {
        "var a = 100\n"
        "var t = {\n"
        "   1, 2, 3,\n"
        "   name: \"jack\",\n"
        "   [1+a] = 1.1,\n"
        "   nested: {\n"
        "       7, 8, 9"
        "   }\n"
        "}\n"
    };
//    static const char z[] = {
//        "function: [unnamed], params: 0, vargs: 0, max-stack: 2, upvals: 0\n"
//        "file name: :memory:\n"
//        "const pool: 3\n"
//        " [0] (-1) t\n"
//        " [1] (-2) 1\n"
//        " [2] (-3) field\n"
//        ".............................\n"
//        "[000] LoadGlobal 0 0 ; line: 1\n"
//        "[003] GetField 0 -2 ; line: 1\n"
//        "[006] LoadGlobal 1 0 ; line: 1\n"
//        "[009] GetField 1 -3 ; line: 1\n"
//        "[012] Ret 0 2 ; line: 2\n"
//    };
    HandleScope handle_scope(isolate_);
    Handle<NyFunction> script(NyFunction::Compile(s, core_));
    ASSERT_TRUE(script.is_valid());
    //ASSERT_EQ(10, script->max_stack());
    
    BytecodeArrayDisassembler::Disassembly(core_, script, stdout);
}

} // namespace nyaa
    
} // namespace mai
