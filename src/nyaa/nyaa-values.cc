#include "nyaa/nyaa-values.h"
#include "nyaa/thread.h"
#include "nyaa/nyaa-core.h"
#include "nyaa/heap.h"
#include "nyaa/object-factory.h"
#include "nyaa/memory.h"
#include "nyaa/builtin.h"
#include "base/hash.h"
#include "mai-lang/call-info.h"

namespace mai {
    
namespace nyaa {

/*static*/ Object *const Object::kNil = nullptr;
    
//static Object *CallBinaryMetaFunction(NyString *name, NyObject *lhs, Object *rhs, NyaaCore *N) {
//    Handle<NyRunnable> metafn(lhs->GetMetatable()->RawGet(name, N));
//    if (metafn.is_null()) {
//        N->Raisef("attempt to call nil `%s' meta function.", name->bytes());
//        return nullptr;
//    }
//    Arguments args(2);
//    args.Set(0, Local<Value>::New(lhs));
//    args.Set(1, Local<Value>::New(rhs));
//    int rv = metafn->Apply(&args, N);
//    if (rv < 0) {
//        return nullptr;
//    }
//
//    Handle<Object> result(N->Get(-rv));
//    N->Pop(rv);
//    return *result;
//}
//
//static Object *CallUnaryMetaFunction(NyString *name, NyObject *lhs, NyaaCore *N) {
//    Handle<NyRunnable> metafn(lhs->GetMetatable()->RawGet(name, N));
//    if (metafn.is_null()) {
//        N->Raisef("attempt to call nil `%s' meta function.", name->bytes());
//        return nullptr;
//    }
//    Arguments args(1);
//    args.Set(0, Local<Value>::New(lhs));
//    int rv = metafn->Apply(&args, N);
//    if (rv < 0) {
//        return nullptr;
//    }
//
//    Handle<Object> result(N->Get(-rv));
//    N->Pop(rv);
//    return *result;
//}
    
bool Object::IsKey(NyaaCore *N) const {
    if (IsSmi()) {
        return true;
    }
    auto ob = ToHeapObject();
    switch (static_cast<BuiltinType>(ob->GetMetatable()->kid())) {
        case kTypeString:
        case kTypeFloat64:
        case kTypeLong:
            return true;
        default:
            break;
    }
    return false;
}

uint32_t Object::HashVal(NyaaCore *N) const {
    if (IsSmi()) {
        return static_cast<uint32_t>(ToSmi());
    }
    auto ob = ToHeapObject();
    switch (static_cast<BuiltinType>(ob->GetMetatable()->kid())) {
        case kTypeString:
            return static_cast<const NyString *>(this)->hash_val();
        case kTypeFloat64:
            return static_cast<const NyFloat64 *>(this)->HashVal();
        case kTypeLong:
            // TODO:
        default:
            break;
    }
    DLOG(FATAL) << "Noreached!";
    return 0;
}
    
/*static*/ bool Object::Equals(Object *lhs, Object *rhs, NyaaCore *N) {
    if (lhs == rhs) {
        return true;
    }
    if (lhs == kNil || rhs == kNil) {
        return false;
    }
    if (lhs->IsSmi()) {
        if (rhs->IsSmi()) {
            return lhs->ToSmi() == rhs->ToSmi();
        } else {
            return rhs->ToHeapObject()->Equals(lhs, N);
        }
    }
    return lhs->ToHeapObject()->Equals(rhs, N);
}
    
bool Object::IsFalse() const {
    if (IsNil()) {
        return false;
    }
    if (IsSmi()) {
        return ToSmi() == 0;
    }
    NyObject *ob = ToHeapObject();
    switch (static_cast<BuiltinType>(ob->GetMetatable()->kid())) {
        case kTypeString: {
            auto val = ob->ToString();
            return val->size() == 0;
        } break;
            
        case kTypeLong:
            return ob->ToLong()->IsZero();
            
        case kTypeFloat64:
            return ob->ToFloat64()->value() == 0;

        default:
            break;
    }
    return false;
}
    
/*static*/ Object *Object::Add(Object *lhs, Object *rhs, NyaaCore *N) {
    if (lhs == Object::kNil || rhs == Object::kNil) {
        return Object::kNil;
    }
    if (lhs->IsObject()) {
        return lhs->ToHeapObject()->Add(rhs, N);
    } else {
        return NySmi::Add(lhs, rhs, N);
    }
}
    
NyString *Object::ToString(NyaaCore *N) {
    if (this == kNil) {
        return N->bkz_pool()->kNil;
    }
    if (IsSmi()) {
        return N->factory()->Sprintf("%" PRId64 , ToSmi());
    }
    return ToHeapObject()->ToString(N);
}
    
/*static*/ Object *NySmi::Add(Object *lhs, Object *rhs, NyaaCore *N) {
    DCHECK(lhs->IsSmi());
    auto lval = lhs->ToSmi();
    if (rhs->IsSmi()) {
        return New(lval + rhs->ToSmi());
    }
    auto rval = rhs->ToHeapObject();
    if (rval->IsString()) {
        return lhs->ToString(N)->NyObject::Add(rval, N);
    }
    
    N->Raisef("smi attempt to call nil `__add__' meta function.");
    return nullptr;
}
    
void NyObject::SetMetatable(NyMap *mt, NyaaCore *N) {
    if (mt == Object::kNil) {
        return; // ignore
    }
    N->heap()->BarrierWr(this, reinterpret_cast<Object **>(&mtword_), mt);
    mtword_ = reinterpret_cast<uintptr_t>(mt);
}

bool NyObject::Equals(Object *rhs, NyaaCore *N) {
    if (rhs == nullptr) {
        return false;
    }

    switch (static_cast<BuiltinType>(GetMetatable()->kid())) {
        case kTypeString: {
            if (rhs->IsSmi() || !rhs->ToHeapObject()->IsString()) {
                return false;
            }
            auto lv = ToString();
            auto rv = static_cast<const NyString *>(rhs);
            if (lv->size() != rv->size()) {
                return false;
            }
            return ::memcmp(lv->bytes(), rv->bytes(), lv->size()) == 0;
        } break;
            
        case kTypeLong: {
            auto lval = ToLong();
            if (rhs->IsSmi()) {
                return lval->ToInt64Val() == rhs->ToSmi();
            } else if (rhs->ToHeapObject()->IsLong()) {
                return lval->Equals(rhs->ToHeapObject()->ToLong());
            } else if (rhs->ToHeapObject()->IsFloat64()) {
                return lval->ToFloat64Val() == rhs->ToHeapObject()->ToFloat64()->value();
            } else {
                return false;
            }
        } break;
            
        case kTypeFloat64: {
            auto lval = ToFloat64()->value();
            if (rhs->IsSmi()) {
                return lval == rhs->ToSmi();
            } else if (rhs->ToHeapObject()->IsLong()) {
                return lval == rhs->ToHeapObject()->ToLong()->ToFloat64Val();
            } else if (rhs->ToHeapObject()->IsFloat64()) {
                return lval == rhs->ToHeapObject()->ToFloat64()->value();
            } else {
                return false;
            }
        } break;
            
        default:
            break;
    }

    DLOG(FATAL) << "Noreached!";
    return false;
//    HandleScope scope(N->isolate());
//    Handle<Object> result = CallBinaryMetaFunction(N->bkz_pool()->kInnerEq, this, rhs, N);
//    return result->IsTrue();
}
    
size_t NyObject::PlacedSize() const {
    size_t bytes = 0;
    switch (static_cast<BuiltinType>(GetMetatable()->kid())) {
#define DEFINE_PLACED_SIZE(type) \
    case kType##type: \
        bytes = static_cast<const Ny##type *>(this)->PlacedSize();
            
        DECL_BUILTIN_TYPES(DEFINE_PLACED_SIZE)
#undef DEFINE_PLACED_SIZE
        default: { // UDOs
            DCHECK_GT(GetMetatable()->kid(), kUdoKidBegin);
            DLOG(FATAL) << "TODO:";
        } break;
    }
    return RoundUp(bytes, kAllocateAlignmentSize);
}
    
Object *NyObject::Add(Object *rhs, NyaaCore *N) {
    HandleScope scope(N->isolate());

    switch (static_cast<BuiltinType>(GetMetatable()->kid())) {
        case kTypeString: {
            Handle<NyString> lval(ToString());
            Handle<NyString> rval(rhs->ToString(N));
            return lval->Add(*rval, N)->Done(N);
        } break;
            
        case kTypeLong:
            return ToLong()->Add(rhs, N);
            
        case kTypeFloat64:
            return ToFloat64()->Add(rhs, N);
            
        default:
            break;
    }
    DLOG(FATAL) << "Noreached!";
    return nullptr;
    //return CallBinaryMetaFunction(N->bkz_pool()->kInnerAdd, this, rhs, N);
}
    
NyString *NyObject::ToString(NyaaCore *N) {
    HandleScope scope(N->isolate());
    
    switch (static_cast<BuiltinType>(GetMetatable()->kid())) {
        case kTypeString:
            return (const_cast<NyObject *>(this))->ToString();
            
        case kTypeLong:
            return ToLong()->ToString(N);
            
        case kTypeFloat64:
            return N->factory()->Sprintf("%f", ToFloat64()->value());

        default:
            break;
    }
    DLOG(FATAL) << "Noreached!";
    return nullptr;
}
    
Object *NyFloat64::Add(Object *rhs, NyaaCore *N) const {
    if (rhs->IsObject() && rhs->ToHeapObject()->IsString()) {
        rhs = rhs->ToHeapObject()->ToString()->TryNumeric(N);
    }

    auto lval = ToFloat64()->value();
    if (rhs->IsSmi()) {
        return N->factory()->NewFloat64(lval + rhs->ToSmi());
    } else if (rhs->ToHeapObject()->IsLong()) {
        auto rval = rhs->ToHeapObject()->ToLong()->ToFloat64Val();
        return N->factory()->NewFloat64(lval + rval);
    } else if (rhs->ToHeapObject()->IsFloat64()) {
        auto rval = rhs->ToHeapObject()->ToFloat64()->value();
        return N->factory()->NewFloat64(lval + rval);
    }
    N->Raisef("attempt to call nil `__add__' meta function.");
    return nullptr;
}

Object *NyLong::Add(Object *rhs, NyaaCore *N) const {
    if (rhs->IsObject() && rhs->ToHeapObject()->IsString()) {
        rhs = rhs->ToHeapObject()->ToString()->TryNumeric(N);
    }

    auto lval = ToLong();
    if (rhs->IsSmi()) {
        return lval->Add(rhs->ToSmi(), N);
    } else if (rhs->ToHeapObject()->IsLong()) {
        return lval->Add(rhs->ToHeapObject()->ToLong(), N);
    } else if (rhs->ToHeapObject()->IsFloat64()) {
        auto rval = rhs->ToHeapObject()->ToFloat64()->value();
        return N->factory()->NewFloat64(lval->ToFloat64Val() + rval);
    }
    N->Raisef("attempt to call nil `__add__' meta function.");
    return nullptr;
}

NyLong *NyLong::Add(int64_t rhs, NyaaCore *N) const {
    // TODO:
    return nullptr;
}

NyLong *NyLong::Add(const NyLong *rhs, NyaaCore *N) const {
    // TODO:
    return nullptr;
}
    
uint32_t NyLong::HashVal() const {
    // TODO:
    return 0;
}
    
bool NyLong::IsZero() const {
    // TODO:
    return false;
}

f64_t NyLong::ToFloat64Val() const {
    // TODO:
    return 0;
}

int64_t NyLong::ToInt64Val() const {
    // TODO:
    return 0;
}
    
NyString *NyLong::ToString(NyaaCore *N) const {
    // TODO:
    return nullptr;
}
    
/*static*/ int NyLong::Compare(const NyLong *lhs, const NyLong *rhs) {
    // TODO:
    return 0;
}
    
NyMap::NyMap(NyObject *maybe, uint64_t kid, bool linear, NyaaCore *N)
    : generic_(maybe)
    , kid_(kid)
    , linear_(linear) {
    //DCHECK(maybe->IsArray() || maybe->IsTable());
    N->BarrierWr(this, &generic_, maybe);
    DCHECK_LE(kid, 0x00ffffffffffffffull);
}
    
uint32_t NyMap::Length() const { return linear_ ? array_->size() : table_->size(); }
    
void NyMap::RawPut(Object *key, Object *value, NyaaCore *N) {
    NyObject *old = generic_;
    
    if (!linear_) {
        table_ = table_->Put(key, value, N);
        if (generic_ != old) {
            N->BarrierWr(this, &generic_, generic_);
        }
        return;
    }
    
    bool should_table = false;
    int64_t index = 0;
    if (key->IsSmi()) {
        index = key->ToSmi();
        if (index + 1024 > array_->capacity()) {
            should_table = true;
        }
    } else {
        should_table = linear_;
    }

    if (should_table) {
        HandleScope scope(N->isolate());
        Handle<NyTable> table(N->factory()->NewTable(array_->capacity(), rand()));
        for (int64_t i = 0; i < array_->size(); ++i) {
            if (array_->Get(i) == Object::kNil) {
                continue;
            }
            table = table->Put(NySmi::New(i), array_->Get(i), N);
        }
        table_ = *table;
        linear_ = false;
    }
    
    if (linear_) {
        array_ = array_->Put(index, value, N);
    } else {
        table_ = table_->Put(key, value, N);
    }
    if (generic_ != old) {
        N->BarrierWr(this, &generic_, generic_);
    }
}

Object *NyMap::RawGet(Object *key, NyaaCore *N) {
    if (linear_) {
        if (key->IsObject()) {
            return nullptr;
        }
        int64_t index = key->ToSmi();
        if (index < 0 || index > array_->size()) {
            return nullptr;
        }
        return array_->Get(index);
    }
    return table_->Get(key, N);
}
    
NyTable::NyTable(uint32_t seed, uint32_t capacity)
    : seed_(seed)
    , size_(0)
    , capacity_(capacity) {
    DCHECK_GT(capacity_, 4);
    for (size_t i = 0; i < n_slots(); ++i) {
        entries_[i].kind  = kSlot;
        entries_[i].key   = nullptr;
        entries_[i].value = nullptr;
        entries_[i].next  = nullptr;
    }
    for (size_t i = n_slots(); i < capacity_ - 1; ++i) {
        entries_[i].kind = kFree;
        entries_[i].next = &entries_[i + 1];
    }
    entries_[capacity_ - 1].kind = kFree;
    entries_[capacity_ - 1].next = nullptr;

    free_ = &entries_[n_slots()];
}
    
NyTable *NyTable::Put(Object *key, Object *value, NyaaCore *N) {
    if (key != nullptr && key->IsNotKey(N)) {
        N->Raisef("Incorrect hash key.");
        return nullptr;
    }

    NyTable *ob = this;
    if (value) {
        if (!free_) { // no free nodes
            ob = N->factory()->NewTable(capacity_ << 1, seed_, ob);
            if (!ob) {
                return nullptr;
            }
        }
    }
    ob->DoPut(key, value, N);
    return ob;
}

Object *NyTable::Get(Object *key, NyaaCore *N) {
    if (key != nullptr && key->IsNotKey(N)) {
        return kNil;
    }
    
    Entry *slot = GetSlot(key, N);
    Entry *p = slot;
    while (p) {
        if (Object::Equals(key, p->key, N)) {
            break;
        }
        p = p->next;
    }
    return !p ? kNil : p->value;
}

NyTable *NyTable::Rehash(NyTable *origin, NyaaCore *N) {
    NyTable *ob = this;
    for (size_t i = 0; i < origin->n_slots(); ++i) {
        DCHECK_EQ(kSlot, origin->entries_[i].kind);
        if (!origin->entries_[i].value) {
            continue;
        }
        for (Entry *p = &origin->entries_[i]; p; p = p->next) {
            ob = ob->Put(p->key, p->value, N);
        }
    }
    return ob;
}
    
bool NyTable::DoPut(Object *key, Object *value, NyaaCore *N) {
    Entry *slot = GetSlot(key, N);
    Entry *prev = nullptr, *p = slot;
    while (p) {
        if (Object::Equals(key, p->key, N)) {
            break;
        }
        prev = p;
        p = p->next;
    }
    
    if (value) {
        if (p) {
            p->value = value;
        } else {
            if (p == slot || slot->value == Object::kNil) {
                N->BarrierWr(this, &slot->key, key);
                slot->key = key;
                N->BarrierWr(this, &slot->value, value);
                slot->value = value;
            } else {
                Entry *n = free_;
                free_ = free_->next;
                DCHECK_EQ(kFree, n->kind);
                n->kind = kNode;
                N->BarrierWr(this, &n->key, key);
                n->key = key;
                N->BarrierWr(this, &n->value, value);
                n->value = value;
                n->next = nullptr;
                prev->next = n;
            }
            ++size_;
        }
    } else {
        if (p) {
            if (p == slot) {
                N->BarrierWr(this, &p->key, Object::kNil);
                p->key = Object::kNil;
                N->BarrierWr(this, &p->value, Object::kNil);
                p->value = Object::kNil;
            } else {
                prev->next = p->next;
                p->kind = kFree;
                p->next = free_;
                free_ = p;
            }
            --size_;
        }
    }
    return p != nullptr;
}

NyString::NyString(const char *s, size_t n, NyaaCore *N)
    : NyByteArray(static_cast<uint32_t>(n) + kHeaderSize + 1) {
    size_ += kHeaderSize;
    Add(s, n, nullptr);
    elems_[size_] = 0;
    Done(N);
}
    
NyString *NyString::Done(NyaaCore *N) {
    uint32_t *hash_val = reinterpret_cast<uint32_t *>(data());
    if (size() > kLargeStringLength) {
        *hash_val = -1;
    } else {
        *hash_val = base::Hash::Js(bytes(), size());
    }
    // TODO:
    
    return this;
}
    
Object *NyString::TryNumeric(NyaaCore *N) const {
    // TODO:
    return const_cast<NyString *>(this);
}

NyByteArray *NyByteArray::Add(Byte value, NyaaCore *N) {
    NyByteArray *ob = this;
    if (size() + 1 > capacity()) {
        ob = N->factory()->NewByteArray(capacity_ << 1, this);
    }
    ob->elems_[size_++] = value;
    return ob;
}

NyByteArray *NyByteArray::Add(const void *value, size_t n, NyaaCore *N) {
    NyByteArray *ob = this;
    const uint32_t k = static_cast<uint32_t>(n);
    if (size() + k > capacity()) {
        ob = N->factory()->NewByteArray((capacity_ << 1) + k, this);
    }
    ::memcpy(elems_ + size(), value, n);
    size_ += k;
    return ob;
}

NyInt32Array *NyInt32Array::Add(int32_t value, NyaaCore *N) {
    NyInt32Array *ob = this;
    if (size() + 1 > capacity()) {
        ob = N->factory()->NewInt32Array(capacity_ << 1, this);
    }
    ob->elems_[size_++] = value;
    return ob;
}
    
NyArray *NyArray::Put(int64_t key, Object *value, NyaaCore *N) {
    NyArray *ob = this;
    if (key > capacity()) {
        uint32_t new_cap = static_cast<uint32_t>(key + (key - capacity()) * 2);
        ob = N->factory()->NewArray(new_cap, this);
    }

    N->BarrierWr(this, ob->elems_ + key, value);
    ob->elems_[key] = value;
    
    if (key > size()) {
        size_ = static_cast<uint32_t>(key);
    }
    return ob;
}

NyArray *NyArray::Add(Object *value, NyaaCore *N) {
    NyArray *ob = this;
    if (size() + 1 > capacity()) {
        ob = N->factory()->NewArray(capacity_ << 1, this);
    }

    N->BarrierWr(this, ob->elems_ + size_, value);
    ob->elems_[size_++] = value;
    
    return ob;
}
    
void NyArray::Refill(const NyArray *base, NyaaCore *N) {
    for (int64_t i = 0; i < base->size(); ++i) {
        N->BarrierWr(this, elems_ + i, base->Get(i));
    }
    NyArrayBase<Object*>::Refill(base);
}
    
NyScript::NyScript(NyString *file_name, NyInt32Array *file_info, NyByteArray *bcbuf,
                   NyArray *const_pool, NyaaCore *N) {
    N->BarrierWr(this, &bcbuf_, bcbuf);
    bcbuf_      = DCHECK_NOTNULL(bcbuf);
    N->BarrierWr(this, &const_pool_, const_pool);
    const_pool_ = const_pool;
    N->BarrierWr(this, &file_name_, file_name);
    file_name_  = file_name;
    N->BarrierWr(this, &file_info_, file_info);
    file_info_  = file_info;
}
    
int NyRunnable::Apply(Arguments *args, NyaaCore *N) {
    if (args) {
        args->SetCallee(Local<Value>::New(this));
    }
    if (IsScript()) {
        return static_cast<NyScript *>(this)->Run(N);
    } else if (IsDelegated()) {
        return static_cast<NyDelegated *>(this)->Call(args, N);
    } else if (IsFunction()) {
        return static_cast<NyFunction *>(this)->Call(args, N);
    }
    DLOG(FATAL) << "Noreached!";
    return -1;
}
    
void NyDelegated::Bind(int i, Object *upval, NyaaCore *N) {
    DCHECK_GE(i, 0);
    DCHECK_LT(i, n_upvals_);
    N->BarrierWr(this, upvals_ + i, upval);
    upvals_[i] = upval;
}
    
NyFunction::NyFunction(uint8_t n_params,
                       bool vargs,
                       uint32_t max_stack_size,
                       uint32_t n_upvals,
                       NyScript *script,
                       NyaaCore *N)
    : n_params_(n_params)
    , vargs_(vargs)
    , max_stack_size_(max_stack_size)
    , n_upvals_(n_upvals)
    , script_(script) {
    N->BarrierWr(this, &script_, script);
    ::memset(upvals_, 0, sizeof(n_upvals) * sizeof(Object *));
}
    
void NyFunction::Bind(int i, Object *upval, NyaaCore *N) {
    DCHECK_GE(i, 0);
    DCHECK_LT(i, n_upvals_);
    N->BarrierWr(this, upvals_ + i, upval);
    upvals_[i] = upval;
}
    
int NyFunction::Call(Arguments *args, NyaaCore *N) { return N->curr_thd()->Run(this, args, -1); }
    
int NyScript::Run(NyaaCore *N) { return N->main_thd()->Run(this); }
    
int NyDelegated::Call(Arguments *args, NyaaCore *N) {
    switch (kind()) {
        case kArg0:
            return call0_(N->stub());
        case kArg1:
            return call1_(args->Get(0), N->stub());
        case kArg2:
            return call2_(args->Get(0), args->Get(1), N->stub());
        case kArg3:
            return call3_(args->Get(0), args->Get(1), args->Get(2), N->stub());
        case kUniversal:
            return calln_(args, N->stub());
        case kPropertyGetter:
        case kPropertySetter:
        default:
            DLOG(FATAL) << "TODO:";
            break;
    }
    return -1;
}

#define DEFINE_TYPE_CHECK(type, name) \
    bool Ny##type::EnsureIs(const NyObject *o, NyaaCore *N) { \
    if (!o->Is##type ()) { \
        N->Raisef("unexpected type: " name "."); \
            return false; \
        } \
        return true; \
    }
    
DEFINE_TYPE_CHECK(Float64, "float")
DEFINE_TYPE_CHECK(String, "string")
DEFINE_TYPE_CHECK(Delegated, "delegated")
DEFINE_TYPE_CHECK(Map, "map")
DEFINE_TYPE_CHECK(Table, "table")
DEFINE_TYPE_CHECK(ByteArray, "array[byte]")
DEFINE_TYPE_CHECK(Int32Array, "array[int32]")
DEFINE_TYPE_CHECK(Array, "array")
DEFINE_TYPE_CHECK(Script, "script")
DEFINE_TYPE_CHECK(Function, "function")
DEFINE_TYPE_CHECK(Thread, "thread")

} // namespace nyaa
    
} // namespace mai
