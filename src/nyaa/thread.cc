#include "nyaa/thread.h"
#include "nyaa/nyaa-values.h"
#include "nyaa/nyaa-core.h"

namespace mai {

namespace nyaa {

const int32_t State::kOffsetStackTop = Template::OffsetOf(&State::stack_top_);
const int32_t State::kOffsetStack = Template::OffsetOf(&State::stack_);

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
    stack_bottom_ = &stack_[initial_stack_size];
    stack_top_ = stack_bottom_;
}

State::~State() {
    
}

int State::Run(NyRunnable *fn, Object *argv[], int argc, int wanted, NyMap *env) {
    TODO();
    return -1;
}

int State::Resume(Object *argv[], int argc, int wanted, NyMap *env) {
    TODO();
    return -1;
}

void State::Yield() {
    TODO();
}

void State::Vraisef(const char *fmt, va_list ap) {
    TODO();
}

void State::Raise(NyString *msg, Object *ex) {
    TODO();
}
    
void State::IterateRoot(RootVisitor *visitor) {
    // TODO: Visit self
    DCHECK_LE(stack_top_, stack_bottom_);
    visitor->VisitRootPointers(stack_top_, stack_bottom_);
}

const int32_t NyThread::kOffsetCore = Template::OffsetOf(&NyThread::core_);

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
