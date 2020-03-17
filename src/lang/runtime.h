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
class Throwable;
class String;
class Class;
class Function;

#define DECLARE_RUNTIME_FUNCTIONS(V) \
    V(BoolValueOf, "lang.Bool::valueOf") \
    V(I8ValueOf, "lang.I8::valueOf") \
    V(U8ValueOf, "lang.U8::valueOf") \
    V(I16ValueOf, "lang.I16::valueOf") \
    V(U16ValueOf, "lang.U16::valueOf") \
    V(I32ValueOf, "lang.I32::valueOf") \
    V(U32ValueOf, "lang.U32::valueOf") \
    V(IntValueOf, "lang.Int::valueOf") \
    V(UIntValueOf, "lang.UInt::valueOf") \
    V(I64ValueOf, "lang.I64::valueOf") \
    V(U64ValueOf, "lang.U64::valueOf") \
    V(F32ValueOf, "lang.F32::valueOf") \
    V(F64ValueOf, "lang.F64::valueOf") \
    V(BoolToString, "lang.Bool::toString") \
    V(I8ToString, "lang.I8::toString") \
    V(U8ToString, "lang.U8::toString") \
    V(I16ToString, "lang.I16::toString") \
    V(U16ToString, "lang.U16::toString") \
    V(I32ToString, "lang.I32::toString") \
    V(U32ToString, "lang.U32::toString") \
    V(IntToString, "lang.Int::toString") \
    V(UIntToString, "lang.UInt::toString") \
    V(I64ToString, "lang.I64::toString") \
    V(U64ToString, "lang.U64::toString") \
    V(F32ToString, "lang.F32::toString") \
    V(F64ToString, "lang.F64::toString") \
    V(Println, "lang.println") \
    V(Assert, "lang.assert") \
    V(Object_HashCode, "lang.Object::hashCode") \
    V(Object_ToString, "lang.Object::toString") \
    V(Exception_PrintStackstrace, "lang.Exception::printStackstrace")

// The runtime functions definition
struct Runtime {
    enum Id {
    #define DEFINE_RUNTIME_ID(name, ...) k##name,
        DECLARE_RUNTIME_FUNCTIONS(DEFINE_RUNTIME_ID)
    #undef DEFINE_RUNTIME_ID
    };
    
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
    static String *BoolToString(int value);
    static String *I8ToString(int8_t value);
    static String *U8ToString(uint8_t value);
    static String *I16ToString(int16_t value);
    static String *U16ToString(uint16_t value);
    static String *I32ToString(int32_t value);
    static String *U32ToString(uint32_t value);
    static String *IntToString(int value);
    static String *UIntToString(unsigned value);
    static String *I64ToString(int64_t value);
    static String *U64ToString(uint64_t value);
    static String *F32ToString(float value);
    static String *F64ToString(double value);
    
    static String *StringContact(String **parts, String **end);
    
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
    
    // Panic Object
    static Throwable *NewNilPointerPanic();
    static Throwable *NewStackoverflowPanic();
    
    // Close Function and make a closure
    static Closure *CloseFunction(Function *func, uint32_t flags);
    
    // Coroutine
    static Coroutine *RunCoroutine(uint32_t flags, Closure *entry_point, Address params,
                                   uint32_t params_bytes_size);

    // Debug abort message output and fast abort code execution
    static void DebugAbort(const char *message);
    
    
    // Stand library native functions
    static void Println(String *input);
    static void Assert(int expect, String *message);
    

    static int Object_HashCode(Any *any);
    static String *Object_ToString(Any *any);
    
    static void Exception_PrintStackstrace(Any *any);
}; // struct Runtime

} // namespace lang

} // namespace mai

#endif // MAI_LANG_RUNTIME_H_
