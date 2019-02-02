#include "sql/heap-tuple.h"

namespace mai {
    
namespace sql {
    
static size_t GetSize(const HeapRawData &data, SQLType type) {
    switch (type) {
        case SQL_BIGINT:
        case SQL_MEDIUMINT:
        case SQL_INT:
        case SQL_SMALLINT:
        case SQL_TINYINT:
            return sizeof(uint64_t);
            
        case SQL_FLOAT:
        case SQL_DOUBLE:
            return sizeof(double);
            
        case SQL_DATE:
            return sizeof(SQLDate);
        case SQL_TIME:
            return sizeof(SQLTime);
        case SQL_DATETIME:
            return sizeof(SQLDateTime);
            
        case SQL_CHAR:
        case SQL_VARCHAR:
        case SQL_BINARY:
        case SQL_VARBINARY:
            return sizeof(data.str);
            
            // TODO:
        case SQL_DECIMAL:
        case SQL_NUMERIC:
        default:
            DLOG(FATAL) << "noreached";
            break;
    }
    return 0;
}
    
bool ColumnDescriptor::CoverU64() const {
    switch (type()) {
        case SQL_BIGINT:
        case SQL_MEDIUMINT:
        case SQL_INT:
        case SQL_SMALLINT:
        case SQL_TINYINT:
            return is_unsigned();
            
        default:
            break;
    }
    return false;
}

bool ColumnDescriptor::CoverI64() const {
    switch (type()) {
        case SQL_BIGINT:
        case SQL_MEDIUMINT:
        case SQL_INT:
        case SQL_SMALLINT:
        case SQL_TINYINT:
            return !is_unsigned();
            
        default:
            break;
    }
    return false;
}
    
bool ColumnDescriptor::CoverF64() const {
    switch (type()) {
        case SQL_FLOAT:
        case SQL_DOUBLE:
            return true;

        default:
            break;
    }
    return false;
}

bool ColumnDescriptor::CoverDateTime() const {
    switch (type()) {
        case SQL_CHAR:
        case SQL_VARCHAR:
        case SQL_BINARY:
        case SQL_VARBINARY:
            return true;
            
        default:
            break;
    }
    return false;
}
    
bool ColumnDescriptor::CoverString() const {
    switch (type()) {
        case SQL_CHAR:
        case SQL_VARCHAR:
        case SQL_BINARY:
        case SQL_VARBINARY:
            return true;
            
        default:
            break;
    }
    return false;
}
    
SQLDateTime HeapTuple::GetDateTime(const ColumnDescriptor *cd) const {
    SQLDateTime dt = SQLDateTime::Zero();
    if (IsNull(cd) || !cd->CoverDateTime()) {
        return dt;
    }
    if (cd->type() == SQL_TIME) {
        dt.time = *address<SQLTime>(cd);
    } else if (cd->type() == SQL_DATE) {
        dt.date = *address<SQLDate>(cd);
    } else {
        DCHECK_EQ(SQL_DATETIME, cd->type());
        dt = *address<SQLDateTime>(cd);
    }
    return dt;
}
    
size_t HeapTuple::GetOffset(const ColumnDescriptor *cd) const {
    size_t offset = 0;
    for (int i = 0; i < cd->index(); ++i) {
        if (null_bitmap_[i / 32] & (1u << (i % 32))) {
            continue;
        }
        offset += GetSize(HeapRawData{}, cd->type());
    }
    return offset;
}
    
VirtualSchema *VirtualSchemaBuilder::Build() {
    if (error_.fail()) {
        return nullptr;
    }
    VirtualSchema *schema = new VirtualSchema(name_);
    schema->origin_table_ = origin_table_;
    schema->columns_ = std::move(columns_);
    schema->column_names_ = std::move(column_names_);
    return schema;
}
    
void VirtualSchemaBuilder::AddColumn(const InnerBuilder &inner) {
    if (error_.fail()) {
        return;
    }
    std::string full_name;
    if (inner.table_name_.empty()) {
        full_name = inner.name_;
    } else {
        full_name = inner.table_name_ + "." + inner.name_;
    }
    
    auto iter = column_names_.find(full_name);
    if (iter != column_names_.end()) {
        error_ = MAI_CORRUPTION("Duplicated column name: " + inner.name_);
        return;
    }
    
    ColumnDescriptor *cd = new ColumnDescriptor();
    cd->origin_ = inner.origin_;
    cd->origin_table_ = inner.origin_table_;
    cd->name_ = std::move(inner.name_);
    cd->table_name_ = std::move(inner.table_name_);
    cd->is_unsigned_ = inner.is_unsigned_;
    cd->type_ = inner.type_;
    cd->m_size_ = inner.m_size_;
    cd->d_size_ = inner.d_size_;
    cd->index_ = static_cast<int>(columns_.size());

    column_names_.insert({std::move(full_name), columns_.size()});
    columns_.push_back(cd);
}

HeapTupleBuilder::HeapTupleBuilder(const VirtualSchema *schema,
                                   base::Arena *arena)
    : schema_(schema)
    , null_bitmap_(nullptr)
    , arena_(arena)
    , unpacked_(arena_) {
    int64_t i = (schema_->columns_size() + 31) / 32;
    if (i > 0) {
        null_bitmap_.reset(new uint32_t[i]);
        while (i--) {
            null_bitmap_[i] = 0xffffffff;
        }
    }
    unpacked_.resize(schema->columns_size());
}
    
void HeapTupleBuilder::Reset() {
    int64_t i = (schema_->columns_size() + 31) / 32;
    while (i--) {
        null_bitmap_[i] = 0xffffffff;
    }
    unpacked_.clear();
    unpacked_.resize(schema_->columns_size());
}

HeapTuple *HeapTupleBuilder::Build() {
    DCHECK_GT(schema_->columns_size(), 0);
    
    size_t null_bitmap_size = (schema_->columns_size() + 31) / 32;
    size_t size = sizeof(HeapTuple) + (null_bitmap_size - 1) * sizeof(uint32_t);
    size_t data_size = 0;
    for (size_t i = 0; i < unpacked_.size(); ++i) {
        if (null_bitmap_[i / 32] & (1u << (i % 32))) {
            continue;
        }
        data_size += GetSize(unpacked_[i], schema_->column(i)->type());
    }
    size += data_size;
    
    DCHECK_GE(size, sizeof(HeapTuple));
    void *chunk = arena_->Allocate(size);
    
    HeapTuple *tuple = new (chunk) HeapTuple(schema_, null_bitmap_.get(),
                                             null_bitmap_size, data_size);
    size_t offset = 0;
    for (size_t i = 0; i < unpacked_.size(); ++i) {
        if (null_bitmap_[i / 32] & (1u << (i % 32))) {
            continue;
        }
        
        void *addr = tuple->address(offset);
        switch (schema_->column(i)->type()) {
            case SQL_BIGINT:
            case SQL_MEDIUMINT:
            case SQL_INT:
            case SQL_SMALLINT:
            case SQL_TINYINT:
                if (schema_->column(i)->is_unsigned()) {
                    *static_cast<uint64_t *>(addr) = unpacked_[i].u64;
                } else {
                    *static_cast<int64_t *>(addr)  = unpacked_[i].i64;
                }
                offset += sizeof(uint64_t);
                break;
                
            case SQL_FLOAT:
            case SQL_DOUBLE:
                *static_cast<double *>(tuple->address(offset))
                    = unpacked_[i].f64;
                offset += sizeof(double);
                break;
                
            case SQL_DATE:
                *static_cast<SQLDate *>(tuple->address(offset))
                    = unpacked_[i].dt.date;
                offset += sizeof(SQLDate);
                break;
            case SQL_TIME:
                *static_cast<SQLTime *>(tuple->address(offset))
                    = unpacked_[i].dt.time;
                offset += sizeof(SQLTime);
                break;
            case SQL_DATETIME:
                *static_cast<SQLDateTime *>(tuple->address(offset))
                    = unpacked_[i].dt;
                offset += sizeof(SQLDateTime);
                break;
                
            case SQL_CHAR:
            case SQL_VARCHAR:
            case SQL_BINARY:
            case SQL_VARBINARY:
                *static_cast<base::ArenaString **>(tuple->address(offset))
                    = unpacked_[i].str;
                offset += sizeof(unpacked_[i].str);
                break;
                
                // TODO:
            case SQL_DECIMAL:
            case SQL_NUMERIC:
            default:
                DLOG(FATAL) << "noreached";
                break;
        }
    }
    DCHECK_EQ(data_size, offset);
    return tuple;
}
    
} // namespace sql
    
} // namespace mai
