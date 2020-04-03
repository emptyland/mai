#ifndef MAI_BASE_BASE_H_
#define MAI_BASE_BASE_H_

#include <stddef.h>
#include <assert.h>

namespace mai {

#define DISALLOW_IMPLICIT_CONSTRUCTORS(clazz_name) \
    clazz_name (const clazz_name &) = delete;      \
    clazz_name (clazz_name &&) = delete;           \
    void operator = (const clazz_name &) = delete;

#define DISALLOW_ALL_CONSTRUCTORS(clazz_name)      \
    clazz_name () = delete;                        \
    DISALLOW_IMPLICIT_CONSTRUCTORS(clazz_name)     \

/**
 * disable copy constructor, assign operator function.
 *
 */
class DisallowImplicitConstructors {
public:
    DisallowImplicitConstructors() = default;

    DISALLOW_IMPLICIT_CONSTRUCTORS(DisallowImplicitConstructors)
};

/**
 * Get size of array.
 */
template <class T, size_t N>
char (&ArraySizeHelper(T (&array)[N]))[N];

#ifndef _MSC_VER
template <class T, size_t N>
char (&ArraySizeHelper(const T (&array)[N]))[N];
#endif

#define arraysize(array) (sizeof(::mai::ArraySizeHelper(array)))

/**
 * Safaty down cast
 */
template<class T, class F>
inline T *down_cast(F *from) {
#if defined(DEBUG) || defined(_DEBUG)
    assert(!from || dynamic_cast<T *>(from) && "Can not cast to.");
#endif
    return static_cast<T *>(from);
}

template<class T, class F>
inline T bit_cast(F from) {
    static_assert(sizeof(T) >= sizeof(F), "Incorrect T and F size!");
    union {
        T as_to;
        F as_from;
    };
    as_from = from;
    return as_to;
}

#define IS_POWER_OF_TWO(x) (((x) & ((x) - 1)) == 0)

// Returns true iff x is a power of 2 (or zero). Cannot be used with the
// maximally negative value of the type T (the -1 overflows).
template <typename T>
constexpr inline bool IsPowerOf2(T x) {
    return IS_POWER_OF_TWO(x);
}

// Compute the 0-relative offset of some absolute value x of type T.
// This allows conversion of Addresses and integral types into
// 0-relative int offsets.
template <typename T>
constexpr inline intptr_t OffsetFrom(T x) {
    return x - static_cast<T>(0);
}


// Compute the absolute value of type T for some 0-relative offset x.
// This allows conversion of 0-relative int offsets into Addresses and
// integral types.
template <typename T>
constexpr inline T AddressFrom(intptr_t x) {
    return static_cast<T>(static_cast<T>(0) + x);
}


// Return the largest multiple of m which is <= x.
template <typename T>
constexpr inline T RoundDown(T x, intptr_t m) {
    assert(IsPowerOf2(m));
    return AddressFrom<T>(OffsetFrom(x) & -m);
}


// Return the smallest multiple of m which is >= x.
template <typename T>
constexpr inline T RoundUp(T x, intptr_t m) {
    return RoundDown<T>(static_cast<T>(x + m - 1), m);
}

template <class T>
constexpr inline T AlignDownBounds(T bounds, size_t value) {
    return (value + bounds - 1) & (~(bounds - 1));
}

template<class T>
inline int ComputeValueShift(T value) {
    int shift;
    for (shift = 0; (1 << shift) < value; shift++) {
    }
    return shift;
}

// Array view template
template<class T>
struct View {
    T const     *z;
    size_t const n;
};

template<class T>
struct MutView {
    T     *z;
    size_t n;
};

template<class T>
inline View<T> MakeView(T const *z, size_t n) { return View<T>{z, n}; }

template<class T>
inline MutView<T> MakeMutView(T *z, size_t n) { return MutView<T>{z, n}; }

/**
 * define getter/mutable_getter/setter
 */
#define DEF_VAL_PROP_RMW(type, name) \
    DEF_VAL_GETTER(type, name) \
    DEF_VAL_MUTABLE_GETTER(type, name) \
    DEF_VAL_SETTER(type, name)

#define DEF_VAL_PROP_RM(type, name) \
    DEF_VAL_GETTER(type, name) \
    DEF_VAL_MUTABLE_GETTER(type, name)

#define DEF_VAL_PROP_RW(type, name) \
    DEF_VAL_GETTER(type, name) \
    DEF_VAL_SETTER(type, name)

#define DEF_PTR_PROP_RW(type, name) \
    DEF_PTR_GETTER(type, name) \
    DEF_PTR_SETTER(type, name)

#define DEF_PTR_PROP_RW_NOTNULL1(type, name) \
    DEF_PTR_GETTER(type, name) \
    DEF_PTR_SETTER_NOTNULL(type, name)

#define DEF_PTR_PROP_RW_NOTNULL2(type, name) \
    DEF_PTR_GETTER_NOTNULL(type, name) \
    DEF_PTR_SETTER_NOTNULL(type, name)

#define DEF_VAL_GETTER(type, name) \
    inline const type &name() const { return name##_; }

#define DEF_VAL_MUTABLE_GETTER(type, name) \
    inline type *mutable_##name() { return &name##_; }

#define DEF_VAL_SETTER(type, name) \
    inline void set_##name(const type &value) { name##_ = value; }

#define DEF_PTR_GETTER(type, name) \
    inline type *name() const { return name##_; }

#define DEF_PTR_SETTER(type, name) \
    inline void set_##name(type *value) { name##_ = value; }

#define DEF_PTR_GETTER_NOTNULL(type, name) \
    inline type *name() const { return DCHECK_NOTNULL(name##_); }

#define DEF_PTR_SETTER_NOTNULL(type, name) \
    inline void set_##name(type *value) { name##_ = DCHECK_NOTNULL(value); }

// Unittest Uilts
#define FRIEND_UNITTEST_CASE(test_name, test_case) \
    friend class test_name##_##test_case##_Test

// OS platform macros
#if defined(_WIN32) || defined(WIN32) || defined(_WIN64)
#   define MAI_OS_WINDOWS 1
#   define MAI_OS_POSIX   0
#endif

#if defined(unix) || defined(__unix) || defined(__unix__)
#   define MAI_OS_UNIX   1
#   define MAI_OS_POSIX  1
#endif

#if defined(linux) || defined(__linux) || defined(__linux__) || defined(__gnu_linux__)
#   define MAI_OS_LINUX  1
#   define MAI_OS_POSIX  1
#endif

#if defined(__APPLE__)
#   define MAI_OS_DARWIN 1
#   define MAI_OS_POSIX  1
#endif

// [[gnu::always_inline]]
#if defined(DEBUG) || defined(_DEBUG)
#define ALWAYS_INLINE inline
#else
#define ALWAYS_INLINE inline
#endif // defined(DEBUG) || defined(_DEBUG)

// CPU Arch macros
    
#if defined(__amd64) || defined(__amd64__) || defined(__x86_64) || defined(__x86_64__)
#   define MAI_ARCH_X64 1
#endif

#if defined(__ARM64_ARCH_8__)
#   define MAI_ARCH_ARM64 1
#endif
    
#define NOREACHED() DLOG(FATAL) << "Noreached! "
#define TODO()      DLOG(FATAL) << "TODO: "

enum Initializer {
    LAZY_INSTANCE_INITIALIZER,
    ON_EXIT_SCOPE_INITIALIZER
};

using Byte = uint8_t;
using Address = Byte *;

static constexpr int kPointerSize = sizeof(void *);
static constexpr int kPointerShift = 3;

namespace base {
static constexpr int kKB = 1024;
static constexpr int kMB = 1024 * kKB;
static constexpr int kGB = 1024 * kMB;
} // namespace base

} // namespace mai

#endif // MAI_BASE_BASE_H_
