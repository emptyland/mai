#include "sql/types.h"
#include <ctype.h>
#include <stdlib.h>

namespace mai {
    
namespace sql {

#define DECL_DESC(name, a1, a2, a3) {#name, a1, a2, a3},
const SQLTypeDescEntry kSQLTypeDesc[] = {
    {"NULL", 0, 0, 0},
    DEFINE_SQL_TYPES(DECL_DESC)
};
#undef  DECL_DESC

#define DECL_TEXT(name, text) text,
const char *kSQLKeyText[] = {
    DEFINE_SQL_KEYS(DECL_TEXT)
};
#undef  DECL_TEXT
    
/*static*/ SQLDateTime SQLDateTime::Now() {
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
            static_cast<uint32_t>(m.tm_sec), true, 0
        }
    };
}
    
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
            result->time.micros = m;
        } return has_date ? 'c' : 't';
        default:
            break;
    }
    return 0;
}
    
} // namespace sql
    
} // namespace mai
