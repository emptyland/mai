#ifndef MAI_CORE_ITERATOR_WARPPER_H_
#define MAI_CORE_ITERATOR_WARPPER_H_

#include "mai/iterator.h"
#include "base/base.h"

namespace mai {
 
namespace core {

class IteratorWarpper : public Iterator {
public:
    IteratorWarpper() {}
    virtual ~IteratorWarpper();

    void set_delegated(Iterator *delegated) { delegated_.reset(delegated); }
    
    virtual bool Valid() const override;
    virtual void SeekToFirst() override;
    virtual void SeekToLast() override;
    virtual void Seek(std::string_view target) override;
    virtual void Next() override;
    virtual void Prev() override;
    virtual std::string_view key() const override;
    virtual std::string_view value() const override;
    virtual Error error() const override;

    DISALLOW_IMPLICIT_CONSTRUCTORS(IteratorWarpper);
private:
    void Update() {
        valid_ = delegated_->Valid();
        if (valid_) {
            key_ = delegated_->key();
        }
    }
    
    std::unique_ptr<Iterator> delegated_;
    bool valid_ = false;
    std::string_view key_;
    
};

} // namespace core
    
} // namespace mai

#endif // MAI_CORE_ITERATOR_WARPPER_H_
