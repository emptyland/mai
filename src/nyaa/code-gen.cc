#include "nyaa/code-gen.h"
#include "nyaa/nyaa-core.h"
#include "mai-lang/nyaa.h"
#include "glog/logging.h"

namespace mai {
    
namespace nyaa {

Handle<NyFunction> Bytecode_CodeGenerate(Handle<NyString> file_name, ast::Block *root,
                                         base::Arena *arena, NyaaCore *core);
    
/*static*/ Handle<NyFunction> CodeGen::Generate(Handle<NyString> file_name, ast::Block *root,
                                                base::Arena *arena, NyaaCore *core) {
    switch (core->stub()->exec()) {
        case Nyaa::kInterpreter:
            return Bytecode_CodeGenerate(file_name, root, arena, core);
            
        case Nyaa::kAOT:
            break;
        default:
            break;
    }
    return Handle<NyFunction>::Empty();
}
    
} // namespace nyaa
    
} // namespace mai
