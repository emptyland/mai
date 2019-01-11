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
    
#define DEFINE_SQL_CMP_OPS(V) \
    V(CMP_EQ, "=",  "%s = %s" ) \
    V(CMP_NE, "<>", "%s <> %s") \
    V(CMP_GT, ">",  "%s > %s" ) \
    V(CMP_GE, ">=", "%s >= %s") \
    V(CMP_LT, "<",  "%s < %s" ) \
    V(CMP_LE, "<=", "%s <= %s")
    
#define DEFINE_SQL_UNARY_OPS(V) \
    V(MINUS,       "-",           "-%s") \
    V(NOT,         "NOT",         "NOT %s") \
    V(BIT_INV,     "~",           "~%s") \
    V(IS_NOT_NULL, "IS NOT NULL", "%s IS NOT NULL") \
    V(IS_NULL,     "IS NULL",     "%s IS NULL")

#define DEFINE_SQL_BINARY_OPS(V) \
    V(ASSIGN,  ":=",  "%s := %s") \
    V(PLUS,    "+",   "%s + %s") \
    V(SUB,     "-",   "%s - %s") \
    V(MUL,     "*",   "%s * %s") \
    V(DIV,     "/",   "%s / %s") \
    V(MOD,     "%",   "%s % %s") \
    V(BIT_XOR, "^",   "%s ^ %s") \
    V(BIT_OR,  "|",   "%s | %s") \
    V(BIT_AND, "&",   "%s & %s") \
    V(LSHIFT,  "<<",  "%s << %s") \
    V(RSHIFT,  ">>",  "%s >> %s") \
    V(AND,     "AND", "%s AND %s") \
    V(OR,      "OR",  "%s OR %s")
    
enum SQLOperator : int {
#define DECL_ENUM(name, op, fmt) SQL_##name,
    DEFINE_SQL_CMP_OPS(DECL_ENUM)
    DEFINE_SQL_UNARY_OPS(DECL_ENUM)
    DEFINE_SQL_BINARY_OPS(DECL_ENUM)
    SQL_MAX_OP,
#undef  DECL_ENUM
};
    
enum SQLJoinKind : int {
    SQL_CROSS_JOIN,
    SQL_INNER_JOIN,
    SQL_OUTTER_JOIN,
    SQL_LEFT_OUTTER_JOIN,
    SQL_RIGHT_OUTTER_JOIN,
};

} // namespace sql
    
} // namespace mai


#endif // MAI_SQL_TYPES_H_
