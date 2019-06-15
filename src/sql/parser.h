#ifndef MAI_SQL_PARSER_H_
#define MAI_SQL_PARSER_H_

#include "base/base.h"
#include "mai/error.h"

namespace mai {
namespace base {
class ArenaString;
class Arena;
} // namespace base
namespace sql {
namespace ast {
class AstNode;
class Block;
} // namespace ast
    
class Parser final {
public:
    struct Result {
        const base::ArenaString *error;
        int line;
        int column;
        ast::AstNode *ast;
        ast::Block *block;
        
        std::string FormatError();
    };

    static Error Parse(const char *s, base::Arena *arena, Result *result);
    
    static Error Parse(const char *s, size_t n, base::Arena *arena,
                       Result *result);
    
    DISALLOW_ALL_CONSTRUCTORS(Parser);
}; // class Parser
    
} // namespace sql
    
} // namespace mai

#endif // MAI_SQL_PARSER_H_
