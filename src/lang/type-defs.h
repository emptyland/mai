#ifndef MAI_LANG_TYPE_DEFS_H_
#define MAI_LANG_TYPE_DEFS_H_

namespace mai {

namespace lang {

// primitive
#define DECLARE_PRIMITIVE_TYPES(V) \
    V(boolean, bool) \
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


enum BuiltinType: int {
#define DEFINE_ENUM(name, ...) kType_##name,
    DECLARE_PRIMITIVE_TYPES(DEFINE_ENUM)
#undef DEFINE_ENUM
};

} // namespace lang

} // namespace mai

#endif // MAI_LANG_TYPE_DEFS_H_
