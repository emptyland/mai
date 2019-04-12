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
    
    static const char z[] = {
        "[000] LoadNil 0 2 \n"
        "[003] Mul 2 -1 0 \n"
        "[007] Div 3 -2 1 \n"
        "[011] Add 3 2 3 \n"
        "[015] Ret 3 1 \n"
    };
    std::string buf;
    BytecodeArrayDisassembler::Disassembly(script->bcbuf(), Handle<NyInt32Array>::Null(), &buf,
                                           1024);
    ASSERT_EQ(z, buf);
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
    
    static const char z[] = {
        "[000] LoadGlobal 0 0 \n"
        "[003] LoadGlobal 1 2 \n"
        "[006] Add 1 -2 1 \n"
        "[010] Mul 1 1 -4 \n"
        "[014] Sub 1 1 -5 \n"
        "[018] LoadConst 2 5 \n"
        "[021] LoadGlobal 3 6 \n"
        "[024] Call 0 3 0 \n"
    };
    std::string buf;
    BytecodeArrayDisassembler::Disassembly(script->bcbuf(), Handle<NyInt32Array>::Null(),
                                           &buf, 1024);
    ASSERT_EQ(z, buf);
}
    
TEST_F(NyaaCodeGenTest, Compile) {
    static const char s[] = {
        "var a, b, c = 2\n"
        "\n"
        "return a + b / c - 2\n"
    };
    static const char z[] = {
        "[000] LoadConst 0 0 ; line: 1\n"
        "[003] LoadNil 1 1 ; line: 1\n"
        "[006] LoadNil 2 1 ; line: 1\n"
        "[009] Div 3 1 2 ; line: 3\n"
        "[013] Add 3 0 3 ; line: 3\n"
        "[017] Sub 3 3 -1 ; line: 3\n"
        "[021] Ret 3 1 ; line: 3\n"
    };
    
    HandleScope handle_scope(isolate_);
    Handle<NyFunction> script(NyFunction::Compile(s, core_));
    ASSERT_TRUE(script.is_valid());
    ASSERT_EQ(4, script->max_stack());
    
    std::string buf;
    BytecodeArrayDisassembler::Disassembly(script->bcbuf(), script->file_info(), &buf, 1024);
    ASSERT_EQ(z, buf);
}
    
TEST_F(NyaaCodeGenTest, MutilReceive) {
    static const char s[] = {
        "var a, b, c = foo()\n"
        "return a * b + c / b, c\n"
    };
    static const char z[] = {
        "[000] LoadGlobal 0 0 ; line: 1\n"
        "[003] Call 0 0 3 ; line: 1\n"
        "[007] Mul 3 0 1 ; line: 2\n"
        "[011] Div 4 2 1 ; line: 2\n"
        "[015] Add 4 3 4 ; line: 2\n"
        "[019] Move 5 2 ; line: 2\n"
        "[022] Ret 4 2 ; line: 2\n"
    };
    
    HandleScope handle_scope(isolate_);
    Handle<NyFunction> script(NyFunction::Compile(s, core_));
    ASSERT_TRUE(script.is_valid());
    ASSERT_EQ(6, script->max_stack());
    
    std::string buf;    
    BytecodeArrayDisassembler::Disassembly(script->bcbuf(), script->file_info(), &buf, 1024);
    ASSERT_EQ(z, buf);
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
    static const char z[] = {
        "function: [unnamed], params: 0, vargs: 0, max-stack: 2, upvals: 0\n"
        "file name: :memory:\n"
        "const pool: 1\n"
        " [0] (-1) 100\n"
        ".............................\n"
        "[000] LoadConst 0 0 ; line: 1\n"
        "[003] Closure 1 0 ; line: 2\n"
        "-----------------------------\n"
        "function: [unnamed], params: 1, vargs: 0, max-stack: 2, upvals: 1\n"
        ".............................\n"
        "[000] LoadUp 1 0 ; line: 2\n"
        "[003] Add 1 0 1 ; line: 2\n"
        "[007] Ret 1 1 ; line: 2\n"
        "[010] Ret 0 0 ; line: 0\n"
    };
    
    HandleScope handle_scope(isolate_);
    Handle<NyFunction> script(NyFunction::Compile(s, core_));
    ASSERT_TRUE(script.is_valid());
    ASSERT_EQ(2, script->max_stack());
    
    std::string buf;
    BytecodeArrayDisassembler::Disassembly(core_, script, &buf, 4096);
    //puts(buf.c_str());
    ASSERT_EQ(z, buf);
}

} // namespace nyaa
    
} // namespace mai
