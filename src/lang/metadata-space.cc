#include "lang/metadata-space.h"
#include "lang/value-inl.h"
#include "lang/stack.h"
#include "lang/macro-assembler-x64.h"

namespace mai {

namespace lang {

MetadataSpace::MetadataSpace(Allocator *lla)
    : Space(kMetadataSpace, lla)
    , dummy_page_(new PageHeader(kDummySpace))
    , dummy_code_(new PageHeader(kDummySpace))
    , dummy_large_(new PageHeader(kDummySpace)) {
}

MetadataSpace::~MetadataSpace() {
    while (!QUEUE_EMPTY(dummy_large_)) {
        auto x = LargePage::Cast(dummy_large_->next_);
        QUEUE_REMOVE(x);
        x->Dispose(lla_);
    }
    delete dummy_large_;
    
    while (!QUEUE_EMPTY(dummy_code_)) {
        auto x = LinearPage::Cast(dummy_code_->next_);
        QUEUE_REMOVE(x);
        x->Dispose(lla_);
    }
    delete dummy_code_;
    
    while (!QUEUE_EMPTY(dummy_page_)) {
        auto x = LinearPage::Cast(dummy_page_->next_);
        QUEUE_REMOVE(x);
        x->Dispose(lla_);
    }
    delete dummy_page_;
}

// Init builtin types
Error MetadataSpace::Initialize() {
    // Void type
    Class *clazz = New<Class>();
    clazz->id_ = NextTypeId();
    clazz->ref_size_ = 0;
    clazz->base_ = nullptr;
    clazz->name_ = NewString("void");
    clazz->tags_ = Type::kBuiltinTag;
    InsertClass("void", clazz);
    DCHECK_EQ(kType_void, clazz->id()) << "void";
    
#define DEFINE_PRIMITIVE_CLASS(literal, kind, ...) \
    clazz = New<Class>(); \
    clazz->id_ = NextTypeId(); \
    clazz->ref_size_ = sizeof(kind); \
    clazz->name_ = NewString(#literal); \
    clazz->tags_ = Type::kBuiltinTag|Type::kPrimitiveTag; \
    InsertClass(#literal, clazz); \
    DCHECK_EQ(kType_##literal, clazz->id()) << #literal;

    DECLARE_PRIMITIVE_TYPES(DEFINE_PRIMITIVE_CLASS)

#undef DEFINE_PRIMITIVE_CLASS

    // Any class
    ClassBuilder("any")
        .tags(Type::kBuiltinTag|Type::kReferenceTag)
        .ref_size(kPointerSize)
        .field("klass")
            .type(builtin_type(kType_u64))
            .flags(Field::kPrivate|Field::kRdWr)
            .tag(1)
            .offset(Any::kOffsetKlass)
        .End()
        .field("tags")
            .type(builtin_type(kType_u32))
            .flags(Field::kPrivate|Field::kRdWr)
            .tag(2)
            .offset(Any::kOffsetTags)
        .End()
    .Build(this);
    
#define DEFINE_BUILTIN_CLASS(literal, primitive, kind, ...) \
    clazz = ClassBuilder(#literal) \
        .tags(Type::kBuiltinTag|Type::kReferenceTag) \
        .base(builtin_type(kType_any)) \
        .ref_size(kPointerSize) \
        .field("value") \
            .type(builtin_type(kType_##primitive)) \
            .flags(Field::kPrivate|Field::kRead) \
            .tag(1) \
            .offset(AbstractValue::kOffsetValue) \
        .End() \
    .Build(this); \
    DCHECK_EQ(kType_##literal, clazz->id()) << #literal;

    DECLARE_BOX_NUMBER_TYPES(DEFINE_BUILTIN_CLASS)
        
#undef DEFINE_BUILTIN_CLASS
    
    clazz = ClassBuilder("captured_value")
        .tags(Type::kBuiltinTag|Type::kReferenceTag)
        .base(builtin_type(kType_any))
        .ref_size(kPointerSize)
        .field("value")
            .type(builtin_type(kType_any))
            .flags(Field::kPrivate|Field::kRead)
            .tag(1)
            .offset(AbstractValue::kOffsetValue)
        .End()
    .Build(this);
    DCHECK_EQ(kType_captured_value, clazz->id()) << "captured_value";
    
    clazz = ClassBuilder("closure")
        .tags(Type::kBuiltinTag|Type::kReferenceTag)
        .base(builtin_type(kType_any))
        .ref_size(kPointerSize)
        .field("proto")
            .type(builtin_type(kType_u64))
            .flags(Field::kPrivate|Field::kRead)
            .tag(1)
            .offset(Closure::kOffsetProto)
        .End()
        .field("captured_var_size")
            .type(builtin_type(kType_u32))
            .flags(Field::kPrivate|Field::kRead)
            .tag(2)
            .offset(Closure::kOffsetCapturedVarSize)
        .End()
        .field("captured_var")
            .type(builtin_type(kType_captured_value))
            .flags(Field::kPrivate|Field::kRead)
            .tag(3)
            .offset(Closure::kOffsetCapturedVar)
        .End()
    .Build(this);
    DCHECK_EQ(kType_closure, clazz->id()) << "closure";
    
    // String class
    ClassBuilder("string")
        .tags(Type::kBuiltinTag|Type::kReferenceTag)
        .ref_size(kPointerSize)
        .field("capacity")
            .type(builtin_type(kType_u32))
            .flags(Field::kPrivate|Field::kRead)
            .tag(1)
            .offset(String::kOffsetCapacity)
        .End()
        .field("length")
            .type(builtin_type(kType_u32))
            .flags(Field::kPublic|Field::kRead)
            .tag(2)
            .offset(String::kOffsetLength)
        .End()
        .field("elems")
            .type(TypeOf<char>())
            .flags(Field::kPublic|Field::kRead|Field::kArray)
            .tag(3)
            .offset(String::kOffsetElems)
        .End()
    .Build(this);

    // MutableMap::Entry
    clazz = ClassBuilder("mutable_map.entry")
        .tags(Type::kBuiltinTag|Type::kReferenceTag)
        .ref_size(kPointerSize)
        .field("next")
            .type(builtin_type(kType_i8))
            .flags(Field::kPublic|Field::kRead)
            .tag(1)
            .offset(MutableMapEntry::kOffsetNext)
        .End()
        .field("hash")
            .type(builtin_type(kType_u32))
            .flags(Field::kPublic|Field::kRead)
            .tag(2)
            .offset(MutableMapEntry::kOffsetHash)
        .End()
        .field("key")
            .type(builtin_type(kType_any))
            .flags(Field::kPublic|Field::kRdWr)
            .tag(3)
            .offset(MutableMapEntry::kOffsetKey)
        .End()
        .field("value")
            .type(builtin_type(kType_any))
            .flags(Field::kPublic|Field::kRdWr)
            .tag(4)
            .offset(MutableMapEntry::kOffsetValue)
        .End()
    .Build(this);
    DCHECK_EQ(kType_mutable_map_entry, clazz->id());
    clazz->fields_[0].type_ = clazz; // Fix self type

#define DEFINE_BUILTIN_CLASS(literal, kind, ...) \
    clazz = ClassBuilder(#literal) \
        .tags(Type::kBuiltinTag|Type::kReferenceTag) \
        .ref_size(kPointerSize) \
        .field("capacity") \
            .type(builtin_type(kType_u32)) \
            .flags(Field::kPrivate|Field::kRead) \
            .tag(1) \
            .offset(Array<kind>::kOffsetCapacity) \
        .End() \
        .field("length") \
            .type(builtin_type(kType_u32)) \
            .flags(Field::kPublic|Field::kRead) \
            .tag(2) \
            .offset(Array<kind>::kOffsetLength) \
        .End() \
        .field("elems") \
            .type(TypeOf<kind>()) \
            .flags(Field::kPublic|Field::kRead|Field::kArray) \
            .tag(3) \
            .offset(Array<kind>::kOffsetElems) \
        .End() \
    .Build(this); \
    DCHECK_EQ(kType_##literal, clazz->id()) << #literal;
    
    DECLARE_ARRAY_TYPES(DEFINE_BUILTIN_CLASS)
    DECLARE_MUTABLE_ARRAY_TYPES(DEFINE_BUILTIN_CLASS)
    
#undef DEFINE_BUILTIN_CLASS
    
    // Other types from this id;
    DCHECK_LT(next_type_id_, kUserTypeIdBase);
    next_type_id_ = kUserTypeIdBase;
    
    if (Error err = GenerateBuiltinCode(); err.fail()) {
        return err;
    }
    return Error::OK();
}

Error MetadataSpace::GenerateBuiltinCode() {
    MacroAssembler masm;

    Generate_SanityTestStub(&masm);
    sanity_test_stub_code_ = NewCode(Code::BUILTIN, masm.buf(), nullptr);
    if (!sanity_test_stub_code_) {
        return MAI_CORRUPTION("OOM by generate code");
    }
    masm.Reset();

    // Make sure code generation is ok
    CallStub<intptr_t(intptr_t, intptr_t)> call_stub(sanity_test_stub_code_);
    if (call_stub.entry()(1, -1) != 0) {
        return MAI_CORRUPTION("Incorrect code generator");
    }

    Generate_SwitchSystemStackCall(&masm);
    switch_system_stack_call_code_ = NewCode(Code::BUILTIN, masm.buf(), nullptr);
    if (!switch_system_stack_call_code_) {
        return MAI_CORRUPTION("OOM by generate code");
    }
    masm.Reset();
    
    Generate_FunctionTemplateTestDummy(&masm);
    function_template_dummy_code_ = NewCode(Code::BUILTIN, masm.buf(), nullptr);
    if (!function_template_dummy_code_) {
        return MAI_CORRUPTION("OOM by generate code");
    }
    masm.Reset();

    return Error::OK();
}

const Field *MetadataSpace::FindClassFieldOrNull(const Class *clazz, const char *field_name) {
    ClassFieldKey key{clazz, field_name};
    {
        std::shared_lock<std::shared_mutex> lock(class_fields_mutex_);
        auto iter = named_class_fields_.find(key);
        if (iter != named_class_fields_.end()) {
            return iter->second;
        }
    }

    const Field *field = nullptr;
    for (uint32_t i = 0; i < clazz->n_fields(); i++) {
        if (::strcmp(field_name, clazz->field(i)->name()) == 0) {
            field = clazz->field(i);
            break;
        }
    }
    if (!field) {
        return nullptr;
    }
    
    // Use self space's string
    key.field_name = field->name();
    std::lock_guard<std::shared_mutex> lock(class_fields_mutex_);
    named_class_fields_[key] = field;
    return field;
}

AllocationResult MetadataSpace::Allocate(size_t n, bool exec) {
    if (!n) {
        return AllocationResult(AllocationResult::NOTHING, nullptr);
    }
    n = RoundUp(n, kAligmentSize);

    int access = Allocator::kWr | Allocator::kRd | (exec ? Allocator::kEx : 0);
    if (LinearPage::IsLarge(n)) {
        auto page = LargePage::New(kind(), access, n, lla_);
        if (!page) {
            return AllocationResult(AllocationResult::OOM, nullptr);
        }
        QUEUE_INSERT_TAIL(dummy_large_, page);
        used_size_ += n;
        large_size_ += page->size();
        return AllocationResult(AllocationResult::OK, page->chunk());
    }

    LinearPage *page = nullptr;
    if (exec) {
        if (QUEUE_EMPTY(dummy_code_) || n > code_head()->GetFreeSize()) {
            page = AppendPage(access);
        } else {
            page = code_head();
        }
    } else {
        if (QUEUE_EMPTY(dummy_page_) || n > page_head()->GetFreeSize()) {
            page = AppendPage(access);
        } else {
            page = page_head();
        }
    }
    if (!page) {
        return AllocationResult(AllocationResult::OOM, nullptr);
    }
    Address chunk = page->Advance(n);
    if (!chunk) {
        return AllocationResult(AllocationResult::FAIL, nullptr);
    }
    used_size_ += n;
    return AllocationResult(AllocationResult::OK, chunk);
}

Class *ClassBuilder::Build(MetadataSpace *space) const {
    Class *clazz = space->New<Class>();
    if (!clazz) {
        return nullptr;
    }
    clazz->name_ = space->NewString(name_);
    clazz->base_ = base_;
    clazz->tags_ = tags_;
    clazz->ref_size_ = ref_size_;
    
    Field *fields = space->NewArray<Field>(fields_.size());
    clazz->n_fields_ = static_cast<uint32_t>(fields_.size());
    clazz->fields_ = fields;
    for (size_t i = 0; i < fields_.size(); i++) {
        BuildField(fields + i, fields_[i], space);
    }
    
    Method *methods = space->NewArray<Method>(named_methods_.size());
    clazz->n_methods_ = static_cast<uint32_t>(named_methods_.size());
    clazz->methods_ = methods;
    size_t i = 0;
    for (auto pair : named_methods_) {
        methods[i].name_ = space->NewString(pair.first);
        methods[i].tags_ = pair.second->tags_;
        methods[i].fn_ = pair.second->fn_;
        i++;
    }

    clazz->id_ = space->NextTypeId();
    space->InsertClass(name_, clazz);
    return clazz;
}

bool FunctionBuilder::Valid() const {
    if (name_.empty() || prototype_.size() < 1) {
        DLOG(ERROR) << "name or prototype is empty: " << name_ << ":" << prototype_.size();
        return false;
    }
    
    if (auto required = (const_pool_.size() + 31) / 32; const_pool_bitmap_.size() != required) {
        DLOG(ERROR) << "Incorrect const_pool_bitmap_.size: "
                    << const_pool_bitmap_.size() << " vs " << required;
        return false;
    }
    
    if (stack_size_ % kStackAligmentSize) {
        DLOG(ERROR) << "stack_size_ is not aligment: " << stack_size_;
        return false;
    }
    
    if (auto required = ((stack_size_ / kStackAligmentSize) + 31) / 32; stack_bitmap_.size() != required) {
        DLOG(ERROR) << "Incorrect stack_bitmap_.size: "
                    << stack_bitmap_.size() << " vs " << required;
        return false;
    }

    if (kind_ == Function::BYTECODE) {
        if (!bytecode_) {
            DLOG(ERROR) << "Bytecode function bytecode_ == nullptr";
            return false;
        }
    }
    
    if (kind_ == Function::COMPILED) {
        if (!code_) {
            DLOG(ERROR) << "Compied function code_ == nullptr";
            return false;
        }
    }
    
    return true;
}

Function *FunctionBuilder::Build(MetadataSpace *space) const {
    DCHECK(Valid()) << "Function is NOT valid";
    
    Function *func = space->New<Function>();
    func->name_ = space->NewString(name_);
    func->kind_ = kind_;
    func->prototype_ = space->NewPrototypeDesc(prototype_, has_vargs_);
    func->code_ = code_;
    func->bytecode_ = bytecode_;
    func->source_line_info_ = source_line_info_;
    
    func->stack_bitmap_ = space->NewArray<uint32_t>(stack_bitmap_.size());
    ::memcpy(func->stack_bitmap_, &stack_bitmap_[0], stack_bitmap_.size() * sizeof(uint32_t));
    func->stack_size_ = stack_size_;

    func->const_pool_ = space->NewArray<Span32>(const_pool_.size());
    ::memcpy(func->const_pool_, &const_pool_[0], const_pool_.size() * sizeof(Span32));
    func->const_pool_size_ = static_cast<uint32_t>(const_pool_.size() * sizeof(Span32));
    func->const_pool_bitmap_ = space->NewArray<uint32_t>(const_pool_bitmap_.size());
    ::memcpy(func->const_pool_bitmap_, &const_pool_bitmap_[0],
             const_pool_bitmap_.size() * sizeof(uint32_t));

    func->captured_vars_ = space->NewArray<Function::CapturedVarDesc>(captured_vars_.size());
    for (size_t i = 0; i < captured_vars_.size(); i++) {
        func->captured_vars_[i].name  = space->NewString(captured_vars_[i].name);
        func->captured_vars_[i].kind  = captured_vars_[i].kind;
        func->captured_vars_[i].index = captured_vars_[i].index;
    }
    func->captured_var_size_ = static_cast<uint32_t>(captured_vars_.size());

    func->exception_table_ = space->NewArray<Function::ExceptionHandlerDesc>(exception_table_.size());
    ::memcpy(func->exception_table_, &exception_table_[0],
             exception_table_.size() * sizeof(Function::ExceptionHandlerDesc));
    func->exception_table_size_ = static_cast<uint32_t>(exception_table_.size());
    return func;
}

} // namespace lang

} // namespace mai
