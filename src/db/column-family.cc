#include "db/column-family.h"
#include "db/version.h"

namespace mai {
    
namespace db {
    
////////////////////////////////////////////////////////////////////////////////
/// class ColumnFamilyImpl
////////////////////////////////////////////////////////////////////////////////
    
ColumnFamilyImpl::ColumnFamilyImpl(const std::string &name, uint32_t id,
                                   const ColumnFamilyOptions &options,
                                   VersionSet *versions,
                                   ColumnFamilySet *owner)
    : name_(name)
    , id_(id)
    , options_(options)
    , dummy_versions_(new Version(this))
    , current_(dummy_versions_)
    , ikcmp_(DCHECK_NOTNULL(options.comparator))
    , owner_(owner)
    , ref_count_(0)
    , dropped_(false) {
    AddRef();
    dummy_versions_->next_ = dummy_versions_;
    dummy_versions_->prev_ = dummy_versions_;
}
    
/*virtual*/ ColumnFamilyImpl::~ColumnFamilyImpl() {
    DCHECK_EQ(ref_count_.load(std::memory_order_relaxed), 0);
    // remove from linked list
    ColumnFamilyImpl *prev = prev_;
    ColumnFamilyImpl *next = next_;
    prev->next_ = next;
    next->prev_ = prev;
    
    if (!dropped_ && owner_) {
        owner_->RemoveColumnFamily(this);
    }
    
    while (dummy_versions_->next() != dummy_versions_) {
        Version *x = dummy_versions_->next();
        Version *prev = x->prev();
        Version *next = x->next();
        prev->next_ = next;
        next->prev_ = prev;
        delete x;
    }
    // TODO:
}
    
void ColumnFamilyImpl::SetDropped() {
    DCHECK_NE(id_, 0) << "Can not drop default column family!";
    dropped_ = true;
    
    if (owner_) {
        owner_->RemoveColumnFamily(this);
    }
}
    
void ColumnFamilyImpl::Append(Version *version) {
    // add linked list
    version->next_ = dummy_versions_;
    Version *prev = dummy_versions_->prev_;
    version->prev_ = prev;
    prev->next_ = version;
    dummy_versions_->prev_ = version;
    current_ = version;
}

////////////////////////////////////////////////////////////////////////////////
/// class ColumnFamilyHandle
////////////////////////////////////////////////////////////////////////////////
    
/*virtual*/ ColumnFamilyHandle::~ColumnFamilyHandle() {}

/*virtual*/ std::string ColumnFamilyHandle::name() const {
    return impl_->name();
}

/*virtual*/ uint32_t ColumnFamilyHandle::id() const {
    return impl_->id();
}

/*virtual*/ const Comparator *ColumnFamilyHandle::comparator() const {
    return impl_->ikcmp()->ucmp();
}
    
/*virtual*/ Error
ColumnFamilyHandle::GetDescriptor(ColumnFamilyDescriptor *desc) const {
    if (impl_->IsDropped()) {
        return MAI_CORRUPTION("Column family has dropped!");
    }
    desc->name    = impl_->name();
    desc->options = impl_->options();
    return Error::OK();
}

////////////////////////////////////////////////////////////////////////////////
/// class ColumnFamilySet
////////////////////////////////////////////////////////////////////////////////

ColumnFamilySet::ColumnFamilySet(const std::string &db_name)
    : db_name_(db_name)
    , dummy_cfd_(new ColumnFamilyImpl("", 0, ColumnFamilyOptions{}, nullptr,
                                      nullptr)) {
    dummy_cfd_->prev_ = dummy_cfd_;
    dummy_cfd_->next_ = dummy_cfd_;
}

ColumnFamilySet::~ColumnFamilySet() {
    while (column_family_impls_.size() > 0) {
        // cfd destructor will delete itself from column_family_data_
        ColumnFamilyImpl *cfd = column_family_impls_.begin()->second;
        bool last_ref = cfd->ReleaseRef();
        DCHECK(last_ref); (void)last_ref;
        delete cfd;
    }
    bool dummy_last_ref = dummy_cfd_->ReleaseRef();
    DCHECK(dummy_last_ref); (void)dummy_last_ref;
    delete dummy_cfd_;
}

ColumnFamilyImpl *ColumnFamilySet::NewColumnFamily(const ColumnFamilyOptions opts,
                                                   const std::string &name,
                                                   uint32_t id,
                                                   VersionSet *versions) {
    DCHECK(column_families_.find(name) == column_families_.end());
    DCHECK(column_family_impls_.find(id) == column_family_impls_.end());
    
    ColumnFamilyImpl *cfd = new ColumnFamilyImpl(name, id, opts, versions, this);
    
    column_families_.insert({name, id});
    column_family_impls_.insert({id, cfd});
    max_column_family_ = std::max(id, max_column_family_);
    
    // add linked list
    cfd->next_ = dummy_cfd_;
    ColumnFamilyImpl *prev = dummy_cfd_->prev_;
    cfd->prev_ = prev;
    prev->next_ = cfd;
    dummy_cfd_->prev_ = cfd;

    if (id == 0) {
        default_cfd_ = cfd;
    }
    return cfd;
}

} // namespace db
    
} // namespace mai
