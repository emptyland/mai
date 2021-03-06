#ifndef MAI_LANG_TYPE_DEFS_H_
#define MAI_LANG_TYPE_DEFS_H_

#include <stdint.h>

namespace mai {

namespace lang {

class Any;
class String;
class Closure;
class Kode;
class Panic;
class Channel;
class Throwable;
class Class;
class AbstractMap;
template<class T> class ImplementMap;
template<class T> class Number;
template<class T, bool R> class Array;
template<class K, class V, bool KT, bool VT> class Map;

// primitive
#define DECLARE_PRIMITIVE_TYPES(V) \
    V(bool, bool) \
    V(i8, int8_t) \
    V(u8, uint8_t) \
    V(i16, int16_t) \
    V(u16, uint16_t) \
    V(i32, int32_t) \
    V(u32, uint32_t) \
    V(int, int32_t) \
    V(uint, uint32_t) \
    V(i64, int64_t) \
    V(u64, uint64_t) \
    V(f32, float) \
    V(f64, double)

#define DECLARE_BOX_NUMBER_TYPES(V) \
    V(Bool, bool, bool) \
    V(I8, i8, int8_t) \
    V(U8, u8, uint8_t) \
    V(I16, i16, int16_t) \
    V(U16, u16, uint16_t) \
    V(I32, i32, int32_t) \
    V(U32, u32, uint32_t) \
    V(Int, int, int32_t) \
    V(UInt, uint, uint32_t) \
    V(I64, i64, int64_t) \
    V(U64, u64, uint64_t) \
    V(F32, f32, float) \
    V(F64, f64, double)

#define DECLARE_ARRAY_TYPES(V) \
    V(array8, uint8_t) \
    V(array16, uint16_t) \
    V(array32, uint32_t) \
    V(array64, uint64_t) \
    V(array, Any*)

#define DECLARE_MAP_TYPES(V) \
    V(map8, uint8_t) \
    V(map16, uint16_t) \
    V(map32, uint32_t) \
    V(map64, uint64_t) \
    V(map, Any*)

#define DECLARE_CONTAINER_TYPES(V) \
    DECLARE_ARRAY_TYPES(V) \
    DECLARE_MAP_TYPES(V)

enum BuiltinType: int {
    kType_void,
#define DEFINE_ENUM(name, ...) kType_##name,
    DECLARE_PRIMITIVE_TYPES(DEFINE_ENUM)
    kType_any,
    DECLARE_BOX_NUMBER_TYPES(DEFINE_ENUM)
    kType_captured_value,
    kType_closure,
    kType_string,
    kType_channel,
    DECLARE_CONTAINER_TYPES(DEFINE_ENUM)
    kType_Code,
    kType_Throwable,
    kType_Panic,

    kMax_Types
#undef DEFINE_ENUM
};

static constexpr uint32_t kUserTypeIdBase = 0x1000;


template<class T>
struct TypeTraits {
    static constexpr BuiltinType kType = static_cast<BuiltinType>(-1);
    static constexpr char kName[] = "";
};

template<>
struct TypeTraits<void> {
    static constexpr BuiltinType kType = kType_void;
    static constexpr char kName[] = "void";
};

template<>
struct TypeTraits<bool> {
    static constexpr BuiltinType kType = kType_bool;
    static constexpr char kName[] = "bool";
};

template<>
struct TypeTraits<char> {
    static constexpr BuiltinType kType = kType_i8;
    static constexpr char kName[] = "i8";
};

template<>
struct TypeTraits<int8_t> {
    static constexpr BuiltinType kType = kType_i8;
    static constexpr char kName[] = "i8";
};

template<>
struct TypeTraits<uint8_t> {
    static constexpr BuiltinType kType = kType_u8;
    static constexpr char kName[] = "u8";
};

template<>
struct TypeTraits<int16_t> {
    static constexpr BuiltinType kType = kType_i16;
    static constexpr char kName[] = "i16";
};

template<>
struct TypeTraits<uint16_t> {
    static constexpr BuiltinType kType = kType_u16;
    static constexpr char kName[] = "u16";
};

template<>
struct TypeTraits<int32_t> {
    static constexpr BuiltinType kType = kType_i32;
    static constexpr char kName[] = "i32";
};

template<>
struct TypeTraits<uint32_t> {
    static constexpr BuiltinType kType = kType_u32;
    static constexpr char kName[] = "u32";
};

template<>
struct TypeTraits<int64_t> {
    static constexpr BuiltinType kType = kType_i64;
    static constexpr char kName[] = "i64";
};

template<>
struct TypeTraits<uint64_t> {
    static constexpr BuiltinType kType = kType_u64;
    static constexpr char kName[] = "u64";
};

template<>
struct TypeTraits<intptr_t> {
    static constexpr BuiltinType kType = kType_i64;
    static constexpr char kName[] = "i64";
};

template<>
struct TypeTraits<uintptr_t> {
    static constexpr BuiltinType kType = kType_u64;
    static constexpr char kName[] = "u64";
};

template<>
struct TypeTraits<float> {
    static constexpr BuiltinType kType = kType_f32;
    static constexpr char kName[] = "f32";
};

template<>
struct TypeTraits<double> {
    static constexpr BuiltinType kType = kType_f64;
    static constexpr char kName[] = "f64";
};


template<>
struct TypeTraits<Class> {
    static constexpr BuiltinType kType = kType_u64;
    static constexpr char kName[] = "class";
};

template<>
struct TypeTraits<Closure> {
    static constexpr BuiltinType kType = kType_closure;
    static constexpr char kName[] = "closure";
};

template<>
struct TypeTraits<String> {
    static constexpr BuiltinType kType = kType_string;
    static constexpr char kName[] = "string";
};

template<>
struct TypeTraits<Channel> {
    static constexpr BuiltinType kType = kType_channel;
    static constexpr char kName[] = "channel";
};

template<>
struct TypeTraits<Kode> {
    static constexpr BuiltinType kType = kType_Code;
    static constexpr char kName[] = "code";
};

template<>
struct TypeTraits<Number<bool>> {
    static constexpr BuiltinType kType = kType_Bool;
    static constexpr char kName[] = "Bool";
};

template<>
struct TypeTraits<Number<char>> {
    static constexpr BuiltinType kType = kType_I8;
    static constexpr char kName[] = "I8";
};

template<>
struct TypeTraits<Number<int8_t>> {
    static constexpr BuiltinType kType = kType_I8;
    static constexpr char kName[] = "I8";
};

template<>
struct TypeTraits<Number<uint8_t>> {
    static constexpr BuiltinType kType = kType_U8;
    static constexpr char kName[] = "U8";
};

template<>
struct TypeTraits<Number<int16_t>> {
    static constexpr BuiltinType kType = kType_I16;
    static constexpr char kName[] = "I16";
};

template<>
struct TypeTraits<Number<uint16_t>> {
    static constexpr BuiltinType kType = kType_U16;
    static constexpr char kName[] = "U16";
};

template<>
struct TypeTraits<Number<int32_t>> {
    static constexpr BuiltinType kType = kType_I32;
    static constexpr char kName[] = "I32";
};

template<>
struct TypeTraits<Number<uint32_t>> {
    static constexpr BuiltinType kType = kType_U32;
    static constexpr char kName[] = "U32";
};

template<>
struct TypeTraits<Number<int64_t>> {
    static constexpr BuiltinType kType = kType_I64;
    static constexpr char kName[] = "I64";
};

template<>
struct TypeTraits<Number<uint64_t>> {
    static constexpr BuiltinType kType = kType_U64;
    static constexpr char kName[] = "U64";
};

template<>
struct TypeTraits<Number<float>> {
    static constexpr BuiltinType kType = kType_F32;
    static constexpr char kName[] = "F32";
};

template<>
struct TypeTraits<Number<double>> {
    static constexpr BuiltinType kType = kType_F64;
    static constexpr char kName[] = "F64";
};

template<>
struct TypeTraits<Any> {
    static constexpr BuiltinType kType = kType_any;
    static constexpr char kName[] = "any";
};

template<>
struct TypeTraits<Any *> {
    static constexpr BuiltinType kType = kType_any;
    static constexpr char kName[] = "any";
};

template<>
struct TypeTraits<Array<int8_t, false>> {
    static constexpr BuiltinType kType = kType_array8;
    static constexpr char kName[] = "array8";
};

template<>
struct TypeTraits<Array<uint8_t, false>> {
    static constexpr BuiltinType kType = kType_array8;
    static constexpr char kName[] = "array8";
};

template<>
struct TypeTraits<Array<int16_t, false>> {
    static constexpr BuiltinType kType = kType_array16;
    static constexpr char kName[] = "array16";
};

template<>
struct TypeTraits<Array<uint16_t, false>> {
    static constexpr BuiltinType kType = kType_array16;
    static constexpr char kName[] = "array16";
};

template<>
struct TypeTraits<Array<int32_t, false>> {
    static constexpr BuiltinType kType = kType_array32;
    static constexpr char kName[] = "array32";
};

template<>
struct TypeTraits<Array<uint32_t, false>> {
    static constexpr BuiltinType kType = kType_array32;
    static constexpr char kName[] = "array32";
};

template<>
struct TypeTraits<Array<int64_t, false>> {
    static constexpr BuiltinType kType = kType_array64;
    static constexpr char kName[] = "array64";
};

template<>
struct TypeTraits<Array<uint64_t, false>> {
    static constexpr BuiltinType kType = kType_array64;
    static constexpr char kName[] = "array64";
};

template<>
struct TypeTraits<Array<float, false>> {
    static constexpr BuiltinType kType = kType_array32;
    static constexpr char kName[] = "array32";
};

template<>
struct TypeTraits<Array<double, false>> {
    static constexpr BuiltinType kType = kType_array64;
    static constexpr char kName[] = "array64";
};

template<class T>
struct TypeTraits<Array<T, true>> {
    static constexpr BuiltinType kType = kType_array;
    static constexpr char kName[] = "array";
};

template<>
struct TypeTraits<AbstractMap> {
    static constexpr BuiltinType kType = kType_any;
    static constexpr char kName[] = "any";
};

template<>
struct TypeTraits<ImplementMap<bool>> {
    static constexpr BuiltinType kType = kType_map8;
    static constexpr char kName[] = "map8";
};

template<>
struct TypeTraits<ImplementMap<int8_t>> {
    static constexpr BuiltinType kType = kType_map8;
    static constexpr char kName[] = "map8";
};

template<>
struct TypeTraits<ImplementMap<uint8_t>> {
    static constexpr BuiltinType kType = kType_map8;
    static constexpr char kName[] = "map8";
};

template<>
struct TypeTraits<ImplementMap<int16_t>> {
    static constexpr BuiltinType kType = kType_map16;
    static constexpr char kName[] = "map16";
};

template<>
struct TypeTraits<ImplementMap<uint16_t>> {
    static constexpr BuiltinType kType = kType_map16;
    static constexpr char kName[] = "map16";
};

template<>
struct TypeTraits<ImplementMap<int32_t>> {
    static constexpr BuiltinType kType = kType_map32;
    static constexpr char kName[] = "map32";
};

template<>
struct TypeTraits<ImplementMap<uint32_t>> {
    static constexpr BuiltinType kType = kType_map32;
    static constexpr char kName[] = "map32";
};

template<>
struct TypeTraits<ImplementMap<float>> {
    static constexpr BuiltinType kType = kType_map32;
    static constexpr char kName[] = "map32";
};

template<>
struct TypeTraits<ImplementMap<int64_t>> {
    static constexpr BuiltinType kType = kType_map64;
    static constexpr char kName[] = "map32";
};

template<>
struct TypeTraits<ImplementMap<uint64_t>> {
    static constexpr BuiltinType kType = kType_map64;
    static constexpr char kName[] = "map64";
};

template<>
struct TypeTraits<ImplementMap<double>> {
    static constexpr BuiltinType kType = kType_map64;
    static constexpr char kName[] = "map64";
};

template<class T>
struct TypeTraits<ImplementMap<T>> {
    static constexpr BuiltinType kType = kType_map;
    static constexpr char kName[] = "map";
};

template<class V, bool S>
struct TypeTraits<Map<bool, V, false, S>> {
    static constexpr BuiltinType kType = kType_map8;
    static constexpr char kName[] = "map8";
};

template<class V, bool S>
struct TypeTraits<Map<int8_t, V, false, S>> {
    static constexpr BuiltinType kType = kType_map8;
    static constexpr char kName[] = "map8";
};

template<class V, bool S>
struct TypeTraits<Map<uint8_t, V, false, S>> {
    static constexpr BuiltinType kType = kType_map8;
    static constexpr char kName[] = "map8";
};

template<class V, bool S>
struct TypeTraits<Map<int16_t, V, false, S>> {
    static constexpr BuiltinType kType = kType_map16;
    static constexpr char kName[] = "map16";
};

template<class V, bool S>
struct TypeTraits<Map<uint16_t, V, false, S>> {
    static constexpr BuiltinType kType = kType_map16;
    static constexpr char kName[] = "map16";
};

template<class V, bool S>
struct TypeTraits<Map<int32_t, V, false, S>> {
    static constexpr BuiltinType kType = kType_map32;
    static constexpr char kName[] = "map32";
};

template<class V, bool S>
struct TypeTraits<Map<uint32_t, V, false, S>> {
    static constexpr BuiltinType kType = kType_map32;
    static constexpr char kName[] = "map32";
};

template<class V, bool S>
struct TypeTraits<Map<float, V, false, S>> {
    static constexpr BuiltinType kType = kType_map32;
    static constexpr char kName[] = "map32";
};

template<class V, bool S>
struct TypeTraits<Map<int64_t, V, false, S>> {
    static constexpr BuiltinType kType = kType_map64;
    static constexpr char kName[] = "map64";
};

template<class V, bool S>
struct TypeTraits<Map<uint64_t, V, false, S>> {
    static constexpr BuiltinType kType = kType_map64;
    static constexpr char kName[] = "map64";
};

template<class V, bool S>
struct TypeTraits<Map<double, V, false, S>> {
    static constexpr BuiltinType kType = kType_map64;
    static constexpr char kName[] = "map64";
};

template<class K, class V, bool T, bool S>
struct TypeTraits<Map<K, V, T, S>> {
    static constexpr BuiltinType kType = kType_map;
    static constexpr char kName[] = "map";
};

template<>
struct TypeTraits<Panic> {
    static constexpr BuiltinType kType = kType_Panic;
    static constexpr char kName[] = "Panic";
};

} // namespace lang

} // namespace mai

#endif // MAI_LANG_TYPE_DEFS_H_
