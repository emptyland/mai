#ifndef MAI_LANG_METADATA_SPACE_H_
#define MAI_LANG_METADATA_SPACE_H_

#include "lang/metadata.h"
#include "lang/space.h"
#include "lang/type-defs.h"
#include "base/hash.h"
#include <map>
#include <shared_mutex>
#include <unordered_map>

namespace mai {

namespace lang {

class MetadataSpace final : public Space {
public:
    MetadataSpace(Allocator *lla);
    ~MetadataSpace();
    
    // Init builtin types
    Error Initialize();
    
    const Field *FindClassFieldOrNull(const Class *clazz, const char *field_name);
    
    const Class *builtin_type(BuiltinType type) const {
        DCHECK_GE(static_cast<int>(type), 0);
        DCHECK_LT(static_cast<int>(type), classes_.size());
        return classes_[static_cast<int>(type)];
    }

    template<class T>
    const Class *TypeOf() const {
        return builtin_type(TypeTraits<T>::kType);
    }

    const Class *FindClassOrNull(const std::string &name) const {
        auto iter = named_classes_.find(name);
        return iter == named_classes_.end() ? nullptr : iter->second;
    }
    
    DEF_VAL_GETTER(size_t, used_size);
    
    size_t GetRSS() const { return n_pages_ * kPageSize + large_size_; }
    
    MDStr NewString(const char *s) { return NewString(s, strlen(s)); }

    MDStr NewString(const char *z, size_t n) {
        auto result = Allocate(sizeof(MDStrHeader) + n + 1, false/*exec*/);
        if (!result.ok()) {
            return nullptr;
        }
        MDStrHeader *str = new (result.ptr()) MDStrHeader(z, n);
        return str->data();
    }
    
    Code *NewCode(Code::Kind kind, Address instructions, size_t size) {
        auto result = Allocate(sizeof(Code) + size, true/*exec*/);
        if (!result.ok()) {
            return nullptr;
        }
        return new (result.ptr()) Code(kind, instructions, static_cast<uint32_t>(size));
    }
    
    // mark all pages to readonly
    void MarkReadonly(bool readonly) {
        // TODO:
        TODO();
    }
    
    void InvalidateAllLookupTables() {
        std::lock_guard<std::shared_mutex> lock(class_fields_mutex_);
        named_class_fields_.clear();
        // TODO:
    }

    // allocate flat memory block
    AllocationResult Allocate(size_t n, bool exec);
    
    friend class ClassBuilder;
    DISALLOW_IMPLICIT_CONSTRUCTORS(MetadataSpace);
private:
    struct ClassFieldKey {
        const Class *clazz;
        std::string_view field_name;
    }; // struct ClassFieldKey

    struct ClassFieldHash : public std::unary_function<ClassFieldKey, size_t> {
        size_t operator () (ClassFieldKey key) const {
            return std::hash<const Class *>{}(key.clazz) +
                base::Hash::Js(key.field_name.data(), key.field_name.size());
        }
    }; // struct ClassFieldHash
    
    struct ClassFieldEqualTo : public std::binary_function<ClassFieldKey, ClassFieldKey, bool> {
        bool operator () (ClassFieldKey lhs, ClassFieldKey rhs) const {
            return lhs.clazz == rhs.clazz && lhs.field_name.compare(rhs.field_name) == 0;
        }
    }; // struct ClassFieldEqualTo

    template<class T>
    inline T *NewArray(size_t n) {
        auto result = Allocate(n * sizeof(T), false);
        if (!result.ptr()) {
            return nullptr;
        }
        T *arr = static_cast<T *>(result.ptr());
        for (size_t i = 0; i  < n; i++) {
            T *obj = new (arr + i) T();
            DCHECK_EQ(obj, arr + i);
        }
        return arr;
    }
    
    template<class T>
    inline T *New() {
        auto result = Allocate(sizeof(T), false);
        if (!result.ptr()) {
            return nullptr;
        }
        return new (result.ptr()) T();
    }
    
    LinearPage *AppendPage(int access) {
        LinearPage *page = LinearPage::New(kind(), access, lla_);
        if (!page) {
            return nullptr;
        }
        if (access & Allocator::kEx) {
            QUEUE_INSERT_HEAD(dummy_code_, page);
        } else {
            QUEUE_INSERT_HEAD(dummy_page_, page);
        }
        n_pages_++;
        return page;
    }
    
    uint32_t NextTypeId() { return next_type_id_++; }
    
    void InsertClass(const std::string &name, Class *clazz) {
#if defined(DEBUG) || defined(_DEBUG)
        auto iter = named_classes_.find(name);
        DCHECK(iter == named_classes_.end());
#endif
        classes_.push_back(clazz);
        named_classes_[name] = clazz;
    }

    LinearPage *code_head() const { return LinearPage::Cast(dummy_code_->next_); }
    
    LinearPage *page_head() const { return LinearPage::Cast(dummy_page_->next_); }
    
    PageHeader *dummy_page_; // Linear page double-linked list dummy (LinearPage)
    PageHeader *dummy_code_; // Linear page double-linked list dummy (LinearPage)
    PageHeader *dummy_large_; // Large page double-linked list dummy (LargePage)
    std::vector<Class *> classes_; // All classes
    std::unordered_map<std::string, Class *> named_classes_; // All named classes
    uint32_t next_type_id_ = 0; // Next type unique id
    size_t n_pages_ = 0; // Number of linear pages.
    size_t large_size_ = 0; // All large page's size
    size_t used_size_ = 0; // Allocated used bytes
    
    // Class with field name
    using ClassFieldMap = std::unordered_map<ClassFieldKey, const Field*, ClassFieldHash,
        ClassFieldEqualTo>;
    ClassFieldMap named_class_fields_;
    std::shared_mutex class_fields_mutex_;
}; // class MetadataSpace


// Build classes
class ClassBuilder {
public:
    class FieldBuilder {
    public:
        FieldBuilder(const std::string &name, ClassBuilder *owner)
            : name_(name)
            , owner_(owner) {}
        
        FieldBuilder &type(const Class *value) {
            type_ = value;
            return *this;
        }
        
        FieldBuilder &tag(uint32_t value) {
            tag_ = value;
            return *this;
        }
        
        FieldBuilder &flags(uint32_t value) {
            flags_ = value;
            return *this;
        }
        
        FieldBuilder &offset(uint32_t value) {
            offset_ = value;
            return *this;
        }
        
        ClassBuilder &End() { return *owner_; }
        
        friend class ClassBuilder;
    private:
        std::string name_;
        ClassBuilder *owner_;
        const Class *type_;
        uint32_t tag_;
        uint32_t flags_;
        uint32_t offset_;
    }; // class FieldBuilder
    
    
    class MethodBuilder {
    public:
        MethodBuilder(const std::string &name, ClassBuilder *owner)
            : name_(name)
            , owner_(owner) {}
        
        MethodBuilder &tags(uint32_t value) {
            tags_ = value;
            return *this;
        }
        
        MethodBuilder &fn(Closure *value) {
            fn_ = value;
            return *this;
        }
        
        ClassBuilder &End() {
            DCHECK_NOTNULL(fn_);
            return *owner_;
        }
        
        friend class ClassBuilder;
    private:
        std::string name_;
        ClassBuilder *owner_;
        uint32_t tags_ = 0;
        Closure *fn_ = nullptr;
    }; // class MethodBuilder
    
    ClassBuilder(const std::string &name) : name_(name) {}
    
    ~ClassBuilder() {
        for (auto field : fields_) {
            delete field;
        }
    }
    
    ClassBuilder &tags(uint32_t value) {
        tags_ = value;
        return *this;
    }
    
    ClassBuilder &ref_size(uint32_t value) {
        ref_size_ = value;
        return *this;
    }
    
    ClassBuilder &base(const Class *value) {
        base_ = value;
        return *this;
    }
    
    FieldBuilder &field(const std::string &name) {
        auto iter = named_fields_.find(name);
        if (iter != named_fields_.end()) {
            return *iter->second;
        }
        FieldBuilder *field = new FieldBuilder(name, this);
        fields_.push_back(field);
        named_fields_[name] = field;
        return *field;
    }
    
    MethodBuilder &method(const std::string &name) {
        auto iter = named_methods_.find(name);
        if (iter != named_methods_.end()) {
            return *iter->second;
        }
        MethodBuilder *method = new MethodBuilder(name, this);
        named_methods_[name] = method;
        return *method;
    }
    
    Class *Build(MetadataSpace *space) const;
    
    DISALLOW_IMPLICIT_CONSTRUCTORS(ClassBuilder);
private:
    void BuildField(Field *field, FieldBuilder *builder, MetadataSpace *space) const {
        field->name_ = space->NewString(builder->name_.data(), builder->name_.size());
        field->type_ = builder->type_;
        field->tag_ = builder->tag_;
        field->flags_ = builder->flags_;
        field->offset_ = builder->offset_;
    }
    
    std::string name_;
    uint32_t tags_ = 0;
    uint32_t ref_size_ = 0;
    std::vector<FieldBuilder *> fields_;
    std::map<std::string, FieldBuilder *> named_fields_;
    std::map<std::string, MethodBuilder *> named_methods_;
    const Class *base_ = nullptr;
}; // class ClassBuilder

} // namespace lang

} // namespace mai

#endif // MAI_LANG_METADATA_SPACE_H_
