#ifndef MAI_DB_COLUMN_FAMILY_H_
#define MAI_DB_COLUMN_FAMILY_H_

#include "base/reference-count.h"
#include "base/base.h"
#include "core/memory-table.h"
#include "mai/db.h"
#include "glog/logging.h"
#include <unordered_map>

namespace mai {
    
namespace db {
    
class Version;
class VersionSet;
class ColumnFamilySet;

class ColumnFamilyImpl final : public ColumnFamily {
public:
    ColumnFamilyImpl(const std::string &name, uint32_t id,
                     const ColumnFamilyOptions &options,
                     Version *dummy_versions,
                     ColumnFamilySet *onwer);
    virtual ~ColumnFamilyImpl();
    
    virtual std::string name() override { return name_; }
    virtual uint32_t id() override { return id_; }
    virtual const Comparator *comparator() override {
        return DCHECK_NOTNULL(options_.comparator);
    }
    virtual Error GetDescriptor(ColumnFamilyDescriptor *desc) override {
        desc->name    = name_;
        desc->options = options_;
        return Error::OK();
    }
    
    void AddRef() const { ref_count_.fetch_add(1, std::memory_order_relaxed); }
    
    bool ReleaseRef() const {
        int old_val = ref_count_.fetch_sub(1, std::memory_order_relaxed);
        DCHECK_GT(old_val, 0);
        return old_val == 1;
    }
    
    void SetDropped();
    bool IsDropped() const { return dropped_.load(); }
    
    DEF_PTR_GETTER(ColumnFamilyImpl, next);
    DEF_PTR_GETTER(ColumnFamilyImpl, prev);
    DEF_VAL_GETTER(ColumnFamilyOptions, options);

    friend class ColumnFamilySet;
    DISALLOW_IMPLICIT_CONSTRUCTORS(ColumnFamilyImpl);
private:
    const std::string name_;
    const uint32_t id_;
    const ColumnFamilyOptions options_;
    ColumnFamilySet *const owner_;
    mutable std::atomic<int> ref_count_;
    std::atomic<bool> dropped_;
    Version *dummy_versions_;
    
    Version *current_ = nullptr;
    
    base::Handle<core::MemoryTable> mutable_;
    base::Handle<core::MemoryTable> immutable_;
    
    ColumnFamilyImpl *next_ = nullptr;
    ColumnFamilyImpl *prev_ = nullptr;
    
}; // class ColumnFamilyImpl

class ColumnFamilySet final {
public:
    ColumnFamilySet(const std::string &db_name);
    ~ColumnFamilySet();
    
    ColumnFamilyImpl *GetDefault() const { return DCHECK_NOTNULL(default_cfd_); }
    
    ColumnFamilyImpl *GetColumnFamily(uint32_t id) const {
        auto iter = column_family_impls_.find(id);
        if (iter == column_family_impls_.end()) {
            return nullptr;
        }
        return iter->second;
    }
    
    ColumnFamilyImpl *GetColumnFamily(const std::string &name) const {
        auto iter = column_families_.find(name);
        if (iter == column_families_.end()) {
            return nullptr;
        }
        return GetColumnFamily(iter->second);
    }
    
    uint32_t NextColumnFamilyId() { return ++max_column_family_; }
    uint32_t GetMaxColumnFamily() const { return max_column_family_; }
    void UpdateColumnFamilyId(uint32_t new_id) {
        max_column_family_ = std::max(new_id, max_column_family_);
    }

    ColumnFamilyImpl *NewColumnFamily(const ColumnFamilyOptions opts,
                                      const std::string &name, uint32_t id,
                                      Version *dummy_versions);
    
    void RemoveColumnFamily(ColumnFamilyImpl *cfd) {
        auto iter = column_family_impls_.find(cfd->id());
        DCHECK(iter != column_family_impls_.end());
        column_family_impls_.erase(iter);
        column_families_.erase(cfd->name());
    }
    
    DISALLOW_IMPLICIT_CONSTRUCTORS(ColumnFamilySet);
private:
    const std::string db_name_;
    ColumnFamilyImpl *dummy_cfd_;
    uint32_t max_column_family_ = 0;
    ColumnFamilyImpl *default_cfd_ = nullptr;
    std::unordered_map<std::string, uint32_t> column_families_;
    std::unordered_map<uint32_t, ColumnFamilyImpl *> column_family_impls_;
}; // class ColumnFamilySet
    
} // namespace db
    
} // namespace mai


#endif // MAI_DB_COLUMN_FAMILY_H_
