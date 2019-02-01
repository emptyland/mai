#ifndef MAI_RESULT_SET_H_
#define MAI_RESULT_SET_H_

#include "my/types.h"
#include "mai/error.h"

namespace mai {
    
namespace sql {

struct Time final {
    bool negative    = false;
    int day          = 0;
    int hour         = 0;
    int minute       = 0;
    int second       = 0;
    int micro_second = 0;
}; // struct Time
    
struct DateTime final {
    int year         = 0;
    int month        = 0;
    int day          = 0;
    int hour         = 0;
    int minute       = 0;
    int second       = 0;
    int micro_second = 0;
}; // struct DateTime

} // namespace sql
    
class ResultSet {
public:
    ResultSet() {}
    virtual ~ResultSet() {}
    
    static ResultSet *AsError(Error err);
    static ResultSet *AsNone();
    
    virtual Error error() const = 0;
    
    virtual int errors() const = 0;
    
    virtual int warnings() const = 0;
    
    // Number of affected rows.
    virtual int affects() const = 0;
    
    virtual int64_t latest_id() const = 0;
    
    virtual int columns_count() const = 0;
    
    virtual std::string column_name(int i) const = 0;
    
    virtual my::Protocol::Type column_type(int i) const = 0;
    
    virtual int column_decimals() const = 0;
    
    /*
     ProtocolBinary::MYSQL_TYPE_STRING,
     ProtocolBinary::MYSQL_TYPE_VARCHAR,
     ProtocolBinary::MYSQL_TYPE_VAR_STRING,
     ProtocolBinary::MYSQL_TYPE_ENUM,
     ProtocolBinary::MYSQL_TYPE_SET,
     ProtocolBinary::MYSQL_TYPE_LONG_BLOB,
     ProtocolBinary::MYSQL_TYPE_MEDIUM_BLOB,
     ProtocolBinary::MYSQL_TYPE_BLOB,
     ProtocolBinary::MYSQL_TYPE_TINY_BLOB,
     ProtocolBinary::MYSQL_TYPE_GEOMETRY,
     ProtocolBinary::MYSQL_TYPE_BIT,
     ProtocolBinary::MYSQL_TYPE_DECIMAL,
     ProtocolBinary::MYSQL_TYPE_NEWDECIMAL:
     */
    virtual std::string_view GetString(int i) const = 0;
    
    // ProtocolBinary::MYSQL_TYPE_TINY
    virtual int8_t  GetInt8(int i) const = 0;
    
    // ProtocolBinary::MYSQL_TYPE_SHORT, ProtocolBinary::MYSQL_TYPE_YEAR
    virtual int16_t GetInt16(int i) const = 0;
    
    // ProtocolBinary::MYSQL_TYPE_INT24, ProtocolBinary::MYSQL_TYPE_LONG
    virtual int32_t GetInt32(int i) const = 0;
    
    // ProtocolBinary::MYSQL_TYPE_LONGLONG
    virtual int64_t GetInt64(int i) const = 0;
    
    // ProtocolBinary::MYSQL_TYPE_FLOAT
    virtual float   GetFloat32(int i) const = 0;
    
    // ProtocolBinary::MYSQL_TYPE_DOUBLE
    virtual double  GetFloat64(int i) const = 0;
    
    // ProtocolBinary::MYSQL_TYPE_DATE, ProtocolBinary::MYSQL_TYPE_DATETIME,
    // ProtocolBinary::MYSQL_TYPE_TIMESTAMP:
    virtual sql::DateTime GetDateTime(int i) const =0;
    
    // ProtocolBinary::MYSQL_TYPE_TIME:
    virtual sql::Time GetTime(int i) const = 0;
    
    // ProtocolBinary::MYSQL_TYPE_NULL
    virtual bool IsNull(int i) const = 0;
    
    bool IsNotNull(int i) { return !IsNull(i); }
    
    virtual bool Valid() const = 0;
    
    virtual void Next() = 0;
    
    ResultSet(const ResultSet &) = delete;
    void operator = (const ResultSet &) = delete;
}; // class ResultSet
    
} // namespace mai


#endif // MAI_RESULT_SET_H_
