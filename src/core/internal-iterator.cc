#include "core/internal-iterator.h"
#include "glog/logging.h"

namespace mai {
    
namespace core {
    
/*virtual*/ InternalIterator::~InternalIterator() {}

namespace {

class ErrorInternalIterator : public InternalIterator {
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
        LOG(FATAL) << "Noreached! " << error_.ToString();
    }
    
    Error error_;
}; // class ErrorInternalIterator

} // namespace

/*static*/ InternalIterator *InternalIterator::AsError(Error error) {
    DCHECK(error.fail());
    return new ErrorInternalIterator(error);
}

} // namespace core
    
} // namespace mai
