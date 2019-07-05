#ifndef MAI_NYAA_PARSER_CTX_H_
#define MAI_NYAA_PARSER_CTX_H_

#include <stddef.h>
#include <stdio.h>

namespace mai {
namespace base {
class ArenaString;
class Arena;
} // namespace base
namespace nyaa {
namespace ast {
class Factory;
class AstNode;
class Block;
} // namespace ast

struct parser_ctx {
    void *lex = nullptr;
    ast::Factory *factory = nullptr;
    base::Arena *arena = nullptr;
    ast::Block *block = nullptr;
    int next_trace_id = 0;
    const base::ArenaString *err_msg = nullptr;
    int err_line = 0;
    int err_column = 0;
}; // struct parser_ctx
    
struct lexer_extra {
    int old_state = 0;
    int column    = 1;
    ast::Factory *factory = nullptr;
    base::Arena *arena = nullptr;
};

} // namespace nyaa
    
} // namespace mai

using ::mai::nyaa::parser_ctx;

#endif // MAI_NYAA_PARSER_CTX_H_
