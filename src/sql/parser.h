#ifndef MAI_SQL_PARSER_H_
#define MAI_SQL_PARSER_H_

#include "base/base.h"
#include "mai/error.h"

namespace mai {
namespace base {
class ArenaString;
} // namespace base
namespace sql {
    
class AstFactory;
class AstNode;
    
class Parser final {
public:
    struct Result {
        const base::ArenaString *error;
        int line;
        int column;
        AstNode *ast;
    };

    static Error Parse(const char *s, AstFactory *factory, Result *result);
    
    DISALLOW_ALL_CONSTRUCTORS(Parser);
}; // class Parser
    
} // namespace sql
    
} // namespace mai

#endif // MAI_SQL_PARSER_H_
