#ifndef MAI_DB_SNAPSHOT_IMPL_H_
#define MAI_DB_SNAPSHOT_IMPL_H_

#include "core/key-boundle.h"
#include "mai/db.h"

namespace mai {
    
namespace db {
    
class SnapshotList;
    
class SnapshotImpl final : public Snapshot {
public:
    SnapshotImpl(core::SequenceNumber sequence_number,
                 uint64_t unix_time,
                 SnapshotList *owns)
        : sequence_number_(sequence_number)
        , unix_time_(unix_time)
        , owns_(owns) {}
    virtual ~SnapshotImpl() {}
    
    DEF_VAL_GETTER(core::SequenceNumber, sequence_number);
    
    static SnapshotImpl *Cast(Snapshot *s) {
        if (!s) { return nullptr; }
        return down_cast<SnapshotImpl>(s);
    }
    
    static const SnapshotImpl *Cast(const Snapshot *s) {
        if (!s) { return nullptr; }
        return down_cast<const SnapshotImpl>(s);
    }
    
    friend class SnapshotList;
    DISALLOW_IMPLICIT_CONSTRUCTORS(SnapshotImpl);
private:
    core::SequenceNumber sequence_number_;
    uint64_t unix_time_;
    SnapshotList *owns_;
    
    SnapshotImpl *next_ = this;
    SnapshotImpl *prev_ = this;
}; // class SnapshotImpl
    

class SnapshotList final {
public:
    SnapshotList() : dummy_(new SnapshotImpl(0, 0, this)) {}
    
    ~SnapshotList() {
        while (dummy_->next_ != dummy_) {
            SnapshotImpl *x = dummy_->next_;
            Remove(x);
            delete x;
        }
        delete dummy_;
    }
    
    SnapshotImpl *NewSnapshot(core::SequenceNumber sequence_number,
                              uint64_t unix_time) {
        SnapshotImpl *x = new SnapshotImpl(sequence_number, unix_time, this);
        x->next_ = dummy_;
        SnapshotImpl *prev = dummy_->prev_;
        x->prev_ = prev;
        prev->next_ = x;
        dummy_->prev_ = x;
        return x;
    }
    
    void DeleteSnapshot(SnapshotImpl *x) {
        Remove(x);
        delete x;
    }
    
    SnapshotImpl *oldest() const { return dummy_->next_; }
    
    bool empty() const { return dummy_->next_ == dummy_; }
    
    DISALLOW_IMPLICIT_CONSTRUCTORS(SnapshotList);
private:
    static void Remove(SnapshotImpl *x) {
        x->prev_->next_ = x->next_;
        x->next_->prev_ = x->prev_;
    }
    
    SnapshotImpl *dummy_;
}; // class SnapshotList
    
} // namespace db

} // namespace mai

#endif // MAI_DB_SNAPSHOT_IMPL_H_
