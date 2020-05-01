#include "mai/at-exit.h"
#include "base/base.h"
#include "glog/logging.h"

namespace mai {

namespace {

AtExit *top = nullptr;

} // namespace

struct AtExit::Hook {
    Hook *next;
    Callback callback;
    void *params;
};

AtExit::AtExit(Linker)
    : prev_(top) {
    DCHECK_NE(top, this);
    top = this;
}

AtExit::~AtExit() {
    DCHECK_EQ(top, this);
    Hook *p = nullptr;
    while (hook_) {
        hook_->callback(hook_->params);
        p = hook_;
        hook_ = hook_->next;
        delete p;
    }
    top = prev_;
}

/*static*/ AtExit *AtExit::This() { return DCHECK_NOTNULL(top); }

void AtExit::Register(Callback callback, void *params) {
    std::lock_guard<std::mutex> lock(mutex_);
    hook_ = new Hook{hook_, callback, params};
}

} // namespace mai
