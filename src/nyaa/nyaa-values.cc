#include "nyaa/nyaa-values.h"
#include "nyaa/nyaa-core.h"
#include "nyaa/heap.h"
#include "nyaa/object-factory.h"
#include "nyaa/memory.h"
#include "base/hash.h"

namespace mai {
    
namespace nyaa {

/*static*/ Object *const Object::kNil = nullptr;
    
bool Object::IsKey(NyaaCore *N) const {
    if (is_smi()) {
        return true;
    }
    auto ob = heap_object();
    if (ob->GetMetatable() == N->kMtString) {
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
    if (ob->GetMetatable() == N->kMtString) {
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
    
void NyObject::SetMetatable(NyTable *mt, NyaaCore *N) {
    mtword_ = reinterpret_cast<uintptr_t>(mt);
    N->heap()->BarrierWr(this, mt);
}

bool NyObject::Equals(Object *rhs, NyaaCore *N) const {
    if (rhs == nullptr) {
        return false;
    }
    if (GetMetatable() == N->kMtString) {
        if (rhs->is_smi() || rhs->heap_object()->GetMetatable() != N->kMtString) {
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

} // namespace nyaa
    
} // namespace mai
