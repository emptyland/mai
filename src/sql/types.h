#ifndef MAI_SQL_TYPES_H_
#define MAI_SQL_TYPES_H_

#include <stdint.h>
#include <time.h>
#include <string.h>

namespace mai {
namespace base {
class Arena;
class ArenaString;
} // namespace base
namespace core {
namespace v2 {
class Decimal;
} // namespace v2
} // namespace core
namespace sql {

#define DEFINE_SQL_TYPES(V) \
    V(NULL_TYPE, 1, 0,  0) \
    V(BIGINT,    1, 12, 0) \
    V(INT,       1, 12, 0) \
    V(MEDIUMINT, 1, 12, 0) \
    V(SMALLINT,  1, 12, 0) \
    V(TINYINT,   1, 12, 0) \
    V(DECIMAL,   1, 64, 30) \
    V(NUMERIC,   1, 64, 30) \
    V(FLOAT,     1, 64, 30) \
    V(DOUBLE,    1, 64, 30) \
    V(CHAR,      1, 255, 0) \
    V(VARCHAR,   1, 65535, 0) \
    V(BINARY,    1, 255, 0) \
    V(VARBINARY, 1, 65535, 0) \
    V(DATE,      0, 0, 0) \
    V(TIME,      0, 0, 0) \
    V(DATETIME,  0, 0, 0)
    
enum SQLType : int {
#define DECL_ENUM(name, a1, a2, a3) SQL_##name,
    DEFINE_SQL_TYPES(DECL_ENUM)
#undef  DECL_ENUM
};
    
#define SQL_TINYINT_U_MAX 0xff
#define SQL_TINYINT_MAX   127
#define SQL_TINYINT_MIN   -128
    
#define SQL_SMALLINT_U_MAX 0xffff
#define SQL_SMALLINT_MAX   32767
#define SQL_SMALLINT_MIN   -32768
    
#define SQL_MEDIUMINT_U_MAX 0xffffff
#define SQL_MEDIUMINT_MAX   8388607
#define SQL_MEDIUMINT_MIN   -8388608
    
#define SQL_INT_U_MAX 0xffffffff
#define SQL_INT_MAX   2147483647
#define SQL_INT_MIN   -2147483648
    
#define SQL_BIGINT_U_MAX 0xffffffffffffffff
#define SQL_BIGINT_MAX   9223372036854775807
#define SQL_BIGINT_MIN   -9223372036854775808
    
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
    
enum StorageKind : int {
    SQL_ROW_STORE,
    SQL_COLUMN_STORE,
};
    
enum SQLJoinKind : int {
    SQL_CROSS_JOIN,
    SQL_INNER_JOIN,
    SQL_OUTTER_JOIN,
    SQL_LEFT_OUTTER_JOIN,
    SQL_RIGHT_OUTTER_JOIN,
};
    
#define DEFINE_SQL_ALL_FUNCTIONS(V) \
    DEFINE_SQL_FUNCTIONS(V) \
    DEFINE_SQL_AGGREGATE(V)
    
#define DEFINE_SQL_FUNCTIONS(V) \
    V(ABS) \
    V(NOW) \
    V(DATE)

#define DEFINE_SQL_AGGREGATE(V) \
    V(COUNT) \
    V(AVG) \
    V(MAX) \
    V(MIN) \
    V(SUM) \
    V(BIT_XOR) \
    V(BIT_OR) \
    V(BIT_AND)
    
enum SQLFunction {
#define DECL_ENUM(name) SQL_F_##name,
    DEFINE_SQL_ALL_FUNCTIONS(DECL_ENUM)
#undef  DECL_ENUM
    SQL_MAX_F,
}; // enum SQLFunction
    
using SQLDecimal = core::v2::Decimal;
    
struct SQLTime {
    static constexpr const int kExp = 6;
    
    uint32_t  hour      : 16;
    uint32_t  minute    : 6;
    uint32_t  second    : 6;
    uint32_t  negative  : 4;
    uint32_t  micros;
    
    // 17:08:14 -> 170814
    uint64_t ToU64() const {
        return static_cast<uint64_t>(second) +
               static_cast<uint64_t>(minute) * 100 +
               static_cast<uint64_t>(hour) * 10000;
    }
    
    int64_t ToI64() const { return negative ? -ToU64() : ToU64(); }
    
    SQLDecimal *ToDecimal(base::Arena *arena) const;
    
    static SQLTime Now(bool negative) {
        ::time_t t = ::time(nullptr);
        ::tm m;
        ::localtime_r(&t, &m);
        return {
            static_cast<uint32_t>(m.tm_hour),
            static_cast<uint32_t>(m.tm_min),
            static_cast<uint32_t>(m.tm_sec), negative, 0
        };
    }
    
    static SQLTime Zero() { return {0, 0, 0, 0, 0}; }
};
    
static_assert(sizeof(SQLTime) == sizeof(uint64_t), "SQLTime too large.");
    
struct SQLDate {
    uint32_t  year  : 16;
    uint32_t  month : 8;
    uint32_t  day   : 8;

    // 1999-01-01 -> 19990101
    uint64_t ToU64() const {
        return static_cast<uint64_t>(day) +
               static_cast<uint64_t>(month) * 100 +
               static_cast<uint64_t>(year) * 10000;
    }
    
    static SQLDate Now() {
        ::time_t t = ::time(nullptr);
        ::tm m;
        ::localtime_r(&t, &m);
        return {
            static_cast<uint32_t>(m.tm_year) + 1900,
            static_cast<uint32_t>(m.tm_mon),
            static_cast<uint32_t>(m.tm_mday)
        };
    }
    
    static SQLDate Zero() { return {0, 0, 0}; }
};
    
static_assert(sizeof(SQLDate) == sizeof(uint32_t), "SQLDate too large.");
    
struct SQLDateTime {
    SQLDate date;
    SQLTime time;
    
    uint64_t ToU64() const { return time.ToU64() + date.ToU64() * 1000000; }
    
    SQLDecimal *ToDecimal(base::Arena *arena) const;
    
    static SQLDateTime Now();
    static SQLDateTime Zero() { return {SQLDate::Zero(), SQLTime::Zero()}; }
};
    
struct SQLTimeUtils {
    
    static uint64_t ToU64(const SQLTime &value) { return value.ToU64(); }
    static uint64_t ToU64(const SQLDate &value) { return value.ToU64(); }
    static uint64_t ToU64(const SQLDateTime &value) { return value.ToU64(); }
    
    static SQLTime ConvertToTime(const SQLDate &value) { return {0,0,0,0,0}; }
    static SQLDateTime ConvertToDateTime(const SQLDate &value) {
        return {value, {0,0,0,0,0}};
    }
    static SQLDate ConvertToDate(const SQLDateTime &value) { return value.date; }
    static SQLTime ConvertToTime(const SQLDateTime &value) { return value.time; }
    
    static SQLDate ConvertToDate(const SQLTime &value) {
        return SQLDate::Now();
    }
    static SQLDateTime ConvertToDateTime(const SQLTime &value) {
        return {SQLDate::Now(), value};
    }
    
    // return:
    // 0 = not a date/date-time/time
    // 'c' = date-time : 0000-00-00 00:00:00
    // 'd' = date      : 0000-00-00
    // 't' = time      : 00:00:00
    static int Parse(const char *s, SQLDateTime *result) {
        return Parse(s, !s ? 0 : strlen(s), result);
    }
    
    static int Parse(const char *s, size_t n, SQLDateTime *result);
};
    
struct SQLValue {
    enum Kind : int {
        kNull,
        kString,
        kDecimal,
        kU64,
        kI64,
        kF64,
        kDate,
        kTime,
        kDateTime,
    };
    
    Kind kind;
    union {
        const base::ArenaString *str_val;
        const SQLDecimal *dec_val;
        double  f64_val;
        uint64_t u64_val;
        int64_t i64_val;
        SQLDateTime dt_val;
    };
    
    bool is_floating() const { return kind == kF64; }
    bool is_integral() const { return kind == kI64 || kind == kU64; }
    bool is_decimal() const { return kind == kDecimal; }
    bool is_number() const {
        return is_floating() || is_integral() || is_decimal();
    }
    bool is_null() const { return kind == kNull; }
    bool is_not_null() const { return !is_null(); }
    
    bool IsZero() const;
    
    SQLValue ToNumeric(base::Arena *arena, Kind hint = kNull) const;
    SQLValue ToIntegral(base::Arena *arena, Kind hint = kNull) const;
    SQLValue ToFloating(base::Arena *arena, Kind hint = kNull) const;
    SQLValue ToDecimal(base::Arena *arena, Kind hint = kNull) const;
    
    bool StrictEquals(const SQLValue &rhs) const;
    int Compare(const SQLValue &rhs, base::Arena *arena = nullptr) const;
    
    void Minus();
}; // struct RawData

struct SQLTypeDescEntry {
    const char *name;
    int size_kind;
    int max_m_size;
    int max_d_size;
};

extern const SQLTypeDescEntry kSQLTypeDesc[];

extern const char *kSQLKeyText[];
    
struct SQLFunctionDescEntry {
    const char *name;
    SQLFunction fnid;
    int aggregate;
};
    
extern const SQLFunctionDescEntry kSQLFunctionDesc[];

} // namespace sql
    
} // namespace mai


#endif // MAI_SQL_TYPES_H_
