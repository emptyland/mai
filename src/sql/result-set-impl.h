#ifndef MAI_SQL_RESULT_SET_H_
#define MAI_SQL_RESULT_SET_H_

#include "base/base.h"
#include "mai-sql/result-set.h"
#include "mai/error.h"

namespace mai {
    
namespace sql {
    
    class ResultSetStub : public ResultSet {
public:
    ResultSetStub() {}
    virtual ~ResultSetStub() override {}
    
    virtual Error error() const override { return MAI_NOT_SUPPORTED("TODO:"); }
    virtual int errors() const override { return 0; }
    virtual int warnings() const override { return 0; }
    virtual int affects() const override { return 0; }
    virtual int64_t latest_id() const override { return 0; }
    virtual int columns_count() const override { return 0; }
    virtual std::string column_name(int i) const override { return ""; }
    virtual my::Protocol::Type column_type(int i) const override {
        return my::Protocol::MYSQL_TYPE_NULL;
    }
    virtual int column_decimals() const override { return 0; }
    virtual std::string_view GetString(int) const override { return ""; }
    virtual int8_t  GetInt8(int) const override { return 0; }
    virtual int16_t GetInt16(int) const override { return 0; }
    virtual int32_t GetInt32(int) const override { return 0; }
    virtual int64_t GetInt64(int) const override { return 0; }
    virtual float   GetFloat32(int) const override { return 0; }
    virtual double  GetFloat64(int) const override { return 0; }
    virtual sql::DateTime GetDateTime(int) const override {
        return sql::DateTime{};
    }
    virtual sql::Time GetTime(int) const override { return sql::Time{}; }
    virtual bool IsNull(int) const override { return false; }
    virtual bool Valid() const override { return false; }
    virtual void Next() override {}
    
    DISALLOW_IMPLICIT_CONSTRUCTORS(ResultSetStub);
}; // class ResultSetStub


class ErrorResultSet : public ResultSetStub {
public:
    ErrorResultSet(Error err) : error_(err) {}
    virtual ~ErrorResultSet() override {}
    
    virtual Error error() const override { return error_; }

    virtual int errors() const override {
        return error_.ok() ? 0 : 1;
    }
    
    DISALLOW_IMPLICIT_CONSTRUCTORS(ErrorResultSet);
private:
    Error error_;
};
    
} // namespace sql
    
} // namespace mai



#endif // MAI_SQL_RESULT_SET_H_
