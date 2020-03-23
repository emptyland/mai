#ifndef MAI_NYAA_PARSER_H_
#define MAI_NYAA_PARSER_H_

#include "base/base.h"
#include <string>

namespace mai {
namespace base {
class ArenaString;
class Arena;
} // namespace base
namespace nyaa {
namespace ast {
class AstNode;
class Block;
using String = base::ArenaString;
} // namespace ast

struct Parser final {
    struct Result {
        const ast::String *error;
        int error_line;
        int error_column;
        ast::Block *block;
        int next_trace_id;
        
        std::string ToString() const;
    };

    static Result Parse(const char *s, int max_trace_id, base::Arena *arena);

    static Result Parse(const char *s, size_t n, int max_trace_id, base::Arena *arena);
    
    static Result Parse(FILE *fp, int max_trace_id, base::Arena *arena);
    
    DISALLOW_ALL_CONSTRUCTORS(Parser);
}; // class Parser

} // namespace nyaa
    
} // namespace mai

#endif // MAI_NYAA_PARSER_H_
