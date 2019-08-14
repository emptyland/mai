#include "sql/types.h"
#include "core/decimal-v2.h"
#include "base/arenas.h"
#include "base/arena-utils.h"
#include "base/slice.h"
#include <ctype.h>
#include <stdlib.h>

namespace mai {
    
namespace sql {
    
using ::mai::base::Slice;

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


const SQLFunctionDescEntry kSQLFunctionDesc[] = {
#define DECL_DESC(name) {#name, SQL_F_##name, 0},
    DEFINE_SQL_FUNCTIONS(DECL_DESC)
#undef  DECL_DESC
    
#define DECL_DESC(name) {#name, SQL_F_##name, 1},
    DEFINE_SQL_AGGREGATE(DECL_DESC)
#undef  DECL_DESC
};
    
SQLDecimal *SQLTime::ToDecimal(base::Arena *arena) const {
    base::ScopedArena scoped_buf;
    SQLDecimal *rv = SQLDecimal::NewU64(ToU64(), &scoped_buf);
    rv = rv->Mul(SQLDecimal::GetFastPow10(kExp), &scoped_buf);
    SQLDecimal *add = SQLDecimal::NewU64(micros, &scoped_buf);
    rv = rv->Add(add, arena);
    rv->set_negative_and_exp(negative, SQLTime::kExp);
    return rv;
}
    
SQLDecimal *SQLDateTime::ToDecimal(base::Arena *arena) const {
    base::ScopedArena scoped_buf;
    SQLDecimal *rv = SQLDecimal::NewU64(ToU64(), &scoped_buf);
    rv = rv->Mul(SQLDecimal::GetFastPow10(SQLTime::kExp), &scoped_buf);
    SQLDecimal *add = SQLDecimal::NewU64(time.micros, &scoped_buf);
    rv = rv->Add(add, arena);
    rv->set_negative_and_exp(time.negative, SQLTime::kExp);
    return rv;
}
    
/*static*/ SQLDateTime SQLDateTime::Now() {
    ::time_t t = ::time(nullptr);
    ::tm m;
    ::localtime_r(&t, &m);
    return {
        {
            static_cast<uint32_t>(m.tm_year) + 1900,
            static_cast<uint32_t>(m.tm_mon),
            static_cast<uint32_t>(m.tm_mday)
        }, {
            static_cast<uint32_t>(m.tm_hour),
            static_cast<uint32_t>(m.tm_min),
            static_cast<uint32_t>(m.tm_sec), true, 0
        }
    };
}
    
static const uint32_t kTimeMicrosPow10Rexp[7] = {
    1000000,
    100000,
    10000,
    1000,
    100,
    10,
    1,
};
    
/*static*/ int
SQLTimeUtils::Parse(const char *s, size_t n, SQLDateTime *result) {
    enum State : int {
        kInit,
        kPrefixNegative,
        kPrefixNum,
        kBeginMonth,
        kMonth,
        kBeginDay,
        kDay,
        kDate,
        kHour,
        kBeginMinute,
        kMinute,
        kBeginSecond,
        kSecond,
        kBeginMicroSecond,
        kMicroSecond,
    };
    result->time.negative = false;
    
    bool has_date = false;
    State state = kInit;
    char num[8] = {0};
    const char *e = s + n;
    int i = 0;
    while (s < e) {
        char c = *s++;
        
        if (c >= '0' && c <= '9') {
            if (i >= 6) {
                return 0;
            }

            num[i++] = c;
            if (state == kInit) {
                state = kPrefixNum;
            } else if (state == kPrefixNum) {
                // ignore
            } else if (state == kPrefixNegative) {
                result->time.negative = true;
                state = kPrefixNum;
            } else if (state == kBeginMonth ||
                       state == kBeginDay ||
                       state == kBeginMinute ||
                       state == kBeginSecond ||
                       state == kBeginMicroSecond) {
                state = static_cast<State>(static_cast<int>(state) + 1);
            } else if (state == kMonth ||
                       state == kDay ||
                       state == kMinute ||
                       state == kSecond ||
                       state == kMicroSecond) {
                if (state != kMicroSecond && i > 2) {
                    return 0;
                }
            } else {
                return 0;
            }
        } else if (c == '-') {
            num[i] = 0;
            i = 0;

            if (state == kInit) {
                state = kPrefixNegative;
            } else if (state == kPrefixNum) {
                auto y = atoi(num);
                if (y < 1000 || y > 9999) {
                    return 0;
                }
                result->date.year = y;
                state = kBeginMonth;
            } else if (state == kMonth) {
                auto m = atoi(num);
                if (m < 1 || m > 12) {
                    return 0;
                }
                result->date.month = m;
                state = kBeginDay;
            }  else {
                return 0;
            }
        } else if (c == ':') {
            num[i] = 0;
            i = 0;
            
            if (state == kPrefixNum) {
                auto h = atoi(num);
                if (has_date) {
                    if (h < 0 || h > 23) {
                        return 0;
                    }
                } else {
                    if (h < 0 || h > 999) {
                        return 0;
                    }
                }
                result->time.hour = h;
                state = kBeginMinute;
            } else if (state == kMinute) {
                auto m = atoi(num);
                if (m < 0 || m > 59) {
                    return 0;
                }
                result->time.minute = m;
                state = kBeginSecond;
            } else {
                return 0;
            }
        } else if (c == '.') {
            num[i] = 0;
            i = 0;
            
            if (state == kSecond) {
                auto s = atoi(num);
                if (s < 0 || s > 60) {
                    return 0;
                }
                result->time.second = s;
                state = kBeginMicroSecond;
            } else {
                return 0;
            }
        } else if (c == ' ') {
            num[i] = 0;
            i = 0;
            
            if (state == kDay) {
                auto d = atoi(num);
                if (d < 1 || d > 31) {
                    return 0;
                }
                result->date.day = d;
                state = kInit;
                has_date = true;
            } else {
                return 0;
            }
        } else {
            return 0;
        }
    }
    
    switch (state) {
        case kDay: {
            num[i] = 0;
            auto d = atoi(num);
            if (d < 1 || d > 31) {
                return 0;
            }
            result->date.day = d;
        } return 'd';
        case kSecond: {
            num[i] = 0;
            auto s = atoi(num);
            if (s < 0 || s > 60) {
                return 0;
            }
            result->time.second = s;
        } return has_date ? 'c' : 't';
        case kMicroSecond: {
            num[i] = 0;
            auto m = atoi(num);
            if (m < 0 || m > 999999) {
                return 0;
            }
            DCHECK_LT(i, arraysize(kTimeMicrosPow10Rexp));
            result->time.micros = m * kTimeMicrosPow10Rexp[i];
        } return has_date ? 'c' : 't';
        default:
            break;
    }
    return 0;
}


////////////////////////////////////////////////////////////////////////////////
// class Value
////////////////////////////////////////////////////////////////////////////////

bool SQLValue::IsZero() const {
    if (!is_number()) {
        return false;
    }
    switch (kind) {
        case kI64:
            return i64_val == 0;
        case kU64:
            return u64_val == 0;
        case kF64:
            return f64_val == 0;
        case kDecimal:
            return dec_val->zero();
        default:
            NOREACHED();
            break;
    }
    return false;
}

static inline SQLValue Str2U64(const base::ArenaString *str_val, base::Arena *arena) {
    SQLValue v;
    int s = Slice::ParseU64(str_val->data(), str_val->size(), &v.u64_val);
    DCHECK_GE(s, 0);
    if (s > 0) {
        v.kind = SQLValue::kDecimal;
        v.dec_val = SQLDecimal::NewDecLiteral(str_val->data(), str_val->size(),
                                              arena);
    } else {
        v.kind = SQLValue::kU64;
    }
    return v;
}

static inline SQLValue Str2H64(const base::ArenaString *str_val, bool sign, base::Arena *arena) {
    DCHECK_EQ('0', str_val->data()[0]);
    DCHECK_EQ('x', str_val->data()[1]);
    SQLValue v;
    int s = Slice::ParseH64(str_val->data() + 2, str_val->size() - 2, &v.u64_val);
    DCHECK_GE(s, 0);
    if (s > 0) {
        v.kind = SQLValue::kDecimal;
        v.dec_val = SQLDecimal::NewHexLiteral(str_val->data(), str_val->size(),
                                              arena);
    } else {
        v.kind = sign ? SQLValue::kI64 : SQLValue::kU64;
    }
    return v;
}

static inline SQLValue Str2O64(const base::ArenaString *str_val, bool sign, base::Arena *arena) {
    DCHECK_EQ('0', str_val->data()[0]);
    SQLValue v;
    int s = Slice::ParseO64(str_val->data() + 1, str_val->size() - 1, &v.u64_val);
    DCHECK_GE(s, 0);
    if (s > 0) {
        v.kind = SQLValue::kDecimal;
        v.dec_val = SQLDecimal::NewOctLiteral(str_val->data(), str_val->size(),
                                              arena);
    } else {
        v.kind = sign ? SQLValue::kI64 : SQLValue::kU64;
    }
    return v;
}

static inline SQLValue Str2I64(const base::ArenaString *str_val, base::Arena *arena) {
    SQLValue v;
    int s = Slice::ParseI64(str_val->data(), str_val->size(), &v.i64_val);
    DCHECK_GE(s, 0);
    if (s > 0) {
        v.kind = SQLValue::kDecimal;
        v.dec_val = SQLDecimal::NewDecLiteral(str_val->data(), str_val->size(),
                                              arena);
    } else {
        v.kind = SQLValue::kI64;
    }
    return v;
}

SQLValue SQLValue::ToNumeric(base::Arena *arena, Kind hint) const {
    SQLValue v;
    v.kind = kI64;
    v.i64_val = 0;
    
    int rv = 0;
    switch (kind) {
        case kNull:
        case kF64:
            return *this;
            
        case kI64:
            if (hint == kU64) {
                v.kind = kU64;
                v.u64_val = static_cast<uint64_t>(i64_val);
            } else {
                return *this;
            }
            break;
            
        case kU64:
            if (hint == kI64) {
                v.kind = kI64;
                v.i64_val = static_cast<int64_t>(u64_val);
            } else {
                return *this;
            }
            break;
            
        case kDecimal:
            return *this;
            
        case kString:
            switch (hint) {
                case kDate:
                case kTime:
                case kDateTime:
                    v.dt_val.time.micros = 0;
                    rv = SQLTimeUtils::Parse(str_val->data(), str_val->size(), &v.dt_val);
                    if (rv == 'd') {
                        v.kind = kDate;
                    } else if (rv == 't') {
                        v.kind = kTime;
                    } else if (rv == 'c') {
                        v.kind = kDateTime;
                    }
                    if (rv) {
                        v = v.ToNumeric(arena, hint);
                        return v;
                    }
                    break;
                default:
                    break;
            }
            rv = Slice::LikeNumber(str_val->data(), str_val->size());
            switch (rv) {
                case 'd':
                    if (hint == kI64) {
                        v = Str2I64(str_val, arena);
                    } else {
                        v = Str2U64(str_val, arena);
                    }
                    break;
                case 's':
                    if (hint == kU64) {
                        v = Str2U64(str_val, arena);
                    } else {
                        v = Str2I64(str_val, arena);
                    }
                    break;
                case 'o':
                    v = Str2O64(str_val, hint == kI64, arena);
                    break;
                case 'h':
                    v = Str2H64(str_val, hint == kI64, arena);
                    break;
                case 'f':
                    v.kind = kDecimal;
                    v.dec_val = SQLDecimal::NewPointLiteral(str_val->data(), str_val->size(), arena);
                    break;
                case 'e':
                    v.kind = kDecimal;
                    v.dec_val = SQLDecimal::NewExpLiteral(str_val->data(), str_val->size(), arena);
                    break;
                default:
                    v.kind = kU64;
                    v.u64_val = 0;
                    break;
            }
            break;
            
        case kDate:
            switch (hint) {
                case kDateTime:
                    v.kind = kU64;
                    v.u64_val = SQLTimeUtils::ConvertToDateTime(dt_val.date).ToU64();
                    break;
                case kTime:
                    v.kind = kU64;
                    v.u64_val = SQLTimeUtils::ConvertToTime(dt_val.date).ToU64();
                    break;
                default:
                    v.kind = kU64;
                    v.u64_val = dt_val.date.ToU64();
                    break;
            }
            break;
            
        case kTime:
            switch (hint) {
                case kDateTime:
                    v.kind = kU64;
                    v.u64_val = SQLTimeUtils::ConvertToDateTime(dt_val.time).ToU64();
                    break;
                case kDate:
                    v.kind = kU64;
                    v.u64_val = SQLTimeUtils::ConvertToDate(dt_val.time).ToU64();
                    break;
                default:
                    if (dt_val.time.micros > 0) {
                        v.kind = kDecimal;
                        v.dec_val = dt_val.time.ToDecimal(arena);
                    } else {
                        v.kind = kU64;
                        v.u64_val = dt_val.time.ToU64();
                    }
                    break;
            }
            break;
            
        case kDateTime:
            switch (hint) {
                case kTime:
                    v.kind = kU64;
                    v.u64_val = SQLTimeUtils::ConvertToTime(dt_val).ToU64();
                    break;
                case kDate:
                    v.kind = kU64;
                    v.u64_val = SQLTimeUtils::ConvertToDate(dt_val).ToU64();
                    break;
                default:
                    if (dt_val.time.micros > 0) {
                        v.kind = kDecimal;
                        v.dec_val = dt_val.ToDecimal(arena);
                    } else {
                        v.kind = kU64;
                        v.u64_val = dt_val.ToU64();
                    }
                    break;
            }
            break;
            
        default:
            NOREACHED();
            break;
    }
    return v;
}

SQLValue SQLValue::ToIntegral(base::Arena *arena, Kind hint) const {
    SQLValue v = ToNumeric(arena, hint);
    switch (v.kind) {
        case kNull:
            break;
        case kF64:
            v.kind = SQLValue::kI64;
            v.i64_val = static_cast<int64_t>(v.f64_val);
            break;
        case kI64:
            break;
        case kU64:
            break;
        case kDecimal:
            if (hint == kU64) {
                v.kind = kU64;
                v.u64_val = v.dec_val->ToU64();
            } else {
                v.kind = kI64;
                v.i64_val = v.dec_val->ToI64();
            }
            break;
        default:
            NOREACHED();
            break;
    }
    return v;
}

SQLValue SQLValue::ToFloating(base::Arena *arena, Kind hint) const {
    SQLValue v = ToNumeric(arena, hint);
    switch (v.kind) {
        case kNull:
            break;
        case kF64:
            break;
        case kI64:
            v.kind = kF64;
            v.f64_val = static_cast<double>(v.i64_val);
            break;
        case kU64:
            v.kind = kF64;
            v.f64_val = static_cast<double>(v.u64_val);
            break;
        case kDecimal:
            v.kind = kF64;
            v.f64_val = v.dec_val->ToF64();
            break;
        default:
            NOREACHED();
            break;
    }
    return v;
}

SQLValue SQLValue::ToDecimal(base::Arena *arena, Kind hint) const {
    SQLValue v = ToNumeric(arena, hint);
    switch (v.kind) {
        case kNull:
            break;
        case kF64:
            DLOG(FATAL) << "f64 can nto convert to decimal.";
            break;
        case kI64:
            v.kind = kDecimal;
            v.dec_val = SQLDecimal::NewI64(v.i64_val, arena);
            break;
        case kU64:
            v.kind = kDecimal;
            v.dec_val = SQLDecimal::NewU64(v.u64_val, arena);
            break;
        case kDecimal:
            break;
        default:
            NOREACHED();
            break;
    }
    return v;
}

bool SQLValue::StrictEquals(const SQLValue &v) const {
    
    switch (kind) {
        case kNull:
            return v.kind == kNull;
        case kF64:
            return v.kind == kind && v.f64_val == f64_val;
        case kI64:
            return v.kind == kind && v.i64_val == i64_val;
        case kString:
            return v.kind == kind && ::strncmp(v.str_val->data(), str_val->data(),
                                               str_val->size()) == 0;
        case kDecimal:
            return v.kind == kind && v.dec_val->Compare(dec_val) == 0;
        case kDate:
            return v.kind == kind && v.dt_val.date.ToU64() == dt_val.date.ToU64();
        case kTime:
            return v.kind == kind && ::memcmp(&dt_val.time, &v.dt_val.time,
                                              sizeof(SQLTime)) == 0;
        case kDateTime:
            return v.kind == kind && ::memcmp(&dt_val, &v.dt_val, sizeof(SQLDateTime)) == 0;
        default:
            NOREACHED();
            break;
    }
    return false;
}

#define COMPARE_VAL(lhs, rhs) ((lhs) == (rhs) ? 0 : ((lhs) < (rhs) ? -1 : 1))

int SQLValue::Compare(const SQLValue &rhs, base::Arena *arena) const {
    DCHECK_NE(kNull, kind);
    DCHECK_NE(kNull, rhs.kind);
    
    base::ScopedArena scoped_buf;
    if (arena == nullptr) {
        arena = &scoped_buf;
    }
    SQLValue r;
    switch (kind) {
        case kF64:
            r = rhs.ToFloating(arena);
            return COMPARE_VAL(f64_val, r.f64_val);
        case kI64:
            r = rhs.ToIntegral(arena);
            if (r.kind == kI64) {
                return COMPARE_VAL(i64_val, r.i64_val);
            } else {
                if (i64_val < 0) {
                    return -1;
                } else {
                    return COMPARE_VAL(u64_val, r.u64_val);
                }
            }
        case kU64:
            r = rhs.ToIntegral(arena);
            if (r.kind == kU64) {
                return COMPARE_VAL(u64_val, r.u64_val);
            } else {
                if (r.i64_val < 0) {
                    return 1;
                } else {
                    return COMPARE_VAL(u64_val, r.u64_val);
                }
            }
        case kString:
            if (rhs.kind == kString) {
                return ::strncmp(str_val->data(), rhs.str_val->data(), str_val->size());
            }
            return -rhs.Compare(*this, arena);
        case kDecimal:
            r = rhs.ToNumeric(arena, kDecimal);
            if (r.is_floating()) {
                auto l = ToFloating(arena);
                return COMPARE_VAL(l.f64_val, r.f64_val);
            }
            DCHECK_EQ(kDecimal, r.kind);
            if (r.is_decimal()) {
                int max_exp = std::max(r.dec_val->exp(), dec_val->exp());
                auto lval =   dec_val->NewPrecision(max_exp, arena);
                auto rval = r.dec_val->NewPrecision(max_exp, arena);
                return lval->Compare(rval);
            }
            break;
        case kTime:
            r = rhs.ToIntegral(arena, kTime);
            return COMPARE_VAL(dt_val.time.ToU64(), r.u64_val);
        case kDate:
            r = rhs.ToIntegral(arena, kDate);
            return COMPARE_VAL(dt_val.date.ToU64(), r.u64_val);
        case kDateTime:
            r = rhs.ToIntegral(arena, kDateTime);
            return COMPARE_VAL(dt_val.ToU64(), r.u64_val);
        default:
            NOREACHED();
            break;
    }
    return 0;
}
    
} // namespace sql
    
} // namespace mai
