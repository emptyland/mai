#ifndef MAI_SQL_FORM_SCHEMA_SET_H_
#define MAI_SQL_FORM_SCHEMA_SET_H_

#include "sql/heap-tuple.h"
#include "sql/types.h"
#include "base/reference-count.h"
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
namespace base {
class BufferReader;
class Arena;
} // namespace base
namespace db {
class LogWriter;
} // namespace db
namespace sql {
namespace eval {
class Expression;
} // namespace eval
namespace ast {
class CreateTable;
class ColumnDefinition;
} // namespace ast


class FormBuilder;
class FormSchemaSet;
//class VirtualSchema;
    
struct Column {
    std::string name;
    SQLType type;
    uint32_t cid = 0;
    int m_size = 0;
    int d_size = 0;
    bool not_null = false;
    bool auto_increment = false;
    SQLKeyType key = SQL_NOT_KEY;
    eval::Expression *default_val;
    std::string comment;
}; // struct Column
    
    
struct Index {
    std::string name;
    SQLKeyType  key;
    uint32_t iid = 0;
    std::vector<std::shared_ptr<Column>> ref_columns;
};

class Form final {
public:
    using Column = Column;
    using Index  = Index;
    
    DEF_VAL_GETTER(int, version);
    DEF_VAL_GETTER(uint32_t, tid);
    DEF_VAL_GETTER(std::string, table_name);
    DEF_VAL_GETTER(std::string, engine_name);
    
    const VirtualSchema *virtual_schema() const {
        return virtual_schema_.get();
    }
    
    const Column *column(size_t i) const {
        DCHECK_LT(i, columns_.size());
        return columns_[i].get();
    }
    
    size_t columns_size() const { return columns_.size(); }
    
    const Index *index(size_t i) const {
        DCHECK_LT(i, indices_.size());
        return indices_[i].get();
    }

    size_t indices_size() const { return indices_.size(); }

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
    
    void ToCreateTable(std::string *buf) const;
    const VirtualSchema *ToVirtualSchema() const;
    
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
    std::vector<size_t> column_assoc_index_;
    int primary_key_ = -1;

    uint32_t tid_ = 0;
    int version_ = 0;
    Form *older_ = nullptr;
    std::unique_ptr<const VirtualSchema> virtual_schema_;
}; // class Form
    
    
struct FormColumnPatch {
    std::shared_ptr<Column> data;
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
    
    void AddColumn(const Column &col) {
        DCHECK(kCreatTable == kind_ ||
               kAddColumn == kind_);
        columns_.emplace_back(new Column(col));
    }
    
    void AddIndex(const std::string &name, SQLKeyType key,
                  const std::string &column_name) {
        DCHECK_EQ(kAddIndex, kind_);
        name_ = name;
        key_  = key;
        column_names_.push_back(column_name);
    }
    
    void SetColumn(const std::string &name, const Column &new_def,
                   bool after, std::string &hint) {
        DCHECK(kAddColumn == kind_ || kChangeColumn == kind_);
        name_ = name;
        column_.after = after;
        column_.data.reset(new Column(new_def));
    }
    
    void Rename(const std::string &old_name, const std::string &new_name) {
        DCHECK(kRenameIndex == kind_ || kRenameColumn == kind_ ||
               kRenameTable == kind_);
        name_     = old_name;
        new_name_ = new_name;
    }
    
    void SetName(const std::string &name) {
        DCHECK(kDropIndex == kind_ || kDropColumn == kind_);
        name_ = name;
    }
    
    const std::vector<std::shared_ptr<Column>> &GetColumns() const {
        DCHECK(kCreatTable == kind_|| kAddColumn == kind_);
        return columns_;
    }
    
    const std::vector<std::string> &GetColumnNames() const {
        DCHECK_EQ(kAddIndex, kind_);
        return column_names_;
    }
    
    const FormColumnPatch &GetColumn() const {
        DCHECK(kAddColumn == kind_ || kChangeColumn == kind_);
        return column_;
    }
    
    const std::string &GetName() const {
        DCHECK(kAddIndex == kind_ || kAddColumn == kind_ ||
               kChangeColumn == kind_ || kRenameIndex == kind_ ||
               kRenameColumn == kind_ || kRenameTable == kind_);
        return name_;
    }
    
    const std::string &GetNewName() const {
        DCHECK(kRenameIndex == kind_ || kRenameColumn == kind_ ||
               kRenameTable == kind_);
        return new_name_;
    }
    
    SQLKeyType GetKey() const { DCHECK_EQ(kAddIndex, kind_); return key_; }
    
    DISALLOW_IMPLICIT_CONSTRUCTORS(FormSpecPatch);
private:
    Kind kind_;
    std::vector<std::shared_ptr<Column>> columns_;
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
    //DEF_VAL_PROP_RW(uint64_t, next_file_number);

    void AddSpec(FormSpecPatch *spec) { specs_.emplace_back(spec); }
    
    void PartialEncode(const std::string &new_name, std::string *buf) const;
    void PartialDecode(std::string_view buf, std::string *new_name);
    
    friend class FormSchemaSet;
    DISALLOW_IMPLICIT_CONSTRUCTORS(FormSchemaPatch);
private:
    FormSchemaPatch() {}
    void PartialDecode(base::BufferReader *rd, std::string *new_name);
    
    std::string db_name_;
    std::string table_name_;
    std::string engine_name_;
    
    uint32_t version_ = 0;
    //uint64_t next_file_number_ = 0;
    
    std::vector<std::shared_ptr<FormSpecPatch>> specs_;
}; // class FormSchemaPatch
    
    
class MetadataPatch final {
public:
    MetadataPatch() {}
    
    DEF_VAL_PROP_RW(uint32_t, next_column_id);
    DEF_VAL_PROP_RW(uint32_t, next_table_id);
    DEF_VAL_PROP_RW(uint64_t, next_file_number);
    
    void Encode(std::string *buf) const;
    void Decode(std::string_view buf);
    
    friend class FormSchemaSet;
    DISALLOW_IMPLICIT_CONSTRUCTORS(MetadataPatch);
private:
    void Decode(base::BufferReader *rd);
    
    uint32_t next_column_id_ = 0;
    uint32_t next_table_id_ = 0;
    uint64_t next_file_number_ = 0;
}; // class MetadataPatch
    
    
class FormSchemaSet final {
public:
    FormSchemaSet(const std::string &abs_meta_dir,
                  const std::string &abs_data_dir, Env *env);
    ~FormSchemaSet();
    
    DEF_VAL_GETTER(uint64_t, meta_file_number);
    DEF_VAL_GETTER(uint64_t, next_file_number);
    DEF_VAL_GETTER(std::string, abs_meta_dir);
    DEF_VAL_GETTER(std::string, abs_data_dir);
    
    Error Recovery(uint64_t file_number);
    
    uint64_t GenerateFileNumber() { return next_file_number_++; }
    
    void ReuseFileNumber(uint64_t fn) {
        if (fn + 1 == next_file_number_) {
            next_file_number_ = fn;
        }
    }
    
    uint32_t GenerateColumnId() { return next_column_id_++; }
    
    void ReuseColumnId(uint32_t id) {
        if (id + 1 == next_column_id_) {
            next_column_id_ = id;
        }
    }
    
    uint32_t GenerateTableId() { return next_table_id_++; }
    
    void ReuseTableId(uint32_t id) {
        if (id + 1 == next_table_id_) {
            next_table_id_ = id;
        }
    }

    Error LogAndApply(FormSchemaPatch *patch);
    
    Error LogAndApply(const std::string &db_name, const std::string &table_name,
                      Form *form);
    
    Error NewDatabase(const std::string &name,
                      const std::string &engine_name,
                      StorageKind kind);
    
    Error GetDatabaseEngineName(const std::string &db_name,
                                std::string *engine_name) const {
        auto iter = dbs_.find(db_name);
        if (iter == dbs_.end()) {
            return MAI_CORRUPTION("Database not found.");
        }
        *engine_name = iter->second.engine_name;
        return Error::OK();
    }
    
    void
    GetDatabaseEngineName(std::map<std::string, std::string> *profiles) const {
        profiles->clear();
        for (const auto &pair : dbs_) {
            profiles->insert({pair.first, pair.second.engine_name});
        }
    }

    Error AcquireFormSchema(const std::string &db_name,
                            const std::string &table_name,
                            base::intrusive_ptr<Form> *result) const;
    
    struct DbSlot {
        DbSlot(const std::string &e, StorageKind k)
            : engine_name(e)
            , storage_kind(k) {}
        std::string engine_name;
        StorageKind storage_kind = SQL_ROW_STORE;
        uint32_t version = 0;
        std::unordered_map<std::string, Form *> tables;
    };
    
    const std::map<std::string, DbSlot> &GetDatabases() const { return dbs_; }
    
    Error BuildForm(const std::string &db_name, const ast::CreateTable *ast,
                    Form **result);
    
    Error Ast2Form(const std::string &table_name,
                   const std::string &engine_name,
                   const ast::CreateTable *ast, Form **result);
    Error Ast2Column(const ast::ColumnDefinition *ast, Form::Column **result);
private:
    enum RecoredKind : uint8_t {
        kUpdateDatabase,
        kDropDatabase,
        kUpdateTable,
        kDropTable,
        kUpdateMetadata,
    };
    
    Error ReadBuffer(std::string_view buf, std::map<std::string, DbSlot> *dbs);
    Error ReadForms(std::map<std::string, DbSlot> *dbs);
    
    Error BuildNewForm(DbSlot *db, FormSchemaPatch *patch,
                       FormSpecPatch *spec,
                       FormBuilder *builder);
    Error BuildAddColumn(DbSlot *db, FormSchemaPatch *patch,
                         FormSpecPatch *spec,
                         FormBuilder *builder);
    
    Error CreateMetaFile();
    Error WriteSnapshot();
    
    Error LogForm(const Form *form, FormSchemaPatch *patch);
    
    Error EnsureWrite(std::string_view data);
    
    void AppendMetadata(std::string *buf) const {
        buf->append(1, kUpdateMetadata);
        MetadataPatch patch;
        patch.set_next_column_id(next_column_id_);
        patch.set_next_table_id(next_table_id_);
        patch.set_next_file_number(next_file_number_);
        patch.Encode(buf);
    }
    
    std::string const abs_meta_dir_;
    std::string const abs_data_dir_;
    Env *const env_;
    base::Arena *arena_;
    uint64_t meta_file_number_ = 0;
    uint64_t next_file_number_ = 0;
    uint32_t next_column_id_ = 0;
    uint32_t next_table_id_ = 0;
    std::map<std::string, DbSlot> dbs_;
    std::unique_ptr<WritableFile> log_file_;
    std::unique_ptr<db::LogWriter> logger_;
}; // class FormSchemaSet
    
    
} // namespace sql
    
} // namespace mai

#endif // MAI_SQL_FORM_SCHEMA_SET_H_
