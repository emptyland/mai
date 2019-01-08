#ifndef MAI_SQL_AST_FACTORY_H_
#define MAI_SQL_AST_FACTORY_H_

#include "sql/ast.h"
#include "base/slice.h"
#include <stdarg.h>

namespace mai {
    
namespace sql {
    
class AstFactory final {
public:
    AstFactory(base::Arena *arena) : arena_(arena) {}
    ~AstFactory() {}

    CreateTable *NewCreateTable(const AstString *schema_name) {
        return new (arena_) CreateTable(schema_name);
    }
    
    const AstString *NewString(const char *s, size_t n) {
        return AstString::New(arena_, s, n);
    }
    
    const AstString *NewString(const char *s) {
        return AstString::New(arena_, s);
    }
    
    const AstString *Format(const char *fmt, ...) {
        va_list ap;
        va_start(ap, fmt);
        std::string str(base::Slice::Vsprintf(fmt, ap));
        va_end(ap);
        return AstString::New(arena_, str);
    }
    
    DEF_PTR_GETTER_NOTNULL(base::Arena, arena);
private:
    base::Arena *const arena_;
}; // class AstFactory
    
} // namespace sql
    
} // namespace mai


#endif // MAI_SQL_AST_FACTORY_H_
