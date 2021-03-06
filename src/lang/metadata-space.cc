#include "lang/metadata-space.h"
#include "lang/channel.h"
#include "lang/value-inl.h"
#include "lang/stack.h"
#include "lang/object-visitor.h"
#include "lang/isolate-inl.h"
#include "lang/macro-assembler-x64.h"


namespace mai {

namespace lang {

MetadataSpace::MetadataSpace(Allocator *lla)
    : Space(kMetadataSpace, lla)
    , page_dummy_(new PageHeader(kDummySpace))
    , code_dummy_(new PageHeader(kDummySpace))
    , large_dummy_(new PageHeader(kDummySpace)) {
    ::memset(bytecode_handlers_, 0, sizeof(bytecode_handlers_[0]) * kMax_Bytecodes);
    ::memset(tracing_handlers_, 0, sizeof(tracing_handlers_[0]) * kMax_Bytecodes);
}

MetadataSpace::~MetadataSpace() {
    while (!QUEUE_EMPTY(large_dummy_)) {
        auto x = LargePage::Cast(large_dummy_->next_);
        QUEUE_REMOVE(x);
        x->Dispose(lla_);
    }
    delete large_dummy_;
    
    while (!QUEUE_EMPTY(code_dummy_)) {
        auto x = LinearPage::Cast(code_dummy_->next_);
        QUEUE_REMOVE(x);
        x->Dispose(lla_);
    }
    delete code_dummy_;
    
    while (!QUEUE_EMPTY(page_dummy_)) {
        auto x = LinearPage::Cast(page_dummy_->next_);
        QUEUE_REMOVE(x);
        x->Dispose(lla_);
    }
    delete page_dummy_;
}

// Init builtin types
Error MetadataSpace::Initialize(bool enable_debug, bool enable_jit) {
    // Void type
    Class *clazz = New<Class>();
    clazz->id_ = NextTypeId();
    clazz->reference_size_ = 0;
    clazz->instrance_size_ = 0;
    clazz->base_ = nullptr;
    clazz->name_ = NewString("void");
    clazz->tags_ = Type::kBuiltinTag;
    InsertClass("void", clazz);
    DCHECK_EQ(kType_void, clazz->id()) << "void";

#define DEFINE_PRIMITIVE_CLASS(literal, kind, ...) \
    clazz = New<Class>(); \
    clazz->id_ = NextTypeId(); \
    clazz->reference_size_ = sizeof(kind); \
    clazz->instrance_size_ = sizeof(kind); \
    clazz->name_ = NewString(#literal); \
    clazz->tags_ = Type::kBuiltinTag|Type::kPrimitiveTag; \
    InsertClass(#literal, clazz); \
    DCHECK_EQ(kType_##literal, clazz->id()) << #literal;

    DECLARE_PRIMITIVE_TYPES(DEFINE_PRIMITIVE_CLASS)

#undef DEFINE_PRIMITIVE_CLASS

    // Any class
    ClassBuilder("any")
        .tags(Type::kBuiltinTag|Type::kReferenceTag)
        .reference_size(kPointerSize)
        .instrance_size(sizeof(Any))
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
        .reference_size(kPointerSize) \
        .instrance_size(sizeof(Number<kind>)) \
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
        .reference_size(kPointerSize)
        .instrance_size(sizeof(CapturedValue))
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
        .reference_size(kPointerSize)
        .instrance_size(sizeof(Closure))
        .field("proto")
            .type(builtin_type(kType_u64))
            .flags(Field::kPrivate|Field::kRead)
            .tag(1)
            .offset(Closure::kOffsetProto)
        .End()
        .field("capturedVarSize")
            .type(builtin_type(kType_u32))
            .flags(Field::kPrivate|Field::kRead)
            .tag(2)
            .offset(Closure::kOffsetCapturedVarSize)
        .End()
        .field("capturedVar")
            .type(builtin_type(kType_captured_value))
            .flags(Field::kPrivate|Field::kRead)
            .tag(3)
            .offset(Closure::kOffsetCapturedVar)
        .End()
    .Build(this);
    DCHECK_EQ(kType_closure, clazz->id()) << "closure";

    // String class
    clazz = ClassBuilder("string")
        .tags(Type::kBuiltinTag|Type::kReferenceTag)
        .reference_size(kPointerSize)
        .instrance_size(sizeof(String))
        .base(builtin_type(kType_any))
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
    DCHECK_EQ(kType_string, clazz->id()) << "string";

    clazz = ClassBuilder("channel")
        .tags(Type::kBuiltinTag|Type::kReferenceTag)
        .reference_size(kPointerSize)
        .instrance_size(sizeof(Channel))
        .base(builtin_type(kType_any))
        .field("hasClose")
            .type(builtin_type(kType_bool))
            .flags(Field::kVolatile|Field::kPublic|Field::kRead)
            .tag(1)
            .offset(Channel::kOffsetClose)
        .End()
        .field("capacity")
            .type(builtin_type(kType_u32))
            .flags(Field::kPublic|Field::kRead)
            .tag(2)
            .offset(Channel::kOffsetCapacity)
        .End()
        .field("length")
            .type(builtin_type(kType_u32))
            .flags(Field::kVolatile|Field::kPublic|Field::kRead)
            .tag(3)
            .offset(Channel::kOffsetLength)
        .End()
        .method("close")
            .tags(Method::kNative)
            .fn(nullptr)
        .End()
    .Build(this);
    DCHECK_EQ(kType_channel, clazz->id()) << "channel";

#define DEFINE_BUILTIN_CLASS(literal, kind, ...) \
    clazz = ClassBuilder(#literal) \
        .tags(Type::kBuiltinTag|Type::kReferenceTag) \
        .reference_size(kPointerSize) \
        .instrance_size(sizeof(Array<kind>)) \
        .base(builtin_type(kType_any)) \
        .field("elemType") \
            .type(builtin_type(kType_u64)) \
            .flags(Field::kPrivate|Field::kRead) \
            .tag(1) \
            .offset(Array<kind>::kOffsetElemType) \
        .End() \
        .field("capacity") \
            .type(builtin_type(kType_u32)) \
            .flags(Field::kPrivate|Field::kRead) \
            .tag(2) \
            .offset(Array<kind>::kOffsetCapacity) \
        .End() \
        .field("length") \
            .type(builtin_type(kType_u32)) \
            .flags(Field::kPublic|Field::kRead) \
            .tag(3) \
            .offset(Array<kind>::kOffsetLength) \
        .End() \
        .field("elems") \
            .type(TypeOf<kind>()) \
            .flags(Field::kPublic|Field::kRead|Field::kArray) \
            .tag(4) \
            .offset(Array<kind>::kOffsetElems) \
        .End() \
        .method("resize") \
            .tags(Method::kNative) \
            .fn(nullptr) \
        .End() \
    .Build(this); \
    DCHECK_EQ(kType_##literal, clazz->id()) << #literal;
    
    DECLARE_ARRAY_TYPES(DEFINE_BUILTIN_CLASS)
    
#undef DEFINE_BUILTIN_CLASS
    
#define DEFINE_BUILTIN_CLASS(literal, kind, ...) \
    clazz = ClassBuilder(#literal) \
        .tags(Type::kBuiltinTag|Type::kReferenceTag) \
        .reference_size(kPointerSize) \
        .instrance_size(sizeof(Map<kind, Any *>)) \
        .base(builtin_type(kType_any)) \
        .field("length") \
            .type(builtin_type(kType_u32)) \
            .flags(Field::kPublic|Field::kRead) \
            .tag(1) \
            .offset(AbstractMap::kOffsetLength) \
        .End() \
        .field("randomSeed") \
            .type(builtin_type(kType_u32)) \
            .flags(Field::kPublic|Field::kRead) \
            .tag(2) \
            .offset(AbstractMap::kOffsetRandomSeed) \
        .End() \
        .field("entries") \
            .type(TypeOf<kind>()) \
            .flags(Field::kPublic|Field::kRead|Field::kArray) \
            .tag(3) \
            .offset(ImplementMap<kind>::kOffsetEntries) \
        .End() \
        .method("put") \
            .tags(Method::kNative|Method::kTailAlignBit) \
            .fn(nullptr) \
        .End() \
        .method("remove") \
            .tags(Method::kNative) \
            .fn(nullptr) \
        .End() \
        .method("containsKey") \
            .tags(Method::kNative) \
            .fn(nullptr) \
        .End() \
    .Build(this); \
    DCHECK_EQ(kType_##literal, clazz->id()) << #literal;

    DECLARE_MAP_TYPES(DEFINE_BUILTIN_CLASS)

#undef DEFINE_BUILTIN_CLASS
    
    clazz = ClassBuilder("Code")
        .tags(Type::kBuiltinTag|Type::kReferenceTag)
        .reference_size(kPointerSize)
        .instrance_size(sizeof(Kode))
        .base(builtin_type(kType_any))
        .field("kind")
            .type(builtin_type(kType_int))
            .flags(Field::kPrivate|Field::kRead)
            .tag(1)
            .offset(Kode::kOffsetKind)
        .End()
        .field("optimizationLevel")
            .type(builtin_type(kType_i32))
            .flags(Field::kPrivate|Field::kRead)
            .tag(2)
            .offset(Kode::kOffsetOptimizationLevel)
        .End()
        .field("sourceLineInfo")
            .type(builtin_type(kType_array32))
            .flags(Field::kPrivate|Field::kRead)
            .tag(3)
            .offset(Kode::kOffsetSourceLineInfo)
        .End()
        .field("size")
            .type(builtin_type(kType_u32))
            .flags(Field::kPrivate|Field::kRead)
            .tag(4)
            .offset(Kode::kOffsetSize)
        .End()
        .field("instructions")
            .type(builtin_type(kType_u64))
            .flags(Field::kPrivate|Field::kRead)
            .tag(5)
            .offset(Kode::kOffsetInstructions)
        .End()
    .Build(this);
    DCHECK_EQ(kType_Code, clazz->id()) << "Code";

    clazz = ClassBuilder("Throwable")
        .tags(Type::kBuiltinTag|Type::kReferenceTag)
        .reference_size(kPointerSize)
        .instrance_size(sizeof(Throwable))
        .base(builtin_type(kType_any))
        .field("stacktrace")
            .type(builtin_type(kType_array))
            .flags(Field::kPrivate|Field::kRead)
            .tag(1)
            .offset(Throwable::kOffsetStacktrace)
        .End()
    .Build(this);
    DCHECK_EQ(kType_Throwable, clazz->id()) << "Throwable";
    
    clazz = ClassBuilder("Panic")
        .tags(Type::kBuiltinTag|Type::kReferenceTag)
        .reference_size(kPointerSize)
        .instrance_size(sizeof(Panic))
        .base(builtin_type(kType_Throwable))
        .field("code")
            .type(builtin_type(kType_int))
            .flags(Field::kPrivate|Field::kRead)
            .tag(1)
            .offset(Panic::kOffsetCode)
        .End()
        .field("message")
            .type(builtin_type(kType_string))
            .flags(Field::kPrivate|Field::kRead)
            .tag(2)
            .offset(Panic::kOffsetMessage)
        .End()
    .Build(this);
    DCHECK_EQ(kType_Panic, clazz->id()) << "Panic";
    
    // Other types from this id;
    DCHECK_LT(next_type_id_, kUserTypeIdBase);
    next_type_id_ = kUserTypeIdBase;
    
    if (Error err = GenerateBuiltinCode(enable_debug, enable_jit); err.fail()) {
        return err;
    }
    if (Error err = GenerateBytecodeHandlerCode(enable_debug, enable_jit); err.fail()) {
        return err;
    }
    return Error::OK();
}

void MetadataSpace::VisitRoot(RootVisitor *visitor) {
    for (auto klass : classes_) {
        if (klass->init()) {
            visitor->VisitRootPointer(reinterpret_cast<Any **>(&klass->init()->fn_));
        }
        
        for (uint32_t i = 0; i < klass->n_methods(); i++) {
            Method *method = klass->methods_ + i;
            visitor->VisitRootPointer(reinterpret_cast<Any **>(&method->fn_));
        }
    }
}

Error MetadataSpace::GenerateBuiltinCode(bool enable_debug, bool enable_jit) {
    MacroAssembler masm;

    Generate_SanityTestStub(&masm);
    if (!(sanity_test_stub_code_ = NewCode(Code::BUILTIN, masm.buf(), nullptr))) {
        return MAI_CORRUPTION("OOM by generate code");
    }
    masm.AligmentPatch();
    masm.Reset();

    // Make sure code generation is ok
    CallStub<intptr_t(intptr_t, intptr_t)> call_stub(sanity_test_stub_code_);
    if (call_stub.entry()(1, -1) != 0) {
        return MAI_CORRUPTION("Incorrect code generator");
    }

    Generate_SwitchSystemStackCall(&masm, enable_jit);
    if (!(switch_system_stack_call_code_ = NewCode(Code::BUILTIN, masm.buf(), nullptr))) {
        return MAI_CORRUPTION("OOM by generate code");
    }
    masm.AligmentPatch();
    masm.Reset();
    
    Generate_FunctionTemplateTestDummy(&masm);
    if (!(function_template_dummy_code_ = NewCode(Code::BUILTIN, masm.buf(), nullptr))) {
        return MAI_CORRUPTION("OOM by generate code");
    }
    masm.AligmentPatch();
    masm.Reset();
    
    Generate_InterpreterPump(&masm, switch_system_stack_call_code_->entry(), enable_jit);
    if (!(interpreter_pump_code_ = NewCode(Code::BUILTIN, masm.buf(), nullptr))) {
        return MAI_CORRUPTION("OOM by generate code");
    }
    masm.AligmentPatch();
    masm.Reset();

    int pc = 0;
    Generate_Trampoline(&masm, switch_system_stack_call_code_->entry(),
                        interpreter_pump_code_->entry(), enable_jit, &pc);
    if (!(trampoline_code_ = NewCode(Code::BUILTIN, masm.buf(), nullptr))) {
        return MAI_CORRUPTION("OOM by generate code");
    }
    masm.AligmentPatch();
    masm.Reset();
    trampoline_suspend_point_ = trampoline_code_->entry() + pc;
    
    return Error::OK();
}

Error MetadataSpace::GenerateBytecodeHandlerCode(bool enable_debug, bool enable_jit) {
    MacroAssembler masm;
    std::unique_ptr<AbstractBytecodeEmitter>
    builder(AbstractBytecodeEmitter::New(this, enable_debug, enable_jit));

#define DEFINE_BYTECODE_EMIT(name, kind, ...) \
    builder->Emit##name(&masm); \
    masm.int3(); \
    masm.AligmentPatch(); \
    bytecode_handlers_[k##name] = NewCode(Code::HANDLER, masm.buf(), nullptr); \
    if (!bytecode_handlers_[k##name]) { \
        return MAI_CORRUPTION("OOM by generate code"); \
    } \
    masm.Reset();
    
    DECLARE_ALL_BYTECODE(DEFINE_BYTECODE_EMIT)
    
#undef DEFINE_BYTECODE_EMIT
    
    // Set up CallBytecodeFunction return address
    call_bytecode_return_address_ = bytecode_handlers_[kCallBytecodeFunction]->entry() +
        builder->call_bytecode_return_point();
    if (!enable_jit) {
        return Error::OK();
    }

#define DEFINE_BYTECODE_EMIT(name, kind, ...) \
    Patch_Tracing(&masm); \
    builder->Emit##name(&masm); \
    masm.int3(); \
    masm.AligmentPatch(); \
    tracing_handlers_[k##name] = NewCode(Code::HANDLER, masm.buf(), nullptr); \
    if (!tracing_handlers_[k##name]) { \
        return MAI_CORRUPTION("OOM by generate code"); \
    } \
    masm.Reset();

    DECLARE_ALL_BYTECODE(DEFINE_BYTECODE_EMIT)
        
#undef DEFINE_BYTECODE_EMIT
    
    Patch_Tracing(&masm);
    Patch_BackwardJump(&masm);
    masm.int3();
    masm.AligmentPatch();
    tracing_handlers_[kBackwardJump] = NewCode(Code::HANDLER, masm.buf(), nullptr);
    masm.Reset();
    
    Patch_Tracing(&masm);
    Patch_CheckStack(&masm);
    masm.int3();
    masm.AligmentPatch();
    tracing_handlers_[kCheckStack] = NewCode(Code::HANDLER, masm.buf(), nullptr);
    masm.Reset();
    return Error::OK();
}

const Class *MetadataSpace::GetNumberClassOrNull(BuiltinType primitive_type) const {
    switch (primitive_type) {
    #define DEFINE_TYPE_CASE(src, dst, ...) \
        case kType_##dst: \
            return builtin_type(kType_##src);

        DECLARE_BOX_NUMBER_TYPES(DEFINE_TYPE_CASE)

    #undef DEFINE_TYPE_CASE
        default:
            return nullptr;
    }
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
    key.name = field->name();
    std::lock_guard<std::shared_mutex> lock(class_fields_mutex_);
    named_class_fields_[key] = field;
    return field;
}

PrototypeDesc *MetadataSpace::NewPrototypeDesc(const uint32_t *parameters, size_t n, bool has_vargs,
                                               uint32_t return_type) {
    char stack[sizeof(PrototypeDesc) + 5 * sizeof(uint32_t)];
    std::unique_ptr<char []> heap;
    const size_t required_size = sizeof(PrototypeDesc) + n *sizeof(parameters[0]);
    PrototypeKey key;
    void *buf = stack;
    if (n > 5) {
        heap.reset(new char[required_size]);
        buf = heap.get();
    }
    key.desc = new (buf) PrototypeDesc(parameters, static_cast<uint32_t>(n), has_vargs, return_type);
    if (auto iter = unique_prototype_.find(key); iter != unique_prototype_.end()) {
        return iter->second;
    }
    
    auto result = Allocate(required_size, false/*exec*/);
    if (!result.ok()) {
        return nullptr;
    }
    PrototypeDesc *value = new (result.ptr()) PrototypeDesc(parameters, static_cast<uint32_t>(n),
                                                            has_vargs, return_type);
    key.desc = value;
    unique_prototype_[key] = value;
    return value;
}

std::tuple<const Class *, const Method *>
MetadataSpace::FindClassMethod(const Class *clazz, const char *method_name) {
    ClassFieldKey key{clazz, method_name};
    {
        std::shared_lock<std::shared_mutex> lock(class_methods_mutex_);
        auto iter = named_class_methods_.find(key);
        if (iter != named_class_methods_.end()) {
            return {iter->second.owns, iter->second.method};
        }
    }

    const Method *method = nullptr;
    const Class *klass = nullptr;
    for (klass = clazz; klass != nullptr; klass = klass->base()) {
        for (uint32_t i = 0; i < klass->n_methods(); i++) {
            if (::strcmp(method_name, klass->method(i)->name()) == 0) {
                method = klass->method(i);
                break;
            }
        }
        if (method) {
            break;
        }
    }
    if (!method) {
        return {nullptr, nullptr};
    }

    // Use self space's string
    key.name = method->name();
    std::lock_guard<std::shared_mutex> lock(class_methods_mutex_);
    named_class_methods_[key] = ClassMethodPair{klass, method};
    return {klass, method};
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
        QUEUE_INSERT_TAIL(large_dummy_, page);
        used_size_ += n;
        large_size_ += page->size();
        return AllocationResult(AllocationResult::OK, page->chunk());
    }

    LinearPage *page = nullptr;
    if (exec) {
        if (QUEUE_EMPTY(code_dummy_) || n > code_head()->GetFreeSize()) {
            page = AppendPage(access);
        } else {
            page = code_head();
        }
    } else {
        if (QUEUE_EMPTY(page_dummy_) || n > page_head()->GetFreeSize()) {
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
    Class *clazz = space->PrepareClass(name_, tags_, reference_size_);
    if (!clazz) {
        return nullptr;
    }
    BuildWithPreparedClass(space, clazz);
    return clazz;
}

Class *ClassBuilder::BuildWithPreparedClass(MetadataSpace *space, Class *clazz) const {
    if (!clazz) {
        return nullptr;
    }
    DCHECK(clazz->name_ != nullptr);
    clazz->base_ = base_;
    clazz->tags_ = tags_;
    clazz->reference_size_ = reference_size_;
    clazz->instrance_size_ = instrance_size_;
    clazz->init_ = space->New<Method>();
    clazz->init_->name_ = clazz->name_;
    clazz->init_->tags_ = 0;
    clazz->init_->fn_   = init_;

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
        func->captured_vars_[i].type  = captured_vars_[i].type;
        func->captured_vars_[i].index = captured_vars_[i].index;
    }
    func->captured_var_size_ = static_cast<uint32_t>(captured_vars_.size());

    func->exception_table_ = space->NewArray<Function::ExceptionHandlerDesc>(exception_table_.size());
    ::memcpy(func->exception_table_, &exception_table_[0],
             exception_table_.size() * sizeof(Function::ExceptionHandlerDesc));
    func->exception_table_size_ = static_cast<uint32_t>(exception_table_.size());
    
    func->invoking_hint_ = space->NewArray<Function::InvokingHint>(invoking_hint_.size());
    ::memcpy(func->invoking_hint_, &invoking_hint_[0],
             invoking_hint_.size() * sizeof(Function::InvokingHint));
    func->invoking_hint_size_ = static_cast<uint32_t>(invoking_hint_.size());
    return func;
}

} // namespace lang

} // namespace mai
