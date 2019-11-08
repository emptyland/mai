#include "nyaa/thread.h"
#include "nyaa/nyaa-core.h"

namespace mai {

namespace nyaa {

/*static*/ State *State::New(NyaaCore *owns, size_t initial_stack_size) {
    void *chunk = malloc(sizeof(State) + initial_stack_size * sizeof(Object *));
    return new (chunk) State(owns, initial_stack_size);
}

/*static*/ void State::Delete(State *state) {
    state->~State();
    free(state);
}
    
State::State(NyaaCore *owns, size_t initial_stack_size)
    : owns_(DCHECK_NOTNULL(owns))
    , stack_size_(initial_stack_size) {
    ::memset(stack_, 0, stack_size_ * sizeof(stack_[0]));
}

State::~State() {
    
}
    
void State::IterateRoot(RootVisitor *visitor) {
    // TODO: Visit self
    TODO();
}

void NyThread::SetPrev(NyThread *prev, NyaaCore *N) {
    prev_ = prev;
    N->BarrierWr(this, &prev_, prev);
}

/*static*/ void NyThread::Finalizer(NyUDO *base, NyaaCore *N) {
    NyThread *thread = static_cast<NyThread *>(base);
    //thread->RemoveSelf();
    State::Delete(thread->core_);
}

} // namespace nyaa

} // namespace mai
