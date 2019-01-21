#ifndef MAI_SQL_TYPES_H_
#define MAI_SQL_TYPES_H_

#include <stdint.h>

namespace mai {
    
namespace sql {

#define DEFINE_SQL_TYPES(V) \
    V(BIGINT,   1, 12, 0) \
    V(INT,      1, 12, 0) \
    V(SMALLINT, 1, 12, 0) \
    V(TINYINT,  1, 12, 0) \
    V(DECIMAL,  1, 12, 12) \
    V(NUMERIC,  1, 12, 12) \
    V(CHAR,     1, 256, 0) \
    V(VARCHAR,  1, 1024, 0) \
    V(DATE,     0, 0, 0) \
    V(DATETIME, 0, 0, 0)
    
enum SQLType : int {
#define DECL_ENUM(name, a1, a2, a3) SQL_##name,
    DEFINE_SQL_TYPES(DECL_ENUM)
#undef  DECL_ENUM
};
    
#define DEFINE_SQL_KEYS(V) \
    V(NOT_KEY,     "") \
    V(KEY,         "KEY") \
    V(UNIQUE_KEY,  "UNIQUE KEY") \
    V(PRIMARY_KEY, "PRIMARY KEY")

enum SQLKeyType : int {
#define DECL_ENUM(name, txt) SQL_##name,
    DEFINE_SQL_KEYS(DECL_ENUM)
#undef  DECL_ENUM
};
    
#define DEFINE_SQL_CMP_OPS(V) \
    V(CMP_EQ, "=",  0) \
    V(CMP_STRICT_EQ, "<=>", 0) \
    V(CMP_NE, "<>", 0) \
    V(CMP_GT, ">",  0) \
    V(CMP_GE, ">=", 0) \
    V(CMP_LT, "<",  0) \
    V(CMP_LE, "<=", 0)
    
#define DEFINE_SQL_UNARY_OPS(V) \
    V(MINUS,       "-",           1) \
    V(NOT,         "NOT",         0) \
    V(BIT_INV,     "~",           1) \
    V(IS_NOT_NULL, "IS NOT NULL", 0) \
    V(IS_NULL,     "IS NULL",     0)

#define DEFINE_SQL_BINARY_OPS(V) \
    V(ASSIGN,  ":=", -1) \
    V(PLUS,    "+",   1) \
    V(SUB,     "-",   1) \
    V(MUL,     "*",   1) \
    V(DIV,     "/",   1) \
    V(MOD,     "%",   1) \
    V(BIT_XOR, "^",   1) \
    V(BIT_OR,  "|",   1) \
    V(BIT_AND, "&",   1) \
    V(LSHIFT,  "<<",  1) \
    V(RSHIFT,  ">>",  1) \
    V(AND,     "AND", 0) \
    V(XOR,     "XOR", 0) \
    V(OR,      "OR",  0) \
    V(LIKE,    "LIKE", 0) \
    V(NOT_LIKE, "NOT LIKE", 0)
    
#define DEFINE_SQL_MULTI_OPS(V) \
    V(IN,      "IN",      0) \
    V(NOT_IN,  "NOT IN",  0) \
    V(BETWEEN, "BETWEEN", 0) \
    V(NOT_BETWEEN, "NOT BETWEEN", 0)
    
enum SQLOperator : int {
#define DECL_ENUM(name, op, fmt) SQL_##name,
    DEFINE_SQL_CMP_OPS(DECL_ENUM)
    DEFINE_SQL_UNARY_OPS(DECL_ENUM)
    DEFINE_SQL_BINARY_OPS(DECL_ENUM)
    DEFINE_SQL_MULTI_OPS(DECL_ENUM)
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
    

struct SQLTypeDescEntry {
    const char *name;
    int size_kind;
    int max_fixed_size;
    int max_float_size;
};

extern const SQLTypeDescEntry kSQLTypeDesc[];
extern const char *kSQLKeyText[];

} // namespace sql
    
} // namespace mai


#endif // MAI_SQL_TYPES_H_
