#include "sql/types.h"

namespace mai {
    
namespace sql {

#define DECL_DESC(name, a1, a2, a3) {#name, a1, a2, a3},
const SQLTypeDescEntry kSQLTypeDesc[] = {
    DEFINE_SQL_TYPES(DECL_DESC)
};
#undef  DECL_DESC

#define DECL_TEXT(name, text) text,
const char *kSQLKeyText[] = {
    DEFINE_SQL_KEYS(DECL_TEXT)
};
#undef  DECL_TEXT
    
} // namespace sql
    
} // namespace mai
