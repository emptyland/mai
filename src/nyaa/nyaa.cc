#include "nyaa/nyaa-core.h"
#include "mai-lang/nyaa.h"
#include "mai-lang/isolate.h"
#include "glog/logging.h"

namespace mai {
    
namespace nyaa {

Nyaa::Nyaa(Isolate *is)
    : isolate_(DCHECK_NOTNULL(is))
    , core_(new NyaaCore(this)) {
    prev_ = isolate_->GetNyaa();
    DCHECK_NE(prev_, this);
    isolate_->SetNyaa(this);
}

Nyaa::~Nyaa() {
    DCHECK_EQ(this, isolate_->GetNyaa());
    isolate_->SetNyaa(prev_);
    prev_ = nullptr;
}
    
} // namespace nyaa
    
} // namespace mai
