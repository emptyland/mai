#include "sql/result-set-impl.h"

namespace mai {
    
/*static*/ ResultSet *ResultSet::AsError(Error err) {
    return new sql::ErrorResultSet(err);
}
    
/*static*/ ResultSet *ResultSet::AsNone() {
    return new sql::ResultSetStub();
}

namespace sql {
    

    
} // namespace sql
    
} // namespace mai
