#include "lang/metadata-space.h"
#include "lang/value-inl.h"

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
    Class *clazz = nullptr;
    
#define DEFINE_PRIMITIVE_CLASS(literal, ...) \
    clazz = New<Class>(); \
    clazz->id_ = NextTypeId(); \
    clazz->name_ = NewString(#literal); \
    clazz->tags_ = Type::kBuiltinTag|Type::kPrimitiveTag; \
    InsertClass(#literal, clazz); \
    DCHECK_EQ(kType_##literal, clazz->id()) << #literal;

    DECLARE_PRIMITIVE_TYPES(DEFINE_PRIMITIVE_CLASS)

#undef DEFINE_PRIMITIVE_CLASS
    
    // Any class
    ClassBuilder("any")
        .tags(Type::kBuiltinTag)
    .Build(this);
    
    // MutableMap::Entry
    clazz = ClassBuilder("mutable_map::entry")
        .tags(Type::kBuiltinTag)
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
        .tags(Type::kBuiltinTag) \
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
    return Error::OK();
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
            return AllocationResult(AllocationResult::NOT_ENOUGH_MEMORY, nullptr);
        }
        QUEUE_INSERT_TAIL(dummy_large_, page);
        usage_ += n;
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
        return AllocationResult(AllocationResult::NOT_ENOUGH_MEMORY, nullptr);
    }
    Address chunk = page->Advance(n);
    if (!chunk) {
        return AllocationResult(AllocationResult::FAIL, nullptr);
    }
    usage_ += n;
    return AllocationResult(AllocationResult::OK, chunk);
}

Class *ClassBuilder::Build(MetadataSpace *space) const {
    Class *clazz = space->New<Class>();
    clazz->name_ = space->NewString(name_.data(), name_.size());
    clazz->base_ = base_;
    clazz->tags_ = tags_;
    
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
        methods[i].name_ = space->NewString(pair.first.data(), pair.first.size());
        methods[i].tags_ = pair.second->tags_;
        methods[i].fn_ = pair.second->fn_;
        i++;
    }

    clazz->id_ = space->NextTypeId();
    space->InsertClass(name_, clazz);
    return clazz;
}

} // namespace lang

} // namespace mai
