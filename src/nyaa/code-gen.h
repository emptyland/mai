#ifndef MAI_NYAA_CODE_GEN_H_
#define MAI_NYAA_CODE_GEN_H_

#include "base/base.h"
#include "mai-lang/handles.h"

namespace mai {
namespace nyaa {
namespace ast {
class Block;
} // namespace ast
class NyaaCore;
class NyScript;
class NyString;
    
class CodeGen final {
public:
    
    static Handle<NyScript> Generate(Handle<NyString> file_name, ast::Block *root,
                                     NyaaCore *core);
    
    DISALLOW_ALL_CONSTRUCTORS(CodeGen);
}; // class CodeGen

    
} // namespace nyaa
    
} // namespace mai


#endif // MAI_NYAA_CODE_GEN_H_
