#include "sql/mai-sql-connection-impl.h"
#include "sql/mai-sql-impl.h"
#include "sql/form-schema.h"
#include "sql/parser.h"
#include "sql/ast-factory.h"
#include "glog/logging.h"

namespace mai {
    
namespace sql {
    
MaiSQLConnectionImpl::MaiSQLConnectionImpl(const std::string &conn_str,
                                           const uint32_t conn_id,
                                           MaiSQLImpl *owns,
                                           base::Arena *arena,
                                           bool arena_ownership)
    : MaiSQLConnection(conn_str, DCHECK_NOTNULL(owns))
    , conn_id_(conn_id)
    , owns_impl_(owns)
    , arena_(arena)
    , arena_ownership_(arena_ownership) {
}

/*virtual*/ MaiSQLConnectionImpl::~MaiSQLConnectionImpl() {
    if (arena_ownership_) {
        delete arena_;
    }
}

/*virtual*/ uint32_t MaiSQLConnectionImpl::conn_id() const {
    return conn_id_;
}
    
/*virtual*/ base::Arena *MaiSQLConnectionImpl::arena() const {
    return arena_;
}

/*virtual*/ std::string MaiSQLConnectionImpl::GetDB() const {
    return "";
}
    
/*virtual*/ Error MaiSQLConnectionImpl::SwitchDB(const std::string &name) {
    return MAI_NOT_SUPPORTED("TODO:");
}

/*virtual*/ Error MaiSQLConnectionImpl::Execute(const std::string &sql) {
    AstFactory factory(arena_);
    Parser::Result result;
    
    Error rs = Parser::Parse(sql.c_str(), &factory, &result);
    if (!rs) {
        return MAI_CORRUPTION(result.FormatError());
    }
    
    
    return MAI_NOT_SUPPORTED("TODO:");
}
    
} // namespace sql
    
} // namespace mai
