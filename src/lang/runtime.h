#ifndef MAI_LANG_RUNTIME_H_
#define MAI_LANG_RUNTIME_H_

//#include "lang/handle.h"
#include "base/base.h"

namespace mai {

namespace lang {

template<class T> class Handle;
template<class T> class Number;
template<class T, bool R> class Array;
class AbstractValue;
class AbstractArray;
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
    V(NewChannel, "lang.newChannel") \
    V(CloseChannel, "channel::close") \
    V(ChannelRecv32, "channel::recv32") \
    V(ChannelRecv64, "channel::recv64") \
    V(ChannelRecvPtr, "channel::recvPtr") \
    V(ChannelRecvF32, "channel::recvF32") \
    V(ChannelRecvF64, "channel::recvF64") \
    V(ChannelSend32, "channel::send32") \
    V(ChannelSend64, "channel::send64") \
    V(ChannelSendPtr, "channel::sendPtr") \
    V(ChannelSendF32, "channel::sendF32") \
    V(ChannelSendF64, "channel::sendF64") \
    V(ArrayAppend, "array::append") \
    V(Array8Append, "array8::append") \
    V(Array16Append, "array16::append") \
    V(Array32Append, "array32::append") \
    V(Array64Append, "array64::append") \
    V(ArrayPlus, "array::plus") \
    V(Array8Plus, "array8::plus") \
    V(Array16Plus, "array16::plus") \
    V(Array32Plus, "array32::plus") \
    V(Array64Plus, "array64::plus") \
    V(ArrayMinus, "array::minus") \
    V(Array8Minus, "array8::minus") \
    V(Array16Minus, "array16::minus") \
    V(Array32Minus, "array32::minus") \
    V(Array64Minus, "array64::minus") \
    V(ArrayResize, "array::resize") \
    V(Array8Resize, "array8::resize") \
    V(Array16Resize, "array16::resize") \
    V(Array32Resize, "array32::resize") \
    V(Array64Resize, "array64::resize") \
    V(TestAs, "lang.testAs") \
    V(TestIs, "lang.testIs") \
    V(Println, "lang.println") \
    V(Assert, "lang.assert") \
    V(Abort, "lang.abort") \
    V(Fatal, "lang.fatal") \
    V(Object_HashCode, "lang.Object::hashCode") \
    V(Object_ToString, "lang.Object::toString") \
    V(Exception_PrintStackstrace, "lang.Exception::printStackstrace") \
    V(System_CurrentTimeMillis, "lang.System::currentTimeMillis") \
    V(System_MicroTime, "lang.System::microTime") \
    V(System_GC, "lang.System::gc") \
    V(WaitGroup_Init, "lang.WaitGroup::init") \
    V(WaitGroup_Add, "lang.WaitGroup::add") \
    V(WaitGroup_Done, "lang.WaitGroup::done") \
    V(WaitGroup_Wait, "lang.WaitGroup::wait") \
    V(CurrentSourceLine, "runtime.currentSourceLine") \
    V(CurrentSourceName, "runtime.currentSourceName") \
    V(Sleep, "runtime.sleep") \
    V(CurrentMachineID, "runtime.currentMachineID") \
    V(CurrentCoroutineID, "runtime.currentCoroutineID") \
    V(MinorGC, "runtime.minorGC") \
    V(MajorGC, "runtime.majorGC") \
    V(GetMemoryHistogram, "runtime.getMemoryHistogram") \
    V(Schedule, "runtime.schedule")


// The runtime functions definition
struct Runtime {
    enum Id {
    #define DEFINE_RUNTIME_ID(name, ...) k##name,
        DECLARE_RUNTIME_FUNCTIONS(DEFINE_RUNTIME_ID)
    #undef DEFINE_RUNTIME_ID
    };
    
    static Any *NewObject(const Class *clazz, uint32_t flags);

    // Box in functions
    static AbstractValue *BoolValueOf(int8_t value);
    static AbstractValue *I8ValueOf(int8_t value);
    static AbstractValue *U8ValueOf(uint8_t value);
    static AbstractValue *I16ValueOf(int16_t value);
    static AbstractValue *U16ValueOf(uint16_t value);
    static AbstractValue *I32ValueOf(int32_t value);
    static AbstractValue *U32ValueOf(uint32_t value);
    static AbstractValue *IntValueOf(int32_t value);
    static AbstractValue *UIntValueOf(uint32_t value);
    static AbstractValue *I64ValueOf(int64_t value);
    static AbstractValue *U64ValueOf(uint64_t value);
    static AbstractValue *F32ValueOf(float value);
    static AbstractValue *F64ValueOf(double value);

    // Box out functions
    static String *BoolToString(int8_t value);
    static String *I8ToString(int8_t value);
    static String *U8ToString(uint8_t value);
    static String *I16ToString(int16_t value);
    static String *U16ToString(uint16_t value);
    static String *I32ToString(int32_t value);
    static String *U32ToString(uint32_t value);
    static String *IntToString(int32_t value);
    static String *UIntToString(uint32_t value);
    static String *I64ToString(int64_t value);
    static String *U64ToString(uint64_t value);
    static String *F32ToString(float value);
    static String *F64ToString(double value);
    
    static String *StringContact(String **parts, String **end);
    
    // Channel functions:
    static Channel *NewChannel(uint32_t data_typeid, uint32_t capacity);
    static void CloseChannel(Handle<Channel> chan);

    static int32_t ChannelRecv32(Handle<Channel> chan);
    static int64_t ChannelRecv64(Handle<Channel> chan);
    static Any *ChannelRecvPtr(Handle<Channel> chan);
    static float ChannelRecvF32(Handle<Channel> chan);
    static double ChannelRecvF64(Handle<Channel> chan);

    static void ChannelSend32(Handle<Channel> chan, int32_t value);
    static void ChannelSend64(Handle<Channel> chan, int64_t value);
    static void ChannelSendPtr(Handle<Channel> chan, Handle<Any> value);
    static void ChannelSendF32(Handle<Channel> chan, float value);
    static void ChannelSendF64(Handle<Channel> chan, double value);
    
    // Array
    static AbstractArray *NewArray(const Class *element_type, int len);
    static AbstractArray *NewArrayWith(const Class *element_type, Address begin, Address end);
    
    static AbstractArray *ArrayAppend(Handle<Array<Any *, true>> array, Handle<Any> value);
    static AbstractArray *Array8Append(Handle<Array<uint8_t, false>> array, uint8_t value);
    static AbstractArray *Array16Append(Handle<Array<uint16_t, false>> array, uint16_t value);
    static AbstractArray *Array32Append(Handle<Array<uint32_t, false>> array, uint32_t value);
    static AbstractArray *Array64Append(Handle<Array<uint64_t, false>> array, uint64_t value);

    static AbstractArray *ArrayPlus(Handle<Array<Any *, true>> array, int index, Handle<Any> value);
    static AbstractArray *Array8Plus(Handle<Array<uint8_t, false>> array, int index, uint8_t value);
    static AbstractArray *Array16Plus(Handle<Array<uint16_t, false>> array, int index,
                                      uint16_t value);
    static AbstractArray *Array32Plus(Handle<Array<uint32_t, false>> array, int index,
                                      uint32_t value);
    static AbstractArray *Array64Plus(Handle<Array<uint64_t, false>> array, int index,
                                      uint64_t value);
    
    static AbstractArray *ArrayMinus(Handle<Array<Any *, true>> array, int index);
    static AbstractArray *Array8Minus(Handle<Array<uint8_t, false>> array, int index);
    static AbstractArray *Array16Minus(Handle<Array<uint16_t, false>> array, int index);
    static AbstractArray *Array32Minus(Handle<Array<uint32_t, false>> array, int index);
    static AbstractArray *Array64Minus(Handle<Array<uint64_t, false>> array, int index);
    
    static AbstractArray *ArrayResize(Handle<Array<Any *, true>> array, int size);
    static AbstractArray *Array8Resize(Handle<Array<uint8_t, false>> array, int size);
    static AbstractArray *Array16Resize(Handle<Array<uint16_t, false>> array, int size);
    static AbstractArray *Array32Resize(Handle<Array<uint32_t, false>> array, int size);
    static AbstractArray *Array64Resize(Handle<Array<uint64_t, false>> array, int size);
    
    static Any *TestAs(Handle<Any> any, void *param1, void *param2);
    static int TestIs(Handle<Any> any, void *param1, void *param2);

    static int IsSameOrBaseOf(const Any *any, const Class *type);

    static int StringCompareFallback(const String *lhs, const String *rhs);

    // WriteBarrier
    static Any *WriteBarrierWithOffset(Any *host, int32_t offset);
    static Any *WriteBarrierWithAddress(Any *host, Any **address);
    
    // Panic Object
    static Throwable *NewNilPointerPanic();
    static Throwable *NewStackoverflowPanic();
    static Throwable *NewOutOfBoundPanic();
    static Throwable *NewArithmeticPanic();
    static Throwable *NewBadCastPanic(const Class *lhs, const Class *rhs);
    
    // Make current stackstrace
    static Throwable *MakeStacktrace(Throwable *expect);
    
    // Close Function and make a closure
    static Closure *CloseFunction(Function *func, uint32_t flags);
    
    // Coroutine
    static Coroutine *RunCoroutine(uint32_t flags, Closure *entry_point, Address params,
                                   uint32_t params_bytes_size);

    // Debug abort message output and fast abort code execution
    static void DebugAbort(const char *message);
    
    
    // Stand library native functions
    static void Println(Handle<String> input);
    static void Assert(int expect, Handle<String> message);
    static void Abort(Handle<String> message);
    static void Fatal(Handle<String> message);

    static int Object_HashCode(Handle<Any> any);
    static String *Object_ToString(Handle<Any> any);

    static void Exception_PrintStackstrace(Handle<Any> any);

    static int64_t System_CurrentTimeMillis(Handle<Any> any);
    static int64_t System_MicroTime(Handle<Any> any);
    static void System_GC(Handle<Any> any);
    
    static Any *WaitGroup_Init(Handle<Any> any);
    static void WaitGroup_Add(Handle<Any> any, int n);
    static void WaitGroup_Done(Handle<Any> any);
    static void WaitGroup_Wait(Handle<Any> any);
    
    static int CurrentSourceLine(int level);
    static String *CurrentSourceName(int level);
    static void Sleep(uint64_t mills);
    static int CurrentMachineID();
    static uint64_t CurrentCoroutineID();
    static void MinorGC();
    static void MajorGC();
    static Any *GetMemoryHistogram();
    static void Schedule();
}; // struct Runtime

} // namespace lang

} // namespace mai

#endif // MAI_LANG_RUNTIME_H_
