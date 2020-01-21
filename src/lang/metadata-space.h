#ifndef MAI_LANG_METADATA_SPACE_H_
#define MAI_LANG_METADATA_SPACE_H_

#include "lang/metadata.h"
#include "lang/space.h"

namespace mai {

namespace lang {

class MetadataSpace final : public Space {
public:
    MetadataSpace(Allocator *lla)
        : Space(kMetadataSpace, lla)
        , dummy_page_(new PageHeader(kDummySpace))
        , dummy_code_(new PageHeader(kDummySpace))
        , dummy_large_(new PageHeader(kDummySpace))
        , usage_(0) {
    }

    ~MetadataSpace() {
        delete dummy_large_;
        delete dummy_code_;
        delete dummy_page_;
    }
    
    template<class T>
    inline T *NewArray(size_t n) {
        auto result = Allocate(n * sizeof(T), false);
        if (!result.ok()) {
            return nullptr;
        }
        return static_cast<T *>(result.ptr());
    }
    
    MDStr NewString(const char *s) { return NewString(s, strlen(s)); }

    MDStr NewString(const char *z, size_t n) {
        auto result = Allocate(sizeof(MDStrHeader) + n + 1, false/*exec*/);
        if (!result.ok()) {
            return nullptr;
        }
        MDStrHeader *str = new (result.ptr()) MDStrHeader(z, n);
        return str->data();
    }
    
    // mark all pages to readonly
    void MarkReadonly(bool readonly) {
        // TODO:
    }

private:
    // allocate flat memory block
    AllocationResult Allocate(size_t n, bool exec);
    
    LinearPage *AppendPage(int access) {
        LinearPage *page = LinearPage::New(kind(), access, lla_);
        if (!page) {
            return nullptr;
        }
        if (access & Allocator::kEx) {
            QUEUE_INSERT_TAIL(dummy_code_, page);
        } else {
            QUEUE_INSERT_TAIL(dummy_page_, page);
        }
        return page;
    }
    
    LinearPage *code_head() const { return static_cast<LinearPage *>(dummy_code_->next_); }
    
    LinearPage *page_head() const { return static_cast<LinearPage *>(dummy_page_->next_); }
    
    PageHeader *dummy_page_; // Linear page double-linked list dummy (LinearPage)
    PageHeader *dummy_code_; // Linear page double-linked list dummy (LinearPage)
    PageHeader *dummy_large_; // Large page double-linked list dummy (LargePage)
    size_t usage_; // Allocated used bytes
}; // class MetadataSpace

} // namespace lang

} // namespace mai

#endif // MAI_LANG_METADATA_SPACE_H_
