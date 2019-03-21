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
    
bool Object::IsKey(NyaaCore *N) const {
    if (is_smi()) {
        return true;
    }
    auto ob = heap_object();
    if (ob->IsString(N)) {
        return true;
        // TODO:
    }
    return false;
}

uint32_t Object::HashVal(NyaaCore *N) const {
    if (is_smi()) {
        return static_cast<uint32_t>(smi());
    }
    auto ob = heap_object();
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
    if (lhs->is_smi()) {
        if (rhs->is_smi()) {
            return lhs->smi() == rhs->smi();
        } else {
            // TODO:
            return false;
        }
    }
    return lhs->heap_object()->Equals(rhs, N);
}
    
NyString *Object::Str(NyaaCore *N) const {
    if (this == kNil) {
        return N->bkz_pool()->kNil;
    }
    if (is_smi()) {
        return N->factory()->Sprintf("%" PRId64 , smi());
    }
    return heap_object()->Str(N);
}
    
void NyObject::SetMetatable(NyTable *mt, NyaaCore *N) {
    N->heap()->BarrierWr(this, mt);
    mtword_ = reinterpret_cast<uintptr_t>(mt);
}

bool NyObject::Equals(Object *rhs, NyaaCore *N) const {
    if (rhs == nullptr) {
        return false;
    }
    if (IsString(N)) {
        if (rhs->is_smi() || rhs->heap_object()->IsString(N)) {
            return false;
        }
        auto lv = static_cast<const NyString *>(this);
        auto rv = static_cast<const NyString *>(rhs);
        if (lv->size() != rv->size()) {
            return false;
        }
        return ::memcmp(lv->bytes(), rv->bytes(), lv->size()) == 0;
    }
    // TODO:
    return false;
}
    
bool NyObject::IsTable(NyaaCore *N) const {
    // TODO:
    return false;
}
    
NyString *NyObject::Str(NyaaCore *N) const {
    auto tmp = const_cast<NyObject *>(this);
    if (IsString(N)) {
        return static_cast<NyString *>(tmp);
    } else if (IsByteArray(N)) {
        // TODO:
    } else if (IsInt32Array(N)) {
        // TODO:
    } else if (IsArray(N)) {
        // TODO:
    } else if (IsDelegated(N)) {
        // TODO:
    } else if (IsScript(N)) {
        // TODO:
    }
    
    HandleScope scope(N->isolate());
    
    Handle<NyTable> mt(GetMetatable());
    DCHECK(mt.is_valid() && mt->IsTable(N));
    
    Handle<NyDelegated> fn(mt->Get(N->bkz_pool()->kInnerStr, N));
    if (fn.is_valid()) {
        DCHECK(fn->IsDelegated(N));
        
        Arguments args(1);
        args.Set(0, Local<Value>::New(tmp));
        if (fn->Apply(&args, N) < 0) {
            return nullptr;
        }
    }
    return N->bkz_pool()->kEmpty;
}
    
uint32_t NyMap::Length() const {
    // TODO:
    return 0;
}
    
NyMap *NyMap::Put(Object *key, Object *value, NyaaCore *N) {
    // TODO:
    return nullptr;
}

Object *NyMap::Get(Object *key, NyaaCore *N) {
    // TODO:
    return nullptr;
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

NyString::NyString(const char *s, size_t n)
    : size_(static_cast<uint32_t>(n)) {
    if (size_ > kLargeStringLength) {
        hash_val_ = -1;
    } else {
        hash_val_ = base::Hash::Js(s, n);
    }
    ::memcpy(bytes_, s, n);
    bytes_[n] = 0;
}
    
NyByteArray *NyByteArray::Put(int64_t key, Byte value, NyaaCore *N) {
    NyByteArray *ob = this;
    if (key > capacity()) {
        uint32_t new_cap = static_cast<uint32_t>(key + (key - capacity()) * 2);
        ob = N->factory()->NewByteArray(new_cap, this);
    }
    ob->elems_[key] = value;
    if (key > size()) {
        size_ = static_cast<uint32_t>(key);
    }
    return ob;
}

NyByteArray *NyByteArray::Add(Byte value, NyaaCore *N) {
    NyByteArray *ob = this;
    if (size() + 1 > capacity()) {
        ob = N->factory()->NewByteArray(capacity_ << 1, this);
    }
    ob->elems_[size_++] = value;
    return ob;
}

NyByteArray *NyByteArray::Add(const Byte *value, size_t n, NyaaCore *N) {
    NyByteArray *ob = this;
    const uint32_t k = static_cast<uint32_t>(n);
    if (size() + k > capacity()) {
        ob = N->factory()->NewByteArray((capacity_ << 1) + k, this);
    }
    ::memcpy(elems_ + size(), value, n);
    size_ += k;
    return ob;
}
    
NyInt32Array *NyInt32Array::Put(int64_t key, int32_t value, NyaaCore *N) {
    NyInt32Array *ob = this;
    if (key > capacity()) {
        uint32_t new_cap = static_cast<uint32_t>(key + (key - capacity()) * 2);
        ob = N->factory()->NewInt32Array(new_cap, this);
    }
    ob->elems_[key] = value;
    if (key > size()) {
        size_ = static_cast<uint32_t>(key);
    }
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
    }
    DLOG(FATAL) << "Noreached!";
    return -1;
}
    
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
DEFINE_TYPE_CHECK(ByteArray, "array[byte]")
DEFINE_TYPE_CHECK(Int32Array, "array[int32]")
DEFINE_TYPE_CHECK(Array, "array")
DEFINE_TYPE_CHECK(Script, "script")
DEFINE_TYPE_CHECK(Thread, "thread")

} // namespace nyaa
    
} // namespace mai
