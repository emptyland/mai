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
namespace lir {
class Function;
} // namespace lir
class NyaaCore;
class NyClosure;
class NyFunction;
class NyString;
class NyArray;
struct UpvalDesc;

class CodeGen final {
public:
    // Generate bytecode or AOT machine code
    static Handle<NyFunction> Generate(Handle<NyString> file_name, ast::Block *root,
                                       base::Arena *arena, NyaaCore *core);

    // Generate LIR code to machine code
    static Handle<NyCode> Generate(const lir::Function *ir_code, // low-level-ir code
                                   const BuiltinType *args, // type of arguments
                                   size_t argc, // number of arguments
                                   NyaaCore *core);

    // Generate high-level IR
    static Error GenerateHIR(Handle<NyClosure> rb,
                             const BuiltinType *args, // type of arguments
                             size_t argc, // number of arguments
                             hir::Function **rv, // result
                             // TODO: constant pool
                             base::Arena *arena,
                             NyaaCore *core);

    // Generate high-level IR
    static Error GenerateHIR(const BuiltinType *args, // type of arguments
                             size_t argc, // number of arguments
                             const UpvalDesc *desc, // up-value descriptor
                             const BuiltinType *upvals, // type of up-values
                             size_t n_upvals, // number of up-values
                             ast::AstNode *ast, // AST for generate
                             hir::Function **rv, // result
                             // TODO: constant pool
                             base::Arena *arena,
                             NyaaCore *core);
    
    // Generate low-level IR
    static Error GenerateLIR(hir::Function *func, // high-level-ir function
                             Handle<NyArray> *kpool, // received constant pool
                             lir::Function **rv, // received low-level-ir function
                             base::Arena *arena,
                             NyaaCore *core);

    DISALLOW_ALL_CONSTRUCTORS(CodeGen);
}; // class CodeGen


} // namespace nyaa

} // namespace mai


#endif // MAI_NYAA_CODE_GEN_H_
