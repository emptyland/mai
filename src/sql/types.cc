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
    
/*static*/ SQLDateTime SQLDateTime::Now(uint32_t precision) {
    ::time_t t = ::time(nullptr);
    ::tm m;
    ::localtime_r(&t, &m);
    return {
        {
            static_cast<uint32_t>(m.tm_year),
            static_cast<uint32_t>(m.tm_mon),
            static_cast<uint32_t>(m.tm_mday)
        }, {
            static_cast<uint32_t>(m.tm_hour),
            static_cast<uint32_t>(m.tm_min),
            static_cast<uint32_t>(m.tm_sec), precision, 0
        }
    };
}
    
} // namespace sql
    
} // namespace mai
