#include "sql/mai-sql-connection-impl.h"
#include "sql/mai-sql-impl.h"
#include "sql/form-schema.h"
#include "sql/storage-engine.h"
#include "sql/parser.h"
#include "sql/ast.h"
#include "mai-sql/result-set.h"
#include "glog/logging.h"

namespace mai {
    
namespace sql {
    
MaiSQLConnectionImpl::MaiSQLConnectionImpl(const std::string &conn_str,
                                           const uint32_t conn_id,
                                           MaiSQLImpl *owns,
                                           StorageEngine *engine,
                                           base::Arena *arena,
                                           bool arena_ownership)
    : MaiSQLConnection(conn_str, DCHECK_NOTNULL(owns))
    , db_name_(conn_str)
    , conn_id_(conn_id)
    , owns_impl_(owns)
    , engine_(engine)
    , arena_(arena)
    , arena_ownership_(arena_ownership) {
}

/*virtual*/ MaiSQLConnectionImpl::~MaiSQLConnectionImpl() {
    if (arena_ownership_) {
        delete arena_;
    }
    
    owns_impl_->RemoveConnection(this);
}
    
Error MaiSQLConnectionImpl::Reinitialize(const std::string &init_db_name,
                                        base::Arena *arena,
                                        bool arena_ownership) {
    engine_ = nullptr;
    if (arena_ownership_) {
        delete arena_;
    }
    arena_  = DCHECK_NOTNULL(arena);
    arena_ownership_ = arena_ownership;
    return SwitchDB(init_db_name);
}

/*virtual*/ uint32_t MaiSQLConnectionImpl::conn_id() const {
    return conn_id_;
}
    
/*virtual*/ base::Arena *MaiSQLConnectionImpl::arena() const {
    return arena_;
}

/*virtual*/ std::string MaiSQLConnectionImpl::GetDB() const {
    return db_name_;
}
    
/*virtual*/ Error MaiSQLConnectionImpl::SwitchDB(const std::string &name) {
    engine_ = owns_impl_->GetDatabaseEngineOrNull(name);
    if (!engine_) {
        return MAI_CORRUPTION("Database name: " + name + " not found.");
    }
    db_name_ = name;
    return Error::OK();
}

/*virtual*/ Error MaiSQLConnectionImpl::Execute(const std::string &sql) {
    Parser::Result result;
    Error rs = Parser::Parse(sql.data(), sql.size(), arena_, &result);
    if (!rs) {
        return MAI_CORRUPTION(result.FormatError());
    }
    
    std::unique_ptr<ResultSet> stub;
    for (auto stmt : *result.block) {
        stub.reset(ExecuteStatement(stmt));
        
        if (stub->error().fail()) {
            return stub->error();
        }
        
        while (stub->Valid()) {
            stub->Next();
        }
    }
    return Error::OK();
}
    
/*virtual*/
ResultSet *MaiSQLConnectionImpl::Query(const std::string &sql) {
    // TODO:
    return nullptr;
}
    
/*virtual*/ PreparedStatement *MaiSQLConnectionImpl::Prepare(const std::string &sql) {
    // TODO:
    return nullptr;
}
    
ResultSet *MaiSQLConnectionImpl::ExecuteStatement(const Statement *stmt) {
    if (stmt->is_ddl()) { // execute utils command
        switch (stmt->kind()) {
            case AstNode::kCreateTable: {
                auto ast = CreateTable::Cast(stmt);
                Error rs = ExecuteCreateTable(ast);
                if (!rs) {
                    return ResultSet::AsError(rs);
                }
            } break;
                
            // TODO:

            default:
                break;
        }
    }
    
    if (stmt->is_dml()) {
        switch (stmt->kind()) {
            case AstNode::kInsert:
                break;
                
            // TODO:
                
            default:
                break;
        }
    }
    return ResultSet::AsError(Error::OK());
}
    
Error MaiSQLConnectionImpl::ExecuteCreateTable(const CreateTable *ast) {
    Form *frm = nullptr;
    std::lock_guard<std::mutex> lock(owns_impl_->form_schema_mutex_);
    Error rs = owns_impl_->form_schema_->BuildForm(db_name_, ast, &frm);
    if (!rs) {
        return rs;
    }
    rs = engine_->NewTable(db_name_, frm);
    if (!rs) {
        return rs;
    }
    rs = owns_impl_->form_schema_->LogAndApply(db_name_, frm->table_name(),
                                               frm);
    if (!rs) {
        return rs;
    }
    return Error::OK();
}
    
} // namespace sql
    
} // namespace mai
