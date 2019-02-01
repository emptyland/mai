#include "sql/types.h"
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
    EXPECT_EQ(9, dt.time.micros);
    
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
    
TEST(TypesTest, LikeNegativeTime) {
    SQLDateTime dt;
    EXPECT_EQ('t', sql::SQLTimeUtils::Parse("-199:00:00.001", &dt));
    EXPECT_TRUE(dt.time.negative);
    EXPECT_EQ(199, dt.time.hour);
    //EXPECT_EQ(199, dt.time.hour);
}
    
} // namespace sql
    
} // namespace mai
