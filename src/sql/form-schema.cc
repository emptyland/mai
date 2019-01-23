#include "sql/form-schema.h"
#include "sql/ast.h"
#include "sql/config.h"
#include "sql/parser.h"
#include "db/write-ahead-log.h"
#include "base/standalone-arena.h"
#include "base/hash.h"
#include "base/slice.h"
#include "base/io-utils.h"
#include "mai/env.h"

namespace mai {
    
namespace sql {
    
using ::mai::base::Slice;
using ::mai::base::BufferReader;
    
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
        column_assoc_index_ = form->column_assoc_index_;
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
    
    Error RemoveColumn(const std::string &name) {
        auto col_iter = column_names_.find(name);
        if (col_iter == column_names_.end()) {
            return MAI_CORRUPTION("Column name: " + name + " not found");
        }
        columns_.erase(columns_.begin() + col_iter->second);
        
        RefreshNames(false);
        RefreshAssocIndices();
        return Error::OK();
    }
    
    Error RenameColumn(const std::string &name, const std::string &new_name) {
        auto col_iter = column_names_.find(name);
        if (col_iter == column_names_.end()) {
            return MAI_CORRUPTION("Column name: " + name + " not found");
        }
        
        auto new_iter = column_names_.find(new_name);
        if (new_iter != column_names_.end() && new_iter != col_iter) {
            return MAI_CORRUPTION("Duplicated column name: " + new_name);
        }
        
        auto col_pos = col_iter->second;
        column_names_.erase(col_iter);
        column_names_.insert({new_name, col_pos});
        columns_[col_iter->second]->name = new_name;
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

        if (oldcol->key == SQL_NOT_KEY) {
            if (column->key != SQL_NOT_KEY) { // Add index
                AddIndexIfNeeded(col_iter->second, column);
                RefreshAssocIndices();
            }
        } else {
            auto idx_pos = column_assoc_index_[col_iter->second];
            DCHECK_NE(-1, idx_pos);
            if (column->key == SQL_NOT_KEY) { // Drop index
                indices_.erase(indices_.begin() + idx_pos);
                RefreshAssocIndices();
            } else { // Alter index
                indices_[idx_pos]->key = column->key;
            }
        }
        
        RefreshNames(false);
        return Error::OK();
    }
    
    Error AddIndex(const std::string &name, SQLKeyType key,
                   const std::vector<std::string> &columns) {
        auto idx_iter = index_names_.find(name);
        if (idx_iter != index_names_.end()) {
            return MAI_CORRUPTION("Duplicated index name: " + name);
        }
        
        std::vector<std::shared_ptr<Column>> cols;
        for (const auto &col_name : columns) {
            auto col_iter = column_names_.find(col_name);
            if (col_iter == column_names_.end()) {
                return MAI_CORRUPTION("Column name: " + col_name + " not found");
            }
            auto col = columns_[col_iter->second];
            if (col->key != key || col->key == SQL_NOT_KEY) {
                return MAI_CORRUPTION("Invalid column key.");
            }
            col->key = key;
            cols.push_back(col);
        }
        
        std::shared_ptr<Index> idx(new Index);
        idx->key = key;
        idx->name = name;
        idx->ref_columns = std::move(cols);
        
        auto pos = indices_.size();
        indices_.push_back(idx);
        index_names_.insert({name, pos});
        
        RefreshAssocIndices();
        return Error::OK();
    }
    
    Error RenameIndex(const std::string &name, const std::string &new_name) {
        auto idx_iter = index_names_.find(name);
        if (idx_iter == index_names_.end()) {
            return MAI_CORRUPTION("Index name: " + name + " not found");
        }

        auto new_iter = index_names_.find(new_name);
        if (new_iter != index_names_.end() && new_iter != idx_iter) {
            return MAI_CORRUPTION("Duplicated index name: " + new_name);
        }
        
        auto idx_pos = idx_iter->second;
        index_names_.erase(idx_iter);
        index_names_.insert({new_name, idx_pos});
        indices_[idx_iter->second]->name = new_name;
        return Error::OK();
    }
    
    Error RemoveIndex(const std::string &name) {
        auto idx_iter = index_names_.find(name);
        if (idx_iter == index_names_.end()) {
            return MAI_CORRUPTION("Index name: " + name + " not found.");
        }
        
        auto idx = indices_[idx_iter->second];
        for (auto col : idx->ref_columns) {
            col->key = SQL_NOT_KEY;
        }
        
        indices_.erase(indices_.begin() + idx_iter->second);
        RefreshNames(false);
        RefreshAssocIndices();
        return Error::OK();
    }
    
    Error RemovePrimaryKey() {
        if (primary_key_ < 0) {
            return MAI_CORRUPTION("No primary key.");
        }
        columns_[primary_key_]->key = SQL_NOT_KEY;
        
        for (size_t i = 0; i < indices_.size(); ++i) {
            if (indices_[i]->key == SQL_PRIMARY_KEY) {
                indices_.erase(indices_.begin() + i);
            }
        }
        RefreshNames(false);
        RefreshAssocIndices();
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
        if (older) {
            version_ = older->version_ + 1;
            older_   = older;
        }
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
        form->column_assoc_index_ = std::move(column_assoc_index_);
        return form;
    }

private:
    void AddIndexIfNeeded(size_t col_pos, std::shared_ptr<Column> column) {
        if (column->key == SQL_NOT_KEY) {
            return;
        }
        if (column->key == SQL_PRIMARY_KEY) {
            primary_key_ = static_cast<int>(col_pos);
        }
        std::shared_ptr<Index> idx(new Index);
        idx->key = column->key;
        idx->name = column->name;
        idx->ref_columns.push_back(column);
        
        auto idx_pos = indices_.size();
        indices_.push_back(idx);
        index_names_.insert({column->name, idx_pos});
        //column_assoc_index_[col_pos] = idx_pos;
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
    
    void RefreshAssocIndices() {
        column_assoc_index_.resize(columns_.size(), -1);
        
        for (size_t i = 0; i < column_assoc_index_.size(); ++i) {
            auto idx = indices_[i];
            
            for (auto col : idx->ref_columns) {
                auto col_iter = column_names_.find(col->name);
                DCHECK(col_iter != column_names_.end());

                column_assoc_index_[col_iter->second] = i;
            }
        }
    }
    
    std::string table_name_;
    std::string engine_name_;
    std::vector<std::shared_ptr<Column>> columns_;
    std::unordered_map<std::string, size_t> column_names_;
    std::vector<std::shared_ptr<Index>>  indices_;
    std::unordered_map<std::string, size_t> index_names_;
    std::vector<size_t> column_assoc_index_;
    int primary_key_ = -1;
    int version_ = 0;
    Form *older_ = nullptr;
}; // class FormBuilder
    

////////////////////////////////////////////////////////////////////////////////
/// class Form
////////////////////////////////////////////////////////////////////////////////
    
void Form::ToCreateTable(std::string *buf) const {
    buf->append("CREATE TABLE `").append(table_name()).append("` (\n");
    
    bool first = true;
    for (auto col : columns_) {
        if (!first) {
            buf->append(",\n");
        }

        const auto &desc = kSQLTypeDesc[col->type];
        buf->append("    `").append(col->name).append("` ").append(desc.name)
            .append(" ");
        switch (desc.size_kind) {
        case 0:
            buf->append(" ");
            break;
        case 1:
            if (col->fixed_size > 0) {
                buf->append(Slice::Sprintf("(%d) ", col->fixed_size));
            }
            break;
        case 2:
            if (col->fixed_size > 0 || col->float_size > 0) {
                buf->append(Slice::Sprintf("(%d, %d) ", col->fixed_size,
                                           col->float_size));
            }
            break;
        default:
            break;
        }
        if (col->not_null) {
            buf->append("NOT NULL ");
        } else {
            buf->append("NULL ");
        }
        if (col->auto_increment) {
            buf->append("AUTO_INCREMENT ");
        }
        if (!col->default_val.empty()) {
            buf->append("\'").append(col->default_val).append("\' ");
        }
        if (col->key != SQL_NOT_KEY) {
            buf->append(kSQLKeyText[col->key]).append(" ");
        }
        if (!col->comment.empty()) {
            buf->append("COMMENT \"").append(col->comment).append("\"");
        }
        first = false;
    }
    
    //buf->append(") ENGINE \"").append(engine_name_).append("\";\n");
    buf->append(");\n");
}


////////////////////////////////////////////////////////////////////////////////
/// class FormSchemaPatch
////////////////////////////////////////////////////////////////////////////////
    
void FormSchemaPatch::PartialEncode(const std::string &new_name,
                                    std::string *buf) const {
    Slice::WriteString(buf, db_name_);
    Slice::WriteString(buf, table_name_);
    Slice::WriteString(buf, new_name);
    Slice::WriteVarint32(buf, version_);
    Slice::WriteString(buf, engine_name_);
}

void FormSchemaPatch::PartialDecode(std::string_view buf,
                                    std::string *new_name) {
    BufferReader rd(buf);
    PartialDecode(&rd, new_name);
}
    
void FormSchemaPatch::PartialDecode(base::BufferReader *rd,
                                    std::string *new_name) {
    db_name_ = rd->ReadString();
    table_name_ = rd->ReadString();
    *new_name = rd->ReadString();
    version_ = rd->ReadVarint32();
    engine_name_ = rd->ReadString();
}

////////////////////////////////////////////////////////////////////////////////
/// class MetadataPatch
////////////////////////////////////////////////////////////////////////////////
    
void MetadataPatch::Encode(std::string *buf) const {
    Slice::WriteVarint32(buf, next_index_id_);
    Slice::WriteVarint32(buf, next_table_id_);
    Slice::WriteVarint64(buf, next_file_number_);
}

void MetadataPatch::Decode(std::string_view buf) {
    BufferReader rd(buf);
    Decode(&rd);
}

void MetadataPatch::Decode(base::BufferReader *rd) {
    next_index_id_ = rd->ReadVarint32();
    next_table_id_ = rd->ReadVarint32();
    next_file_number_ = rd->ReadVarint64();
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
        for (const auto &inner : outter.second.tables) {
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
    
Error FormSchemaSet::Recovery(uint64_t file_number) {
    std::string file_name(Files::MetaFileName(abs_meta_dir_, file_number));
    std::unique_ptr<SequentialFile> file;
    Error rs = env_->NewSequentialFile(file_name, &file);
    if (!rs) {
        return rs;
    }
    
    std::map<std::string, DbSlot> dbs;
    
    db::LogReader logger(file.get(), true, db::WAL::kDefaultBlockSize);
    std::string_view buf;
    std::string scratch;
    while (logger.Read(&buf, &scratch)) {
        rs = ReadBuffer(buf, &dbs);
        if (!rs) {
            return rs;
        }
    }
    if (!logger.error().ok() && !logger.error().IsEof()) {
        return logger.error();
    }
    rs = ReadForms(&dbs);
    if (!rs) {
        return rs;
    }

    meta_file_number_ = file_number;
    dbs_ = std::move(dbs);
    return Error::OK();
}
    
Error FormSchemaSet::ReadForms(std::map<std::string, DbSlot> *dbs) {
    Error rs;
    for (auto &outter : *dbs) {
        const std::string db_name(outter.first);
        for (auto &inner : outter.second.tables) {
            const std::string
            file_name(Files::FrmFileName(abs_meta_dir_, db_name, inner.first));
            
            std::string buf;
            rs = base::FileReader::ReadAll(file_name, &buf, env_);
            if (!rs) {
                return rs;
            }
            
            BufferReader rd(buf); // crc32 checksum;
            uint32_t check = rd.ReadFixed32();
            if (check != base::Hash::Crc32(buf.data() + 4, buf.size() - 4)) {
                return MAI_IO_ERROR("CRC32 checksum fail!");
            }
            
            const std::string tdb_name(rd.ReadString());
            if (tdb_name != db_name) {
                return MAI_IO_ERROR("Incorrect data, different db_name.");
            }
            
            const uint32_t version = rd.ReadVarint32();
            if (version != outter.second.version) {
                return MAI_IO_ERROR("Incorrect data, different version.");
            }
            
            auto sql = rd.Remaining();
            base::StandaloneArena arena(env_->GetLowLevelAllocator());
            Parser::Result result;
            rs = Parser::Parse(sql.data(), sql.size(), &arena, &result);
            if (!rs) {
                return MAI_IO_ERROR("Incorrect table schema. " +
                                    result.FormatError() + "\n" + std::string(sql));
            }
            
            auto ast = CreateTable::Cast(result.block->stmt(0));
            auto form = Ast2Form(inner.first, outter.second.engine_name,
                                 DCHECK_NOTNULL(ast));
            form->version_ = version;
            form->AddRef();
            inner.second = form;
        }
    }
    return rs;
}
    
Error FormSchemaSet::ReadBuffer(std::string_view buf,
                                std::map<std::string, DbSlot> *dbs) {
    BufferReader rd(buf);
    
    while (!rd.Eof()) {
        switch (static_cast<RecoredKind>(rd.ReadByte())) {
            case kUpdateDatabase: {
                std::string name(rd.ReadString());
                std::string engine_name(rd.ReadString());
                StorageKind kind = static_cast<StorageKind>(rd.ReadFixed32());
                
                auto iter = dbs->find(name);
                if (iter == dbs->end()) {
                    dbs->insert({name, DbSlot(engine_name, kind)});
                } else {
                    iter->second.engine_name = std::move(engine_name);
                }
            } break;

            case kDropDatabase: {
                std::string name(rd.ReadString());
                dbs->erase(name);
            } break;

            case kUpdateTable:{
                FormSchemaPatch patch;
                std::string new_name;
                patch.PartialDecode(&rd, &new_name);
                auto iter = dbs->find(patch.db_name());
                if (iter == dbs->end()) {
                    return MAI_IO_ERROR("Incorrect data, db name not found!");
                }
                if (patch.engine_name() != iter->second.engine_name) {
                    return MAI_IO_ERROR("Incorrect data, different engine name.");
                }
                
                auto db = &iter->second.tables;
                auto it = db->find(patch.table_name());
                if (new_name != patch.table_name()) {
                    if (it == db->end()) {
                        return MAI_IO_ERROR("Incorrect data, table name not found!");
                    }
                    db->erase(it);
                }
                db->insert({new_name, nullptr});
                iter->second.version = patch.version();
            } break;

            case kDropTable: {
                std::string db_name(rd.ReadString());
                std::string table_name(rd.ReadString());
                
                auto iter = dbs->find(db_name);
                if (iter == dbs->end()) {
                    return MAI_IO_ERROR("Incorrect data, db name not found!");
                }
                auto it = iter->second.tables.find(table_name);
                if (it == iter->second.tables.end()) {
                    return MAI_IO_ERROR("Incorrect data, table name not found!");
                }
                iter->second.tables.erase(it);
            } break;

            case kUpdateMetadata: {
                MetadataPatch patch;
                patch.Decode(&rd);
                
                next_file_number_ = patch.next_file_number();
                patch.next_index_id(); // TODO:
                patch.next_table_id(); // TODO:
            } break;
                
            default:
                return MAI_IO_ERROR("Incorrect data, unexpected record tag.");
        }
    }
    
    return Error::OK();
}

Error FormSchemaSet::LogAndApply(FormSchemaPatch *patch) {
    auto iter = dbs_.find(patch->db_name());
    if (iter == dbs_.end()) {
        return MAI_CORRUPTION("Database: " + patch->db_name() + " not found!");
    }
    auto db = &iter->second.tables;
    auto it = db->find(patch->table_name());

    FormBuilder builder(it == db->end() ? nullptr : it->second);
    Error rs;
    for (auto spec : patch->specs_) {
        switch (spec->kind()) {
            case FormSpecPatch::kCreatTable:
                rs = BuildNewForm(&iter->second, patch, spec.get(), &builder);
                break;
                
            case FormSpecPatch::kAddColumn:
                rs = BuildAddColumn(&iter->second, patch, spec.get(), &builder);
                break;
                
            case FormSpecPatch::kAddIndex:
                rs = builder.AddIndex(spec->GetName(), spec->GetKey(),
                                      spec->GetColumnNames());
                break;
                
            case FormSpecPatch::kChangeColumn:
                rs = builder.UpdateColumn(spec->GetName(),
                                          spec->GetColumn().data);
                break;
                
            case FormSpecPatch::kDropIndex:
                rs = builder.RemoveIndex(spec->GetName());
                break;
                
            case FormSpecPatch::kDropPrimaryKey:
                rs = builder.RemovePrimaryKey();
                break;
                
            case FormSpecPatch::kDropColumn:
                rs = builder.RemoveColumn(spec->GetName());
                break;
                
            case FormSpecPatch::kRenameIndex:
                rs = builder.RenameIndex(spec->GetName(), spec->GetNewName());
                break;
                
            case FormSpecPatch::kRenameColumn:
                rs = builder.RenameColumn(spec->GetName(), spec->GetNewName());
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

    builder.SetOlder(it == db->end() ? nullptr : it->second);

    std::unique_ptr<Form> form(builder.Build());
    rs = LogForm(form.get(), patch);
    if (!rs) {
        return rs;
    }
    // TODO:

    form->AddRef();
    if (form->table_name() != patch->table_name()) {
        // Renamed:
        db->erase(patch->table_name());
    }
    iter->second.version = form->version();
    db->insert({patch->table_name(), form.release()});

    return Error::OK();
}
    
Error FormSchemaSet::LogAndApply(const std::string &db_name,
                                 const std::string &table_name,
                                 Form *form) {
    FormSchemaPatch patch(db_name, table_name);
    
    auto iter = dbs_.find(db_name);
    if (iter == dbs_.end()) {
        return MAI_CORRUPTION("Database: " + db_name + " not found!");
    }
    auto db = &iter->second.tables;
    
    patch.set_engine_name(iter->second.engine_name);
    patch.set_version(form->version());
    
    Error rs = LogForm(form, &patch);
    if (!rs) {
        return rs;
    }
    // TODO:
    
    form->AddRef();
    if (form->table_name() != table_name) {
        // Renamed:
        db->erase(table_name);
    }
    iter->second.version = form->version();
    db->insert({table_name, form});
    return Error::OK();
}
    
Error FormSchemaSet::BuildForm(const std::string &db_name,
                               const CreateTable *ast, Form **result) {
    auto table_name = ast->table_name()->name()->ToString();
    
    auto iter = dbs_.find(db_name);
    if (iter == dbs_.end()) {
        return MAI_CORRUPTION("Database: " + db_name + " not found!");
    }
    auto db = &iter->second.tables;
    auto it = db->find(table_name);
    
    FormBuilder builder(it == db->end() ? nullptr : it->second);
    builder.set_table_name(table_name);
    builder.set_engine_name(iter->second.engine_name);
    builder.SetOlder(it == db->end() ? nullptr : it->second);

    Error rs;
    for (auto col_ast : *ast) {
        std::shared_ptr<FormColumn> col(Ast2Column(col_ast));
        rs = builder.AddColumn(col);
        if (!rs) {
            return rs;
        }
    }
    *result = builder.Build();
    return Error::OK();
}

Error FormSchemaSet::NewDatabase(const std::string &name,
                                 const std::string &engine_name,
                                 StorageKind kind) {
    Error rs;
    auto iter = dbs_.find(name);
    if (iter != dbs_.end()) {
        return MAI_CORRUPTION("Database has exists. " + name);
    }
    
    std::string dir(abs_data_dir_);
    dir.append("/").append(name);
    rs = env_->MakeDirectory(dir, true);
    if (!rs) {
        return rs;
    }
    dir = abs_meta_dir_;
    dir.append("/").append(name);
    if (!env_->FileExists(dir)) {
        rs = env_->MakeDirectory(dir, true);
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

    std::string buf;
    buf.append(1, kUpdateDatabase);
    Slice::WriteString(&buf, name);
    Slice::WriteString(&buf, engine_name);
    Slice::WriteFixed32(&buf, kind);
    AppendMetadata(&buf);
    rs = EnsureWrite(buf);
    if (!rs) {
        return rs;
    }
    dbs_.insert({name, DbSlot(engine_name, kind)});
    return Error::OK();
}
    
Error FormSchemaSet::AcquireFormSchema(const std::string &db_name,
                                       const std::string &table_name,
                                       base::intrusive_ptr<Form> *result) const {
    auto iter = dbs_.find(db_name);
    if (iter == dbs_.end()) {
        return MAI_CORRUPTION("Database not found.");
    }
    
    auto it = iter->second.tables.find(table_name);
    if (it == iter->second.tables.end()) {
        return MAI_CORRUPTION("Table not found.");
    }

    result->reset(it->second);
    return Error::OK();
}
    
/*static*/ Form *FormSchemaSet::Ast2Form(const std::string &table_name,
                                         const std::string &engine_name,
                                         const CreateTable *ast) {
    FormBuilder builder(nullptr);
    builder.set_table_name(table_name);
    builder.set_engine_name(engine_name);
    DCHECK_EQ(table_name, ast->table_name()->ToString());
    if (!ast->engine_name()->empty()) {
        DCHECK_EQ(engine_name, ast->engine_name()->ToString());
    }
    
    for (auto col_ast : *ast) {
        std::shared_ptr<FormColumn> col(Ast2Column(col_ast));
        builder.AddColumn(col);
    }
    return builder.Build();
}
    
/*static*/ Form::Column *FormSchemaSet::Ast2Column(const ColumnDefinition *ast) {
    Form::Column *col = new FormColumn();
    col->name = ast->name()->ToString();
    col->type = ast->type()->code();
    col->fixed_size = ast->type()->fixed_size();
    col->float_size = ast->type()->float_size();
    col->auto_increment = ast->auto_increment();
    col->not_null = ast->is_not_null();
    col->key = ast->key();
    col->default_val = ""; // TODO:
    col->comment = ast->comment()->ToString();
    return col;
}
    
Error FormSchemaSet::BuildNewForm(DbSlot *db, FormSchemaPatch *patch,
                                  FormSpecPatch *spec,
                                  FormBuilder *builder) {
    auto iter = db->tables.find(patch->table_name());
    if (iter != db->tables.end()) {
        return MAI_CORRUPTION("Duplicated table name: " +
                              patch->table_name());
    }

    builder->set_table_name(patch->table_name());
    builder->set_engine_name(patch->engine_name());
    for (auto column : spec->GetColumns()) {
        Error rs = builder->AddColumn(column);
        if (!rs) {
            return rs;
        }
    }
    return Error::OK();
}
    
Error FormSchemaSet::BuildAddColumn(DbSlot *db, FormSchemaPatch *patch,
                                    FormSpecPatch *spec,
                                    FormBuilder *builder) {
    auto iter = db->tables.find(patch->table_name());
    if (iter == db->tables.end()) {
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
    std::string file_name(Files::MetaFileName(abs_meta_dir_, meta_file_number_));    
    Error rs = env_->NewWritableFile(file_name, true, &log_file_);
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
        buf.append(1, kUpdateDatabase);
        Slice::WriteString(&buf, db_pair.first);
        Slice::WriteString(&buf, db_pair.second.engine_name);

        for (const auto &table_pair : db_pair.second.tables) {
            FormSchemaPatch patch(db_pair.first, table_pair.first);
            patch.set_version(table_pair.second->version());
            patch.set_engine_name(table_pair.second->engine_name());
            
            buf.append(1, kUpdateTable);
            patch.PartialEncode(patch.table_name(), &buf);
        }
    }
    AppendMetadata(&buf);
    Error rs = EnsureWrite(buf);
    if (!rs) {
        return rs;
    }

    std::string file_name(Files::CurrentFileName(abs_meta_dir_));
    std::string content(base::Slice::Sprintf("%" PRIu64, meta_file_number_));
    return base::FileWriter::WriteAll(file_name, content, env_);
}

Error FormSchemaSet::LogForm(const Form *form, FormSchemaPatch *patch) {
    std::string buf;
    Slice::WriteFixed32(&buf, 0); // crc32 header;
    Slice::WriteString(&buf, patch->db_name());
    Slice::WriteVarint32(&buf, form->version());
    form->ToCreateTable(&buf);
    
    uint32_t crc32 = base::Hash::Crc32(buf.data() + 4, buf.size() - 4);
    *reinterpret_cast<uint32_t *>(&buf[0]) = crc32;
    
    std::string file_name = Files::FrmFileName(abs_meta_dir_, patch->db_name(),
                                               form->table_name());
    Error rs = base::FileWriter::WriteAll(file_name, buf, env_);
    if (!rs) {
        return rs;
    }
    if (form->table_name() != patch->table_name()) {
        file_name = Files::FrmFileName(abs_meta_dir_, patch->db_name(),
                                       patch->table_name());
        env_->DeleteFile(file_name, false);
    }
    
    buf.clear();
    buf.append(1, kUpdateTable);
    patch->PartialEncode(form->table_name(), &buf);
    AppendMetadata(&buf);
    return EnsureWrite(buf);
}
    
Error FormSchemaSet::EnsureWrite(std::string_view data) {
    Error rs =  logger_->Append(data);
    if (!rs) {
        return rs;
    }
    rs = log_file_->Flush();
    if (!rs) {
        return rs;
    }
    return log_file_->Sync();
}

} // namespace sql
    
} // namespace mai
