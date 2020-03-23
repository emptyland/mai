#pragma once
#ifndef MAI_LANG_METADATA_SPACE_H_
#define MAI_LANG_METADATA_SPACE_H_

#include "lang/metadata.h"
#include "lang/bytecode.h"
#include "lang/space.h"
#include "lang/type-defs.h"
#include "base/hash.h"
#include <map>
#include <shared_mutex>
#include <unordered_map>

namespace mai {

namespace lang {

class RootVisitor;

class MetadataSpace final : public Space {
public:
    MetadataSpace(Allocator *lla);
    ~MetadataSpace();
    
    // Init builtin types
    Error Initialize();
    
    void VisitRoot(RootVisitor *visitor);
    
    const Field *FindClassFieldOrNull(const Class *clazz, const char *field_name);
    
    const Method *FindClassMethodOrNull(const Class *clazz, const char *method_name) {
        return std::get<1>(FindClassMethod(clazz, method_name));
    }
    
    std::tuple<const Class *, const Method *>
    FindClassMethod(const Class *clazz, const char *method_name);
    
    const Class *builtin_type(BuiltinType type) const {
        DCHECK_GE(static_cast<int>(type), 0);
        DCHECK_LT(static_cast<int>(type), kMax_Types);
        return DCHECK_NOTNULL(classes_[static_cast<int>(type)]);
    }
    
    const Class *type(uint32_t id) const {
        size_t index = id >= kUserTypeIdBase ? id - kUserTypeIdBase + kMax_Types : id;
        DCHECK_LT(index, classes_.size());
        return DCHECK_NOTNULL(classes_[index]);
    }

    template<class T>
    const Class *TypeOf() const {
        return builtin_type(TypeTraits<T>::kType);
    }
    
    const Class *GetNumberClassOrNull(BuiltinType primitive_type) const;

    const Class *FindClassOrNull(const std::string &name) const {
        auto iter = named_classes_.find(name);
        return iter == named_classes_.end() ? nullptr : iter->second;
    }
    
    DEF_VAL_GETTER(size_t, used_size);
    
    // Language Classes:
    DEF_PTR_GETTER_NOTNULL(const Class, class_object);
    DEF_PTR_GETTER_NOTNULL(const Class, class_exception);
    // Builtin Code:
    DEF_PTR_GETTER_NOTNULL(Code, switch_system_stack_call_code);
    DEF_PTR_GETTER_NOTNULL(Code, function_template_dummy_code);
    DEF_PTR_GETTER_NOTNULL(Code, trampoline_code);
    DEF_VAL_GETTER(Address, trampoline_suspend_point);
    DEF_PTR_GETTER_NOTNULL(Code, interpreter_pump_code);

    Code **bytecode_handlers() { return bytecode_handlers_; }
    
    size_t GetRSS() const { return n_pages_ * kPageSize + large_size_; }
    
    MDStr NewString(const std::string &s) { return NewString(s.data(), s.size()); }
    
    MDStr NewString(const char *s) { return NewString(s, strlen(s)); }

    inline MDStr NewString(const char *z, size_t n);
    
    Code *NewCode(Code::Kind kind, const std::string &instructions, PrototypeDesc *prototype) {
        return NewCode(kind, reinterpret_cast<Address>(const_cast<char *>(instructions.data())),
                       instructions.size(), prototype);
    }
    
    inline Code *NewCode(Code::Kind kind, Address instructions, size_t size,
                         PrototypeDesc *prototype);
    
    BytecodeArray *NewBytecodeArray(const std::vector<BytecodeInstruction> &instructions) {
        return NewBytecodeArray(instructions.data(), instructions.size());
    }

    inline BytecodeArray *NewBytecodeArray(const BytecodeInstruction *instructions, size_t size);
    
    SourceLineInfo *NewSourceLineInfo(const char *file_name, const std::vector<int> &lines) {
        return NewSourceLineInfo(file_name, &lines[0], lines.size());
    }

    inline SourceLineInfo *NewSourceLineInfo(const char *file_name, const int *lines, size_t n);
    
    PrototypeDesc *NewPrototypeDesc(const std::vector<uint32_t> &desc, bool has_vargs) {
        DCHECK_GT(desc.size(), 0) << "Tail must be return type.";
        return NewPrototypeDesc(&desc[0], desc.size() - 1, has_vargs, desc.back());
    }
    
    inline PrototypeDesc *NewPrototypeDesc(const uint32_t *parameters, size_t n, bool has_vargs,
                                           uint32_t return_type);
    
    // mark all pages to readonly
    void MarkReadonly(bool readonly) {
        // TODO:
        TODO();
    }

    // Rollback to snapshot type id
    void Rollback(uint32_t snapshot) {
        // TODO:
        TODO();
    }

    void InvalidateAllLookupTables() {
        {
            std::lock_guard<std::shared_mutex> lock(class_fields_mutex_);
            named_class_fields_.clear();
        } {
            std::lock_guard<std::shared_mutex> lock(class_methods_mutex_);
            named_class_methods_.clear();
        }
        // TODO:
    }
    
    void InvalidateLanguageClasses() {
        class_object_ = DCHECK_NOTNULL(FindClassOrNull("lang.Object"));
        class_exception_ = DCHECK_NOTNULL(FindClassOrNull("lang.Exception"));
    }
    
    Class *PrepareClass(const std::string &name, uint32_t tags, uint32_t reference_size) {
        Class *clazz = New<Class>();
        clazz->id_ = NextTypeId();
        clazz->name_ = NewString(name);
        clazz->tags_ = tags;
        clazz->reference_size_ = reference_size;
        InsertClass(name, clazz);
        return clazz;
    }

    // allocate flat memory block
    AllocationResult Allocate(size_t n, bool exec);
    
    friend class ClassBuilder;
    friend class FunctionBuilder;
    DISALLOW_IMPLICIT_CONSTRUCTORS(MetadataSpace);
private:
    Error GenerateBuiltinCode();
    
    Error GenerateBytecodeHandlerCode();
    
    struct ClassFieldKey {
        const Class *clazz;
        std::string_view name;
    }; // struct ClassFieldKey
    
    struct ClassMethodPair {
        const Class *owns;
        const Method *method;
    }; // struct ClassMethodPair

    struct ClassFieldHash : public std::unary_function<ClassFieldKey, size_t> {
        size_t operator () (ClassFieldKey key) const {
            return std::hash<const Class *>{}(key.clazz) +
                base::Hash::Js(key.name.data(), key.name.size());
        }
    }; // struct ClassFieldHash
    
    struct ClassFieldEqualTo : public std::binary_function<ClassFieldKey, ClassFieldKey, bool> {
        bool operator () (ClassFieldKey lhs, ClassFieldKey rhs) const {
            return lhs.clazz == rhs.clazz && lhs.name.compare(rhs.name) == 0;
        }
    }; // struct ClassFieldEqualTo

    template<class T> inline T *NewArray(size_t n);

    template<class T> inline T *New();
    
    inline LinearPage *AppendPage(int access);
    
    uint32_t NextTypeId() { return next_type_id_++; }
    
    void InsertClass(const std::string &name, Class *clazz) {
#if defined(DEBUG) || defined(_DEBUG)
        auto iter = named_classes_.find(name);
        DCHECK(iter == named_classes_.end());
#endif
        classes_.push_back(clazz);
        named_classes_[name] = clazz;
    }

    LinearPage *code_head() const { return LinearPage::Cast(code_dummy_->next_); }
    LinearPage *page_head() const { return LinearPage::Cast(page_dummy_->next_); }
    
    PageHeader *page_dummy_; // Linear page double-linked list dummy (LinearPage)
    PageHeader *code_dummy_; // Linear page double-linked list dummy (LinearPage)
    PageHeader *large_dummy_; // Large page double-linked list dummy (LargePage)
    std::vector<Class *> classes_; // All classes
    std::unordered_map<std::string, Class *> named_classes_; // All named classes
    uint32_t next_type_id_ = 0; // Next type unique id
    size_t n_pages_ = 0; // Number of linear pages.
    size_t large_size_ = 0; // All large page's size
    size_t used_size_ = 0; // Allocated used bytes
    
    // Class with field name
    using ClassFieldMap = std::unordered_map<ClassFieldKey, const Field*, ClassFieldHash,
        ClassFieldEqualTo>;
    using ClassMethodMap = std::unordered_map<ClassFieldKey, ClassMethodPair, ClassFieldHash,
        ClassFieldEqualTo>;
    ClassFieldMap named_class_fields_;
    ClassMethodMap named_class_methods_;
    std::shared_mutex class_fields_mutex_;
    std::shared_mutex class_methods_mutex_;
    
    // Language classes:
    const Class *class_object_ = nullptr;
    const Class *class_exception_ = nullptr;
    
    // Builtin codes:
    Code *bytecode_handlers_[kMax_Bytecodes]; // Bytecode handler array
    
    Code *sanity_test_stub_code_ = nullptr; // For code sanity testing

    // For enter mai execution env
    // Prototype: Trampoline(Coroutine *co);
    Code *trampoline_code_ = nullptr;
    Address trampoline_suspend_point_ = nullptr;
    
    // For bytecode execution
    // Prototype: InterpreterPump(Closure *fn);
    Code *interpreter_pump_code_ = nullptr;

    // For function template testing
    // Prototype: Dummy(Coroutine *co);
    Code *function_template_dummy_code_ = nullptr;

    // Switch system stack(C++ stack) and call a function
    Code *switch_system_stack_call_code_ = nullptr;
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
        DISALLOW_IMPLICIT_CONSTRUCTORS(FieldBuilder);
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
            //DCHECK(fn_ != nullptr);
            return *owner_;
        }
        
        friend class ClassBuilder;
        DISALLOW_IMPLICIT_CONSTRUCTORS(MethodBuilder);
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
    
    ClassBuilder &init(Closure *value) {
        init_ = value;
        return *this;
    }
    
    ClassBuilder &tags(uint32_t value) {
        tags_ = value;
        return *this;
    }
    
    ClassBuilder &reference_size(uint32_t value) {
        reference_size_ = value;
        return *this;
    }
    
    ClassBuilder &instrance_size(uint32_t value) {
        instrance_size_ = value;
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
    
    Class *BuildWithPreparedClass(MetadataSpace *space, Class *clazz) const;
    
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
    uint32_t reference_size_ = 0;
    uint32_t instrance_size_ = 0;
    Closure *init_ = 0;
    std::vector<FieldBuilder *> fields_;
    std::map<std::string, FieldBuilder *> named_fields_;
    std::map<std::string, MethodBuilder *> named_methods_;
    const Class *base_ = nullptr;
}; // class ClassBuilder


class FunctionBuilder final {
public:
    FunctionBuilder(const std::string name) : name_(name) {}
    
    FunctionBuilder &name(const std::string &value) {
        name_ = value;
        return *this;
    }
    
    FunctionBuilder &kind(Function::Kind value) {
        kind_ = value;
        return *this;
    }
    
    FunctionBuilder &prototype(const std::vector<uint32_t> &parameters, bool has_vargs, uint32_t return_type) {
        prototype_ = parameters;
        prototype_.push_back(return_type);
        has_vargs_ = has_vargs;
        return *this;
    }

    FunctionBuilder &code(Code *value) {
        code_ = DCHECK_NOTNULL(value);
        return *this;
    }

    FunctionBuilder &bytecode(BytecodeArray *value) {
        bytecode_ = DCHECK_NOTNULL(value);
        return *this;
    }

    FunctionBuilder &stack_size(uint32_t value) {
        DCHECK_GT(0x1000, value) << "stack-size too big";
        stack_size_ = value;
        return *this;
    }

    FunctionBuilder &stack_bitmap(const uint32_t *value, size_t n) {
        stack_bitmap_.resize(n);
        ::memcpy(&stack_bitmap_[0], value, n * sizeof(value[0]));
        return *this;
    }
    
    FunctionBuilder &stack_bitmap(const std::vector<uint32_t> &value) {
        stack_bitmap_ = value;
        return *this;
    }
    
    FunctionBuilder &const_pool(const std::vector<Span32> &value) {
        const_pool_ = value;
        return *this;
    }
    
    FunctionBuilder &const_pool(std::vector<Span32> &&value) {
        const_pool_ = value;
        return *this;
    }
    
    FunctionBuilder &const_pool(const Span32 *value, size_t n) {
        const_pool_.resize(n);
        ::memcpy(&const_pool_[0], value, n * sizeof(value[0]));
        return *this;
    }
    
    FunctionBuilder &const_pool_bitmap(const std::vector<uint32_t> &value) {
        const_pool_bitmap_ = value;
        return *this;
    }
    
    FunctionBuilder &const_pool_bitmap(std::vector<uint32_t> &&value) {
        const_pool_bitmap_ = value;
        return *this;
    }
    
    FunctionBuilder &const_pool_bitmap(const uint32_t *value, size_t n) {
        const_pool_bitmap_.resize(n);
        ::memcpy(&const_pool_bitmap_[0], value, n * sizeof(value[0]));
        return *this;
    }
    
    FunctionBuilder &source_line_info(SourceLineInfo *value) {
        source_line_info_ = DCHECK_NOTNULL(value);
        return *this;
    }

    FunctionBuilder &movable_invoking_hint(std::vector<Function::InvokingHint> &&value) {
        invoking_hint_ = std::move(value);
        return *this;
    }

    FunctionBuilder &movable_exception_table(std::vector<Function::ExceptionHandlerDesc> &&value) {
        exception_table_ = std::move(value);
        return *this;
    }

    FunctionBuilder &AddCapturedVar(const std::string &name,
                                    const Class *type,
                                    Function::CapturedVarKind kind,
                                    int32_t index) {
        captured_vars_.push_back({name, type, kind, index});
        return *this;
    }

    FunctionBuilder &AddExceptionHandle(const Class *expected,
                                        intptr_t start_pc,
                                        intptr_t stop_pc,
                                        intptr_t handle_pc) {
        exception_table_.push_back({expected, start_pc, stop_pc, handle_pc});
        return *this;
    }
    
    FunctionBuilder &AddInvokingHint(intptr_t pc, uint32_t flags, int stack_ref_top) {
        invoking_hint_.push_back({pc, flags, stack_ref_top});
        return *this;
    }

    bool Valid() const;

    Function *Build(MetadataSpace *space) const;

    DISALLOW_IMPLICIT_CONSTRUCTORS(FunctionBuilder);
private:
    struct CapturedVarDesc {
        std::string name;
        const Class *type;
        Function::CapturedVarKind kind;
        int32_t index;
    }; // struct CapturedVarDesc
    
    std::string name_;
    std::vector<uint32_t> prototype_;
    bool has_vargs_ = false;
    Function::Kind kind_ = Function::BYTECODE;
    Code *code_ = nullptr;
    BytecodeArray *bytecode_ = nullptr;
    uint32_t stack_size_ = 0;
    std::vector<uint32_t> stack_bitmap_;
    std::vector<Span32> const_pool_;
    std::vector<uint32_t> const_pool_bitmap_;
    SourceLineInfo *source_line_info_ = nullptr;
    std::vector<CapturedVarDesc> captured_vars_;
    std::vector<Function::ExceptionHandlerDesc> exception_table_;
    std::vector<Function::InvokingHint> invoking_hint_;
}; // class FunctionBuilder


inline MDStr MetadataSpace::NewString(const char *z, size_t n) {
    auto result = Allocate(sizeof(MDStrHeader) + n + 1, false/*exec*/);
    if (!result.ok()) {
        return nullptr;
    }
    MDStrHeader *str = new (result.ptr()) MDStrHeader(z, n);
    return str->data();
}

inline Code *MetadataSpace::NewCode(Code::Kind kind, Address instructions, size_t size,
                                    PrototypeDesc *prototype) {
    auto result = Allocate(sizeof(Code) + size, true/*exec*/);
    if (!result.ok()) {
        return nullptr;
    }
    return new (result.ptr()) Code(kind, instructions, static_cast<uint32_t>(size), prototype);
}

inline BytecodeArray *MetadataSpace::NewBytecodeArray(const BytecodeInstruction *instructions,
                                                      size_t size) {
    auto result = Allocate(sizeof(BytecodeArray) + size * sizeof(instructions[0]), false/*exec*/);
    if (!result.ok()) {
        return nullptr;
    }
    return new (result.ptr()) BytecodeArray(instructions, static_cast<uint32_t>(size));
}

inline SourceLineInfo *MetadataSpace::NewSourceLineInfo(const char *file_name, const int *lines,
                                                        size_t n) {
    auto result = Allocate(sizeof(SourceLineInfo) + n * sizeof(lines[0]), false/*exec*/);
    if (!result.ok()) {
        return nullptr;
    }
    auto md_file_name = NewString(file_name);
    if (!md_file_name) {
        return nullptr;
    }
    return new (result.ptr()) SourceLineInfo(md_file_name, lines, n);
}

inline PrototypeDesc *MetadataSpace::NewPrototypeDesc(const uint32_t *parameters, size_t n,
                                                      bool has_vargs, uint32_t return_type) {
    auto result = Allocate(sizeof(PrototypeDesc) + n *sizeof(parameters[0]), false/*exec*/);
    if (!result.ok()) {
        return nullptr;
    }
    return new (result.ptr()) PrototypeDesc(parameters, static_cast<uint32_t>(n), has_vargs,
                                            return_type);
}

template<class T>
inline T *MetadataSpace::NewArray(size_t n) {
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
inline T *MetadataSpace::New() {
    auto result = Allocate(sizeof(T), false);
    if (!result.ptr()) {
        return nullptr;
    }
    return new (result.ptr()) T();
}

inline LinearPage *MetadataSpace::AppendPage(int access) {
    LinearPage *page = LinearPage::New(kind(), access, lla_);
    if (!page) {
        return nullptr;
    }
    if (access & Allocator::kEx) {
        QUEUE_INSERT_HEAD(code_dummy_, page);
    } else {
        QUEUE_INSERT_HEAD(page_dummy_, page);
    }
    n_pages_++;
    return page;
}

} // namespace lang

} // namespace mai

#endif // MAI_LANG_METADATA_SPACE_H_
