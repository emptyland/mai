#ifndef MAI_SQL_PARSER_CTX_H_
#define MAI_SQL_PARSER_CTX_H_

#include <stddef.h>
#include <stdio.h>

namespace mai {
    
namespace sql {
    
class AstFactory;
class AstNode;

struct parser_ctx {
    void *lex = nullptr;
    AstFactory *factory = nullptr;
    AstNode *ast = nullptr;
}; // struct parser_ctx
        
} // namespace sql
    
} // namespace mai

using ::mai::sql::parser_ctx;

#endif // MAI_SQL_PARSER_CTX_H_
