#ifndef MAI_LANG_RUNTIME_H_
#define MAI_LANG_RUNTIME_H_

#include "base/base.h"

namespace mai {

namespace lang {

template<class T> class Number;
class AbstractValue;
class Channel;
class Closure;
class Any;
class Coroutine;
class Class;

// The runtime functions definition
struct Runtime {
    
    static Any *NewObject(const Class *clazz, uint32_t flags);

    // Box in functions
    static AbstractValue *BoolValueOf(intptr_t value);
    static AbstractValue *I8ValueOf(intptr_t value);
    static AbstractValue *U8ValueOf(uintptr_t value);
    static AbstractValue *I16ValueOf(intptr_t value);
    static AbstractValue *U16ValueOf(uintptr_t value);
    static AbstractValue *I32ValueOf(intptr_t value);
    static AbstractValue *U32ValueOf(uintptr_t value);
    static AbstractValue *IntValueOf(intptr_t value);
    static AbstractValue *UIntValueOf(uintptr_t value);
    static AbstractValue *I64ValueOf(intptr_t value);
    static AbstractValue *U64ValueOf(uintptr_t value);
    static AbstractValue *F32ValueOf(float value);
    static AbstractValue *F64ValueOf(double value);

    // Box out functions
    
    // Channel functions:
    static Channel *NewChannel(uint32_t data_typeid, uint32_t capacity);
    static void ChannelClose(Channel *chan);
    
    static int32_t ChannelRecv32(Channel *chan);
    static int64_t ChannelRecv64(Channel *chan);
    static Any *ChannelRecvPtr(Channel *chan);
    static float ChannelRecvF32(Channel *chan);
    static double ChannelRecvF64(Channel *chan);

    static void ChannelSend32(Channel *chan, int32_t value);
    static void ChannelSend64(Channel *chan, int64_t value);
    static void ChannelSendPtr(Channel *chan, Any *value);
    static void ChannelSendF32(Channel *chan, float value);
    static void ChannelSendF64(Channel *chan, double value);
    
    // Coroutine
    static Coroutine *RunCoroutine(uint32_t flags, Closure *entry_point, Address params,
                                   uint32_t params_bytes_size);

    // Debug abort message output and fast abort code execution
    static void DebugAbort(const char *message);

}; // struct Runtime

} // namespace lang

} // namespace mai

#endif // MAI_LANG_RUNTIME_H_
