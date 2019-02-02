#ifndef MAI_SQL_HEAP_TUPLE_H_
#define MAI_SQL_HEAP_TUPLE_H_

#include "sql/types.h"
#include "my/types.h"
#include "base/arena-utils.h"
#include "base/arena.h"
#include "base/base.h"
#include "mai/error.h"
#include <string>
#include <vector>
#include <unordered_map>

namespace mai {
    
namespace sql {
    
struct FormColumn;
class Form;
class HeapTupleBuilder;
class VirtualSchemaBuilder;

class ColumnDescriptor final {
public:
    DEF_VAL_GETTER(int, index);
    DEF_PTR_GETTER(const Form, origin_table);
    DEF_PTR_GETTER(const FormColumn, origin);
    DEF_VAL_GETTER(std::string, table_name);
    DEF_VAL_GETTER(std::string, name);
    DEF_VAL_GETTER(SQLType, type);
    DEF_VAL_GETTER(bool, is_unsigned);
    DEF_VAL_GETTER(int, m_size);
    DEF_VAL_GETTER(int, d_size);
    
    bool CoverU64() const;
    bool CoverI64() const;
    bool CoverF64() const;
    bool CoverDateTime() const;
    bool CoverString() const;
    
    friend class HeapTupleBuilder;
    friend class VirtualSchemaBuilder;
    DISALLOW_IMPLICIT_CONSTRUCTORS(ColumnDescriptor);
private:
    ColumnDescriptor() {}
    
    int index_;
    const Form *origin_table_;
    const FormColumn *origin_;
    std::string table_name_;
    std::string name_;
    SQLType type_;
    bool is_unsigned_;
    int m_size_;
    int d_size_;
}; // class ColumnDesc
    
    
class VirtualSchema final {
public:
    using Builder = VirtualSchemaBuilder;
    
    ~VirtualSchema() {
        for (auto col : columns_) { delete col; }
    }
    
    DEF_VAL_GETTER(std::string, name);
    DEF_PTR_GETTER(const Form, origin_table);
    
    size_t columns_size() const { return columns_.size(); }

    const ColumnDescriptor *column(size_t i) const {
        DCHECK_LT(i, columns_size());
        return columns_[i];
    }
    
    const ColumnDescriptor *FindOrNull(const std::string &name) const {
        auto iter = column_names_.find(name);
        return iter == column_names_.end() ? nullptr : column(iter->second);
    }

    friend class VirtualSchemaBuilder;
    DISALLOW_IMPLICIT_CONSTRUCTORS(VirtualSchema);
private:
    VirtualSchema(const std::string &name) : name_(name) {}
    
    std::string name_;
    const Form *origin_table_;
    std::vector<ColumnDescriptor *> columns_;
    std::unordered_map<std::string, size_t> column_names_;
}; // class VirtualSchema


union HeapRawData {
    uint64_t    u64;
    int64_t     i64;
    double      f64;
    base::ArenaString *str;
    SQLDateTime dt;
};
    
//
class HeapTuple final {
public:
    DEF_PTR_GETTER_NOTNULL(const VirtualSchema, schema);
    DEF_VAL_GETTER(size_t, size);
    DEF_VAL_GETTER(size_t, null_bitmap_size);
    
    uint64_t GetU64(const ColumnDescriptor *cd) const {
        return IsNull(cd) || !cd->CoverU64() ? 0 : *address<uint64_t>(cd);
    }
    
    int64_t GetI64(const ColumnDescriptor *cd) const {
        return IsNull(cd) || !cd->CoverI64() ? 0 : *address<int64_t>(cd);
    }
    
    double GetF64(const ColumnDescriptor *cd) const {
        return IsNull(cd) || !cd->CoverF64() ? 0 : *address<double>(cd);
    }
    
    SQLDateTime GetDateTime(const ColumnDescriptor *cd) const;
    
    std::string_view GetSlice(const ColumnDescriptor *cd) const {
        return GetRawString(cd)->ToSlice();
    }

    std::string GetString(const ColumnDescriptor *cd) const {
        return GetRawString(cd)->ToString();
    }
    
    const base::ArenaString *GetRawString(const ColumnDescriptor *cd) const {
        return IsNull(cd) || !cd->CoverString()
               ? base::ArenaString::kEmpty : *address<base::ArenaString *>(cd);
    }
    
    bool IsNull(const ColumnDescriptor *cd) const {
        return null_bitmap_[cd->index() / 32] & (1u << (cd->index() % 32));
    }
    
    bool IsNotNull(const ColumnDescriptor *cd) const { return !IsNull(cd); }
    
    friend class HeapTupleBuilder;
private:
    HeapTuple(const VirtualSchema *schema, const uint32_t *null_bitmap,
              size_t n, size_t size)
        : schema_(schema)
        , size_(size)
        , null_bitmap_size_(n) {
        while (n--) {
            null_bitmap_[n] = null_bitmap[n];
        }
    }

    void *address(size_t offset) {
        DCHECK_LT(offset, size_);
        return reinterpret_cast<char *>(this + 1) +
               (null_bitmap_size_ - 1) * sizeof(uint32_t) + offset;
    }
    
    template<class T>
    const T *address(const ColumnDescriptor *cd) const {
        return static_cast<const T *>(address(GetOffset(cd)));
    }
    
    const void *address(size_t offset) const {
        DCHECK_LT(offset, size_);
        return reinterpret_cast<const char *>(this + 1) +
               (null_bitmap_size_ - 1) * sizeof(uint32_t) + offset;
    }
    
    size_t GetOffset(const ColumnDescriptor *cd) const;
    
    const VirtualSchema *const schema_;
    const size_t size_;
    const size_t null_bitmap_size_;
    uint32_t null_bitmap_[1];
}; // class HeapTuple
    
    
class VirtualSchemaBuilder final {
public:
    class InnerBuilder {
    public:
        InnerBuilder &origin_table(const Form *form) {
            origin_table_ = DCHECK_NOTNULL(form);
            return *this;
        }
        
        InnerBuilder &origin(const FormColumn *column) {
            origin_ = DCHECK_NOTNULL(column);
            return *this;
        }
        
        InnerBuilder &table_name(const std::string &nm) {
            table_name_ = nm;
            return *this;
        }
        
        InnerBuilder &name(const std::string &nm) {
            name_ = nm;
            return *this;
        }
        
        InnerBuilder &type(SQLType ty) {
            type_ = ty;
            return *this;
        }
        
        InnerBuilder &is_unsigned(bool un) {
            is_unsigned_ = un;
            return *this;
        }
        
        InnerBuilder &m_size(int m) {
            m_size_ = m;
            return *this;
        }
        
        InnerBuilder &d_size(int d) {
            d_size_ = d;
            return *this;
        }

        VirtualSchemaBuilder &EndColumn() {
            owns_->AddColumn(*this);
            return *owns_;
        }
        
        friend class VirtualSchemaBuilder;
    private:
        InnerBuilder(const std::string &name, SQLType type,
                     VirtualSchemaBuilder *owns)
            : name_(name)
            , type_(type)
            , owns_(owns) {}
        
        VirtualSchemaBuilder *owns_;
        std::string table_name_;
        std::string name_;
        SQLType type_;
        const Form *origin_table_ = nullptr;
        const FormColumn *origin_ = nullptr;
        bool is_unsigned_ = false;
        int m_size_ = 0;
        int d_size_ = 0;
    };
    
    VirtualSchemaBuilder(const std::string &name) : name_(name) {}
    ~VirtualSchemaBuilder() {
        for (auto col : columns_) { delete col; }
    }
    
    VirtualSchemaBuilder &origin_table(const Form *fm) {
        origin_table_ = DCHECK_NOTNULL(fm);
        return *this;
    }
    
    DEF_VAL_GETTER(Error, error);
    
    InnerBuilder &BeginColumn(const std::string &name, SQLType type) {
        inner_builder_.reset(new InnerBuilder(name, type, this));
        return *inner_builder_;
    }
    
    VirtualSchema *Build();
    
    DISALLOW_IMPLICIT_CONSTRUCTORS(VirtualSchemaBuilder);
private:
    void AddColumn(const InnerBuilder &inner);
    
    std::string name_;
    const Form *origin_table_ = nullptr;
    std::vector<ColumnDescriptor *> columns_;
    std::unordered_map<std::string, size_t> column_names_;
    std::unique_ptr<InnerBuilder> inner_builder_;
    Error error_;
}; // class VirtualSchemaBuilder
    
    
class HeapTupleBuilder final {
    using Binary = base::ArenaString;
    
public:
    HeapTupleBuilder(const VirtualSchema *schema, base::Arena *arena);
    ~HeapTupleBuilder() {}
    
    void SetU64(size_t i, uint64_t value) { PrepareSet(i)->u64 = value; }
    void SetI64(size_t i, int64_t value) { PrepareSet(i)->i64 = value; }
    void SetF64(size_t i, double value) { PrepareSet(i)->f64 = value; }
    void SetDate(size_t i, const SQLDate &value) {
        PrepareSet(i)->dt.date = value;
    }
    void SetTime(size_t i, const SQLTime &value) {
        PrepareSet(i)->dt.time = value;
    }
    void SetDateTime(size_t i, const SQLDateTime &value) {
        PrepareSet(i)->dt = value;
    }
    void SetString(size_t i, std::string_view value) {
        PrepareSet(i)->str = Binary::New(arena_, value.data(), value.size());
    }

    void SetNull(size_t i) {
        DCHECK_LT(i, unpacked_.size());
        null_bitmap_[i / 32] |= (1u << (i % 32));
    }
    
    void Reset();
    
    HeapTuple *Build();
    
    DISALLOW_IMPLICIT_CONSTRUCTORS(HeapTupleBuilder);
private:
    HeapRawData *PrepareSet(size_t i) {
        DCHECK_LT(i, unpacked_.size());
        null_bitmap_[i / 32] &= ~(1u << (i % 32));
        return &unpacked_[i];
    }
    
    const VirtualSchema *const schema_;
    std::unique_ptr<uint32_t[]> null_bitmap_;
    base::Arena *const arena_;
    base::ArenaVector<HeapRawData> unpacked_;
}; // class HeapTupleBuilder
    
} // namespace mai
    
} // namespace mai


#endif // MAI_SQL_HEAP_TUPLE_H_
