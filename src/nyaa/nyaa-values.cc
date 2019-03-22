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
    
static Object *CallBinaryMetaFunction(NyString *name, NyObject *lhs, Object *rhs, NyaaCore *N) {
    Handle<NyRunnable> metafn(lhs->GetMetatable()->Get(name, N));
    if (metafn.is_null()) {
        N->Raisef("attempt to call nil `%s' meta function.", name->bytes());
        return nullptr;
    }
    Arguments args(2);
    args.Set(0, Local<Value>::New(lhs));
    args.Set(1, Local<Value>::New(rhs));
    int rv = metafn->Apply(&args, N);
    if (rv < 0) {
        return nullptr;
    }
    
    Handle<Object> result(N->Get(-rv));
    N->Pop(rv);
    return *result;
}
    
static Object *CallUnaryMetaFunction(NyString *name, NyObject *lhs, NyaaCore *N) {
    Handle<NyRunnable> metafn(lhs->GetMetatable()->Get(name, N));
    if (metafn.is_null()) {
        N->Raisef("attempt to call nil `%s' meta function.", name->bytes());
        return nullptr;
    }
    Arguments args(1);
    args.Set(0, Local<Value>::New(lhs));
    int rv = metafn->Apply(&args, N);
    if (rv < 0) {
        return nullptr;
    }
    
    Handle<Object> result(N->Get(-rv));
    N->Pop(rv);
    return *result;
}
    
bool Object::IsKey(NyaaCore *N) const {
    if (IsSmi()) {
        return true;
    }
    auto ob = ToHeapObject();
    if (ob->IsString(N)) {
        return true;
        // TODO:
    }
    return false;
}

uint32_t Object::HashVal(NyaaCore *N) const {
    if (IsSmi()) {
        return static_cast<uint32_t>(ToSmi());
    }
    auto ob = ToHeapObject();
    if (ob->IsString(N)) {
        return static_cast<NyString *>(ob)->hash_val();
    }
    // TODO:
    
    DLOG(FATAL) << "Noreached!";
    return 0;
}
    
/*static*/ bool Object::Equals(Object *lhs, Object *rhs, NyaaCore *N) {
    if (lhs == rhs) {
        return true;
    }
    if (lhs == nullptr) {
        return rhs == nullptr;
    }
    if (lhs->IsSmi()) {
        if (rhs->IsSmi()) {
            return lhs->ToSmi() == rhs->ToSmi();
        } else {
            // TODO:
            return false;
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
            auto val = static_cast<const NyString *>(ob);
            return val->size() == 0;
        } break;
            
        case kTypeLong: {
            // TODO:
        } break;
            
        case kTypeFlot64: {
            // TODO:
        } break;

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
    
NyString *Object::Str(NyaaCore *N) {
    if (this == kNil) {
        return N->bkz_pool()->kNil;
    }
    if (IsSmi()) {
        return N->factory()->Sprintf("%" PRId64 , ToSmi());
    }
    return ToHeapObject()->Str(N);
}
    
/*static*/ Object *NySmi::Add(Object *lhs, Object *rhs, NyaaCore *N) {
    DCHECK(lhs->IsSmi());
    auto lval = lhs->ToSmi();
    if (rhs->IsSmi()) {
        return New(lval + rhs->ToSmi());
    }
    auto rval = rhs->ToHeapObject();
    if (rval->IsString(N)) {
        return lhs->Str(N)->NyObject::Add(rval, N);
    }
    
    N->Raisef("smi attempt to call nil `__add__' meta function.");
    return nullptr;
}
    
void NyObject::SetMetatable(NyMap *mt, NyaaCore *N) {
    if (mt == Object::kNil) {
        return; // ignore
    }
    N->heap()->BarrierWr(this, mt);
    mtword_ = reinterpret_cast<uintptr_t>(mt);
}

bool NyObject::Equals(Object *rhs, NyaaCore *N) {
    if (rhs == nullptr) {
        return false;
    }

    switch (static_cast<BuiltinType>(GetMetatable()->kid())) {
        case kTypeString: {
            if (rhs->IsSmi() || rhs->ToHeapObject()->IsString(N)) {
                return false;
            }
            auto lv = static_cast<const NyString *>(this);
            auto rv = static_cast<const NyString *>(rhs);
            if (lv->size() != rv->size()) {
                return false;
            }
            return ::memcmp(lv->bytes(), rv->bytes(), lv->size()) == 0;
        } break;
            
        case kTypeLong: {
            // TODO:
        } break;
            
        case kTypeFlot64: {
            // TODO:
        } break;
            
        default:
            break;
    }
    
    HandleScope scope(N->isolate());
    Handle<Object> result = CallBinaryMetaFunction(N->bkz_pool()->kInnerEq, this, rhs, N);
    return result->IsTrue();
}
    
Object *NyObject::Add(Object *rhs, NyaaCore *N) {
    HandleScope scope(N->isolate());

    switch (static_cast<BuiltinType>(GetMetatable()->kid())) {
        case kTypeString: {
            Handle<NyString> lval(static_cast<NyString *>(this));
            Handle<NyString> rval(rhs->Str(N));
            return lval->Add(*rval, N)->Done(N);
        } break;
            
        case kTypeLong: {
            
        } break;
            
        case kTypeFlot64: {
            
        } break;
            
        default:
            break;
    }
    return CallBinaryMetaFunction(N->bkz_pool()->kInnerAdd, this, rhs, N);
}
    
//bool NyObject::IsTable(NyaaCore *N) const {
//    // TODO:
//    return false;
//}
    
NyString *NyObject::Str(NyaaCore *N) {
    HandleScope scope(N->isolate());
    
    switch (static_cast<BuiltinType>(GetMetatable()->kid())) {
        case kTypeString:
            return static_cast<NyString *>(const_cast<NyObject *>(this));
            
        case kTypeLong: {
            // TODO:
        } break;
            
        case kTypeFlot64: {
            // TODO:
        } break;

        default:
            break;
    }
    Handle<Object> result = CallUnaryMetaFunction(N->bkz_pool()->kInnerStr, this, N);
    if (result->IsSmi() || !result->ToHeapObject()->IsString(N)) {
        N->Raisef("`__str__' meta function return not string.");
        return nullptr;
    }
    return static_cast<NyString *>(*result);
}
    
NyMap::NyMap(NyObject *maybe, uint64_t kid, bool linear, NyaaCore *N)
    : generic_(DCHECK_NOTNULL(maybe))
    , kid_(kid)
    , linear_(linear) {
    DCHECK(maybe->IsArray(N) || maybe->IsTable(N));
    N->heap()->BarrierWr(this, maybe);
    DCHECK_LE(kid, 0x00ffffffffffffffull);
}
    
uint32_t NyMap::Length() const { return linear_ ? array_->size() : table_->size(); }
    
void NyMap::Put(Object *key, Object *value, NyaaCore *N) {
    if (!linear_) {
        table_ = table_->Put(key, value, N);
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
}

Object *NyMap::Get(Object *key, NyaaCore *N) {
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
            if (p == slot || slot->value == nullptr) {
                slot->key = key;
                slot->value = value;
            } else {
                Entry *n = free_;
                free_ = free_->next;
                DCHECK_EQ(kFree, n->kind);
                n->kind = kNode;
                n->key = key;
                n->value = value;
                n->next = nullptr;
                prev->next = n;
            }
            ++size_;
        }
    } else {
        if (p) {
            if (p == slot) {
                p->key = nullptr;
                p->value = nullptr;
            } else {
                prev->next = p->next;
                p->kind = kFree;
                p->next = free_;
                free_ = p;
            }
            --size_;
        }
    }
    //printf("slot: %p, key: %p\n", slot, slot->key);
    N->heap()->BarrierWr(this, key, value);
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
    
//NyByteArray *NyByteArray::Put(int64_t key, Byte value, NyaaCore *N) {
//    NyByteArray *ob = this;
//    if (key > capacity()) {
//        uint32_t new_cap = static_cast<uint32_t>(key + (key - capacity()) * 2);
//        ob = N->factory()->NewByteArray(new_cap, this);
//    }
//    ob->elems_[key] = value;
//    if (key > size()) {
//        size_ = static_cast<uint32_t>(key);
//    }
//    return ob;
//}

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
    
//NyInt32Array *NyInt32Array::Put(int64_t key, int32_t value, NyaaCore *N) {
//    NyInt32Array *ob = this;
//    if (key > capacity()) {
//        uint32_t new_cap = static_cast<uint32_t>(key + (key - capacity()) * 2);
//        ob = N->factory()->NewInt32Array(new_cap, this);
//    }
//    ob->elems_[key] = value;
//    if (key > size()) {
//        size_ = static_cast<uint32_t>(key);
//    }
//    return ob;
//}

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

    N->heap()->BarrierWr(this, value);
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

    N->heap()->BarrierWr(this, value);
    ob->elems_[size_++] = value;
    
    return ob;
}
    
void NyArray::Refill(const NyArray *base, NyaaCore *N) {
    for (int64_t i = 0; i < base->size(); ++i) {
        N->heap()->BarrierWr(this, base->Get(i));
    }
    NyArrayBase<Object*>::Refill(base);
}
    
NyScript::NyScript(NyString *file_name, NyInt32Array *file_info, NyByteArray *bcbuf,
                   NyArray *const_pool, NyaaCore *N) {
    N->heap()->BarrierWr(this, file_name, file_info);
    N->heap()->BarrierWr(this, bcbuf    , const_pool);
    bcbuf_      = DCHECK_NOTNULL(bcbuf);
    const_pool_ = const_pool;
    file_name_  = file_name;
    file_info_  = file_info;
}
    
int NyRunnable::Apply(Arguments *args, NyaaCore *N) {
    if (args) {
        args->SetCallee(Local<Value>::New(this));
    }
    if (IsScript(N)) {
        return static_cast<NyScript *>(this)->Run(N);
    } else if (IsDelegated(N)) {
        return static_cast<NyDelegated *>(this)->Call(args, N);
    } else if (IsFunction(N)) {
        return static_cast<NyFunction *>(this)->Call(args, N);
    }
    DLOG(FATAL) << "Noreached!";
    return -1;
}
    
NyFunction::NyFunction(uint8_t n_params,
                       bool vargs,
                       uint32_t max_stack_size,
                       NyScript *script,
                       NyaaCore *N)
    : n_params_(n_params)
    , vargs_(vargs)
    , max_stack_size_(max_stack_size)
    , script_(script) {
    N->heap()->BarrierWr(this, script);
}
    
int NyFunction::Call(Arguments *args, NyaaCore *N) { return N->main_thd()->Run(this, args); }
    
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
    bool NyObject::Is##type (NyaaCore *N) const { \
        return GetMetatable() == N->kmt_pool()->k##type; \
    } \
    bool Ny##type::EnsureIs(const NyObject *o, NyaaCore *N) { \
    if (!o->Is##type (N)) { \
        N->Raisef("Unexpected type: " name "."); \
            return false; \
        } \
        return true; \
    }
    
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
