#include "nyaa/code-gen.h"
#include "nyaa/nyaa-core.h"
#include "nyaa/function.h"
#include "nyaa/code-gen-utils.h"
#include "nyaa/ast.h"
#include "mai-lang/nyaa.h"
#include "glog/logging.h"

namespace mai {
    
namespace nyaa {

extern Handle<NyFunction> Bytecode_CodeGenerate(Handle<NyString> file_name, ast::Block *root,
                                                base::Arena *arena, NyaaCore *core);

extern Handle<NyFunction> AOT_CodeGenerate(Handle<NyString> file_name, ast::Block *root,
                                           base::Arena *arena, NyaaCore *core);
    
extern Error HIR_GenerateHIR(const BuiltinType *argv, size_t argc, const UpvalDesc *desc,
                             const BuiltinType *upvals, size_t n_upvals, ast::AstNode *ast,
                             hir::Function **rv, base::Arena *arena, NyaaCore *core);
    
extern Handle<NyCode> Code_CodeGenerateByLIR(const lir::InstructionBundle *ir_code,
                                             const BuiltinType *args, size_t argc, NyaaCore *core);
    
/*static*/ Handle<NyFunction> CodeGen::Generate(Handle<NyString> file_name, ast::Block *root,
                                                base::Arena *arena, NyaaCore *core) {
    switch (core->stub()->exec()) {
        case Nyaa::kInterpreter:
            return Bytecode_CodeGenerate(file_name, root, arena, core);
            
        case Nyaa::kAOT:
        case Nyaa::kAOT_And_JIT:
            return AOT_CodeGenerate(file_name, root, arena, core);

        default:
            break;
    }
    return Handle<NyFunction>::Empty();
}

/*static*/ Handle<NyCode> CodeGen::Generate(const lir::InstructionBundle *ir_code, // low-level-ir code
                                            const BuiltinType *args, // type of arguments
                                            size_t argc, // number of arguments
                                            NyaaCore *core) {
    return Code_CodeGenerateByLIR(ir_code, args, argc, core);
}
    
/*static*/ Error CodeGen::GenerateHIR(Handle<NyClosure> rb,
                                      const BuiltinType *args, // type of arguments
                                      size_t argc, // number of arguments
                                      hir::Function **rv, // result
                                      // TODO: constant pool
                                      base::Arena *arena,
                                      NyaaCore *core) {
    if (!core->stub()->use_jit()) {
        return MAI_NOT_SUPPORTED("only jit compiler can generate HIR.");
    }
    
    int n_upvals = rb->proto()->n_upvals();
    std::unique_ptr<BuiltinType[]> upvals;
    if (n_upvals > 0) {
        upvals.reset(new BuiltinType[n_upvals]);
        for (int i = 0; i < n_upvals; ++i) {
            upvals[i] = rb->upval(i)->GetType();
        }
    }
    
    Handle<NyString> packed = rb->proto()->packed_ast();
    std::string_view source(packed->bytes(), packed->size());
    ast::Factory factory(arena);
    ast::AstNode *ast = nullptr;
    if (Error rs = LoadCompactedAST(source, &factory, &ast); rs.fail()) {
        return rs;
    }
    
    return GenerateHIR(args, argc,
                       rb->proto()->upvals(),
                       upvals.get(),
                       n_upvals, ast, rv, arena, core);
}

/*static*/ Error CodeGen::GenerateHIR(const BuiltinType *args, // type of arguments
                                      size_t argc, // number of arguments
                                      const UpvalDesc *desc, // up-value descriptor
                                      const BuiltinType *upvals, // type of up-values
                                      size_t n_upvals, // number of up-values
                                      ast::AstNode *ast, // AST for generate
                                      hir::Function **rv, // result
                                      // TODO: constant pool
                                      base::Arena *arena,
                                      NyaaCore *core) {
    // TODO:
    
    return HIR_GenerateHIR(args, argc, desc, upvals, n_upvals, ast, rv, arena, core);
}
    
} // namespace nyaa
    
} // namespace mai
