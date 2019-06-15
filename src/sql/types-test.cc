#include "sql/types.h"
#include "core/decimal-v2.h"
#include "base/arenas.h"
#include "gtest/gtest.h"

namespace mai {
    
namespace sql {
    
TEST(TypesTest, LikeTime) {
    SQLDateTime dt;
    EXPECT_EQ(0, sql::SQLTimeUtils::Parse("0", 1, &dt));
    
    EXPECT_EQ('t', sql::SQLTimeUtils::Parse("00:00:00", &dt));
    EXPECT_EQ('t', sql::SQLTimeUtils::Parse("23:59:59", &dt));
    
    EXPECT_EQ(23, dt.time.hour);
    EXPECT_EQ(59, dt.time.minute);
    EXPECT_EQ(59, dt.time.second);
    
    EXPECT_EQ(0, sql::SQLTimeUtils::Parse("23:59:59 ", &dt));
    
    EXPECT_EQ('t', sql::SQLTimeUtils::Parse("23:59:59.9", &dt));
    EXPECT_FALSE(dt.time.negative);
    EXPECT_EQ(23, dt.time.hour);
    EXPECT_EQ(59, dt.time.minute);
    EXPECT_EQ(59, dt.time.second);
    EXPECT_EQ(900000, dt.time.micros);
    
    EXPECT_EQ('t', sql::SQLTimeUtils::Parse("00:00:00.999999", &dt));
    EXPECT_FALSE(dt.time.negative);
    EXPECT_EQ(0, dt.time.hour);
    EXPECT_EQ(0, dt.time.minute);
    EXPECT_EQ(0, dt.time.second);
    EXPECT_EQ(999999, dt.time.micros);
    
    EXPECT_EQ('d', sql::SQLTimeUtils::Parse("1999-01-02", &dt));
    EXPECT_EQ(1999, dt.date.year);
    EXPECT_EQ(1, dt.date.month);
    EXPECT_EQ(2, dt.date.day);
    
    EXPECT_EQ(0, sql::SQLTimeUtils::Parse("1999-01-02 ", &dt));
}
    
TEST(TypesTest, ParseTime) {
    SQLDateTime dt = SQLDateTime::Zero();
    EXPECT_EQ('t', sql::SQLTimeUtils::Parse("23:12:34", &dt));
    EXPECT_FALSE(dt.time.negative);
    EXPECT_EQ(23, dt.time.hour);
    EXPECT_EQ(12, dt.time.minute);
    EXPECT_EQ(34, dt.time.second);
    EXPECT_EQ(0, dt.time.micros);
    
    base::ScopedArena scoped_buf;
    auto d = dt.time.ToDecimal(&scoped_buf);
    EXPECT_EQ("231234", d->ToString());
}
    
TEST(TypesTest, ParseNegativeTime) {
    SQLDateTime dt;
    EXPECT_EQ('t', sql::SQLTimeUtils::Parse("-199:00:00.001", &dt));
    EXPECT_TRUE(dt.time.negative);
    EXPECT_EQ(199, dt.time.hour);
    EXPECT_EQ(0, dt.time.minute);
    EXPECT_EQ(0, dt.time.second);
    EXPECT_EQ(1000, dt.time.micros);
    
    base::ScopedArena scoped_buf;
    auto d = dt.time.ToDecimal(&scoped_buf);
    EXPECT_EQ("-1990000.001", d->ToString());
}

TEST(TypesTest, ParseTimeMinMicros) {
    SQLDateTime dt = SQLDateTime::Zero();
    EXPECT_EQ('t', sql::SQLTimeUtils::Parse("23:12:34.000001", &dt));
    EXPECT_FALSE(dt.time.negative);
    EXPECT_EQ(23, dt.time.hour);
    EXPECT_EQ(12, dt.time.minute);
    EXPECT_EQ(34, dt.time.second);
    EXPECT_EQ(1, dt.time.micros);
    
    base::ScopedArena scoped_buf;
    auto d = dt.time.ToDecimal(&scoped_buf);
    EXPECT_EQ("231234.000001", d->ToString());
}

TEST(TypesTest, ParseTimeMaxMicros) {
    SQLDateTime dt = SQLDateTime::Zero();
    EXPECT_EQ('t', sql::SQLTimeUtils::Parse("23:12:34.999999", &dt));
    EXPECT_FALSE(dt.time.negative);
    EXPECT_EQ(23, dt.time.hour);
    EXPECT_EQ(12, dt.time.minute);
    EXPECT_EQ(34, dt.time.second);
    EXPECT_EQ(999999, dt.time.micros);
    
    base::ScopedArena scoped_buf;
    auto d = dt.time.ToDecimal(&scoped_buf);
    EXPECT_EQ("231234.999999", d->ToString());
}

TEST(TypesTest, ParseDateTime) {
    SQLDateTime dt = SQLDateTime::Zero();
    EXPECT_EQ('c', sql::SQLTimeUtils::Parse("2019-02-28 23:12:34", &dt));
    EXPECT_EQ(2019, dt.date.year);
    EXPECT_EQ(2, dt.date.month);
    EXPECT_EQ(28, dt.date.day);
    EXPECT_FALSE(dt.time.negative);
    EXPECT_EQ(23, dt.time.hour);
    EXPECT_EQ(12, dt.time.minute);
    EXPECT_EQ(34, dt.time.second);
    EXPECT_EQ(0, dt.time.micros);
    
    base::ScopedArena scoped_buf;
    auto d = dt.ToDecimal(&scoped_buf);
    EXPECT_EQ("20190228231234", d->ToString());
}
    
TEST(TypesTest, ParseDateTimeOneMicros) {
    SQLDateTime dt = SQLDateTime::Zero();
    EXPECT_EQ('c', sql::SQLTimeUtils::Parse("2019-02-28 23:59:60.1", &dt));
    EXPECT_EQ(2019, dt.date.year);
    EXPECT_EQ(2, dt.date.month);
    EXPECT_EQ(28, dt.date.day);
    EXPECT_FALSE(dt.time.negative);
    EXPECT_EQ(23, dt.time.hour);
    EXPECT_EQ(59, dt.time.minute);
    EXPECT_EQ(60, dt.time.second);
    EXPECT_EQ(100000, dt.time.micros);
    
    base::ScopedArena scoped_buf;
    auto d = dt.ToDecimal(&scoped_buf);
    EXPECT_EQ("20190228235960.1", d->ToString());
}

TEST(TypesTest, ParseDateTimeMinMicros) {
    SQLDateTime dt = SQLDateTime::Zero();
    EXPECT_EQ('c', sql::SQLTimeUtils::Parse("1970-01-01 00:00:00.000001", &dt));
    EXPECT_EQ(1970, dt.date.year);
    EXPECT_EQ(1, dt.date.month);
    EXPECT_EQ(1, dt.date.day);
    EXPECT_FALSE(dt.time.negative);
    EXPECT_EQ(0, dt.time.hour);
    EXPECT_EQ(0, dt.time.minute);
    EXPECT_EQ(0, dt.time.second);
    EXPECT_EQ(1, dt.time.micros);
    
    base::ScopedArena scoped_buf;
    auto d = dt.ToDecimal(&scoped_buf);
    EXPECT_EQ("19700101000000.000001", d->ToString());
}

TEST(TypesTest, ParseDateTimeMaxMicros) {
    SQLDateTime dt = SQLDateTime::Zero();
    EXPECT_EQ('c', sql::SQLTimeUtils::Parse("2999-12-31 23:59:59.999999", &dt));
    EXPECT_EQ(2999, dt.date.year);
    EXPECT_EQ(12, dt.date.month);
    EXPECT_EQ(31, dt.date.day);
    EXPECT_FALSE(dt.time.negative);
    EXPECT_EQ(23, dt.time.hour);
    EXPECT_EQ(59, dt.time.minute);
    EXPECT_EQ(59, dt.time.second);
    EXPECT_EQ(999999, dt.time.micros);
    
    base::ScopedArena scoped_buf;
    auto d = dt.ToDecimal(&scoped_buf);
    EXPECT_EQ("29991231235959.999999", d->ToString());
}

} // namespace sql
    
} // namespace mai
