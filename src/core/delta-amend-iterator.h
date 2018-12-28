#ifndef MAI_CORE_DELTA_AMEND_ITERATOR_H_
#define MAI_CORE_DELTA_AMEND_ITERATOR_H_

#include "base/base.h"
#include "mai/iterator.h"
#include "glog/logging.h"

namespace mai {
class Comparator;
namespace core {
    
class DeltaAmendIterator : public Iterator {
public:
    DeltaAmendIterator(const Comparator *cmp, Iterator *base_iter,
                       Iterator *delta_iter)
        : cmp_(DCHECK_NOTNULL(cmp))
        , base_iter_(DCHECK_NOTNULL(base_iter))
        , delta_iter_(DCHECK_NOTNULL(delta_iter)) {}
    virtual ~DeltaAmendIterator() override;
    
    virtual bool Valid() const override;
    virtual void SeekToFirst() override;
    virtual void SeekToLast() override;
    virtual void Seek(std::string_view target) override;
    virtual void Next() override;
    virtual void Prev() override;
    virtual std::string_view key() const override;
    virtual std::string_view value() const override;
    virtual Error error() const override;
    
    DISALLOW_IMPLICIT_CONSTRUCTORS(DeltaAmendIterator);
private:
    void Advance();
    void UpdateCurrent();
    
    void AdvanceBase() {
        if (direction_ == kForward) {
            base_iter_->Next();
        } else {
            base_iter_->Prev();
        }
    }
    
    void AdvanceDelta() {
        if (direction_ == kForward) {
            delta_iter_->Next();
        } else {
            delta_iter_->Prev();
        }
    }
    
    void Assert();
    
    const Comparator *const cmp_;
    std::unique_ptr<Iterator> base_iter_;
    std::unique_ptr<Iterator> delta_iter_;
    Error error_;
    Direction direction_ = kForward;
    bool current_at_base_ = true;
    bool equal_keys_ = false;
}; // class DeltaAmendIterator
    
} // namespace core
    
} // namespace mai

#endif // MAI_CORE_DELTA_AMEND_ITERATOR_H_
