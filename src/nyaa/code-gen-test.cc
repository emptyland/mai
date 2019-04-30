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
    Handle<NyFunction> script(CodeGen::Generate(Handle<NyString>::Null(), block, &arena_, core_));
    
    std::string buf;
    BytecodeArrayDisassembler::Disassembly(script->bcbuf(), script->file_info(), &buf, 512);

    static const char *z =
        "[000] LoadNil 0 3 ; line: 0\n"
        "[003] Ret 0 0 ; line: 0\n"
        "[006] Ret 0 0 ; line: 0\n";
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
    Handle<NyFunction> script(CodeGen::Generate(Handle<NyString>::Null(), block, &arena_, core_));

    std::string buf;
    BytecodeArrayDisassembler::Disassembly(script->bcbuf(), script->file_info(), &buf, 512);
    static const char z[] = {
        "[000] LoadConst 0 0 ; line: 0\n"
        "[003] LoadGlobal 1 1 ; line: 0\n"
        "[006] LoadNil 2 1 ; line: 0\n"
        "[009] Ret 0 0 ; line: 0\n"
        "[012] Ret 0 0 ; line: 0\n"
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
    Handle<NyFunction> script(CodeGen::Generate(Handle<NyString>::Null(), block, &arena_, core_));
    
    static const char z[] = {
        "[000] LoadGlobal 0 0 \n"
        "[003] Call 0 0 3 \n"
        "[007] Ret 0 0 \n"
        "[010] Ret 0 0 \n"
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
    Handle<NyFunction> script(CodeGen::Generate(Handle<NyString>::Null(), block, &arena_, core_));
    
    static const char z[] = {
        "[000] LoadGlobal 0 0 ; line: 0\n"
        "[003] LoadGlobal 1 1 ; line: 0\n"
        "[006] Call 1 0 -1 ; line: 0\n"
        "[010] Call 0 -1 -1 ; line: 0\n"
        "[014] Ret 0 -1 ; line: 0\n"
        "[017] Ret 0 0 ; line: 0\n"
    };
    std::string buf;
    BytecodeArrayDisassembler::Disassembly(script->bcbuf(), script->file_info(), &buf, 512);
    ASSERT_EQ(z, buf);
}
    
TEST_F(NyaaCodeGenTest, ReturnExpression) {
    static const  char z[] = {
        "function: [unnamed], params: 0, vargs: 0, max-stack: 5, upvals: 0\n"
        "const pool: 2\n"
        " [0] (-1) 1.200000\n"
        " [1] (-2) 1.300000\n"
        ".............................\n"
        "[000] LoadNil 0 2 ; line: 0\n"
        "[003] Mul 3 -1 0 ; line: 0\n"
        "[007] Div 4 -2 1 ; line: 0\n"
        "[011] Add 2 3 4 ; line: 0\n"
        "[015] Ret 2 1 ; line: 0\n"
        "[018] Ret 0 0 ; line: 0\n"
    };
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
    Handle<NyFunction> script(CodeGen::Generate(Handle<NyString>::Null(), block, &arena_, core_));
    
    std::string buf;
    BytecodeArrayDisassembler::Disassembly(core_, script, &buf, 1024);
    ASSERT_EQ(z, buf);
}
    
TEST_F(NyaaCodeGenTest, CallExpression) {
    static const char z[] = {
        "function: [unnamed], params: 0, vargs: 0, max-stack: 5, upvals: 0\n"
        "const pool: 7\n"
        " [0] (-1) foo\n"
        " [1] (-2) 1.200000\n"
        " [2] (-3) a\n"
        " [3] (-4) 2.200000\n"
        " [4] (-5) c\n"
        " [5] (-6) 100.000000\n"
        " [6] (-7) d\n"
        ".............................\n"
        "[000] LoadGlobal 0 0 ; line: 0\n"
        "[003] LoadGlobal 4 2 ; line: 0\n"
        "[006] Add 3 -2 4 ; line: 0\n"
        "[010] Mul 2 3 -4 ; line: 0\n"
        "[014] Sub 1 2 -5 ; line: 0\n"
        "[018] LoadConst 2 5 ; line: 0\n"
        "[021] LoadGlobal 3 6 ; line: 0\n"
        "[024] Call 0 3 0 ; line: 0\n"
        "[028] Ret 0 0 ; line: 0\n"
    };
    auto expr = af_.NewBinary(Operator::kAdd, af_.NewApproxLiteral(1.2), af_.NewVariable(ast::String::New(&arena_, "a")));
    expr = af_.NewBinary(Operator::kMul, expr, af_.NewApproxLiteral(2.2));
    expr = af_.NewBinary(Operator::kSub, expr, af_.NewStringLiteral(ast::String::New(&arena_, "c")));
    
    auto args = af_.NewList<ast::Expression *>(expr);
    args->push_back(af_.NewApproxLiteral(100));
    args->push_back(af_.NewVariable(ast::String::New(&arena_, "d")));
    
    auto stmts = af_.NewList<ast::Statement*>(af_.NewCall(af_.NewVariable(ast::String::New(&arena_, "foo")), args));
    auto block = af_.NewBlock(stmts);
    
    HandleScope handle_scope(isolate_);
    Handle<NyFunction> script(CodeGen::Generate(Handle<NyString>::Null(), block, &arena_, core_));
    
    std::string buf;
    BytecodeArrayDisassembler::Disassembly(core_, script, &buf, 1024);
    ASSERT_EQ(z, buf);
}
    
TEST_F(NyaaCodeGenTest, Compile) {
    static const char s[] = {
        "var a, b, c = 2\n"
        "\n"
        "return a + b / c - 2\n"
    };
    static const char z[] = {
        "function: [unnamed], params: 0, vargs: 0, max-stack: 6, upvals: 0\n"
        "file name: :memory:\n"
        "const pool: 1\n"
        " [0] (-1) 2\n"
        ".............................\n"
        "[000] LoadConst 0 0 ; line: 1\n"
        "[003] LoadNil 1 1 ; line: 1\n"
        "[006] LoadNil 2 1 ; line: 1\n"
        "[009] Div 5 1 2 ; line: 3\n"
        "[013] Add 4 0 5 ; line: 3\n"
        "[017] Sub 3 4 -1 ; line: 3\n"
        "[021] Ret 3 1 ; line: 3\n"
        "[024] Ret 0 0 ; line: 0\n"
    };
    
    HandleScope handle_scope(isolate_);
    Handle<NyFunction> script(NyFunction::Compile(s, core_));
    ASSERT_TRUE(script.is_valid());
    ASSERT_EQ(6, script->max_stack());
    
    std::string buf;
    BytecodeArrayDisassembler::Disassembly(core_, script, &buf, 1024);
    ASSERT_EQ(z, buf);
}

TEST_F(NyaaCodeGenTest, MutilReceive) {
    static const char s[] = {
        "var a, b, c = foo()\n"
        "return a * b + c / b, c\n"
    };
    static const char z[] = {
        "function: [unnamed], params: 0, vargs: 0, max-stack: 6, upvals: 0\n"
        "file name: :memory:\n"
        "const pool: 1\n"
        " [0] (-1) foo\n"
        ".............................\n"
        "[000] LoadGlobal 0 0 ; line: 1\n"
        "[003] Call 0 0 3 ; line: 1\n"
        "[007] Mul 4 0 1 ; line: 2\n"
        "[011] Div 5 2 1 ; line: 2\n"
        "[015] Add 3 4 5 ; line: 2\n"
        "[019] Move 4 2 ; line: 2\n"
        "[022] Ret 3 2 ; line: 2\n"
        "[025] Ret 0 0 ; line: 0\n"
    };

    HandleScope handle_scope(isolate_);
    Handle<NyFunction> script(NyFunction::Compile(s, core_));
    ASSERT_TRUE(script.is_valid());

    std::string buf;
    BytecodeArrayDisassembler::Disassembly(core_, script, &buf, 1024);
    ASSERT_EQ(z, buf) << buf;
}

TEST_F(NyaaCodeGenTest, CallRaise) {
    static const char s[] = {
        "raise(\"error!\")"
    };
    static const char z[] = {
        "[000] LoadGlobal 0 0 ; line: 1\n"
        "[003] LoadConst 1 1 ; line: 1\n"
        "[006] Call 0 1 0 ; line: 1\n"
        "[010] Ret 0 0 ; line: 0\n"
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
        "[006] Ret 0 0 ; line: 0\n"
        "-----------------------------\n"
        "function: [unnamed], params: 1, vargs: 0, max-stack: 3, upvals: 1\n"
        "file name: :memory:\n"
        ".............................\n"
        "[000] LoadUp 2 0 ; line: 2\n"
        "[003] Add 1 0 2 ; line: 2\n"
        "[007] Ret 1 1 ; line: 2\n"
        "[010] Ret 0 0 ; line: 0\n"
    };
    
    HandleScope handle_scope(isolate_);
    Handle<NyFunction> script(NyFunction::Compile(s, core_));
    ASSERT_TRUE(script.is_valid());
    ASSERT_EQ(2, script->max_stack());
    
    std::string buf;
    BytecodeArrayDisassembler::Disassembly(core_, script, &buf, 1024);
    ASSERT_EQ(z, buf);
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
        "[047] Ret 0 0 ; line: 0\n"
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
        "function: [unnamed], params: 0, vargs: 0, max-stack: 3, upvals: 0\n"
        "file name: :memory:\n"
        "const pool: 3\n"
        " [0] (-1) t\n"
        " [1] (-2) 1\n"
        " [2] (-3) field\n"
        ".............................\n"
        "[000] LoadGlobal 1 0 ; line: 1\n"
        "[003] GetField 0 1 -2 ; line: 1\n"
        "[007] LoadGlobal 2 0 ; line: 1\n"
        "[010] GetField 1 2 -3 ; line: 1\n"
        "[014] Ret 0 2 ; line: 2\n"
        "[017] Ret 0 0 ; line: 0\n"
    };
    HandleScope handle_scope(isolate_);
    Handle<NyFunction> script(NyFunction::Compile(s, core_));
    ASSERT_TRUE(script.is_valid());
    
    std::string buf;
    BytecodeArrayDisassembler::Disassembly(core_, script, &buf, 1024);
    ASSERT_EQ(z, buf) << buf;
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
    static const char z[] = {
        "function: [unnamed], params: 0, vargs: 0, max-stack: 15, upvals: 0\n"
        "file name: :memory:\n"
        "const pool: 12\n"
        " [0] (-1) 100\n"
        " [1] (-2) 0\n"
        " [2] (-3) 1\n"
        " [3] (-4) 2\n"
        " [4] (-5) 3\n"
        " [5] (-6) name\n"
        " [6] (-7) jack\n"
        " [7] (-8) 1.100000\n"
        " [8] (-9) nested\n"
        " [9] (-10) 7\n"
        " [10] (-11) 8\n"
        " [11] (-12) 9\n"
        ".............................\n"
        "[000] LoadConst 0 0 ; line: 1\n"
        "[003] LoadConst 1 1 ; line: 3\n"
        "[006] LoadConst 2 2 ; line: 3\n"
        "[009] LoadConst 3 2 ; line: 3\n"
        "[012] LoadConst 4 3 ; line: 3\n"
        "[015] LoadConst 5 3 ; line: 3\n"
        "[018] LoadConst 6 4 ; line: 3\n"
        "[021] LoadConst 7 5 ; line: 4\n"
        "[024] LoadConst 8 6 ; line: 4\n"
        "[027] Add 9 -3 0 ; line: 5\n"
        "[031] LoadConst 10 7 ; line: 5\n"
        "[034] LoadConst 11 8 ; line: 6\n"
        "[037] LoadConst 12 9 ; line: 7\n"
        "[040] LoadConst 13 10 ; line: 7\n"
        "[043] LoadConst 14 11 ; line: 7\n"
        "[046] NewMap 12 3 1 ; line: 6\n"
        "[050] NewMap 1 12 0 ; line: 2\n"
        "[054] Ret 0 0 ; line: 0\n"
    };

    HandleScope handle_scope(isolate_);
    Handle<NyFunction> script(NyFunction::Compile(s, core_));
    ASSERT_TRUE(script.is_valid());
    //ASSERT_EQ(10, script->max_stack());
    
    std::string buf;
    BytecodeArrayDisassembler::Disassembly(core_, script, &buf, 1024);
    ASSERT_EQ(z, buf);
}
    
TEST_F(NyaaCodeGenTest, IfElse) {
    static const char s[] = {
        "var a, b, c = 1, 2, 3\n"
        "if (a == b) { print(a) }\n"
        "else if (a < b) { print (c) }\n"
        "else { print(b) }\n"
        "return\n"
    };
    static const char z[] = {
        "function: [unnamed], params: 0, vargs: 0, max-stack: 5, upvals: 0\n"
        "file name: :memory:\n"
        "const pool: 8\n"
        " [0] (-1) 1\n"
        " [1] (-2) 2\n"
        " [2] (-3) 3\n"
        " [3] (-4) 14\n"
        " [4] (-5) print\n"
        " [5] (-6) 34\n"
        " [6] (-7) 14\n"
        " [7] (-8) 12\n"
        ".............................\n"
        "[000] LoadConst 0 0 ; line: 1\n"
        "[003] LoadConst 1 1 ; line: 1\n"
        "[006] LoadConst 2 2 ; line: 1\n"
        "[009] Equal 3 0 1 ; line: 2\n"
        "[013] Test 3 0 0 ; line: 2\n"
        "[017] JumpConst 3 ; line: 2\n"
        "[019] LoadGlobal 3 4 ; line: 2\n"
        "[022] Move 4 0 ; line: 2\n"
        "[025] Call 3 1 0 ; line: 2\n"
        "[029] JumpConst 5 ; line: 2\n"
        "[031] LessThan 3 0 1 ; line: 3\n"
        "[035] Test 3 0 0 ; line: 3\n"
        "[039] JumpConst 6 ; line: 3\n"
        "[041] LoadGlobal 3 4 ; line: 3\n"
        "[044] Move 4 2 ; line: 3\n"
        "[047] Call 3 1 0 ; line: 3\n"
        "[051] JumpConst 7 ; line: 3\n"
        "[053] LoadGlobal 3 4 ; line: 4\n"
        "[056] Move 4 1 ; line: 4\n"
        "[059] Call 3 1 0 ; line: 4\n"
        "[063] Ret 0 0 ; line: 5\n"
        "[066] Ret 0 0 ; line: 0\n"
    };
    HandleScope handle_scope(isolate_);
    Handle<NyFunction> script(NyFunction::Compile(s, core_));
    ASSERT_TRUE(script.is_valid());
    //ASSERT_EQ(10, script->max_stack());
    
    std::string buf;
    BytecodeArrayDisassembler::Disassembly(core_, script, &buf, 1024);
    ASSERT_EQ(z, buf);
}

TEST_F(NyaaCodeGenTest, ObjectDefinition) {
    static const char s[] = {
        "object foo {}\n"
    };
    static const char z[] = {
        "function: [unnamed], params: 0, vargs: 0, max-stack: 2, upvals: 0\n"
        "file name: :memory:\n"
        "const pool: 2\n"
        " [0] (-1) __type__\n"
        " [1] (-2) foo\n"
        ".............................\n"
        "[000] LoadConst 0 0 ; line: 1\n"
        "[003] LoadConst 1 1 ; line: 1\n"
        "[006] NewMap 0 2 -1 ; line: 1\n"
        "[010] New 0 0 0 ; line: 1\n"
        "[014] StoreGlobal 0 1 ; line: 1\n"
        "[017] Ret 0 0 ; line: 0\n"
    };
    HandleScope handle_scope(isolate_);
    Handle<NyFunction> script(NyFunction::Compile(s, core_));
    ASSERT_TRUE(script.is_valid());

    std::string buf;
    BytecodeArrayDisassembler::Disassembly(core_, script, &buf, 1024);
    ASSERT_EQ(z, buf) << buf;
}
    
TEST_F(NyaaCodeGenTest, ClassDefinition) {
    static const char s[] = {
        "class foo {}\n"
    };
    static const char z[] = {
        "function: [unnamed], params: 0, vargs: 0, max-stack: 2, upvals: 0\n"
        "file name: :memory:\n"
        "const pool: 2\n"
        " [0] (-1) __type__\n"
        " [1] (-2) foo\n"
        ".............................\n"
        "[000] LoadConst 0 0 ; line: 1\n"
        "[003] LoadConst 1 1 ; line: 1\n"
        "[006] NewMap 0 2 -1 ; line: 1\n"
        "[010] StoreGlobal 0 1 ; line: 1\n"
        "[013] Ret 0 0 ; line: 0\n"
    };
    HandleScope handle_scope(isolate_);
    Handle<NyFunction> script(NyFunction::Compile(s, core_));
    ASSERT_TRUE(script.is_valid());
    
    std::string buf;
    BytecodeArrayDisassembler::Disassembly(core_, script, &buf, 1024);
    ASSERT_EQ(z, buf) << buf;
}
    
TEST_F(NyaaCodeGenTest, ClassDefinitionWithMembers) {
    static const char s[] = {
        "class foo {\n"
        "   def __init__(self) {}\n"
        "   def __add__(self, other) { return self + other }\n"
        "   property [ro] a, b, c\n"
        "   property [rw] d, e, f\n"
        "}\n"
    };
    static const char z[] = {
        "function: [unnamed], params: 0, vargs: 0, max-stack: 18, upvals: 0\n"
        "file name: :memory:\n"
        "const pool: 10\n"
        " [0] (-1) __init__\n"
        " [1] (-2) __add__\n"
        " [2] (-3) a\n"
        " [3] (-4) b\n"
        " [4] (-5) c\n"
        " [5] (-6) d\n"
        " [6] (-7) e\n"
        " [7] (-8) f\n"
        " [8] (-9) __type__\n"
        " [9] (-10) foo\n"
        ".............................\n"
        "[000] LoadConst 0 0 ; line: 2\n"
        "[003] Closure 1 0 ; line: 2\n"
        "[006] LoadConst 2 1 ; line: 3\n"
        "[009] Closure 3 1 ; line: 3\n"
        "[012] LoadConst 4 2 ; line: 4\n"
        "[015] LoadImm 5 1 ; line: 4\n"
        "[018] LoadConst 6 3 ; line: 4\n"
        "[021] LoadImm 7 5 ; line: 4\n"
        "[024] LoadConst 8 4 ; line: 4\n"
        "[027] LoadImm 9 9 ; line: 4\n"
        "[030] LoadConst 10 5 ; line: 5\n"
        "[033] LoadImm 11 15 ; line: 5\n"
        "[036] LoadConst 12 6 ; line: 5\n"
        "[039] LoadImm 13 19 ; line: 5\n"
        "[042] LoadConst 14 7 ; line: 5\n"
        "[045] LoadImm 15 23 ; line: 5\n"
        "[048] LoadConst 16 8 ; line: 1\n"
        "[051] LoadConst 17 9 ; line: 1\n"
        "[054] NewMap 0 18 -1 ; line: 6\n"
        "[058] StoreGlobal 0 9 ; line: 6\n"
        "[061] Ret 0 0 ; line: 0\n"
        "-----------------------------\n"
        "function: foo::__init__, params: 1, vargs: 0, max-stack: 1, upvals: 0\n"
        "file name: :memory:\n"
        ".............................\n"
        "[000] Ret 0 0 ; line: 0\n"
        "-----------------------------\n"
        "function: foo::__add__, params: 2, vargs: 0, max-stack: 3, upvals: 0\n"
        "file name: :memory:\n"
        ".............................\n"
        "[000] Add 2 0 1 ; line: 3\n"
        "[004] Ret 2 1 ; line: 3\n"
        "[007] Ret 0 0 ; line: 0\n"
    };
    HandleScope handle_scope(isolate_);
    Handle<NyFunction> script(NyFunction::Compile(s, core_));
    ASSERT_TRUE(script.is_valid());
    
    std::string buf;
    BytecodeArrayDisassembler::Disassembly(core_, script, &buf, 4096);
    ASSERT_EQ(z, buf) << buf;
}
    
TEST_F(NyaaCodeGenTest, ClassDefinitionWithBase) {
    static const char s[] = {
        "class [local] foo {\n"
        "   property [ro] a, b\n"
        "}\n"
        "class bar : foo {\n"
        "   property [rw] c, d\n"
        "}\n"
    };
    static const char z[] = {
        "function: [unnamed], params: 0, vargs: 0, max-stack: 9, upvals: 0\n"
        "file name: :memory:\n"
        "const pool: 8\n"
        " [0] (-1) a\n"
        " [1] (-2) b\n"
        " [2] (-3) __type__\n"
        " [3] (-4) foo\n"
        " [4] (-5) c\n"
        " [5] (-6) d\n"
        " [6] (-7) bar\n"
        " [7] (-8) __base__\n"
        ".............................\n"
        "[000] LoadConst 0 0 ; line: 2\n"
        "[003] LoadImm 1 1 ; line: 2\n"
        "[006] LoadConst 2 1 ; line: 2\n"
        "[009] LoadImm 3 5 ; line: 2\n"
        "[012] LoadConst 4 2 ; line: 1\n"
        "[015] LoadConst 5 3 ; line: 1\n"
        "[018] NewMap 0 6 -1 ; line: 3\n"
        "[022] LoadConst 1 4 ; line: 5\n"
        "[025] LoadImm 2 3 ; line: 5\n"
        "[028] LoadConst 3 5 ; line: 5\n"
        "[031] LoadImm 4 7 ; line: 5\n"
        "[034] LoadConst 5 2 ; line: 4\n"
        "[037] LoadConst 6 6 ; line: 4\n"
        "[040] LoadConst 7 7 ; line: 4\n"
        "[043] Move 8 0 ; line: 0\n"
        "[046] NewMap 1 8 -1 ; line: 6\n"
        "[050] StoreGlobal 1 6 ; line: 6\n"
        "[053] Ret 0 0 ; line: 0\n"
    };
    
    HandleScope handle_scope(isolate_);
    Handle<NyFunction> script(NyFunction::Compile(s, core_));
    ASSERT_TRUE(script.is_valid());
    
    std::string buf;
    BytecodeArrayDisassembler::Disassembly(core_, script, &buf, 4096);
    ASSERT_EQ(z, buf) << buf;
}
    
TEST_F(NyaaCodeGenTest, VargsExpression) {
    static const char s[] = {
        "def foo(a, b, ...) {\n"
        "   var c, d = ...\n"
        "   return ...\n"
        "}\n"
    };
    static const char z[] = {
        "function: [unnamed], params: 0, vargs: 0, max-stack: 1, upvals: 0\n"
        "file name: :memory:\n"
        "const pool: 1\n"
        " [0] (-1) foo\n"
        ".............................\n"
        "[000] Closure 0 0 ; line: 1\n"
        "[003] StoreGlobal 0 0 ; line: 1\n"
        "[006] Ret 0 0 ; line: 0\n"
        "-----------------------------\n"
        "function: foo, params: 2, vargs: 1, max-stack: 5, upvals: 0\n"
        "file name: :memory:\n"
        ".............................\n"
        "[000] Vargs 2 2 ; line: 2\n"
        "[003] Vargs 4 -1 ; line: 3\n"
        "[006] Ret 4 -1 ; line: 3\n"
        "[009] Ret 0 0 ; line: 0\n"
    };
    
    HandleScope handle_scope(isolate_);
    Handle<NyFunction> script(NyFunction::Compile(s, core_));
    ASSERT_TRUE(script.is_valid());
    
    std::string buf;
    BytecodeArrayDisassembler::Disassembly(core_, script, &buf, 4096);
    ASSERT_EQ(z, buf) << buf;
}

TEST_F(NyaaCodeGenTest, GetFieldSetField) {
    static const char s[] = {
        "var a = t.a + t.b\n"
        "t.c = a\n"
    };
    static const char z[] = {
        "function: [unnamed], params: 0, vargs: 0, max-stack: 4, upvals: 0\n"
        "file name: :memory:\n"
        "const pool: 4\n"
        " [0] (-1) t\n"
        " [1] (-2) a\n"
        " [2] (-3) b\n"
        " [3] (-4) c\n"
        ".............................\n"
        "[000] LoadGlobal 2 0 ; line: 1\n"
        "[003] GetField 1 2 -2 ; line: 1\n"
        "[007] LoadGlobal 3 0 ; line: 1\n"
        "[010] GetField 2 3 -3 ; line: 1\n"
        "[014] Add 0 1 2 ; line: 1\n"
        "[018] LoadGlobal 1 0 ; line: 2\n"
        "[021] SetField 1 -4 0 ; line: 2\n"
        "[025] Ret 0 0 ; line: 0\n"
    };
    HandleScope handle_scope(isolate_);
    Handle<NyFunction> script(NyFunction::Compile(s, core_));
    ASSERT_TRUE(script.is_valid());
    
    std::string buf;
    BytecodeArrayDisassembler::Disassembly(core_, script, &buf, 4096);
    ASSERT_EQ(z, buf) << buf;
}
    
TEST_F(NyaaCodeGenTest, SelfCall) {
    static const char s[] = {
        "var a = t:foo(1,2,3)\n"
        "return a\n"
    };
    static const char z[] = {
        "function: [unnamed], params: 0, vargs: 0, max-stack: 5, upvals: 0\n"
        "file name: :memory:\n"
        "const pool: 5\n"
        " [0] (-1) t\n"
        " [1] (-2) foo\n"
        " [2] (-3) 1\n"
        " [3] (-4) 2\n"
        " [4] (-5) 3\n"
        ".............................\n"
        "[000] LoadGlobal 2 0 ; line: 1\n"
        "[003] Self 0 2 1 ; line: 1\n"
        "[007] LoadConst 2 2 ; line: 1\n"
        "[010] LoadConst 3 3 ; line: 1\n"
        "[013] LoadConst 4 4 ; line: 1\n"
        "[016] Call 0 3 1 ; line: 1\n"
        "[020] Ret 0 1 ; line: 2\n"
        "[023] Ret 0 0 ; line: 0\n"
    };
    HandleScope handle_scope(isolate_);
    Handle<NyFunction> script(NyFunction::Compile(s, core_));
    ASSERT_TRUE(script.is_valid());
    
    std::string buf;
    BytecodeArrayDisassembler::Disassembly(core_, script, &buf, 4096);
    ASSERT_EQ(z, buf) << buf;
}
    
TEST_F(NyaaCodeGenTest, NewCall) {
    static const char s[] = {
        "var a = new t()\n"
        "a = new t(1,2)\n"
    };
    static const char z[] = {
        "function: [unnamed], params: 0, vargs: 0, max-stack: 5, upvals: 0\n"
        "file name: :memory:\n"
        "const pool: 3\n"
        " [0] (-1) t\n"
        " [1] (-2) 1\n"
        " [2] (-3) 2\n"
        ".............................\n"
        "[000] LoadGlobal 1 0 ; line: 1\n"
        "[003] New 0 1 0 ; line: 1\n"
        "[007] LoadGlobal 2 0 ; line: 2\n"
        "[010] LoadConst 3 1 ; line: 2\n"
        "[013] LoadConst 4 2 ; line: 2\n"
        "[016] New 1 2 2 ; line: 2\n"
        "[020] Move 0 1 ; line: 2\n"
        "[023] Ret 0 0 ; line: 0\n"
    };
    HandleScope handle_scope(isolate_);
    Handle<NyFunction> script(NyFunction::Compile(s, core_));
    ASSERT_TRUE(script.is_valid());
    
    std::string buf;
    BytecodeArrayDisassembler::Disassembly(core_, script, &buf, 4096);
    ASSERT_EQ(z, buf) << buf;
}

TEST_F(NyaaCodeGenTest, WhileLoop) {
    static const char s[] = {
        "while (test()) {\n"
        "   print(1)\n"
        "   break\n"
        "   continue\n"
        "}\n"
    };
    static const char z[] = {
        "function: [unnamed], params: 0, vargs: 0, max-stack: 2, upvals: 0\n"
        "file name: :memory:\n"
        "const pool: 6\n"
        " [0] (-1) -25\n"
        " [1] (-2) test\n"
        " [2] (-3) 18\n"
        " [3] (-4) print\n"
        " [4] (-5) 1\n"
        " [5] (-6) 6\n"
        ".............................\n"
        "[000] LoadGlobal 0 1 ; line: 1\n"
        "[003] Call 0 0 1 ; line: 1\n"
        "[007] Test 0 0 0 ; line: 1\n"
        "[011] JumpConst 2 ; line: 1\n"
        "[013] LoadGlobal 0 3 ; line: 2\n"
        "[016] LoadConst 1 4 ; line: 2\n"
        "[019] Call 0 1 0 ; line: 2\n"
        "[023] JumpConst 5 ; line: 3\n"
        "[025] JumpConst 0 ; line: 4\n"
        "[027] JumpImm -27 ; line: 5\n"
        "[029] Ret 0 0 ; line: 0\n"
    };
    HandleScope handle_scope(isolate_);
    Handle<NyFunction> script(NyFunction::Compile(s, core_));
    ASSERT_TRUE(script.is_valid());
    
    std::string buf;
    BytecodeArrayDisassembler::Disassembly(core_, script, &buf, 4096);
    ASSERT_EQ(z, buf) << buf;
}
    
TEST_F(NyaaCodeGenTest, ForIterateLoop) {
    static const char s[] = {
        "for i, v in pairs(t) {\n"
        "   print(i, v)\n"
        "}\n"
    };
    static const char z[] = {
        "function: [unnamed], params: 0, vargs: 0, max-stack: 6, upvals: 0\n"
        "file name: :memory:\n"
        "const pool: 6\n"
        " [0] (-1) pairs\n"
        " [1] (-2) t\n"
        " [2] (-3) 15\n"
        " [3] (-4) -32\n"
        " [4] (-5) print\n"
        " [5] (-6) 10\n"
        ".............................\n"
        "[000] LoadGlobal 0 0 ; line: 1\n"
        "[003] LoadGlobal 1 1 ; line: 1\n"
        "[006] Call 0 1 1 ; line: 1\n"
        "[010] JumpConst 2 ; line: 1\n"
        "[012] LoadGlobal 3 4 ; line: 2\n"
        "[015] Move 4 1 ; line: 2\n"
        "[018] Move 5 2 ; line: 2\n"
        "[021] Call 3 2 0 ; line: 2\n"
        "[025] Move 3 0 ; line: 3\n"
        "[028] Call 3 0 2 ; line: 3\n"
        "[032] TestNil 3 1 0 ; line: 3\n"
        "[036] JumpConst 5 ; line: 3\n"
        "[038] Move 1 3 ; line: 3\n"
        "[041] Move 2 4 ; line: 3\n"
        "[044] JumpConst 3 ; line: 3\n"
        "[046] Ret 0 0 ; line: 0\n"
    };
    HandleScope handle_scope(isolate_);
    Handle<NyFunction> script(NyFunction::Compile(s, core_));
    ASSERT_TRUE(script.is_valid());
    
    std::string buf;
    BytecodeArrayDisassembler::Disassembly(core_, script, &buf, 4096);
    ASSERT_EQ(z, buf) << buf;
}
    
TEST_F(NyaaCodeGenTest, Concat) {
    static const char s[] = {
        "var x, y = 1, 2\n"
        "var a = x..y..x..y\n"
        "return a\n"
    };
    static const char z[] = {
        "function: [unnamed], params: 0, vargs: 0, max-stack: 6, upvals: 0\n"
        "file name: :memory:\n"
        "const pool: 2\n"
        " [0] (-1) 1\n"
        " [1] (-2) 2\n"
        ".............................\n"
        "[000] LoadConst 0 0 ; line: 1\n"
        "[003] LoadConst 1 1 ; line: 1\n"
        "[006] Move 2 0 ; line: 2\n"
        "[009] Move 3 1 ; line: 2\n"
        "[012] Move 4 0 ; line: 2\n"
        "[015] Move 5 1 ; line: 2\n"
        "[018] Concat 2 2 4 ; line: 2\n"
        "[022] Ret 2 1 ; line: 3\n"
        "[025] Ret 0 0 ; line: 0\n"
    };
    HandleScope handle_scope(isolate_);
    Handle<NyFunction> script(NyFunction::Compile(s, core_));
    ASSERT_TRUE(script.is_valid());
    
    std::string buf;
    BytecodeArrayDisassembler::Disassembly(core_, script, &buf, 4096);
    ASSERT_EQ(z, buf) << buf;
}
    
TEST_F(NyaaCodeGenTest, AndOrSwitch) {
    static const char s[] = {
        "var x, y = 0, 1\n"
        "var a, b = x and y, x or y\n"
        "return a, b\n"
    };
    static const char z[] = {
        "function: [unnamed], params: 0, vargs: 0, max-stack: 4, upvals: 0\n"
        "file name: :memory:\n"
        "const pool: 2\n"
        " [0] (-1) 0\n"
        " [1] (-2) 1\n"
        ".............................\n"
        "[000] LoadConst 0 0 ; line: 1\n"
        "[003] LoadConst 1 1 ; line: 1\n"
        "[006] TestSet 2 0 1 ; line: 2\n"
        "[010] JumpImm 5 ; line: 2\n"
        "[012] Move 2 1 ; line: 2\n"
        "[015] TestSet 3 0 0 ; line: 2\n"
        "[019] JumpImm 5 ; line: 2\n"
        "[021] Move 3 1 ; line: 2\n"
        "[024] Ret 2 2 ; line: 3\n"
        "[027] Ret 0 0 ; line: 0\n"
    };
    HandleScope handle_scope(isolate_);
    Handle<NyFunction> script(NyFunction::Compile(s, core_));
    ASSERT_TRUE(script.is_valid());

    std::string buf;
    BytecodeArrayDisassembler::Disassembly(core_, script, &buf, 4096);
    ASSERT_EQ(z, buf) << buf;
}

} // namespace nyaa
    
} // namespace mai
