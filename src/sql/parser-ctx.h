#ifndef MAI_SQL_PARSER_CTX_H_
#define MAI_SQL_PARSER_CTX_H_

#include <stddef.h>
#include <stdio.h>

namespace mai {
namespace base {
class ArenaString;
} // namespace base
namespace sql {
    
class AstFactory;
class AstNode;
class Block;

struct parser_ctx {
    void *lex = nullptr;
    AstFactory *factory = nullptr;
    AstNode *ast = nullptr;
    Block *block = nullptr;
    
    const base::ArenaString *err_msg = nullptr;
    int err_line = 0;
    int err_column = 0;
}; // struct parser_ctx
    
struct lexer_extra {
    int old_state = 0;
};

} // namespace sql
    
} // namespace mai

using ::mai::sql::parser_ctx;

#endif // MAI_SQL_PARSER_CTX_H_
