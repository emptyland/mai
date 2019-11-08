#ifndef MAI_NYAA_THREAD_H_
#define MAI_NYAA_THREAD_H_

#include "nyaa/nyaa-values.h"

namespace mai {

namespace nyaa {

class NyaaCore;
class NyThread;

class State final {
public:
    static State *New(NyaaCore *owns, size_t initial_stack_size);
    static void Delete(State *state);
    
    friend class NyThread;
private:
    State(NyaaCore *owns, size_t initial_stack_size);
    ~State();
    
    void IterateRoot(RootVisitor *visitor);
    
    NyaaCore *const owns_;
    size_t stack_size_;
    Object *stack_[0];
}; // class State

class NyThread final : public NyUDO {
public:
    using Core = State;
    
    explicit NyThread(Core *core)
        : NyUDO(0, true)
        , core_(core) {}
    
    DEF_PTR_GETTER(Core, core);
    DEF_PTR_GETTER(NyThread, prev);
    DEF_PTR_GETTER(NyThread, next);
    NyaaCore *owns() const { return core_->owns_; }
    
    void SetPrev(NyThread *prev, NyaaCore *N);

    static void Finalizer(NyUDO *thread, NyaaCore *N);
    
    void IterateRoot(RootVisitor *visitor) {
        visitor->VisitRootPointer(reinterpret_cast<Object **>(&next_));
        visitor->VisitRootPointer(reinterpret_cast<Object **>(&prev_));
        core_->IterateRoot(visitor);
    }
    
    void Iterate(ObjectVisitor *visitor) {}
    
    constexpr size_t PlacedSize() const { return sizeof(*this); }

    DEF_HEAP_OBJECT(Thread);
private:
    NyThread *next_ = nullptr; // [strong ref]
    NyThread *prev_ = nullptr; // [strong ref]
    Core *const core_;
}; // class NyThread

} // namespace nyaa

} // namespace mai


#endif // MAI_NYAA_THREAD_H_
