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
    auto names = af_.NewList(ast::String::New(&arena_, "a"));
    names->push_back(ast::String::New(&arena_, "b"));
    names->push_back(ast::String::New(&arena_, "c"));
    
    auto stmts = af_.NewList<ast::Statement*>(af_.NewVarDeclaration(names, nullptr));
    stmts->push_back(af_.NewReturn(nullptr));

    auto block = af_.NewBlock(stmts);
    
    HandleScope handle_scope(isolate_);
    Handle<NyScript> script(CodeGen::Generate(Handle<NyString>::Null(), block, core_));
    
    std::string buf;
    BytecodeArrayDisassembler::Disassembly(script->bcbuf(), script->file_info(), &buf, 512);

    static const char *z =
        "[000] LoadNil 0 3 ; line: 0\n"
        "[003] Ret 0 0 ; line: 0\n";
    ASSERT_EQ(z, buf);
}
    
TEST_F(NyaaCodeGenTest, VarInitializer) {
    auto names = af_.NewList(ast::String::New(&arena_, "a"));
    names->push_back(ast::String::New(&arena_, "b"));
    names->push_back(ast::String::New(&arena_, "c"));
    
    auto inits = af_.NewList<ast::Expression*>(af_.NewApproxLiteral(1.2));
    inits->push_back(af_.NewVariable(ast::String::New(&arena_, "name")));
    
    auto stmts = af_.NewList<ast::Statement*>(af_.NewVarDeclaration(names, inits));
    stmts->push_back(af_.NewReturn(nullptr));
    
    auto block = af_.NewBlock(stmts);
    
    HandleScope handle_scope(isolate_);
    Handle<NyScript> script(CodeGen::Generate(Handle<NyString>::Null(), block, core_));

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
    auto names = af_.NewList(ast::String::New(&arena_, "a"));
    names->push_back(ast::String::New(&arena_, "b"));
    names->push_back(ast::String::New(&arena_, "c"));
    
    auto inits = af_.NewList<ast::Expression *>(af_.NewCall(af_.NewVariable(ast::String::New(&arena_, "foo")), nullptr));
    auto stmts = af_.NewList<ast::Statement*>(af_.NewVarDeclaration(names, inits));
    stmts->push_back(af_.NewReturn(nullptr));
    
    auto block = af_.NewBlock(stmts);
    
    HandleScope handle_scope(isolate_);
    Handle<NyScript> script(CodeGen::Generate(Handle<NyString>::Null(), block, core_));
    
    std::string buf;
    BytecodeArrayDisassembler::Disassembly(script->bcbuf(), script->file_info(), &buf, 512);
    static const char z[] ={
        "[000] LoadGlobal 0 0 ; line: 0\n"
        "[003] Call 0 0 3 ; line: 0\n"
        "[007] Ret 0 0 ; line: 0\n"
    };
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
    Handle<NyScript> script(CodeGen::Generate(Handle<NyString>::Null(), block, core_));
    
    std::string buf;
    BytecodeArrayDisassembler::Disassembly(script->bcbuf(), script->file_info(), &buf, 512);
    static const char z[] = {
        "[000] LoadGlobal 0 0 ; line: 0\n"
        "[003] LoadGlobal 1 1 ; line: 0\n"
        "[006] Call 1 0 -1 ; line: 0\n"
        "[010] Call 0 -1 -1 ; line: 0\n"
        "[014] Ret 0 -1 ; line: 0\n"
    };
    ASSERT_EQ(z, buf);
}
    
TEST_F(NyaaCodeGenTest, ReturnExpression) {
    auto name_a = ast::String::New(&arena_, "a");
    auto name_b = ast::String::New(&arena_, "b");
    auto lhs = af_.NewBinary(Operator::kMul, af_.NewApproxLiteral(1.2), af_.NewVariable(name_a));
    auto rhs = af_.NewBinary(Operator::kDiv, af_.NewApproxLiteral(1.3), af_.NewVariable(name_b));
    auto expr = af_.NewBinary(Operator::kAdd, lhs, rhs);
    auto names = af_.NewList(name_a);
    names->push_back(name_b);
    auto stmts = af_.NewList<ast::Statement*>(af_.NewVarDeclaration(names, nullptr));
    stmts->push_back(af_.NewReturn(af_.NewList<ast::Expression *>(expr)));
    auto block = af_.NewBlock(stmts);
    
    HandleScope handle_scope(isolate_);
    Handle<NyScript> script(CodeGen::Generate(Handle<NyString>::Null(), block, core_));
    
    std::string buf;
    BytecodeArrayDisassembler::Disassembly(script->bcbuf(), script->file_info(), &buf, 1024);
    static const char z[] = {
        "[000] LoadNil 0 2 ; line: 0\n"
        "[003] Mul 0 -1 0 ; line: 0\n"
        "[007] Div 1 -2 1 ; line: 0\n"
        "[011] Add 1 0 1 ; line: 0\n"
        "[015] Ret 1 1 ; line: 0\n"
    };
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
    Handle<NyScript> script(CodeGen::Generate(Handle<NyString>::Null(), block, core_));
    
//    std::string buf;
//    BytecodeArrayDisassembler::Disassembly(script->bcbuf(), script->file_info(), &buf, 512);
//    puts(buf.c_str());
    
    std::string buf;
    BytecodeArrayDisassembler::Disassembly(core_, script, &buf, 1024);
    puts(buf.c_str());
}

} // namespace nyaa
    
} // namespace mai
