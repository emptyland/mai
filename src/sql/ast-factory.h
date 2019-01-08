#ifndef MAI_SQL_AST_FACTORY_H_
#define MAI_SQL_AST_FACTORY_H_

#include "sql/ast.h"

namespace mai {
    
namespace sql {
    
class AstFactory final {
public:
    AstFactory(base::Arena *arena) : arena_(arena) {}
    ~AstFactory() {}

    CreateTable *NewCreateTable() {
        return new (arena_) CreateTable();
    }
    
    DEF_PTR_GETTER_NOTNULL(base::Arena, arena);
private:
    base::Arena *const arena_;
}; // class AstFactory
    
} // namespace sql
    
} // namespace mai


#endif // MAI_SQL_AST_FACTORY_H_
