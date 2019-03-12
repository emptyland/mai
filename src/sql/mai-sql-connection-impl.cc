#include "sql/mai-sql-connection-impl.h"
#include "sql/mai-sql-impl.h"
#include "sql/evaluation.h"
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
    
ResultSet *MaiSQLConnectionImpl::ExecuteStatement(const ast::Statement *stmt) {
    if (stmt->is_ddl()) { // execute utils command
        switch (stmt->kind()) {
            case ast::AstNode::kCreateTable: {
                auto ast = ast::CreateTable::Cast(stmt);
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
        if (!engine_) {
            return ResultSet::AsError(MAI_CORRUPTION("No db be used."));
        }
        switch (stmt->kind()) {
            case ast::AstNode::kInsert: {
                auto ast = ast::Insert::Cast(stmt);
                Error rs = ExecuteInsert(ast);
                if (!rs) {
                    return ResultSet::AsError(rs);
                }
            } break;
                
            // TODO:
                
            default:
                break;
        }
    }
    return ResultSet::AsError(Error::OK());
}
    
Error MaiSQLConnectionImpl::ExecuteCreateTable(const ast::CreateTable *ast) {
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
    
Error MaiSQLConnectionImpl::ExecuteInsert(const ast::Insert *ast) {
    base::intrusive_ptr<Form> form;

    owns_impl_->form_schema_mutex_.lock();
    Error rs = owns_impl_->form_schema_->AcquireFormSchema(db_name_, ast->table_name()->ToString(),
                                                           &form);
    owns_impl_->form_schema_mutex_.unlock();
    if (!rs) {
        return rs;
    }
    
    if (base::ArenaUtils::IsNotEmpty(ast->col_names())) {
        for (auto col_name : *ast->col_names()) {
            if (!form->FindColumnOrNull(col_name->ToString())) {
                return MAI_CORRUPTION("Column name: " + col_name->ToString() + " not exists.");
            }
        }
    }
    
    bool is_simple_insert = false;
    if (ast->select_clause() == nullptr) {
        is_simple_insert = true;
    }

    if (is_simple_insert) {
        return ExecuteSimpleInsert(form.get(), ast);
    }
    
    // TODO:
    return MAI_NOT_SUPPORTED("TODO:");
}
    
Error MaiSQLConnectionImpl::ExecuteSimpleInsert(const Form *form, const ast::Insert *ast) {
    eval::Context ctx(arena_);
    std::unique_ptr<StorageOperation> scoped_batch;
    StorageOperation *op = txn_.get();
    if (!op) {
        scoped_batch.reset(engine_->NewWriter());
        op = scoped_batch.get();
    }

    Error rs;
    if (base::ArenaUtils::IsNotEmpty(ast->col_names())) {
        const VirtualSchema *vs = form->virtual_schema();

        std::map<std::string, const ColumnDescriptor *> left_col;
        for (size_t i = 0; i < vs->columns_size(); ++i) {
            left_col.insert({vs->column(i)->name(), vs->column(i)});
        }
        for (auto name : *ast->col_names()) {
            left_col.erase(name->ToString());
        }
        for (auto pair : left_col) {
            if (pair.second->origin()->not_null) {
                return MAI_CORRUPTION("Not value set to column: " + pair.first);
            }
        }

        for (auto row : *ast->row_values_list()) {
            HeapTuple::Builder builder(form->virtual_schema(), arena_);
            int i = 0;
            for (auto expr : *row) {
                eval::Expression *node;
                rs = Evaluation::BuildExpression(form->virtual_schema(), expr, arena_, &node);
                if (!rs) {
                    return rs;
                }
                rs = node->Evaluate(&ctx);
                if (!rs) {
                    return rs;
                }
                auto cd = form->virtual_schema()->FindOrNull(ast->col_names()->at(i)->ToString());
                rs = SetValue(DCHECK_NOTNULL(cd), ctx.result(), &builder);
                if (!rs) {
                    return rs;
                }
                ++i;
            }

            auto tuple = builder.Build();
            if (ast->overwrite()) {
                rs = op->Put(tuple);
            } else {
                // TODO:
            }
        }
        op->Finialize();
    } else {
        // TODO:
    }
    return Error::OK();
}
    
Error MaiSQLConnectionImpl::SetValue(const ColumnDescriptor *cd, const SQLValue &value,
                                     HeapTupleBuilder *builder) {
    if (value.is_null()) {
        if (cd->origin() && cd->origin()->not_null) {
            return MAI_CORRUPTION("NULL set to not null column: " + cd->origin()->name);
        }
    }
    
    bool is_unsigned = cd->is_unsigned();
    switch (cd->type()) {
        case SQL_INT: {
            eval::Value v = value.ToIntegral(arena_, is_unsigned ? eval::Value::kU64 :
                                             eval::Value::kI64);
            if (is_unsigned) {
                if (v.u64_val > SQL_INT_U_MAX) {
                    return MAI_CORRUPTION("Too large set to column: " + cd->origin()->name);
                }
                builder->SetU64(cd->index(), v.u64_val);
            } else {
                if (v.i64_val > SQL_INT_MAX) {
                    return MAI_CORRUPTION("Too large set to column: " + cd->origin()->name);
                }
                if (v.i64_val < SQL_INT_MIN) {
                    return MAI_CORRUPTION("Too less set to column: " + cd->origin()->name);
                }
                builder->SetI64(cd->index(), v.i64_val);
            }
        } break;
            
            // TODO:
            
        default:
            DLOG(FATAL) << "Noreached!";
            break;
    }
    return Error::OK();
}
    
} // namespace sql
    
} // namespace mai
