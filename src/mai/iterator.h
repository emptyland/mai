#ifndef MAI_ITERATOR_H_
#define MAI_ITERATOR_H_

#include "mai/error.h"
#include <string_view>

namespace mai {
    
class Iterator {
public:
    Iterator() {}
    virtual ~Iterator();
    
    virtual bool Valid() const = 0;
    virtual void SeekToFirst() = 0;
    virtual void SeekToLast() = 0;
    virtual void Seek(std::string_view target) = 0;
    
    virtual void Next() = 0;
    virtual void Prev() = 0;
    
    virtual std::string_view key() const = 0;
    virtual std::string_view value() const = 0;
    
    virtual Error error() const = 0;
    
    static Iterator *AsError(Error error);
    
    typedef void (*cleanup_func_t)(void *, void *);
    inline void RegisterCleanup(cleanup_func_t handler,
                                void *arg1 = nullptr, void *arg2 = nullptr);
    
    Iterator(const Iterator &) = delete;
    Iterator(Iterator &&) = delete;
    void operator = (const Iterator &) = delete;
private:
    struct CleanupEntry {
        CleanupEntry  *next;
        cleanup_func_t handler;
        void          *arg1;
        void          *arg2;
    };
    
    inline void DoCleanup();
    
    CleanupEntry *root_ = nullptr;
}; // class Iterator
    
inline
void Iterator::RegisterCleanup(cleanup_func_t handler, void *arg1, void *arg2) {
    root_ = new CleanupEntry {root_, handler, arg1, arg2};
}

inline void Iterator::DoCleanup() {
    while (root_) {
        CleanupEntry *prev = root_;
        root_->handler(root_->arg1, root_->arg2);
        root_ = root_->next;
        delete prev;
    }
}

} // namespace mai


#endif // MAI_ITERATOR_H_
