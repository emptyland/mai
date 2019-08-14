#include "mai/iterator.h"
#include "base/base.h"
#include "glog/logging.h"

namespace mai {
    
namespace core {

namespace {

class ErrorInternalIterator : public Iterator {
public:
    ErrorInternalIterator(Error error) : error_(error) {}
    virtual ~ErrorInternalIterator() {}
    
    virtual bool Valid() const override { return false; }
    virtual void SeekToFirst() override { Noreached(); }
    virtual void SeekToLast() override { Noreached(); }
    virtual void Seek(std::string_view) override { Noreached(); }
    virtual void Next() override { Noreached(); }
    virtual void Prev() override { Noreached(); }
    virtual std::string_view key() const override {
        Noreached(); return "";
    }
    virtual std::string_view value() const override {
        Noreached(); return "";
    }
    virtual Error error() const override { return error_; }
    
    DISALLOW_IMPLICIT_CONSTRUCTORS(ErrorInternalIterator);
private:
    void Noreached() const {
        NOREACHED() << error_.ToString();
    }
    
    Error error_;
}; // class ErrorInternalIterator

} // namespace

} // namespace core
    
/*virtual*/ Iterator::~Iterator() { DoCleanup(); }
    
/*static*/ Iterator *Iterator::AsError(Error error) {
    DCHECK(error.fail());
    return new core::ErrorInternalIterator(error);
}
    
} // namespace mai
