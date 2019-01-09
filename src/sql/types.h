#ifndef MAI_SQL_TYPES_H_
#define MAI_SQL_TYPES_H_

#include <stdint.h>

namespace mai {
    
namespace sql {

#define DEFINE_SQL_TYPES(V) \
    V(BIGINT) \
    V(INT) \
    V(SMALLINT) \
    V(TINYINT) \
    V(DECIMAL) \
    V(NUMERIC) \
    V(CHAR) \
    V(VARCHAR) \
    V(DATE) \
    V(DATETIME)
    
enum SQLType : int {
#define DECL_ENUM(name) SQL_##name,
    DEFINE_SQL_TYPES(DECL_ENUM)
#undef  DECL_ENUM
};

enum SQLKeyType : int {
    SQL_NOT_KEY,
    SQL_KEY,
    SQL_UNIQUE_KEY,
    SQL_PRIMARY_KEY,
};

} // namespace sql
    
} // namespace mai


#endif // MAI_SQL_TYPES_H_
