#include "sql/ast-factory.h"

namespace mai {
    
namespace sql {

namespace ast {
    
Call * Factory::NewCall(const AstString *name, bool disinct,
                        ExpressionList *parameters, const Location &location) {
    const SQLFunctionDescEntry *desc = nullptr;
    for (int i = 0; i < SQL_MAX_F; ++i) {
        if (::strncasecmp(name->data(), kSQLFunctionDesc[i].name,
                          name->size()) == 0) {
            desc = &kSQLFunctionDesc[i];
            break;
        }
    }
    if (!desc) {
        return new (arena_) Call(name, parameters, location);
    }
    if (desc->aggregate) {
        return new (arena_) Aggregate(desc->fnid, disinct, parameters,
                                      location);
    } else {
        return new (arena_) Call(desc->fnid, parameters, location);
    }
}
    
} // namespace ast

} // namespace sql
    
} // namespace mai
