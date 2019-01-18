#include "sql/form-schema.h"
#include "db/write-ahead-log.h"
#include "base/slice.h"
#include "base/io-utils.h"
#include "mai/env.h"

namespace mai {
    
namespace sql {
    
class FormBuilder final {
public:
    using Column = Form::Column;
    using Index  = Form::Index;
    
    explicit FormBuilder(const Form *form) {
        if (!form) {
            return;
        }
        table_name_ = form->table_name_;
        engine_name_ = form->engine_name_;
        columns_ = form->columns_;
        column_names_ = form->column_names_;
        indices_ = form->indices_;
        index_names_ = form->index_names_;
        primary_key_ = form->primary_key_;
    }
    
    Error InsertColumn(std::shared_ptr<Column> column,
                       bool after, const std::string &hint) {
        Error rs = EnsureColumn(column->name, column);
        if (!rs) {
            return rs;
        }
        auto col_iter = column_names_.find(hint);
        if (col_iter == column_names_.end()) {
            return MAI_CORRUPTION("Column name: " + hint + " not found");
        }
        
        if (!after) {
            columns_.insert(columns_.begin(), column);
            AddIndexIfNeeded(0, column);
        } else {
            DCHECK(!hint.empty());
            auto pos = column_names_[hint];
            columns_.insert(columns_.begin() + pos, column);
            AddIndexIfNeeded(pos, column);
        }

        RefreshNames(false);
        return Error::OK();
    }
    
    Error AddColumn(std::shared_ptr<Column> column) {
        Error rs = EnsureColumn(column->name, column);
        if (!rs) {
            return rs;
        }
        
        auto pos = columns_.size();
        columns_.push_back(column);
        column_names_.insert({column->name, pos});

        AddIndexIfNeeded(pos, column);
        return Error::OK();
    }
    
    Error UpdateColumn(const std::string &name, std::shared_ptr<Column> column) {
        auto col_iter = column_names_.find(name);
        if (col_iter == column_names_.end()) {
            return MAI_CORRUPTION("Column name: " + name + " not found");
        }
        
        auto new_iter = column_names_.find(column->name);
        if (new_iter != column_names_.end() && new_iter != col_iter) {
            return MAI_CORRUPTION("Duplicated column name: " + column->name);
        }
        
        auto oldcol = columns_[col_iter->second];
        columns_[col_iter->second] = column;
        
        RefreshNames(true);
        RefreshIndices();
        return Error::OK();
    }

    Error EnsureColumn(const std::string &name,
                       std::shared_ptr<Column> column) const {
        auto col_iter = column_names_.find(name);
        if (col_iter != column_names_.end()) {
            return MAI_CORRUPTION("Duplicated column name: " + name);
        }
        
        if (column->key == SQL_NOT_KEY) {
            return Error::OK();
        }
        
        auto idx_iter = index_names_.find(name);
        if (idx_iter != index_names_.end()) {
            return MAI_CORRUPTION("Duplicated index name: " + name);
        }
        if (column->key == SQL_PRIMARY_KEY && primary_key_ != -1) {
            return MAI_CORRUPTION("Duplicated primary key: " + name);
        }
        return Error::OK();
    }
    
    DEF_VAL_PROP_RW(std::string, table_name);
    DEF_VAL_PROP_RW(std::string, engine_name);
    
    void SetOlder(Form *older) {
        version_ = older->version_ + 1;
        older_   = older;
    }
    
    Form *Build() {
        DCHECK(!table_name_.empty() && !engine_name_.empty());
        Form *form = new Form(table_name_, engine_name_);
        form->version_ = version_;
        form->older_   = older_;
        form->columns_ = std::move(columns_);
        form->column_names_ = std::move(column_names_);
        form->indices_ = std::move(indices_);
        form->index_names_ = std::move(index_names_);
        return form;
    }

private:
    void AddIndexIfNeeded(size_t pos, std::shared_ptr<Column> column) {
        if (column->key == SQL_NOT_KEY) {
            return;
        }
        if (column->key == SQL_PRIMARY_KEY) {
            primary_key_ = static_cast<int>(pos);
        }
        std::shared_ptr<Index> idx(new Index);
        idx->key = column->key;
        idx->name = column->name;
        idx->ref_columns.push_back(column);
        
        pos = indices_.size();
        indices_.push_back(idx);
        index_names_.insert({column->name, pos});
    }
    
    void RefreshNames(bool only_column) {
        column_names_.clear();
        size_t i = 0;
        primary_key_ = -1;
        for (auto col : columns_) {
            if (col->key == SQL_PRIMARY_KEY) {
                primary_key_ = static_cast<int>(i);
            }
            column_names_.insert({col->name, i++});
        }
        if (only_column) {
            return;
        }
        index_names_.clear();
        i = 0;
        for (auto idx : indices_) {
            index_names_.insert({idx->name, i++});
        }
    }
    
    void RefreshIndices() {
        indices_.clear();
        index_names_.clear();
        
        for (size_t i = 0; i < columns_.size(); ++i) {
            AddIndexIfNeeded(i, columns_[i]);
        }
    }
    
    std::string table_name_;
    std::string engine_name_;
    std::vector<std::shared_ptr<Column>> columns_;
    std::unordered_map<std::string, size_t> column_names_;
    std::vector<std::shared_ptr<Index>>  indices_;
    std::unordered_map<std::string, size_t> index_names_;
    int primary_key_ = -1;
    int version_ = 0;
    Form *older_ = nullptr;
}; // class FormBuilder


////////////////////////////////////////////////////////////////////////////////
/// class FormSchemaPatch
////////////////////////////////////////////////////////////////////////////////
    
void FormSchemaPatch::PartialEncode(bool add, std::string *buf) const {
    using ::mai::base::Slice;

    buf->append(1, add ? 0 : 1);
    Slice::WriteString(buf, db_name_);
    Slice::WriteString(buf, table_name_);
    Slice::WriteFixed32(buf, version_);
    Slice::WriteString(buf, engine_name_);
    Slice::WriteVarint64(buf, next_file_number_);
}
    
////////////////////////////////////////////////////////////////////////////////
/// class FormSchemaSet
////////////////////////////////////////////////////////////////////////////////
    
FormSchemaSet::FormSchemaSet(const std::string &abs_meta_dir,
                             const std::string &abs_data_dir, Env *env)
    : abs_meta_dir_(abs_meta_dir)
    , abs_data_dir_(abs_data_dir)
    , env_(DCHECK_NOTNULL(env)) {
    DCHECK(!abs_meta_dir.empty());
}

FormSchemaSet::~FormSchemaSet() {
    for (const auto &outter : dbs_) {
        for (const auto &inner : outter.second) {
            auto x = inner.second;
            while (x) {
                auto refs = x->ReleaseRef(); (void) refs;
                DCHECK_EQ(1, refs) << "Table: " << inner.first;
                auto p = x;
                x = x->older_;
                delete p;
            }
        }
    }
}

Error FormSchemaSet::LogAndApply(FormSchemaPatch *patch) {
    auto iter = dbs_.find(patch->db_name());
    if (iter == dbs_.end()) {
        return MAI_CORRUPTION("Database: " + patch->db_name() + " not found!");
    }
    auto db = &iter->second;
    auto it = db->find(patch->table_name());

    FormBuilder builder(it == db->end() ? nullptr : it->second);
    Error rs;
    for (auto spec : patch->specs_) {
        switch (spec->kind()) {
            case FormSpecPatch::kCreatTable:
                rs = BuildNewForm(db, patch, spec.get(), &builder);
                break;
                
            case FormSpecPatch::kAddColumn:
                rs = BuildAddColumn(db, patch, spec.get(), &builder);
                break;
                
            case FormSpecPatch::kAddIndex:
                // TODO:
                break;
                
            case FormSpecPatch::kChangeColumn:
                rs = builder.UpdateColumn(spec->GetName(),
                                          spec->GetColumn().data);
                break;
                
            case FormSpecPatch::kDropIndex:
                // TODO:
                break;
                
            case FormSpecPatch::kDropPrimaryKey:
                // TODO:
                break;
                
            case FormSpecPatch::kDropColumn:
                // TODO:
                break;
                
            case FormSpecPatch::kRenameIndex:
                // TODO:
                break;
                
            case FormSpecPatch::kRenameColumn:
                // TODO:
                break;
                
            case FormSpecPatch::kRenameTable:
                builder.set_table_name(spec->GetNewName());
                break;
                
            default:
                break;
        }
    }
    if (!rs) {
        return rs;
    }
    
    if (!log_file_) {
        rs = CreateMetaFile();
        if (!rs) {
            return rs;
        }
    }

    builder.SetOlder(it->second);
    
    patch->set_next_file_number(next_file_number_);
    std::unique_ptr<Form> form(builder.Build());
    rs = LogAndSnync(form.get(), patch);
    if (!rs) {
        return rs;
    }
    // TODO:

    form->AddRef();
    if (form->table_name() != patch->table_name()) {
        // Renamed:
        db->erase(patch->table_name());
    }
    db->insert({patch->table_name(), form.release()});
    
    return Error::OK();
}

Error FormSchemaSet::NewDatabase(const std::string &name) {
    auto iter = dbs_.find(name);
    if (iter != dbs_.end()) {
        return MAI_CORRUPTION("Database has exists. " + name);
    }
    std::string dir(abs_data_dir_);
    dir.append("/").append(name);
    Error rs = env_->MakeDirectory(dir, true);
    if (!rs) {
        return rs;
    }

    // TODO:
    
    dbs_.insert({name, {}});
    return Error::OK();
}
    
Error FormSchemaSet::BuildNewForm(TableSlot *db, FormSchemaPatch *patch,
                                  FormSpecPatch *spec,
                                  FormBuilder *builder) {
    auto iter = db->find(patch->table_name());
    if (iter != db->end()) {
        return MAI_CORRUPTION("Duplicated table name: " +
                              patch->table_name());
    }

    for (auto column : spec->GetColumns()) {
        Error rs = builder->AddColumn(column);
        if (!rs) {
            return rs;
        }
    }
    db->insert({patch->table_name(), nullptr});
    return Error::OK();
}
    
Error FormSchemaSet::BuildAddColumn(TableSlot *db, FormSchemaPatch *patch,
                                    FormSpecPatch *spec,
                                    FormBuilder *builder) {
    auto iter = db->find(patch->table_name());
    if (iter == db->end()) {
        return MAI_CORRUPTION("Table: " + patch->table_name() +
                              " not found!");
    }
    if (builder->table_name().empty()) {
        builder->set_table_name(patch->table_name());
        builder->set_engine_name(patch->engine_name());
    }

    Error rs;
    if (spec->GetColumns().empty()) {
        const auto &boundle = spec->GetColumn();
        if (boundle.after && boundle.hint.empty()) {
            rs = builder->AddColumn(boundle.data);
        } else {
            rs = builder->InsertColumn(boundle.data, boundle.after,
                                       boundle.hint);
        }
    } else {
        for (auto col : spec->GetColumns()) {
            rs = builder->AddColumn(col);
            if (!rs) {
                break;
            }
        }
    }
    return rs;
}
    
Error FormSchemaSet::CreateMetaFile() {
    meta_file_number_ = GenerateFileNumber();
    std::string path(abs_meta_dir_);
    path
        .append("/")
        .append(base::Slice::Sprintf("META-%06" PRIu64 , meta_file_number_));
    
    Error rs = env_->NewWritableFile(path, true, &log_file_);
    if (!rs) {
        return rs;
    }
    logger_.reset(new db::LogWriter(log_file_.get(),
                                    db::WAL::kDefaultBlockSize));
    return WriteSnapshot();
}
    
Error FormSchemaSet::WriteSnapshot() {
    std::string buf;
    for (const auto &db_pair : dbs_) {
        for (const auto &table_pair : db_pair.second) {
            FormSchemaPatch patch(db_pair.first, table_pair.first);
            patch.set_version(table_pair.second->version());
            patch.set_engine_name(table_pair.second->engine_name());
            patch.set_next_file_number(next_file_number_);
            patch.PartialEncode(true, &buf);
        }
    }
    Error rs = logger_->Append(buf);
    if (!rs) {
        return rs;
    }

    std::string file_name(abs_meta_dir_);
    file_name.append("/").append("CURRENT");
    
    std::string content(base::Slice::Sprintf("%" PRIu64, meta_file_number_));
    return base::FileWriter::WriteAll(file_name, content, env_);
}

Error FormSchemaSet::LogAndSnync(const Form *form, FormSchemaPatch *patch) {
    
    return MAI_NOT_SUPPORTED("TODO:");
}

} // namespace sql
    
} // namespace mai
