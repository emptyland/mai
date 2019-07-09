#ifndef MAI_NYAA_CODE_GEN_H_
#define MAI_NYAA_CODE_GEN_H_

#include "base/base.h"
#include "nyaa/builtin.h"
#include "mai-lang/handles.h"
#include "mai/error.h"

namespace mai {
namespace base {
class Arena;
} // namespace base
namespace nyaa {
namespace ast {
class Block;
class AstNode;
} // namespace ast
namespace hir {
class Function;
} // namespace hir
class NyaaCore;
class NyClosure;
class NyFunction;
class NyString;
struct UpvalDesc;
    
class CodeGen final {
public:
    
    static Handle<NyFunction> Generate(Handle<NyString> file_name, ast::Block *root,
                                       base::Arena *arena, NyaaCore *core);
    
    // Generate high-level IR
    static Error GenerateHIR(Handle<NyClosure> rb,
                             BuiltinType *args, // type of arguments
                             size_t argc, // number of arguments
                             hir::Function **rv, // result
                             // TODO: constant pool
                             base::Arena *arena,
                             NyaaCore *core);
    
    // Generate high-level IR
    static Error GenerateHIR(int n_params, // number of parameters in prototype function
                             bool vargs, // is variable arguments?
                             BuiltinType *args, // type of arguments
                             size_t argc, // number of arguments
                             const UpvalDesc *desc, // up-value descriptor
                             BuiltinType *upvals, // type of up-values
                             size_t n_upvals, // number of up-values
                             ast::AstNode *ast, // AST for generate
                             hir::Function **rv, // result
                             // TODO: constant pool
                             base::Arena *arena,
                             NyaaCore *core);
    
    DISALLOW_ALL_CONSTRUCTORS(CodeGen);
}; // class CodeGen

    
} // namespace nyaa
    
} // namespace mai


#endif // MAI_NYAA_CODE_GEN_H_
