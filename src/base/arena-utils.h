#ifndef MAI_BASE_ARENA_UTILS_H_
#define MAI_BASE_ARENA_UTILS_H_

#include "base/hash.h"
#include "base/arena.h"
#include "glog/logging.h"
#include <limits>
#include <vector>
#include <deque>
#include <list>
#include <map>
#include <unordered_map>

namespace mai {
    
namespace base {
    
class ArenaString {
public:
    static const ArenaString *const kEmpty;
    
    DEF_VAL_GETTER(uint32_t, hash_val);
    size_t size() const { return len_;}

    const char *data() const { return buf_; }
    
    bool empty() const { return len_ == 0; }
    
    bool full() const { return !empty(); }
    
    bool Equal(const char *z) const { return ::strncmp(z, data(), size()) == 0; }
    
    std::string ToString() const { return std::string(data(), size()); }
    
    std::string_view ToSlice() const { return std::string_view(data(), size()); }
    
    static ArenaString *New(base::Arena *arena, const char *s, size_t n);
    
    static ArenaString *New(base::Arena *arena, const char *s) {
        return New(arena, s, strlen(s));
    }
    
    static ArenaString *New(base::Arena *arena, std::string_view s) {
        return New(arena, s.data(), s.size());
    }

private:
    ArenaString(const char *s, size_t n);
    
    ArenaString(size_t n);
    
    const uint32_t hash_val_;
    const uint32_t len_;
    char buf_[0];
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
        return lhs->size() == rhs->size() && ::memcmp(lhs->data(), rhs->data(), lhs->size()) == 0;
    }
}; // template <> struct ArenaEqualTo<ArenaString *>

template <> struct ArenaEqualTo<const ArenaString *>
: public std::binary_function<const ArenaString *, const ArenaString *, bool> {
    bool operator () (const ArenaString *lhs, const ArenaString *rhs) const {
        return lhs->size() == rhs->size() && ::memcmp(lhs->data(), rhs->data(), lhs->size()) == 0;
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
    
struct ArenaUtils {
    
    template<class T>
    static inline bool IsEmpty(const ArenaVector<T> *a) { return !a || a->empty(); }
    
    template<class T>
    static inline bool IsNotEmpty(const ArenaVector<T> *a) { return !IsEmpty(a); }
};

#define DEF_ARENA_VECTOR_GETTER(__element_type, __name) \
    inline size_t __name##s_size() const { return __name##s_.size(); } \
    inline __element_type __name(size_t i) const { \
        DCHECK_LT(i, __name##s_.size()); \
        return __name##s_[i]; \
    } \
    inline const base::ArenaVector<__element_type> &__name##s() const { return __name##s_; } \
    inline base::ArenaVector<__element_type> *mutable_##__name##s() { return &__name##s_; }
    
} // namespace base
    
} // namespace mai


#endif // MAI_BASE_ARENA_UTILS_H_
