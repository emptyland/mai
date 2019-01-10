#ifndef MAI_BASE_ARENA_UTILS_H_
#define MAI_BASE_ARENA_UTILS_H_

#include "base/hash.h"
#include "base/arena.h"
//#include "base/hash.h"
#include "glog/logging.h"
#include <limits>
#include <vector>
#include <deque>
#include <list>
#include <map>
#include <unordered_map>

namespace mai {
    
namespace base {
    
class ArenaString final {
public:
    enum Kind {
        kSmall,
        kLarge,
        kOverflow,
    };
    
    static const int kMinLargetSize = 0x8000;
    static const ArenaString *const kEmpty;
    
    DEF_VAL_GETTER(uint32_t, hash_val);
    
    size_t size() const {
        Kind k = kind();
        DCHECK_NE(kOverflow, k);
        if (k == kSmall) {
            return Decode16(buf_);
        } else { // kLarge
            return Decode32(buf_) & ~0xc0000000u;
        }
    }
    
    const char *data() const {
        Kind k = kind();
        DCHECK_NE(kOverflow, k);
        if (k == kSmall) {
            return reinterpret_cast<const char *>(this + 1);
        } else { // kLarge
            return reinterpret_cast<const char *>(this + 1) + 2;
        }
    }
    
    std::string ToString() const { return std::string(data(), size()); }
    
    std::string_view ToStringView() const {
        return std::string_view(data(), size());
    }
    
    Kind kind() const {
        // 1000 0000
        // 0xxx xxxx
        if ((buf_[0] & 0x80) == 0) {
            return kSmall;
        // 1100 0000
        // 10xx xxxx
        } else if ((buf_[0] & 0xc0) == 0x80) {
            return kLarge;
        }
        return kOverflow;
    }
    
    static ArenaString *New(base::Arena *arena, const char *s, size_t n);
    
    static ArenaString *New(base::Arena *arena, const char *s) {
        return New(arena, s, strlen(s));
    }
    
    static ArenaString *New(base::Arena *arena, std::string_view s) {
        return New(arena, s.data(), s.size());
    }

private:
    ArenaString(const char *s, size_t n, Kind kind);
    
    static Kind GetKind(size_t n) {
        if (n < 0x8000) {
            return kSmall;
        // 0100 0000
        // 0011 1111
        } else if (n >= 0x8000 && n < 0x40000000u) {
            return kLarge;
        }
        return kOverflow;
    }
    
    static uint16_t Decode16(const void *x) {
        const uint8_t *buf = static_cast<const uint8_t *>(x);
        uint16_t val = 0;
        val |= (static_cast<uint16_t>(buf[0]) << 8);
        val |= static_cast<uint16_t>(buf[1]);
        return val;
    }
    
    static uint32_t Decode32(const void *x) {
        const uint8_t *buf = static_cast<const uint8_t *>(x);
        uint32_t val = 0;
        val |= (static_cast<uint16_t>(buf[0]) << 24);
        val |= (static_cast<uint16_t>(buf[1]) << 16);
        val |= (static_cast<uint16_t>(buf[2]) << 8);
        val |= static_cast<uint16_t>(buf[3]);
        return val;
    }
    
    const uint32_t hash_val_;
    char buf_[2];
}; // class ArenaString
    

////////////////////////////////////////////////////////////////////////////////
/// class ArenaAllocator: std allocator
////////////////////////////////////////////////////////////////////////////////

template <class T>
class ArenaAllocator {
public:
    typedef T* pointer;
    typedef const T* const_pointer;
    typedef T& reference;
    typedef const T& const_reference;
    typedef T value_type;
    typedef size_t size_type;
    typedef ptrdiff_t difference_type;
    template <class O>
    struct rebind {
        typedef ArenaAllocator<O> other;
    };
    
    //#ifdef V8_CC_MSVC
    //    // MSVS unfortunately requires the default constructor to be defined.
    //    ZoneAllocator() : ZoneAllocator(nullptr) { UNREACHABLE(); }
    //#endif
    explicit ArenaAllocator(Arena* arena) throw() : arena_(arena) {}
    explicit ArenaAllocator(const ArenaAllocator& other) throw()
        : ArenaAllocator<T>(other.arena_) {}
    template <typename U>
    ArenaAllocator(const ArenaAllocator<U>& other) throw()
        : ArenaAllocator<T>(other.arena()) {}
    
    template <typename U>
    friend class ZoneAllocator;
    
    T* address(T& x) const { return &x; }
    const T* address(const T& x) const { return &x; }
    
    T* allocate(size_t n, const void* hint = 0) {
        return arena_->NewArray<T>(n);
    }
    
    void deallocate(T* p, size_t) { /* noop for Zones */
    }
    
    size_t max_size() const throw() {
        return std::numeric_limits<int>::max() / sizeof(T);
    }
    template <typename U, typename... Args>
    void construct(U* p, Args&&... args) {
        void* v_p = const_cast<void*>(static_cast<const void*>(p));
        new (v_p) U(std::forward<Args>(args)...);
    }
    template <typename U>
    void destroy(U* p) {
        p->~U();
    }
    
    bool operator == (ArenaAllocator const& other) const {
        return arena_ == other.arena_;
    }
    bool operator != (ArenaAllocator const& other) const {
        return arena_ != other.arena_;
    }
    
    Arena* arena() const { return arena_; }
    
private:
    Arena* arena_;
}; // template <class T> class ArenaAllocator
    
    
template<class T> struct ArenaLess : public std::binary_function<T, T, bool> {
    bool operator ()(const T &lhs, const T &rhs) const { return lhs < rhs; }
}; // struct ArenaLess

template<> struct ArenaLess<ArenaString *>
: public std::binary_function<ArenaString *, ArenaString *, bool> {
    bool operator ()(ArenaString *lhs, ArenaString *rhs) const {
        return ::strcmp(lhs->data(), rhs->data()) < 0;
    }
}; // template<> struct ArenaLess<ArenaString *>

template<> struct ArenaLess<const ArenaString *>
: public std::binary_function<const ArenaString *, const ArenaString *, bool> {
    bool operator ()(const ArenaString *lhs, const ArenaString *rhs) const {
        return ::strcmp(lhs->data(), rhs->data()) < 0;
    }
}; // template<> struct ArenaLess<const ArenaString *>

template <class T> struct ArenaHash : public std::unary_function<T, size_t> {
    size_t operator () (T value) const { return std::hash<T>{}(value); }
}; // struct ZoneHash

template <> struct ArenaHash<ArenaString *>
    : public std::unary_function<ArenaString *, size_t> {
    size_t operator () (ArenaString * value) const {
        return value->hash_val();
    }
}; // template <> struct ArenaHash<ArenaString *>

template <> struct ArenaHash<const ArenaString *>
: public std::unary_function<const ArenaString *, size_t> {
    size_t operator () (const ArenaString * value) const {
        return value->hash_val();
    }
}; // template <> struct ArenaHash<const ArenaString *>

template <class T>
struct ArenaEqualTo : public std::binary_function<T, T, bool> {
    bool operator () (T lhs, T rhs) {
        return std::equal_to<T>{}(lhs, rhs);
    }
}; // template <class T> struct ArenaEqualTo

template <> struct ArenaEqualTo<ArenaString *>
: public std::binary_function<ArenaString *, ArenaString *, bool> {
    bool operator () (ArenaString * lhs, ArenaString * rhs) const {
        return ::strcmp(lhs->data(), rhs->data()) == 0;
    }
}; // template <> struct ArenaEqualTo<ArenaString *>

template <> struct ArenaEqualTo<const ArenaString *>
: public std::binary_function<const ArenaString *, const ArenaString *, bool> {
    bool operator () (const ArenaString *lhs, const ArenaString *rhs) const {
        return ::strcmp(lhs->data(), rhs->data()) == 0;
    }
}; // template <> struct ArenaEqualTo<const ArenaString *>


////////////////////////////////////////////////////////////////////////////////
/// std::vector warpper from ArenaContainer
////////////////////////////////////////////////////////////////////////////////

template <class T>
class ArenaVector : public std::vector<T, ArenaAllocator<T>> {
public:
    // Constructs an empty vector.
    explicit ArenaVector(Arena* zone)
    : std::vector<T, ArenaAllocator<T>>(ArenaAllocator<T>(zone)) {}
    
    // Constructs a new vector and fills it with {size} elements, each
    // constructed via the default constructor.
    ArenaVector(size_t size, Arena* zone)
    : std::vector<T, ArenaAllocator<T>>(size, T(), ArenaAllocator<T>(zone)) {}
    
    // Constructs a new vector and fills it with {size} elements, each
    // having the value {def}.
    ArenaVector(size_t size, T def, Arena* zone)
    : std::vector<T, ArenaAllocator<T>>(size, def, ArenaAllocator<T>(zone)) {}
    
    // Constructs a new vector and fills it with the contents of the given
    // initializer list.
    ArenaVector(std::initializer_list<T> list, Arena* zone)
    : std::vector<T, ArenaAllocator<T>>(list, ArenaAllocator<T>(zone)) {}
    
    // Constructs a new vector and fills it with the contents of the range
    // [first, last).
    template <class InputIt>
    ArenaVector(InputIt first, InputIt last, Arena* zone)
    : std::vector<T, ArenaAllocator<T>>(first, last, ArenaAllocator<T>(zone)) {}
    
    void *operator new (size_t n, Arena *arena) { return arena->Allocate(n); }
}; // class ZoneVector

////////////////////////////////////////////////////////////////////////////////
/// std::map warpper from ArenaContainer
///
///     A wrapper subclass for std::map to make it easy to construct one that
///     users a zone allocator.
////////////////////////////////////////////////////////////////////////////////
template <class K, class V, class Compare = ArenaLess<K>>
class ArenaMap
: public std::map<K, V, Compare, ArenaAllocator<std::pair<const K, V>>> {
public:
    // Constructs an empty map.
    explicit ArenaMap(Arena* zone)
        : std::map<K, V, Compare, ArenaAllocator<std::pair<const K, V>>>(
              Compare(), ArenaAllocator<std::pair<const K, V>>(zone)) {}
}; // class ZoneMap

////////////////////////////////////////////////////////////////////////////////
/// std::unordered_map warpper from ArenaContainer
///
///     A wrapper subclass for std::unordered_map to make it easy to construct
///     one that uses a zone allocator.
////////////////////////////////////////////////////////////////////////////////
template <class K, class V, class Hash = ArenaHash<K>,
class KeyEqual = std::equal_to<K>>
class ArenaUnorderedMap
: public std::unordered_map<K, V, Hash, KeyEqual,
    ArenaAllocator<std::pair<const K, V>>> {
public:
    // Constructs an empty map.
    explicit ArenaUnorderedMap(Arena* zone)
    : std::unordered_map<K, V, Hash, KeyEqual,
    ArenaAllocator<std::pair<const K, V>>>(
              100, Hash(), KeyEqual(),
              ArenaAllocator<std::pair<const K, V>>(zone)) {}
};
    
} // namespace base
    
} // namespace mai


#endif // MAI_BASE_ARENA_UTILS_H_
