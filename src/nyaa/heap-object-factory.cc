#include "nyaa/object-factory.h"
#include "nyaa/string-pool.h"
#include "nyaa/thread.h"
#include "nyaa/nyaa-values.h"
#include "nyaa/function.h"
#include "nyaa/nyaa-core.h"
#include "nyaa/heap.h"
#include "nyaa/builtin.h"

namespace mai {
    
namespace nyaa {
    
class HeapObjectFactory : public ObjectFactory {
public:
    HeapObjectFactory(NyaaCore *core, Heap *heap)
        : core_(DCHECK_NOTNULL(core))
        , heap_(heap) {
    }

    virtual ~HeapObjectFactory() override {}
    
#define NEW_OBJECT(size, expr, type) \
    auto blk = heap_->Allocate(size, old ? kOldSpace : kNewSpace); \
    if (!blk) { return nullptr; } \
    auto ob = new (blk) expr; \
    ob->SetMetatable(core_->kmt_pool()->k##type, core_); \
    ob->SetColor(heap_->initial_color()); \
    ob->SetType(kType##type)
    
    virtual NyFloat64 *NewFloat64(f64_t value, bool old) override {
        NEW_OBJECT(sizeof(NyFloat64), NyFloat64(value), Float64);
        DCHECK_EQ(sizeof(NyFloat64), ob->PlacedSize());
        return ob;
    }

    virtual NyString *NewString(const char *s, size_t n, bool old) override {
        if (core_->kz_pool()) {
            NyString *kz = core_->kz_pool()->GetOrNull(s, n);
            if (kz) {
                return kz;
            }
        }
        size_t required_size = NyString::RequiredSize(static_cast<uint32_t>(n));
        NEW_OBJECT(required_size, NyString(s, n, core_), String);
        DCHECK_EQ(required_size, ob->PlacedSize());
        return ob;
    }
        
    virtual NyString *NewUninitializedString(size_t capacity, bool old) override {
        size_t required_size = NyString::RequiredSize(static_cast<uint32_t>(capacity));
        NEW_OBJECT(required_size, NyString(capacity), String);
        DCHECK_EQ(required_size, ob->PlacedSize());
        return ob;
    }
        
    virtual NyMap *NewMap(NyObject *maybe, uint64_t kid, bool linear, bool old) override {
        NEW_OBJECT(sizeof(NyMap), NyMap(maybe, kid, linear, core_), Map);
        DCHECK_EQ(sizeof(NyMap), ob->PlacedSize());
        return ob;
    }

    virtual NyTable *NewTable(size_t capacity, uint32_t seed, NyTable *base, bool old) override {
        DCHECK_LE(capacity, UINT32_MAX);
        size_t required_size = NyTable::RequiredSize(static_cast<uint32_t>(capacity));
        NEW_OBJECT(required_size, NyTable(seed, static_cast<uint32_t>(capacity)), Table);
        if (base) {
            DCHECK_GT(capacity, base->capacity());
            ob = ob->Rehash(base, core_);
        }
        DCHECK_EQ(ob->PlacedSize(), required_size);
        return ob;
    }

    virtual NyByteArray *NewByteArray(size_t capacity, NyByteArray *base, bool old) override {
        DCHECK_LE(capacity, UINT32_MAX);
        size_t required_size = NyByteArray::RequiredSize(static_cast<uint32_t>(capacity));
        NEW_OBJECT(required_size, NyByteArray(static_cast<uint32_t>(capacity)), ByteArray);
        if (base) {
            DCHECK_GT(capacity, base->capacity());
            ob->Refill(base);
        }
        DCHECK_EQ(ob->PlacedSize(), required_size);
        return ob;
    }
        
    virtual NyInt32Array *NewInt32Array(size_t capacity, NyInt32Array *base, bool old) override {
        DCHECK_LE(capacity, UINT32_MAX);
        size_t required_size = NyInt32Array::RequiredSize(static_cast<uint32_t>(capacity));
        NEW_OBJECT(required_size, NyInt32Array(static_cast<uint32_t>(capacity)), Int32Array);
        if (base) {
            DCHECK_GT(capacity, base->capacity());
            ob->Refill(base);
        }
        DCHECK_EQ(ob->PlacedSize(), required_size);
        return ob;
    }
        
    virtual NyArray *NewArray(size_t capacity, NyArray *base, bool old) override {
        DCHECK_LE(capacity, UINT32_MAX);
        size_t required_size = NyArray::RequiredSize(static_cast<uint32_t>(capacity));
        NEW_OBJECT(required_size, NyArray(static_cast<uint32_t>(capacity)), Array);
        if (base) {
            DCHECK_GT(capacity, base->capacity());
            ob->Refill(base, core_);
        }
        DCHECK_EQ(ob->PlacedSize(), required_size);
        return ob;
    }
    
    virtual NyBytecodeArray *NewBytecodeArray(NyInt32Array *source_lines, Address bytecodes,
                                              size_t bytecode_bytes_size, bool old) override {
        DCHECK_LE(bytecode_bytes_size, UINT32_MAX);
        size_t required_size = NyBytecodeArray::RequiredSize(static_cast<uint32_t>(bytecode_bytes_size));
        NEW_OBJECT(required_size,
                   NyBytecodeArray(source_lines, bytecodes, static_cast<uint32_t>(bytecode_bytes_size),
                                   core_),
                   BytecodeArray);
        DCHECK_EQ(ob->PlacedSize(), required_size);
        return ob;
    }

    virtual NyFunction *NewFunction(NyString *name, size_t n_params, bool vargs, size_t n_upvals,
                                    size_t max_stack, NyString *file_name, NyObject *exec,
                                    NyArray *proto_pool, NyArray *const_pool, bool old) override {
        DCHECK_LE(n_params, UINT8_MAX);
        DCHECK_LE(n_upvals, UINT32_MAX);
        DCHECK_LE(max_stack, UINT32_MAX);
        size_t required_size = NyFunction::RequiredSize(static_cast<uint32_t>(n_upvals));
        NEW_OBJECT(required_size,
                   NyFunction(name,
                              static_cast<uint8_t>(n_params),
                              vargs,
                              static_cast<uint32_t>(n_upvals),
                              static_cast<uint32_t>(max_stack),
                              file_name,
                              exec,
                              proto_pool,
                              const_pool,
                              core_),
                   Function);
        DCHECK_EQ(ob->PlacedSize(), required_size);
        return ob;
    }

    virtual NyCode *NewCode(int kind, NyInt32Array *source_lines, const uint8_t *instructions,
                            size_t instructions_bytes_size) override {
        DCHECK_LE(instructions_bytes_size, UINT32_MAX);
        size_t required_size = NyCode::RequiredSize(static_cast<uint32_t>(instructions_bytes_size));
        void *blk = heap_->Allocate(required_size, kCodeSpace);
        if (!blk) {
            return nullptr;
        }
        NyCode *ob = new (blk) NyCode(static_cast<NyCode::Kind>(kind),
                                      source_lines, instructions,
                                      static_cast<uint32_t>(instructions_bytes_size), core_);
        ob->SetMetatable(core_->kmt_pool()->kCode, core_);
        ob->SetColor(heap_->initial_color());
        ob->SetType(kTypeCode);
        DCHECK_EQ(ob->PlacedSize(), required_size);
        return ob;
    }

    virtual NyClosure *NewClosure(NyFunction *proto, bool old) override {
        size_t required_size = NyClosure::RequiredSize(static_cast<uint32_t>(proto->n_upvals()));
        NEW_OBJECT(required_size, NyClosure(proto, core_), Closure);
        DCHECK_EQ(required_size, ob->PlacedSize());
        return ob;
    }

    virtual NyDelegated *NewDelegated(DelegatedKind kind, Address fp, size_t n_upvals,
                                      bool old) override {
        DCHECK_LE(n_upvals, UINT32_MAX);
        size_t required_size = NyDelegated::RequiredSize(static_cast<uint32_t>(n_upvals));
        NEW_OBJECT(required_size, NyDelegated(kind, fp, static_cast<uint32_t>(n_upvals)), Delegated);
        DCHECK_EQ(ob->PlacedSize(), required_size);
        return ob;
    }
    
    virtual NyUDO *NewUninitializedUDO(size_t size, NyMap *clazz, bool ignore_managed,
                                       bool old) override {
        size = RoundUp(size, kPointerSize);
        size_t n_fields = NyUDO::GetNFiedls(size);
        void *blk = heap_->Allocate(size, old ? kOldSpace : kNewSpace);
        NyUDO *ob = new (blk) NyUDO(n_fields, ignore_managed);
        ob->SetMetatable(clazz, core_);
        ob->SetColor(heap_->initial_color());
        ob->SetType(kTypeUdo);
        DCHECK_EQ(size, ob->PlacedSize());
        return ob;
    }

    virtual NyThread *NewThread(size_t initial_stack_size, bool old) override {
        State *state = State::New(core_, initial_stack_size); // out of heap memory
        if (!state) {
            return nullptr;
        }
        static const size_t required_size = sizeof(NyThread);
        NEW_OBJECT(required_size, NyThread(state), Thread);
        DCHECK_EQ(ob->PlacedSize(), required_size);
        heap_->AddFinalizer(ob, &NyThread::Finalizer);
        return ob;
    }

private:
    NyaaCore *const core_;
    Heap *const heap_;
}; // class HeapObjectFactory
    
    
/*static*/ ObjectFactory *ObjectFactory::NewHeapFactory(NyaaCore *core, Heap *heap) {
    return new HeapObjectFactory(core, heap);
}

} // namespace nyaa
    
} // namespace mai
