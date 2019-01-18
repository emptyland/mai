#ifndef MAI_SQL_FORM_SCHEMA_SET_H_
#define MAI_SQL_FORM_SCHEMA_SET_H_

#include "sql/types.h"
#include "base/base.h"
#include "mai/error.h"
#include "glog/logging.h"
#include <atomic>
#include <unordered_map>
#include <map>

namespace mai {
class Env;
class WritableFile;
class SequentialFile;
namespace db {
class LogWriter;
} // namespace db
namespace sql {

class FormBuilder;
class FormSchemaSet;
    
struct FormColumn {
    std::string name;
    SQLType type;
    int fixed_size = 0;
    int float_size = 0;
    bool not_null = false;
    bool auto_increment = false;
    SQLKeyType key = SQL_NOT_KEY;
    std::string default_val;
    std::string comment;
}; // struct FormColumn
    
    
struct FormIndex {
    std::string name;
    SQLKeyType  key;
    std::vector<std::shared_ptr<FormColumn>> ref_columns;
};
    
    
class Form final {
public:
    using Column = FormColumn;
    using Index  = FormIndex;
    
    DEF_VAL_GETTER(int, version);
    DEF_VAL_GETTER(std::string, table_name);
    DEF_VAL_GETTER(std::string, engine_name);
    
    // TODO:
    Column *FindColumnOrNull(const std::string &name) const {
        auto iter = column_names_.find(name);
        if (iter == column_names_.end()) {
            return nullptr;
        }
        DCHECK_LT(iter->second, columns_.size());
        DCHECK_EQ(name, columns_[iter->second]->name);
        return columns_[iter->second].get();
    }
    
    Index *FindIndexOrNull(const std::string &name) const {
        auto iter = index_names_.find(name);
        if (iter == index_names_.end()) {
            return nullptr;
        }
        DCHECK_LT(iter->second, columns_.size());
        DCHECK_EQ(name, indices_[iter->second]->name);
        return indices_[iter->second].get();
    }
    
    Column *GetPrimaryKey() const {
        return primary_key_ < 0 ? nullptr : columns_[primary_key_].get();
    }
    
    void AddRef() const { refs_.fetch_add(1); }
    
    int ReleaseRef() const {
        int refs = refs_.fetch_sub(1);
        DCHECK_GT(refs, 0);
        return refs;
    }
    
    int refs() const { return refs_.load(); }
    
    friend class FormBuilder;
    friend class FormSchemaSet;
    DISALLOW_IMPLICIT_CONSTRUCTORS(Form);
private:
    Form(const std::string &table_name, const std::string &engine_name)
        : refs_(0)
        , table_name_(table_name)
        , engine_name_(engine_name) {}
    
    mutable std::atomic<int> refs_;
    std::string const table_name_;
    std::string const engine_name_;
    std::vector<std::shared_ptr<Column>> columns_;
    std::unordered_map<std::string, size_t> column_names_;
    std::vector<std::shared_ptr<Index>>  indices_;
    std::unordered_map<std::string, size_t> index_names_;
    int primary_key_ = -1;
    int version_ = 0;
    Form *older_ = nullptr;
}; // class Form
    
    
struct FormColumnPatch {
    std::shared_ptr<FormColumn> data;
    bool after;
    std::string hint;
}; // struct FormColumnPatch
    
    
class FormSpecPatch {
public:
    enum Kind {
        kCreatTable,
        kAddColumn,
        kAddIndex,
        kChangeColumn,
        kDropIndex,
        kDropPrimaryKey,
        kDropColumn,
        kRenameIndex,
        kRenameColumn,
        kRenameTable,
    };
    
    FormSpecPatch(Kind kind) : kind_(kind) {}
    
    DEF_VAL_GETTER(Kind, kind);
    
    void AddColumn(const FormColumn &col) {
        DCHECK_EQ(kCreatTable, kind_);
        DCHECK_EQ(kAddColumn, kind_);
        columns_.emplace_back(new FormColumn(col));
    }
    
    void AddIndex(const std::string &name, SQLKeyType key,
                  const std::string &column_name) {
        DCHECK_EQ(kAddIndex, kind_);
        name_ = name;
        key_  = key;
        column_names_.push_back(column_name);
    }
    
    void SetColumn(const std::string &name, const FormColumn &new_def,
                   bool after, std::string &hint) {
        DCHECK_EQ(kAddColumn, kind_);
        DCHECK_EQ(kChangeColumn, kind_);
        name_ = name;
        column_.after = after;
        column_.data.reset(new FormColumn(new_def));
    }
    
    void Rename(const std::string &old_name, const std::string &new_name) {
        DCHECK_EQ(kRenameIndex, kind_);
        DCHECK_EQ(kRenameColumn, kind_);
        DCHECK_EQ(kRenameTable, kind_);
        name_     = old_name;
        new_name_ = new_name;
    }
    
    void SetName(const std::string &name) {
        DCHECK_EQ(kDropIndex, kind_);
        DCHECK_EQ(kDropColumn, kind_);
        name_ = name;
    }
    
    const std::vector<std::shared_ptr<FormColumn>> &GetColumns() const {
        DCHECK_EQ(kCreatTable, kind_);
        DCHECK_EQ(kAddColumn, kind_);
        return columns_;
    }
    
    const std::vector<std::string> &GetColumnNames() const {
        DCHECK_EQ(kAddIndex, kind_);
        return column_names_;
    }
    
    const FormColumnPatch &GetColumn() const {
        DCHECK_EQ(kAddColumn, kind_);
        DCHECK_EQ(kChangeColumn, kind_);
        return column_;
    }
    
    const std::string &GetName() const {
        DCHECK_EQ(kAddIndex, kind_);
        DCHECK_EQ(kAddColumn, kind_);
        DCHECK_EQ(kChangeColumn, kind_);
        DCHECK_EQ(kRenameIndex, kind_);
        DCHECK_EQ(kRenameColumn, kind_);
        DCHECK_EQ(kRenameTable, kind_);
        return name_;
    }
    
    const std::string &GetNewName() const {
        DCHECK_EQ(kRenameIndex, kind_);
        DCHECK_EQ(kRenameColumn, kind_);
        DCHECK_EQ(kRenameTable, kind_);
        return new_name_;
    }
    
    SQLKeyType GetKey() const {
        DCHECK_EQ(kAddIndex, kind_);
        return key_;
    }
    
    DISALLOW_IMPLICIT_CONSTRUCTORS(FormSpecPatch);
private:
    Kind kind_;
    std::vector<std::shared_ptr<FormColumn>> columns_;
    std::vector<std::string> column_names_;
    FormColumnPatch column_;
    std::string name_;
    std::string new_name_;
    SQLKeyType key_;
}; // class FormSpecPatch
    
    
class FormSchemaPatch final {
public:
    FormSchemaPatch(const std::string &db_name,
                    const std::string &table_name)
        : db_name_(db_name)
        , table_name_(table_name) {}

    DEF_VAL_GETTER(std::string, db_name);
    DEF_VAL_GETTER(std::string, table_name);
    DEF_VAL_PROP_RW(std::string, engine_name);
    DEF_VAL_PROP_RW(uint32_t, version);
    DEF_VAL_PROP_RW(uint64_t, next_file_number);

    void AddSpec(FormSpecPatch *spec) { specs_.emplace_back(spec); }
    
    void PartialEncode(bool add, std::string *buf) const;
    
    friend class FormSchemaSet;
    DISALLOW_IMPLICIT_CONSTRUCTORS(FormSchemaPatch);
private:
    std::string db_name_;
    std::string table_name_;
    std::string engine_name_;
    
    uint32_t version_ = 0;
    uint64_t next_file_number_ = 0;
    
    std::vector<std::shared_ptr<FormSpecPatch>> specs_;
}; // class FormSchemaPatch
    
    
class FormSchemaSet final {
public:
    FormSchemaSet(const std::string &abs_meta_dir,
                  const std::string &abs_data_dir, Env *env);
    ~FormSchemaSet();
    
    DEF_VAL_GETTER(uint64_t, next_file_number);
    
    uint64_t GenerateFileNumber() { return next_file_number_++; }
    
    void ReuseFileNumber(uint64_t fn) {
        if (fn + 1 == next_file_number_) {
            next_file_number_ = fn;
        }
    }

    Error LogAndApply(FormSchemaPatch *patch);
    
    Error NewDatabase(const std::string &name);
    
    using TableSlot = std::unordered_map<std::string, Form *>;
private:
    Error BuildNewForm(TableSlot *db, FormSchemaPatch *patch,
                       FormSpecPatch *spec,
                       FormBuilder *builder);
    Error BuildAddColumn(TableSlot *db, FormSchemaPatch *patch,
                         FormSpecPatch *spec,
                         FormBuilder *builder);
    
    Error CreateMetaFile();
    Error WriteSnapshot();
    Error LogAndSnync(const Form *form, FormSchemaPatch *patch);
    
    std::string const abs_meta_dir_;
    std::string const abs_data_dir_;
    Env *const env_;
    uint64_t meta_file_number_ = 0;
    uint64_t next_file_number_ = 0;
    std::map<std::string, TableSlot> dbs_;
    std::unique_ptr<WritableFile> log_file_;
    std::unique_ptr<db::LogWriter> logger_;
}; // class FormSchemaSet
    
    
} // namespace sql
    
} // namespace mai

#endif // MAI_SQL_FORM_SCHEMA_SET_H_
