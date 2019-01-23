#ifndef MAI_SQL_RESULT_SET_H_
#define MAI_SQL_RESULT_SET_H_

#include "base/base.h"
#include "mai-sql/mai-sql.h"
#include "mai/error.h"

namespace mai {
    
namespace sql {
    
class ResultSetStub : public ResultSet {
public:
    ResultSetStub() {}
    virtual ~ResultSetStub() override {}
    
    virtual Error error() override { return MAI_NOT_SUPPORTED("TODO:"); }
    virtual int errors() override { return 0; }
    virtual int warnings() override { return 0; }
    virtual int affects() override { return 0; }
    virtual int64_t latest_id() override { return 0; }
    virtual int columns_count() override { return 0; }
    virtual bool Valid() override { return false; }
    virtual void Next() override {}
    
    DISALLOW_IMPLICIT_CONSTRUCTORS(ResultSetStub);
}; // class ResultSetStub


class ErrorResultSet : public ResultSetStub {
public:
    ErrorResultSet(Error err) : error_(err) {}
    virtual ~ErrorResultSet() override {}
    
    virtual Error error() override { return error_; }

    virtual int errors() override {
        return error_.ok() ? 0 : 1;
    }
    
    DISALLOW_IMPLICIT_CONSTRUCTORS(ErrorResultSet);
private:
    Error error_;
};
    
} // namespace sql
    
} // namespace mai



#endif // MAI_SQL_RESULT_SET_H_
