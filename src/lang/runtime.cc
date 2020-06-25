#include "lang/runtime.h"
#include "lang/isolate-inl.h"
#include "lang/garbage-collector.h"
#include "lang/channel.h"
#include "lang/value-inl.h"
#include "lang/machine.h"
#include "lang/coroutine.h"
#include "lang/scheduler.h"
#include "lang/stack-frame.h"
#include "lang/pgo.h"
#include "lang/compiler.h"
#include "base/spin-locking.h"
#include "base/slice.h"
#include "glog/logging.h"
#include <math.h>

namespace mai {

namespace lang {

// [Safepoint]
/*static*/ Any *Runtime::NewObject(const Class *clazz, uint32_t flags) {
    if (!clazz) {
        Machine::This()->ThrowPanic(Panic::kError, STATE->factory()->nil_error_text());
        return nullptr;
    }
    SafepointScope safepoint(STATE->gc());
    return Machine::This()->NewObject(clazz, flags);
}

template<class T>
static inline AbstractValue *ValueOf(T input, BuiltinType type) {
    return Machine::This()->ValueOfNumber(type, &input, sizeof(input));
}

/*static*/ AbstractValue *Runtime::BoolValueOf(int8_t input) {
    int8_t value = static_cast<int8_t>(input);
    return Machine::This()->ValueOfNumber(kType_bool, &value, sizeof(value));
}

/*static*/ AbstractValue *Runtime::I8ValueOf(int8_t value) {
    return ValueOf<int8_t>(value, kType_i8);
}

/*static*/ AbstractValue *Runtime::U8ValueOf(uint8_t value) {
    return ValueOf<uint8_t>(value, kType_u8);
}

/*static*/ AbstractValue *Runtime::I16ValueOf(int16_t value) {
    return ValueOf<int16_t>(value, kType_i16);
}

/*static*/ AbstractValue *Runtime::U16ValueOf(uint16_t value) {
    return ValueOf<uint16_t>(value, kType_u16);
}

/*static*/ AbstractValue *Runtime::I32ValueOf(int32_t value) {
    return ValueOf<int32_t>(value, kType_i32);
}

/*static*/ AbstractValue *Runtime::U32ValueOf(uint32_t value) {
    return ValueOf<uint32_t>(value, kType_u32);
}

/*static*/ AbstractValue *Runtime::IntValueOf(int32_t value) {
    return ValueOf<int32_t>(value, kType_int);
}

/*static*/ AbstractValue *Runtime::UIntValueOf(uint32_t value) {
    return ValueOf<uint32_t>(value, kType_uint);
}

/*static*/ AbstractValue *Runtime::I64ValueOf(int64_t value) {
    return ValueOf<int64_t>(value, kType_i64);
}

/*static*/ AbstractValue *Runtime::U64ValueOf(uint64_t value) {
    return ValueOf<uint64_t>(value, kType_u64);
}

/*static*/ AbstractValue *Runtime::F32ValueOf(float value) {
    return Machine::This()->ValueOfNumber(kType_f32, &value, sizeof(value));
}

/*static*/ AbstractValue *Runtime::F64ValueOf(double value) {
    return Machine::This()->ValueOfNumber(kType_f64, &value, sizeof(value));
}

/*static*/ String *Runtime::BoolToString(int8_t value) {
    return value ? STATE->factory()->true_string() : STATE->factory()->false_string();
}

/*static*/ String *Runtime::I8ToString(int8_t value) {
    return Machine::This()->NewUtf8StringWithFormat(0, "%" PRId8, value);
}

/*static*/ String *Runtime::U8ToString(uint8_t value) {
    return Machine::This()->NewUtf8StringWithFormat(0, "%" PRIu8, value);
}

/*static*/ String *Runtime::I16ToString(int16_t value) {
    return Machine::This()->NewUtf8StringWithFormat(0, "%" PRId16, value);
}

/*static*/ String *Runtime::U16ToString(uint16_t value) {
    return Machine::This()->NewUtf8StringWithFormat(0, "%" PRIu16, value);
}

/*static*/ String *Runtime::I32ToString(int32_t value) {
    return Machine::This()->NewUtf8StringWithFormat(0, "%" PRId32, value);
}

/*static*/ String *Runtime::U32ToString(uint32_t value) {
    return Machine::This()->NewUtf8StringWithFormat(0, "%" PRIu32, value);
}

/*static*/ String *Runtime::IntToString(int value) {
    return Machine::This()->NewUtf8StringWithFormat(0, "%d", value);
}

/*static*/ String *Runtime::UIntToString(unsigned value) {
    return Machine::This()->NewUtf8StringWithFormat(0, "%u", value);
}

/*static*/ String *Runtime::I64ToString(int64_t value) {
    return Machine::This()->NewUtf8StringWithFormat(0, "%" PRId64, value);
}

/*static*/ String *Runtime::U64ToString(uint64_t value) {
    return Machine::This()->NewUtf8StringWithFormat(0, "%" PRIu64, value);
}

/*static*/ String *Runtime::F32ToString(float value) {
    return Machine::This()->NewUtf8StringWithFormat(0, "%f", value);
}

/*static*/ String *Runtime::F64ToString(double value) {
    return Machine::This()->NewUtf8StringWithFormat(0, "%f", value);
}

// [Safepoint]
// 2018846
// 1269759
//  521255
/*static*/ String *Runtime::StringContact(String **parts, String **end) {
    if (auto kind = STATE->gc()->ShouldCollect(); kind == GarbageCollector::kIdle) {
        IncrementalStringBuilder builder;
        for (auto i = end - 1; i >= parts; i--) {
            builder.AppendString(*i);
        }
        return builder.QuickBuild();
    }
    
    HandleScope handle_scope(HandleScope::INITIALIZER);
    const int n = static_cast<int>(end - parts);
    String **handles = reinterpret_cast<String **>(Machine::This()->AdvanceHandleSlots(n));
    int k = 0;
    for (auto i = end - 1; i >= parts; i--) {
        handles[k++] = *i;
    }
    DCHECK_EQ(k, n);

    SafepointScope safepoint(STATE->gc());
    IncrementalStringBuilder builder;
    for (int i = 0; i < n; i++) {
        builder.AppendString(handles[i]);
    }
    return builder.QuickBuild();
}

/*static*/ Channel *Runtime::NewChannel(uint32_t data_typeid, uint32_t capacity) {
    return Machine::This()->NewChannel(data_typeid, capacity, 0/*flags*/);
}

/*static*/ void Runtime::CloseChannel(Handle<Channel> chan) {
    if (chan.is_value_null()) {
        Machine::This()->ThrowPanic(Panic::kError, STATE->factory()->nil_error_text());
        return;
    }

    //std::lock_guard<std::mutex> lock(*chan->mutable_mutex());
    base::SpinLock lock(chan->mutable_mutex());
    if (chan->has_close()) {
        Machine::This()->ThrowPanic(Panic::kError, STATE->factory()->dup_close_chan_error_text());
        return;
    }
    chan->CloseLockless();
    DCHECK(chan->has_close());
}

template<class T>
static inline T InternalChannelRecv(Channel *chan) {
    if (!chan) {
        Machine::This()->ThrowPanic(Panic::kError, STATE->factory()->nil_error_text());
        return 0;
    }
    if (chan->has_close()) {
        return 0;
    }
    T value = 0;
    chan->Recv(&value, sizeof(value));
    return value;
}

template<class T>
static inline void InternalChannelSendNoBarrier(Channel *chan, T value) {
    if (!chan) {
        Machine::This()->ThrowPanic(Panic::kError, STATE->factory()->nil_error_text());
    }
    if (!chan->has_close()) {
        chan->SendNoBarrier(&value, sizeof(value));
    }
}

/*static*/ int32_t Runtime::ChannelRecv32(Handle<Channel> chan) {
    return InternalChannelRecv<int32_t>(*chan);
}

/*static*/ int64_t Runtime::ChannelRecv64(Handle<Channel> chan) {
    return InternalChannelRecv<int64_t>(*chan);
}

/*static*/ Any *Runtime::ChannelRecvPtr(Handle<Channel> chan) {
    return InternalChannelRecv<Any *>(*chan);
}

/*static*/ float Runtime::ChannelRecvF32(Handle<Channel> chan) {
    return InternalChannelRecv<float>(*chan);
}

/*static*/ double Runtime::ChannelRecvF64(Handle<Channel> chan) {
    return InternalChannelRecv<double>(*chan);
}

/*static*/ void Runtime::ChannelSend64(Handle<Channel> chan, int64_t value) {
    InternalChannelSendNoBarrier<int64_t>(*chan, value);
}

/*static*/ void Runtime::ChannelSend32(Handle<Channel> chan, int32_t value) {
    InternalChannelSendNoBarrier<int32_t>(*chan, value);
}

/*static*/ void Runtime::ChannelSendPtr(Handle<Channel> chan, Handle<Any> value) {
    if (chan.is_value_null()) {
        Machine::This()->ThrowPanic(Panic::kError, STATE->factory()->nil_error_text());
    }
    if (chan->has_close()) {
        chan->SendAny(*value);
    }
}

/*static*/ void Runtime::ChannelSendF32(Handle<Channel> chan, float value) {
    InternalChannelSendNoBarrier<float>(*chan, value);
}

/*static*/ void Runtime::ChannelSendF64(Handle<Channel> chan, double value) {
    InternalChannelSendNoBarrier<double>(*chan, value);
}

BuiltinType GetArrayType(const Class *element_type) {
    if (element_type->is_reference()) {
        return kType_array;
    } else {
        switch (element_type->reference_size()) {
            case 1:
                return kType_array8;
            case 2:
                return kType_array16;
            case 4:
                return kType_array32;
            case 8:
                return kType_array64;
            default:
                break;
        }
    }
    NOREACHED();
    return kType_void;
}

/*static*/ AbstractArray *Runtime::NewArray(const Class *element_type, int len) {
    if (len < 0) {
        len = 0;
    }
    int capacity = len < 16 ? len + 16 : len;
    SafepointScope safepoint_scope(STATE->gc());
    return Machine::This()->NewArray(element_type->id(), len, capacity, 0);
}

/*static*/ AbstractArray *Runtime::NewArrayWith(const Class *element_type, Address start,
                                                Address stop) {
    //BuiltinType dest_type = GetArrayType(element_type);
    if (element_type->is_reference()) {
        HandleScope handle_scope(HandleScope::INITIALIZER);
        int length = static_cast<int>((stop - start) >> kPointerShift);
        Any **handles = reinterpret_cast<Any **>(Machine::This()->AdvanceHandleSlots(length));
        ::memcpy(handles, start, stop - start);

        SafepointScope safepoint_scope(STATE->gc());
        
        int capacity = length < 16 ? length + 16 : length;
        Array<Any *> *array = static_cast<Array<Any *> *>(Machine::This()->NewArray(element_type->id(),
                                                                                    length,
                                                                                    capacity, 0));
        array->QuicklySetAll(0, handles, length);
        return array;
    } else {
        SafepointScope safepoint_scope(STATE->gc());

        int element_size = RoundUp(element_type->reference_size(), kStackSizeGranularity);
        int length = static_cast<int>((stop - start) / element_size);
        int capacity = length < 16 ? length + 16 : length;
        AbstractArray *array = Machine::This()->NewArray(element_type->id(), length, capacity, 0);
        const Field *elems_field = array->clazz()->field(3);
        DCHECK(!::strcmp("elems", elems_field->name()));
        ::memcpy(reinterpret_cast<Address>(array) + elems_field->offset(), start, stop - start);
        return array;
    }
}

template<class T>
static inline AbstractArray *TArrayAppend(const Handle<Array<T>> &array, T value) {
    if (array.is_value_null()) {
        Machine::This()->ThrowPanic(Panic::kError, STATE->factory()->nil_error_text());
        return nullptr;
    }
    
    SafepointScope safepoint_scope(STATE->gc());
    Array<T> *spwan = static_cast<Array<T> *>(Machine::This()->NewArrayCopied(*array, 1/*increment*/,
                                                                              0/*flags*/));
    spwan->quickly_set(array->length(), value);
    return spwan;
}

template<class T>
static inline AbstractArray *TArrayPlus(const Handle<Array<T>> &array, int index, T value) {
    if (array.is_value_null()) {
        Machine::This()->ThrowPanic(Panic::kError, STATE->factory()->nil_error_text());
        return nullptr;
    }

    SafepointScope safepoint_scope(STATE->gc());
    return *array->Plus(index, value);
}

template<class T>
static inline AbstractArray *TArrayMinus(const Handle<Array<T>> &array, int index) {
    if (array.is_value_null()) {
        Machine::This()->ThrowPanic(Panic::kError, STATE->factory()->nil_error_text());
        return nullptr;
    }
    if (array->length() == 0 || index < 0 || index >= array->length()) {
        Machine::This()->ThrowPanic(Panic::kError, STATE->factory()->out_of_bound_error_text());
        return nullptr;
    }

    SafepointScope safepoint_scope(STATE->gc());
    return *array->Minus(index);
}

template<class T>
static inline AbstractArray *TArrayResize(const Handle<Array<T>> &array, int size) {
    if (array.is_value_null()) {
        Machine::This()->ThrowPanic(Panic::kError, STATE->factory()->nil_error_text());
        return nullptr;
    }
    if (size < 0) {
        Machine::This()->ThrowPanic(Panic::kError, STATE->factory()->out_of_bound_error_text());
        return nullptr;
    }

    SafepointScope safepoint_scope(STATE->gc());
    return Machine::This()->ResizeArray(*array, size, 0/*flags*/);
}

/*static*/ AbstractArray *Runtime::ArrayAppend(Handle<Array<Any *>> array, Handle<Any> value) {
    if (array.is_value_null()) {
        Machine::This()->ThrowPanic(Panic::kError, STATE->factory()->nil_error_text());
        return nullptr;
    }
    SafepointScope safepoint_scope(STATE->gc());
    Array<Any *> *spawn =
        static_cast<Array<Any *> *>(Machine::This()->NewArrayCopied(*array, 1/*increment*/,
                                                                    0/*flags*/));
    spawn->quickly_set(array->length() - 1, *value);
    return spawn;
}

/*static*/ AbstractArray *Runtime::Array8Append(Handle<Array<uint8_t>> array, uint8_t value) {
    return TArrayAppend<uint8_t>(array, value);
}

/*static*/ AbstractArray *Runtime::Array16Append(Handle<Array<uint16_t>> array, uint16_t value) {
    return TArrayAppend<uint16_t>(array, value);
}

/*static*/ AbstractArray *Runtime::Array32Append(Handle<Array<uint32_t>> array, uint32_t value) {
    return TArrayAppend<uint32_t>(array, value);
}

/*static*/ AbstractArray *Runtime::Array64Append(Handle<Array<uint64_t>> array, uint64_t value) {
    return TArrayAppend<uint64_t>(array, value);
}

/*static*/ AbstractArray *Runtime::ArrayPlus(Handle<Array<Any *>> array, int index,
                                             Handle<Any> value) {
    if (array.is_value_null()) {
        Machine::This()->ThrowPanic(Panic::kError, STATE->factory()->nil_error_text());
        return nullptr;
    }
    SafepointScope safepoint_scope(STATE->gc());
    return *array->Plus(index, value);
}

/*static*/ AbstractArray *
Runtime::Array8Plus(Handle<Array<uint8_t>> array, int index, uint8_t value) {
    return TArrayPlus<uint8_t>(array, index, value);
}

/*static*/ AbstractArray *
Runtime::Array16Plus(Handle<Array<uint16_t>> array, int index, uint16_t value) {
    return TArrayPlus<uint16_t>(array, index, value);
}

/*static*/ AbstractArray *
Runtime::Array32Plus(Handle<Array<uint32_t>> array, int index, uint32_t value) {
    return TArrayPlus<uint32_t>(array, index, value);
}

/*static*/ AbstractArray *
Runtime::Array64Plus(Handle<Array<uint64_t>> array, int index, uint64_t value) {
    return TArrayPlus<uint64_t>(array, index, value);
}

/*static*/ AbstractArray *Runtime::ArrayMinus(Handle<Array<Any *>> array, int index) {
    return TArrayMinus<Any *>(array, index);
}

/*static*/ AbstractArray *Runtime::Array8Minus(Handle<Array<uint8_t>> array, int index) {
    return TArrayMinus<uint8_t>(array, index);
}

/*static*/ AbstractArray *Runtime::Array16Minus(Handle<Array<uint16_t>> array, int index) {
    return TArrayMinus<uint16_t>(array, index);
}

/*static*/ AbstractArray *Runtime::Array32Minus(Handle<Array<uint32_t>> array, int index) {
    return TArrayMinus<uint32_t>(array, index);
}

/*static*/ AbstractArray *Runtime::Array64Minus(Handle<Array<uint64_t>> array, int index) {
    return TArrayMinus<uint64_t>(array, index);
}

/*static*/ AbstractArray *Runtime::ArrayResize(Handle<Array<Any *>> array, int size) {
    return TArrayResize<Any *>(array, size);
}

/*static*/ AbstractArray *Runtime::Array8Resize(Handle<Array<uint8_t>> array, int size) {
    return TArrayResize<uint8_t>(array, size);
}

/*static*/ AbstractArray *Runtime::Array16Resize(Handle<Array<uint16_t>> array, int size) {
    return TArrayResize<uint16_t>(array, size);
}

/*static*/ AbstractArray *Runtime::Array32Resize(Handle<Array<uint32_t>> array, int size) {
    return TArrayResize<uint32_t>(array, size);
}

/*static*/ AbstractArray *Runtime::Array64Resize(Handle<Array<uint64_t>> array, int size) {
    return TArrayResize<uint64_t>(array, size);
}

/*static*/ AbstractMap *Runtime::NewMap(const Class *key_type, const Class *value_type,
                                        uint32_t random_seed, uint32_t bucket_size) {
    SafepointScope safepoint_scope(STATE->gc());
    if (bucket_size & 0x80000000) {
        String *message =
            Machine::This()->NewUtf8StringWithFormat(0, "incorrect bucket_size(%d) for map[%s,%s]",
                                                     bucket_size, key_type->name(),
                                                     value_type->name());
        Machine::This()->ThrowPanic(Panic::kError, message);
        return nullptr;
    }
    return Machine::This()->NewMap(key_type->id(), value_type->id(), bucket_size, random_seed, 0);
}

template<class T>
ImplementMap<T> *TMapPutAll(AbstractMap *map, Address start, Address stop) {
    const uint32_t key_size = RoundUp(map->key_type()->reference_size(), kStackSizeGranularity);
    const uint32_t value_size = RoundUp(map->value_type()->reference_size(), kStackSizeGranularity);
    const int length = static_cast<int>(stop - start) / (key_size + value_size);
    HandleScope handle_scope(HandleScope::INITIALIZER);
    Any **keys = nullptr, **values = nullptr;

    if (map->key_type()->is_reference()) {
        keys = reinterpret_cast<Any **>(Machine::This()->AdvanceHandleSlots(length));
        for (int i = 0; i < length; i++) {
            keys[i] = *reinterpret_cast<Any **>(start + i * (key_size + value_size) + value_size);
        }
    }
    if (map->value_type()->is_reference()) {
        values = reinterpret_cast<Any **>(Machine::This()->AdvanceHandleSlots(length));
        for (int i = 0; i < length; i++) {
            values[i] = *reinterpret_cast<Any **>(start + i * (key_size + value_size));
        }
    }
    
    Local<ImplementMap<T>> impl(static_cast<ImplementMap<T> *>(map));
    if (map->value_type()->is_reference()) {
        for (int i = 0; i < length; i++) {
            SafepointScope safepoint_scope(STATE->gc());
            if (ElementTraits<T>::kIsReferenceType) {
                impl = impl->UnsafePut(bit_cast<T>(keys[i]), values[i]);
            } else {
                T key = *reinterpret_cast<T *>(start + i * (key_size + value_size) + value_size);
                impl = impl->UnsafePut(key, values[i]);
            }
        }
    } else if (value_size == 4) {
        for (int i = 0; i < length; i++) {
            SafepointScope safepoint_scope(STATE->gc());
            uint32_t value = *reinterpret_cast<uint32_t *>(start + i * (key_size + value_size));
            if (ElementTraits<T>::kIsReferenceType) {
                impl = impl->UnsafePut(bit_cast<T>(keys[i]), value);
            } else {
                T key = *reinterpret_cast<T *>(start + i * (key_size + value_size) + value_size);
                impl = impl->UnsafePut(key, value);
            }
        }
    } else {
        DCHECK_EQ(8, value_size);
        for (int i = 0; i < length; i++) {
            SafepointScope safepoint_scope(STATE->gc());
            uint64_t value = *reinterpret_cast<uint64_t *>(start + i * (key_size + value_size));
            if (ElementTraits<T>::kIsReferenceType) {
                impl = impl->UnsafePut(bit_cast<T>(keys[i]), value);
            } else {
                T key = *reinterpret_cast<T *>(start + i * (key_size + value_size) + value_size);
                impl = impl->UnsafePut(key, value);
            }
        }
    }
    return *impl;
}

/*static*/ AbstractMap *Runtime::MapPutAll(AbstractMap *map, Address start, Address stop) {
    if (!map) {
        Machine::This()->ThrowPanic(Panic::kError, STATE->factory()->nil_error_text());
        return nullptr;
    }
    if (map->key_type()->is_reference()) {
        if (map->key_type()->id() == kType_string) {
            return TMapPutAll<String *>(map, start, stop);
        } else {
            return TMapPutAll<Any *>(map, start, stop);
        }
    }
    switch (map->key_type()->reference_size()) {
        case 1:
            return TMapPutAll<uint8_t>(map, start, stop);
        case 2:
            return TMapPutAll<uint16_t>(map, start, stop);
        case 4:
            if (map->key_type()->IsFloating()) {
                return TMapPutAll<float>(map, start, stop);
            } else {
                return TMapPutAll<uint32_t>(map, start, stop);
            }
            break;
        case 8:
            if (map->key_type()->IsFloating()) {
                return TMapPutAll<double>(map, start, stop);
            } else {
                return TMapPutAll<uint64_t>(map, start, stop);
            }
            break;
        default:
            NOREACHED();
            break;
    }
    return nullptr;
}

template<class R>
static inline R TMapGet(AbstractMap *map, Any *key) {
    if (!map) {
        Machine::This()->ThrowPanic(Panic::kError, STATE->factory()->nil_error_text());
        return R(0);
    }
    if (!key) {
        return R(0);
    }
    if (map->key_type()->id() == kType_string) {
        ImplementMap<String *> *impl = static_cast<ImplementMap<String *> *>(map);
        if (key->clazz()->id() != kType_string) {
            return 0;
        }
        return impl->UnsafeGet<R>(static_cast<String *>(key));
    } else {
        ImplementMap<Any *> *impl = static_cast<ImplementMap<Any *> *>(map);
        return impl->UnsafeGet<R>(key);
    }
}

/*static*/ uintptr_t Runtime::MapGet(Handle<AbstractMap> map, Handle<Any> key) {
    return TMapGet<uintptr_t>(*map, *key);
}

/*static*/ float Runtime::MapGetF32(Handle<AbstractMap> map, Handle<Any> key) {
    return TMapGet<float>(*map, *key);
}

/*static*/ double Runtime::MapGetF64(Handle<AbstractMap> map, Handle<Any> key) {
    return TMapGet<double>(*map, *key);
}

template<class R, class T>
static inline R TMapXXGet(ImplementMap<T> *map, T key) {
    if (!map) {
        Machine::This()->ThrowPanic(Panic::kError, STATE->factory()->nil_error_text());
        return 0;
    }
    return map->template UnsafeGet<R>(key);
}

/*static*/ uintptr_t Runtime::Map8Get(Handle<ImplementMap<uint8_t>> map, uint8_t key) {
    return TMapXXGet<uintptr_t>(*map, key);
}

/*static*/ float Runtime::Map8GetF32(Handle<ImplementMap<uint8_t>> map, uint8_t key) {
    return TMapXXGet<float>(*map, key);
}

/*static*/ double Runtime::Map8GetF64(Handle<ImplementMap<uint8_t>> map, uint8_t key) {
    return TMapXXGet<double>(*map, key);
}

/*static*/ uintptr_t Runtime::Map16Get(Handle<ImplementMap<uint16_t>> map, uint16_t key) {
    return TMapXXGet<uintptr_t>(*map, key);
}

/*static*/ float Runtime::Map16GetF32(Handle<ImplementMap<uint16_t>> map, uint16_t key) {
    return TMapXXGet<float>(*map, key);
}

/*static*/ double Runtime::Map16GetF64(Handle<ImplementMap<uint16_t>> map, uint16_t key) {
    return TMapXXGet<double>(*map, key);
}

/*static*/ uintptr_t Runtime::Map32Get(Handle<AbstractMap> map, uint32_t key) {
    if (map.is_value_not_null() && map->key_type()->IsFloating()) {
        return TMapXXGet<uintptr_t>(static_cast<ImplementMap<float> *>(*map), bit_cast<float>(key));
    } else {
        return TMapXXGet<uintptr_t>(static_cast<ImplementMap<uint32_t> *>(*map), key);
    }
}

/*static*/ float Runtime::Map32GetF32(Handle<AbstractMap> map, uint32_t key) {
    if (map.is_value_not_null() && map->key_type()->IsFloating()) {
        return TMapXXGet<float>(static_cast<ImplementMap<float> *>(*map), bit_cast<float>(key));
    } else {
        return TMapXXGet<float>(static_cast<ImplementMap<uint32_t> *>(*map), key);
    }
}

/*static*/ double Runtime::Map32GetF64(Handle<AbstractMap> map, uint32_t key) {
    if (map.is_value_not_null() && map->key_type()->IsFloating()) {
        return TMapXXGet<double>(static_cast<ImplementMap<float> *>(*map), bit_cast<float>(key));
    } else {
        return TMapXXGet<double>(static_cast<ImplementMap<uint32_t> *>(*map), key);
    }
}

/*static*/ uintptr_t Runtime::Map64Get(Handle<AbstractMap> map, uint64_t key) {
    if (map.is_value_not_null() && map->key_type()->IsFloating()) {
        return TMapXXGet<uintptr_t>(static_cast<ImplementMap<double> *>(*map), bit_cast<double>(key));
    } else {
        return TMapXXGet<uintptr_t>(static_cast<ImplementMap<uint64_t> *>(*map), key);
    }
}

/*static*/ float Runtime::Map64GetF32(Handle<AbstractMap> map, uint64_t key) {
    if (map.is_value_not_null() && map->key_type()->IsFloating()) {
        return TMapXXGet<float>(static_cast<ImplementMap<double> *>(*map), bit_cast<double>(key));
    } else {
        return TMapXXGet<float>(static_cast<ImplementMap<uint64_t> *>(*map), key);
    }
}

/*static*/ double Runtime::Map64GetF64(Handle<AbstractMap> map, uint64_t key) {
    if (map.is_value_not_null() && map->key_type()->IsFloating()) {
        return TMapXXGet<double>(static_cast<ImplementMap<double> *>(*map), bit_cast<double>(key));
    } else {
        return TMapXXGet<double>(static_cast<ImplementMap<uint64_t> *>(*map), key);
    }
}

template<class T>
static inline int TMapContainsKey(ImplementMap<T> *map, T key) {
    if (!map) {
        Machine::This()->ThrowPanic(Panic::kError, STATE->factory()->nil_error_text());
        return 0;
    }
    return map->ContainsKey(key);
}

/*static*/ int Runtime::MapContainsKey(Handle<AbstractMap> map, Handle<Any> key) {
    if (key.is_value_null()) {
        return 0;
    }
    if (map->key_type()->id() == kType_string) {
        return TMapContainsKey(static_cast<ImplementMap<String *> *>(*map),
                               static_cast<String *>(*key));
    } else {
        return TMapContainsKey(static_cast<ImplementMap<Any *> *>(*map), *key);
    }
}

/*static*/ int Runtime::Map8ContainsKey(Handle<ImplementMap<uint8_t>> map, uint8_t key) {
    return TMapContainsKey(*map, key);
}

/*static*/ int Runtime::Map16ContainsKey(Handle<ImplementMap<uint16_t>> map, uint16_t key) {
    return TMapContainsKey(*map, key);
}

/*static*/ int Runtime::Map32ContainsKey(Handle<AbstractMap> map, uint32_t key) {
    if (map.is_value_not_null() && map->value_type()->IsFloating()) {
        return TMapContainsKey(static_cast<ImplementMap<float> *>(*map), bit_cast<float>(key));
    } else {
        return TMapContainsKey(static_cast<ImplementMap<uint32_t> *>(*map), key);
    }
}

/*static*/ int Runtime::Map64ContainsKey(Handle<AbstractMap> map, uint64_t key) {
    if (map.is_value_not_null() && map->value_type()->IsFloating()) {
        return TMapContainsKey(static_cast<ImplementMap<double> *>(*map), bit_cast<double>(key));
    } else {
        return TMapContainsKey(static_cast<ImplementMap<uint64_t> *>(*map), key);
    }
}

template<class K, class V>
static inline void TMapSet(ImplementMap<K> *map, K key, V value) {
    uintptr_t data = 0;
    if (map->value_type()->reference_size() < 8) {
        data = reinterpret_cast<uintptr_t>(value) >> 32;
    } else {
        data = reinterpret_cast<uintptr_t>(value);
    }
    map->UnsafeSet(key, data);
}

/*static*/ void Runtime::MapSet(Handle<AbstractMap> map, Handle<Any> key, uintptr_t value) {
    if (key.is_value_null()) {
        return;
    }
    if (map->key_type()->id() == kType_string) {
        TMapSet(static_cast<ImplementMap<String *> *>(*map), static_cast<String *>(*key), value);
    } else {
        TMapSet(static_cast<ImplementMap<Any *> *>(*map), *key, value);
    }
}

/*static*/ void Runtime::Map8Set(Handle<ImplementMap<uint8_t>> map, uint8_t key, uintptr_t value) {
    TMapSet(*map, key, value);
}

/*static*/
void Runtime::Map16Set(Handle<ImplementMap<uint16_t>> map, uint16_t key, uintptr_t value) {
    TMapSet(*map, key, value);
}

/*static*/ void Runtime::Map32Set(Handle<AbstractMap> map, uint32_t key, uintptr_t value) {
    if (map->key_type()->IsFloating()) {
        TMapSet(static_cast<ImplementMap<float> *>(*map), bit_cast<float>(key), value);
    } else {
        TMapSet(static_cast<ImplementMap<uint32_t> *>(*map), key, value);
    }
}

/*static*/ void Runtime::Map64Set(Handle<AbstractMap> map, uint64_t key, uintptr_t value) {
    if (map->key_type()->IsFloating()) {
        TMapSet(static_cast<ImplementMap<double> *>(*map), bit_cast<double>(key), value);
    } else {
        TMapSet(static_cast<ImplementMap<uint64_t> *>(*map), key, value);
    }
}

template<class K, class U>
static inline ImplementMap<K> *TMapOops(const Handle<ImplementMap<K>> &map, K key, uintptr_t value,
                                        U oops) {
    DCHECK(map.is_value_not_null());

    if (map->value_type()->is_reference()) {
        HandleScope handle_scope(HandleScope::INITIALIZER);
        Local<Any> handle(reinterpret_cast<Any *>(value));

        SafepointScope safepoint_scope(STATE->gc());
        return oops(map, key, reinterpret_cast<uintptr_t>(*handle));
    } else {
        SafepointScope safepoint_scope(STATE->gc());
        if (map->value_type()->reference_size() < 8) {
            value >>= 32;
        }
        return oops(map, key, value);
    }
}

template<class K>
static inline ImplementMap<K> *TMapPut(const Handle<ImplementMap<K>> &map, K key, uintptr_t value) {
    return TMapOops(map, key, value, [](const auto &map, K key, uintptr_t value) {
        return map->UnsafePut(key, value);
    });
}

/*static*/ AbstractMap *Runtime::MapPut(Handle<AbstractMap> map, Handle<Any> key, uintptr_t value) {
    if (map.is_value_null()) {
        Machine::This()->ThrowPanic(Panic::kError, STATE->factory()->nil_error_text());
        return nullptr;
    }
    if (key.is_value_null()) {
        return *map;
    }

    if (map->value_type()->is_reference()) {
        HandleScope handle_scope(HandleScope::INITIALIZER);
        Local<Any> handle(reinterpret_cast<Any *>(value));

        SafepointScope safepoint_scope(STATE->gc());
        if (map->key_type()->id() == kType_string) {
            return Handle<ImplementMap<String *>>::Cast(map)->UnsafePut(static_cast<String *>(*key),
                                                                        *handle);
        } else {
            return Handle<ImplementMap<Any *>>::Cast(map)->UnsafePut(*key, *handle);
        }
    } else {
        SafepointScope safepoint_scope(STATE->gc());
        if (map->value_type()->reference_size() < 8) {
            value >>= 32;
        }
        if (map->key_type()->id() == kType_string) {
            return Handle<ImplementMap<String *>>::Cast(map)->UnsafePut(static_cast<String *>(*key),
                                                                        value);
        } else {
            return Handle<ImplementMap<Any *>>::Cast(map)->UnsafePut(*key, value);
        }
    }
}

/*static*/ AbstractMap *
Runtime::Map8Put(Handle<ImplementMap<uint8_t>> map, uint8_t key, uintptr_t value) {
    return TMapPut(map, key, value);
}

/*static*/ AbstractMap *
Runtime::Map16Put(Handle<ImplementMap<uint16_t>> map, uint16_t key, uintptr_t value) {
    return TMapPut(map, key, value);
}

/*static*/ AbstractMap *Runtime::Map32Put(Handle<AbstractMap> map, uint32_t key, uintptr_t value) {
    if (map->key_type()->IsFloating()) {
        return TMapPut(Handle<ImplementMap<float>>::Cast(map), bit_cast<float>(key), value);
    } else {
        return TMapPut(Handle<ImplementMap<uint32_t>>::Cast(map), key, value);
    }
}

/*static*/ AbstractMap *Runtime::Map64Put(Handle<AbstractMap> map, uint64_t key, uintptr_t value) {
    if (map->key_type()->IsFloating()) {
        return TMapPut(Handle<ImplementMap<double>>::Cast(map), bit_cast<double>(key), value);
    } else {
        return TMapPut(Handle<ImplementMap<uint64_t>>::Cast(map), key, value);
    }
}

/*static*/ AbstractMap *Runtime::MapRemove(Handle<AbstractMap> map, Handle<Any> key) {
    if (key.is_value_null()) {
        return *map;
    }

    SafepointScope safepoint_scope(STATE->gc());
    if (map->key_type()->id() == kType_string) {
        return static_cast<ImplementMap<String *> *>(*map)->UnsafeRemove(static_cast<String *>(*key));
    } else {
        return static_cast<ImplementMap<Any *> *>(*map)->UnsafeRemove(*key);
    }
}

/*static*/ AbstractMap *Runtime::Map8Remove(Handle<ImplementMap<uint8_t>> map, uint8_t key) {
    if (map.is_value_null()) {
        Machine::This()->ThrowPanic(Panic::kError, STATE->factory()->nil_error_text());
        return nullptr;
    }
    SafepointScope safepoint_scope(STATE->gc());
    return map->UnsafeRemove(key);
}

/*static*/ AbstractMap *Runtime::Map16Remove(Handle<ImplementMap<uint16_t>> map, uint16_t key) {
    if (map.is_value_null()) {
        Machine::This()->ThrowPanic(Panic::kError, STATE->factory()->nil_error_text());
        return nullptr;
    }
    SafepointScope safepoint_scope(STATE->gc());
    return map->UnsafeRemove(key);
}

/*static*/ AbstractMap *Runtime::Map32Remove(Handle<AbstractMap> map, uint32_t key) {
    if (map.is_value_null()) {
        Machine::This()->ThrowPanic(Panic::kError, STATE->factory()->nil_error_text());
        return nullptr;
    }
    SafepointScope safepoint_scope(STATE->gc());
    if (map->key_type()->IsFloating()) {
        return static_cast<ImplementMap<float> *>(*map)->UnsafeRemove(bit_cast<float>(key));
    } else {
        return static_cast<ImplementMap<uint32_t> *>(*map)->UnsafeRemove(key);
    }
}

/*static*/ AbstractMap *Runtime::Map64Remove(Handle<AbstractMap> map, uint64_t key) {
    if (map.is_value_null()) {
        Machine::This()->ThrowPanic(Panic::kError, STATE->factory()->nil_error_text());
        return nullptr;
    }
    SafepointScope safepoint_scope(STATE->gc());
    if (map->key_type()->IsFloating()) {
        return static_cast<ImplementMap<double> *>(*map)->UnsafeRemove(bit_cast<double>(key));
    } else {
        return static_cast<ImplementMap<uint64_t> *>(*map)->UnsafeRemove(key);
    }
}

/*static*/
AbstractMap *Runtime::MapPlus(Handle<AbstractMap> map, Handle<Any> key, uintptr_t value) {
    DCHECK(map.is_value_not_null());
    if (key.is_value_null()) {
        return *map;
    }
    SafepointScope safepoint_scope(STATE->gc());
    if (map->value_type()->reference_size() < kPointerSize) {
        value >>= 32;
    }
    if (map->key_type()->id() == kType_string) {
        return Handle<ImplementMap<String *>>::Cast(map)->UnsafePlus(static_cast<String *>(*key),
                                                                     value);
    } else {
        return Handle<ImplementMap<Any *>>::Cast(map)->UnsafePlus(*key, value);
    }
}

/*static*/
AbstractMap *Runtime::Map8Plus(Handle<ImplementMap<uint8_t>> map, uint8_t key, uintptr_t value) {
    SafepointScope safepoint_scope(STATE->gc());
    if (map->value_type()->reference_size() < kPointerSize) {
        value >>= 32;
    }
    return map->UnsafePlus(key, value);
}

/*static*/
AbstractMap *Runtime::Map16Plus(Handle<ImplementMap<uint16_t>> map, uint16_t key, uintptr_t value) {
    SafepointScope safepoint_scope(STATE->gc());
    if (map->value_type()->reference_size() < kPointerSize) {
        value >>= 32;
    }
    return map->UnsafePlus(key, value);
}

/*static*/ AbstractMap *Runtime::Map32Plus(Handle<AbstractMap> map, uint32_t key, uintptr_t value) {
    SafepointScope safepoint_scope(STATE->gc());
    if (map->value_type()->reference_size() < kPointerSize) {
        value >>= 32;
    }
    if (map->key_type()->IsFloating()) {
        return Handle<ImplementMap<float>>::Cast(map)->UnsafePlus(bit_cast<float>(key), value);
    } else {
        return Handle<ImplementMap<uint32_t>>::Cast(map)->UnsafePlus(key, value);
    }
}

/*static*/ AbstractMap *Runtime::Map64Plus(Handle<AbstractMap> map, uint64_t key, uintptr_t value) {
    SafepointScope safepoint_scope(STATE->gc());
    if (map->value_type()->reference_size() < kPointerSize) {
        value >>= 32;
    }
    if (map->key_type()->IsFloating()) {
        return Handle<ImplementMap<double>>::Cast(map)->UnsafePlus(bit_cast<double>(key), value);
    } else {
        return Handle<ImplementMap<uint64_t>>::Cast(map)->UnsafePlus(key, value);
    }
}

/*static*/ AbstractMap *Runtime::MapPlusAny(Handle<AbstractMap> map, Handle<Any> key,
                                            Handle<Any> value) {
    if (key.is_value_null()) {
        return *map;
    }
    SafepointScope safepoint_scope(STATE->gc());
    if (map->key_type()->id() == kType_string) {
        return Handle<ImplementMap<String *>>::Cast(map)->UnsafePlus(static_cast<String *>(*key),
                                                                     *value);
    } else {
        return Handle<ImplementMap<Any *>>::Cast(map)->UnsafePlus(*key, *value);
    }
}

/*static*/ AbstractMap *Runtime::Map8PlusAny(Handle<ImplementMap<uint8_t>> map, uint8_t key,
                                             Handle<Any> value) {
    SafepointScope safepoint_scope(STATE->gc());
    return map->UnsafePlus(key, *value);
}

/*static*/ AbstractMap *Runtime::Map16PlusAny(Handle<ImplementMap<uint16_t>> map, uint16_t key,
                                              Handle<Any> value) {
    SafepointScope safepoint_scope(STATE->gc());
    return map->UnsafePlus(key, *value);
}

/*static*/ AbstractMap *Runtime::Map32PlusAny(Handle<AbstractMap> map, uint32_t key,
                                              Handle<Any> value) {
    SafepointScope safepoint_scope(STATE->gc());
    if (map->key_type()->IsFloating()) {
        return Handle<ImplementMap<float>>::Cast(map)->UnsafePlus(bit_cast<float>(key), *value);
    } else {
        return Handle<ImplementMap<uint32_t>>::Cast(map)->UnsafePlus(key, *value);
    }
}

/*static*/ AbstractMap *Runtime::Map64PlusAny(Handle<AbstractMap> map, uint64_t key,
                                              Handle<Any> value) {
    SafepointScope safepoint_scope(STATE->gc());
    if (map->key_type()->IsFloating()) {
        return Handle<ImplementMap<double>>::Cast(map)->UnsafePlus(bit_cast<double>(key), *value);
    } else {
        return Handle<ImplementMap<uint64_t>>::Cast(map)->UnsafePlus(key, *value);
    }
}

/*static*/ AbstractMap *Runtime::MapMinus(Handle<AbstractMap> map, Handle<Any> key) {
    SafepointScope safepoint_scope(STATE->gc());
    if (map->key_type()->id() == kType_string) {
        return Handle<ImplementMap<String *>>::Cast(map)->UnsafeMinus(static_cast<String *>(*key));
    } else {
        return Handle<ImplementMap<Any *>>::Cast(map)->UnsafeMinus(*key);
    }
}

/*static*/ AbstractMap *Runtime::Map8Minus(Handle<ImplementMap<uint8_t>> map, uint8_t key) {
    SafepointScope safepoint_scope(STATE->gc());
    return map->UnsafeMinus(key);
}

/*static*/ AbstractMap *Runtime::Map16Minus(Handle<ImplementMap<uint16_t>> map, uint16_t key) {
    SafepointScope safepoint_scope(STATE->gc());
    return map->UnsafeMinus(key);
}

/*static*/ AbstractMap *Runtime::Map32Minus(Handle<AbstractMap> map, uint32_t key) {
    SafepointScope safepoint_scope(STATE->gc());
    if (map->key_type()->id() == kType_string) {
        return Handle<ImplementMap<float>>::Cast(map)->UnsafeMinus(bit_cast<float>(key));
    } else {
        return Handle<ImplementMap<uint32_t>>::Cast(map)->UnsafeMinus(key);
    }
}

/*static*/ AbstractMap *Runtime::Map64Minus(Handle<AbstractMap> map, uint64_t key) {
    SafepointScope safepoint_scope(STATE->gc());
    if (map->key_type()->id() == kType_string) {
        return Handle<ImplementMap<double>>::Cast(map)->UnsafeMinus(bit_cast<double>(key));
    } else {
        return Handle<ImplementMap<uint64_t>>::Cast(map)->UnsafeMinus(key);
    }
}

union EntryStruct {
    uint64_t packed;
    struct {
        uint32_t lo;
        uint32_t hi;
    } unpacked;
};

template<class T>
uint64_t TMapNext(const Handle<ImplementMap<T>> &map, uint64_t iter) {
    EntryStruct entry;
    if (iter != 0) {
        entry.packed = iter;
        map->FindNextEntry(&entry.unpacked.hi, &entry.unpacked.lo);
        return entry.packed;
    }
    
    entry.unpacked.hi = 1;
    entry.unpacked.lo = 0;
    map->FindNextBucket(&entry.unpacked.hi, &entry.unpacked.lo);
    return entry.packed;
}

/*static*/ uint64_t Runtime::MapNext(Handle<ImplementMap<Any*>> map, uint64_t iter) {
    return TMapNext<Any*>(map, iter);
}

/*static*/ uint64_t Runtime::Map8Next(Handle<ImplementMap<uint8_t>> map, uint64_t iter) {
    return TMapNext<uint8_t>(map, iter);
}

/*static*/ uint64_t Runtime::Map16Next(Handle<ImplementMap<uint16_t>> map, uint64_t iter) {
    return TMapNext<uint16_t>(map, iter);
}

/*static*/ uint64_t Runtime::Map32Next(Handle<ImplementMap<uint32_t>> map, uint64_t iter) {
    return TMapNext<uint32_t>(map, iter);
}

/*static*/ uint64_t Runtime::Map64Next(Handle<ImplementMap<uint64_t>> map, uint64_t iter) {
    return TMapNext<uint64_t>(map, iter);
}

static inline const uintptr_t TMapValue(const Handle<AbstractMap> &map, uint64_t iter) {
    EntryStruct entry;
    entry.packed = iter;
    switch (map->key_type()->reference_size()) {
        case 1:
            return Handle<ImplementMap<uint8_t>>::Cast(map)->GetEntry(entry.unpacked.lo)->value;
        case 2:
            return Handle<ImplementMap<uint16_t>>::Cast(map)->GetEntry(entry.unpacked.lo)->value;
        case 4:
            return Handle<ImplementMap<uint32_t>>::Cast(map)->GetEntry(entry.unpacked.lo)->value;
        case 8:
            return Handle<ImplementMap<uint64_t>>::Cast(map)->GetEntry(entry.unpacked.lo)->value;
        default:
            NOREACHED();
            return 0;
    }
}

/*static*/ uintptr_t Runtime::MapValue(Handle<AbstractMap> map, uint64_t iter) {
    return TMapValue(map, iter);
}

/*static*/ float Runtime::MapValueF32(Handle<AbstractMap> map, uint64_t iter) {
    return bit_cast<float>(TMapValue(map, iter));
}

/*static*/ double Runtime::MapValueF64(Handle<AbstractMap> map, uint64_t iter) {
    return bit_cast<double>(TMapValue(map, iter));
}

/*static*/ uintptr_t Runtime::MapKey(Handle<AbstractMap> map, uint64_t iter) {
    EntryStruct entry;
    entry.packed = iter;
    switch (map->key_type()->reference_size()) {
        case 1:
            return Handle<ImplementMap<uint8_t>>::Cast(map)->GetEntry(entry.unpacked.lo)->key;
        case 2:
            return Handle<ImplementMap<uint16_t>>::Cast(map)->GetEntry(entry.unpacked.lo)->key;
        case 4:
            return Handle<ImplementMap<uint32_t>>::Cast(map)->GetEntry(entry.unpacked.lo)->key;
        case 8:
            return Handle<ImplementMap<uint64_t>>::Cast(map)->GetEntry(entry.unpacked.lo)->key;
        default:
            NOREACHED();
            return 0;
    }
}

/*static*/ float Runtime::MapKeyF32(Handle<ImplementMap<float>> map, uint64_t iter) {
    EntryStruct entry;
    entry.packed = iter;
    return map->GetEntry(entry.unpacked.lo)->key;
}

/*static*/ double Runtime::MapKeyF64(Handle<ImplementMap<double>> map, uint64_t iter) {
    EntryStruct entry;
    entry.packed = iter;
    return map->GetEntry(entry.unpacked.lo)->key;
}

static bool TestIs(const Class *dest, void *param, Any *any, bool strict) {
    switch (static_cast<BuiltinType>(dest->id())) {
        case kType_array:
        case kType_array8:
        case kType_array16:
        case kType_array32:
        case kType_array64: {
            if (!any->clazz()->IsArray()) {
                return false;
            }
            const AbstractArray *array = static_cast<AbstractArray *>(any);
            const Class *expect = static_cast<const Class *>(param);
            if (strict) {
                return array->elem_type()->IsSameOrBaseOf(expect);
            }
            if (expect->IsIntegral()) {
                return array->elem_type()->IsIntegral() &&
                       array->elem_type()->reference_size() == expect->reference_size();
            }
            if (expect->IsFloating()) {
                return array->elem_type()->IsFloating() &&
                       array->elem_type()->reference_size() == expect->reference_size();
            }
            return array->elem_type()->IsSameOrBaseOf(expect);
        } break;

        case kType_channel: {
            if (any->clazz()->id() != kType_channel) {
                return false;
            }
            const Channel *chan = static_cast<Channel *>(any);
            const Class *expect = static_cast<const Class *>(param);
            return chan->data_type()->IsSameOrBaseOf(expect);
        } break;

        case kType_closure: {
            if (any->clazz()->id() != kType_closure) {
                return false;
            }
            const Closure *fun = static_cast<Closure *>(any);
            const PrototypeDesc *expect = static_cast<const PrototypeDesc *>(param);
            const PrototypeDesc *proto = nullptr;
            if (fun->is_cxx_function()) {
                proto = fun->code()->prototype();
            } else {
                proto = fun->function()->prototype();
            }
            return proto == expect;
        } break;

        default:
            NOREACHED() << dest->name();
            break;
    }
    return false;
}

/*static*/ Any *Runtime::TestAs(Handle<Any> any, void *param1, void *param2) {
    if (any.is_value_null()) {
        Machine::This()->ThrowPanic(Panic::kError, STATE->factory()->nil_error_text());
        return nullptr;
    }
    const Class *dest = static_cast<const Class *>(param1);
    if (!lang::TestIs(dest, param2, *any, false/*strict*/)) {
        String *text =Machine::This()->NewUtf8StringWithFormat(0, "Bad cast from %s to %s",
                                                               any->clazz()->name(), dest->name());
        Machine::This()->ThrowPanic(Panic::kError, text);
        return nullptr;
    }
    return *any;
}

/*static*/ int Runtime::TestIs(Handle<Any> any, void *param1, void *param2) {
    return lang::TestIs(static_cast<const Class *>(param1), param2, *any, true/*strict*/);
}

/*static*/ int Runtime::IsSameOrBaseOf(const Any *any, const Class *type) {
    return DCHECK_NOTNULL(any)->clazz()->IsSameOrBaseOf(type);
}

/*static*/ int Runtime::StringCompareFallback(const String *lhs, const String *rhs) {
    const uint32_t min_len = std::min(lhs->length(), rhs->length());
    int r = ::memcmp(lhs->data(), rhs->data(), min_len);
    if (r == 0) {
        if (lhs->length() < rhs->length()) {
            r = -1;
        }
        else if (lhs->length() > rhs->length()) {
            r = +1;
        }
    }
    return r;
}

/*static*/ Address *Runtime::TryTracing(Function *fun, int32_t slot, int32_t pc, Tracer **receiver) {
    DCHECK(fun != nullptr);
    DCHECK_GE(slot, 0);
    DCHECK_GE(pc, 0);
    
    Tracer *tracer = DCHECK_NOTNULL(Machine::This()->tracing());
    tracer->Start(fun, slot, pc);
    if (TracingHook *hook = STATE->tracing_hook()) {
        hook->DidStart(tracer);
    }
    
    STATE->profiler()->Restore(slot, 2);
    
    *receiver = tracer;
    return STATE->tracing_handler_entries();
}

/*static*/ Address *Runtime::FinalizeTracing(int **slots) {
    Tracer *tracer = DCHECK_NOTNULL(Machine::This()->tracing());
    DCHECK_EQ(tracer->state(), Tracer::kPending);
    STATE->profiler()->Restore(tracer->guard_slot(), 2);
    tracer->Finalize();

    std::unique_ptr<CompilationInfo> info(tracer->MakeCompilationInfo(Machine::This(),
                                                                      Coroutine::This()));
    if (TracingHook *hook = STATE->tracing_hook()) {
        hook->DidFinailize(tracer, info.get());
    }
    
//    base::StdFilePrinter printer(stdout);
//    info->Print(&printer);
    
    Compiler::PostTracingBasedJob(STATE, info.release(), 1, false/*enable_debug*/,
                                  true/*enable_jit*/);

    *slots = STATE->profiler()->hot_count_slots();
    return STATE->bytecode_handler_entries();
}

/*static*/ Address *Runtime::AbortTracing(int **slots) {
    Tracer *tracer = DCHECK_NOTNULL(Machine::This()->tracing());
    tracer->Abort();
    if (TracingHook *hook = STATE->tracing_hook()) {
        hook->DidAbort(tracer);
    }

    *slots = STATE->profiler()->hot_count_slots();
    return STATE->bytecode_handler_entries();
}

/*static*/ void Runtime::RepeatTracing() {
    Tracer *tracer = DCHECK_NOTNULL(Machine::This()->tracing());
    tracer->Repeat();
    if (TracingHook *hook = STATE->tracing_hook()) {
        hook->DidRepeat(tracer);
    }
}

/*static*/ void Runtime::TraceInvoke(Function *fun, int32_t slot) {
    DCHECK_NOTNULL(Machine::This()->tracing())->Invoke(fun, slot);
}

/*static*/ Closure *Runtime::LoadVtableFunction(Any *host, String *name) {
    if (!host) {
        Machine::This()->ThrowPanic(Panic::kError, STATE->factory()->nil_error_text());
        return nullptr;
    }
    DCHECK(name != nullptr);
    DCHECK_EQ(name->clazz(), STATE->builtin_type(kType_string));
    // TODO: Inline cache

    auto [clazz, method] = STATE->metadata_space()->FindClassMethod(host->clazz(), name->data());
    return !method ? nullptr : method->fn();
}

/*static*/ Tracer *Runtime::GrowTracingPath() {
    DCHECK(STATE->enable_jit());
    Tracer *tracer = DCHECK_NOTNULL(Machine::This()->tracing());
    tracer->GrowTracingPath();
    return tracer;
}

/*static*/ Any *Runtime::WriteBarrierWithOffset(Any *host, int32_t offset) {
    DCHECK(!STATE->heap()->InNewArea(host));
    DCHECK_GE(offset, sizeof(Any));
    Address address = reinterpret_cast<Address>(host) + offset;
    Machine::This()->UpdateRememberRecords(host, reinterpret_cast<Any **>(address), 1);
    return host;
}

/*static*/ Any *Runtime::WriteBarrierWithAddress(Any *host, Any **address) {
    DCHECK(!STATE->heap()->InNewArea(host));
    Machine::This()->UpdateRememberRecords(host, address, 1);
    return host;
}

/*static*/ Coroutine *Runtime::RunCoroutine(uint32_t flags, Closure *entry_point, Address params,
                                            uint32_t params_bytes_size) {
    if (!entry_point) {
        Machine::This()->ThrowPanic(Panic::kError, STATE->factory()->nil_error_text());
        return nullptr;
    }
    if (STATE->scheduler()->shutting_down() > 0) {
        // Is shutting-donw, Ignore coroutine creation
        Coroutine::This()->Yield();
        return nullptr;
    }

    Coroutine *co = STATE->scheduler()->NewCoroutine(entry_point, false/*co0*/);
    if (!co) {
        Machine::This()->ThrowPanic(Panic::kError, STATE->factory()->new_coroutine_error_text());
        return nullptr;
    }
    
    // Prepare coroutine
    co->SwitchState(Coroutine::kDead, Coroutine::kRunnable);
    co->CopyArgv(params, params_bytes_size);

    STATE->scheduler()->PostRunnableBalanced(co, true/*now*/);
    return co;
}

/*static*/ Throwable *Runtime::NewNilPointerPanic() {
    return Machine::This()->NewPanic(Panic::kFatal, STATE->factory()->nil_error_text(), 0);
}

/*static*/ Throwable *Runtime::NewStackoverflowPanic() {
    return Machine::This()->NewPanic(Panic::kFatal, STATE->factory()->stack_overflow_error_text(),
                                     0);
}

/*static*/ Throwable *Runtime::NewOutOfBoundPanic() {
    return Machine::This()->NewPanic(Panic::kFatal, STATE->factory()->out_of_bound_error_text(), 0);
}

/*static*/ Throwable *Runtime::NewArithmeticPanic() {
    return Machine::This()->NewPanic(Panic::kFatal, STATE->factory()->arithmetic_text(), 0);
}

/*static*/ Throwable *Runtime::NewBadCastPanic(const Class *lhs, const Class *rhs) {
    HandleScope handle_scope(HandleScope::INITIALIZER);
    Local<String> text(Machine::This()->NewUtf8StringWithFormat(0, "Bad cast from %s to %s",
                                                                lhs->name(), rhs->name()));
    return Machine::This()->NewPanic(Panic::kFatal, *text, 0);
}

/*static*/ Throwable *Runtime::MakeStacktrace(Throwable *expect) {
    Address frame_bp = TLS_STORAGE->coroutine->bp1();
    Array<String *> *stackstrace = Throwable::MakeStacktrace(frame_bp);
    if (!stackstrace) {
        return nullptr;
    }
    DCHECK_NOTNULL(expect)->QuickSetStacktrace(stackstrace);
    return expect;
}

/*static*/ Closure *Runtime::CloseFunction(Function *func, uint32_t flags) {
    return Machine::This()->CloseFunction(func, flags);
}

/*static*/ void Runtime::DebugAbort(const char *message) {
    Machine *m = Machine::This();
    Coroutine *co = m->running();
    uint64_t coid = !co ? -1 : co->coid();

    if (co) {
        co->set_state(Coroutine::kPanic);
    }
#if defined(DEBUG) || defined(_DEBUG)
    DLOG(FATAL) << "‼️ Coroutine ABORT:\n    ☠️M:" << m->id() << ":C:" << coid << message;
#else
    fprintf(stdout, "‼️ Coroutine ABORT:\n    ☠️M:%dC:%" PRIu64 ":%s", m->id(), coid, message);
    abort();
#endif
}

/*static*/ int Runtime::Near32(float a, float b, float abs) {
    return ::fabs(a - b) <= ::fabs(abs);
}

/*static*/ int Runtime::Near64(double a, double b, double abs) {
    return ::fabs(a - b) <= ::fabs(abs);
}

/*static*/ void Runtime::Println(Handle<String> input) {
    // TODO:
    ::fwrite(input->data(), 1, input->length(), stdout);
    ::puts("");
}

/*static*/ void Runtime::Assert(int expect, Handle<String> message) {
    if (expect) { return; }
    HandleScope handle_scope(HandleScope::INITIALIZER);
    IncrementalStringBuilder builder;
    builder.AppendString("Assert fail: ");
    if (message.is_value_not_null()) {
        builder.AppendString(message);
    } else {
        builder.AppendString("\"\"");
    }
    Machine::This()->ThrowPanic(Panic::kFatal, builder.QuickBuild());
}

/*static*/ void Runtime::Abort(Handle<String> message) {
    Machine::This()->ThrowPanic(Panic::kError, DCHECK_NOTNULL(*message));
}

/*static*/ void Runtime::Fatal(Handle<String> message) {
    Machine::This()->ThrowPanic(Panic::kFatal, DCHECK_NOTNULL(*message));
}

/*static*/ int Runtime::Object_HashCode(Handle<Any> any) {
    if (any.is_value_null()) {
        Machine::This()->ThrowPanic(Panic::kError, STATE->factory()->nil_error_text());
        return -1;
    }
    return static_cast<int>(bit_cast<intptr_t>(*any) >> 2) | 0x1;
}

/*static*/ String *Runtime::Object_ToString(Handle<Any> any) {
    HandleScope handle_scope(HandleScope::INITIALIZER);
    if (any.is_value_null()) {
        return Machine::This()->NewUtf8String("nil", 3, 0);
    } else {
        return static_cast<Object *>(*any)->ToString();
    }
}

/*static*/ void Runtime::Exception_PrintStackstrace(Handle<Any> /*any*/) {
    // TODO:
}

/*static*/ int64_t Runtime::System_CurrentTimeMillis(Handle<Any> /*any*/) {
    return STATE->env()->CurrentTimeMicros() / 1000L;
}

/*static*/ int64_t Runtime::System_MicroTime(Handle<Any> /*any*/) {
    return STATE->env()->CurrentTimeMicros();
}

/*static*/ void Runtime::System_GC(Handle<Any> /*any*/) { MinorGC(); }

// std::mutex mutex;

/*static*/ Any *Runtime::WaitGroup_Init(Handle<Any> self) {
    DCHECK(!::strcmp("lang.WaitGroup", self->clazz()->name()));
    const Field *number_of_works = self->clazz()->field(0);
    DCHECK(!::strcmp("numberOfWorks", number_of_works->name()));
    const Field *mutex = self->clazz()->field(1);
    DCHECK(!::strcmp("mutex", mutex->name()));
    const Field *request = self->clazz()->field(2);
    DCHECK(!::strcmp("requestDummy", request->name()));
    WaittingRequest *rq = self->UnsafeAccess<WaittingRequest>(request);
    rq->next_ = rq;
    rq->prev_ = rq;
    return *self;
}

/*static*/ void Runtime::WaitGroup_Add(Handle<Any> self, int n) {
    DCHECK(!::strcmp("lang.WaitGroup", self->clazz()->name()));
    const Field *number_of_works_field = self->clazz()->field(0);
    DCHECK(!::strcmp("numberOfWorks", number_of_works_field->name()));
    const Field *mutex_field = self->clazz()->field(1);
    DCHECK(!::strcmp("mutex", mutex_field->name()));
    base::SpinMutex *mutex = self->UnsafeAccess<base::SpinMutex>(mutex_field);
    base::SpinLock lock(mutex);
    int *works = self->UnsafeAccess<int>(number_of_works_field);
    (*works) += n;
}

/*static*/ void Runtime::WaitGroup_Done(Handle<Any> self) {
    DCHECK(!::strcmp("lang.WaitGroup", self->clazz()->name()));
    const Field *number_of_works_field = self->clazz()->field(0);
    DCHECK(!::strcmp("numberOfWorks", number_of_works_field->name()));
    const Field *mutex_field = self->clazz()->field(1);
    DCHECK(!::strcmp("mutex", mutex_field->name()));
    base::SpinMutex *mutex = self->UnsafeAccess<base::SpinMutex>(mutex_field);
    base::SpinLock lock(mutex);
    int *works = self->UnsafeAccess<int>(number_of_works_field);
    if (*works == 0) {
        return; // Ignore
    }
    if (--(*works) == 0) {
        const Field *request = self->clazz()->field(2);
        DCHECK(!::strcmp("requestDummy", request->name()));
        WaittingRequest *req = self->UnsafeAccess<WaittingRequest>(request);
        if (!req->co) {
            return; // No waiter, Ignore
        }

        while (!req->co->AcquireState(Coroutine::kWaitting, Coroutine::kRunnable)) {
            std::this_thread::yield();
        }
        Machine *owner = req->co->owner();
        owner->Wakeup(req->co, true/*now*/);
    }
}

/*static*/ void Runtime::WaitGroup_Wait(Handle<Any> self) {
    DCHECK(!::strcmp("lang.WaitGroup", self->clazz()->name()));
    const Field *number_of_works_field = self->clazz()->field(0);
    DCHECK(!::strcmp("numberOfWorks", number_of_works_field->name()));
    const Field *mutex_field = self->clazz()->field(1);
    DCHECK(!::strcmp("mutex", mutex_field->name()));
    base::SpinMutex *mutex = self->UnsafeAccess<base::SpinMutex>(mutex_field);
    base::SpinLock lock(mutex);

    if (self->UnsafeGetField<int>(number_of_works_field) > 0) {
        Coroutine *co = Coroutine::This();
        const Field *request = self->clazz()->field(2);
        DCHECK(!::strcmp("requestDummy", request->name()));
        WaittingRequest *rq = self->UnsafeAccess<WaittingRequest>(request);
        if (rq->co != nullptr) {
            // TODO:
        }
        rq->co = co;
        co->Yield(rq);
    }
}

/*static*/ int Runtime::CurrentSourceLine(int level) {
    Address frame_bp = Coroutine::This()->bp1();
    while (frame_bp < Coroutine::This()->stack()->stack_hi()) {
        StackFrame::Maker maker = StackFrame::GetMaker(frame_bp);
        switch (maker) {
            case StackFrame::kBytecode: {
                Function *fun = BytecodeStackFrame::GetCallee(frame_bp)->function();
                if (!level--) {
                    if (!fun->source_line_info()) {
                        return 0;
                    }
                    intptr_t pc = BytecodeStackFrame::GetPC(frame_bp);
                    return fun->source_line_info()->FindSourceLine(pc);
                }
            } break;
            case StackFrame::kStub:
                // Skip stub frame
                break;
            case StackFrame::kTrampoline:
                return 0;
            default:
                NOREACHED();
                break;
        }
        frame_bp = *reinterpret_cast<Address *>(frame_bp);
    }
    return 0;
}

/*static*/ String *Runtime::CurrentSourceName(int level) {
    Address frame_bp = Coroutine::This()->bp1();
    while (frame_bp < Coroutine::This()->stack()->stack_hi()) {
        StackFrame::Maker maker = StackFrame::GetMaker(frame_bp);
        switch (maker) {
            case StackFrame::kBytecode: {
                Function *fun = BytecodeStackFrame::GetCallee(frame_bp)->function();
                if (!level--) {
                    if (!fun->source_line_info()) {
                        return STATE->factory()->empty_string();
                    }
                    const MDStrHeader *name = MDStrHeader::FromStr(fun->source_line_info()->file_name());
                    return Machine::This()->NewUtf8String(name->data(), name->length(), 0);
                }
            } break;
            case StackFrame::kStub:
                // Skip stub frame
                break;
            case StackFrame::kTrampoline:
                return STATE->factory()->empty_string();
            default:
                NOREACHED();
                break;
        }
        frame_bp = *reinterpret_cast<Address *>(frame_bp);
    }
    return STATE->factory()->empty_string();
}

/*static*/ void Runtime::Sleep(uint64_t mills) {
    std::this_thread::sleep_for(std::chrono::milliseconds(mills));
}

/*static*/ int Runtime::CurrentMachineID() {
    return Machine::This()->id();
}

/*static*/ uint64_t Runtime::CurrentCoroutineID() {
    return Coroutine::This()->coid();
}

/*static*/ void Runtime::MinorGC() {
    if (STATE->scheduler()->shutting_down()) {
        return;
    }
    if (!STATE->gc()->AcquireState(GarbageCollector::kIdle, GarbageCollector::kReady)) {
        Machine::This()->Park();
        return;
    }
    if (!STATE->scheduler()->Pause()) {
        Machine::This()->Park();
        return;
    }
    STATE->gc()->MinorCollect();
    bool ok = STATE->gc()->AcquireState(GarbageCollector::kDone, GarbageCollector::kIdle);
    DCHECK(ok);
    (void)ok;
    STATE->scheduler()->Resume();
}

/*static*/ void Runtime::MajorGC() {
    if (STATE->scheduler()->shutting_down()) {
        return;
    }
    if (!STATE->gc()->AcquireState(GarbageCollector::kIdle, GarbageCollector::kReady)) {
        Machine::This()->Park();
        return;
    }
    if (!STATE->scheduler()->Pause()) {
        Machine::This()->Park();
        return;
    }
    STATE->gc()->MajorCollect();
    bool ok = STATE->gc()->AcquireState(GarbageCollector::kDone, GarbageCollector::kIdle);
    DCHECK(ok);
    (void)ok;
    STATE->scheduler()->Resume();
}

// [Safepoint]
/*static*/ Any *Runtime::GetMemoryHistogram() {
    SafepointScope safepoint_scope(STATE->gc());
    const Class *type = STATE->metadata_space()->FindClassOrNull("runtime.MemoryHistogram");
    Object *histogram = static_cast<Object *>(Machine::This()->NewObject(type, 0));
    MetadataSpace *ms = STATE->metadata_space();
    Heap *heap = STATE->heap();
    
    const Field *field = ms->FindClassFieldOrNull(type, "newSpaceSize");
    histogram->UnsafeSetField<uint64_t>(field, STATE->new_space_initial_size());
    
    field = ms->FindClassFieldOrNull(type, "oldSpaceSize");
    histogram->UnsafeSetField<uint64_t>(field, STATE->old_space_limit_size());
    
    field = ms->FindClassFieldOrNull(type, "newSpaceUsed");
    histogram->UnsafeSetField<uint64_t>(field, heap->new_space()->GetUsedSize());
    
    field = ms->FindClassFieldOrNull(type, "oldSpaceUsed");
    histogram->UnsafeSetField<uint64_t>(field, heap->old_space()->used_size() +
                                        heap->large_space()->used_size());
    
    field = ms->FindClassFieldOrNull(type, "newSpaceRSS");
    histogram->UnsafeSetField<uint64_t>(field, heap->new_space()->GetRSS());
    
    field = ms->FindClassFieldOrNull(type, "oldSpaceRSS");
    histogram->UnsafeSetField<uint64_t>(field, heap->old_space()->GetRSS() +
                                        heap->large_space()->rss_size());
    
    field = ms->FindClassFieldOrNull(type, "gcTick");
    histogram->UnsafeSetField<uint64_t>(field, STATE->gc()->tick());
    return histogram;
}

/*static*/ void Runtime::Schedule() { Coroutine::This()->Yield(); }

} // namespace lang

} // namespace mai
