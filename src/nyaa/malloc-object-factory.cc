#include "nyaa/object-factory.h"
#include "nyaa/string-pool.h"
#include "nyaa/nyaa-values.h"
#include "nyaa/function.h"
#include "nyaa/nyaa-core.h"
#include "nyaa/heap.h"
#include "nyaa/builtin.h"

namespace mai {
    
namespace nyaa {
    
class MallocObjectFactory : public ObjectFactory {
public:
    
    MallocObjectFactory(NyaaCore *core) : core_(DCHECK_NOTNULL(core)) {}

    virtual ~MallocObjectFactory() override {}
    
    virtual NyFloat64 *NewFloat64(f64_t value, bool old) override {
        auto ob = new NyFloat64(value);
        ob->SetMetatable(core_->kmt_pool()->kFloat64, core_);
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
        void *chunk = ::malloc(required_size);
        auto ob = new (chunk) NyString(s, n, core_);
        DCHECK_EQ(ob->PlacedSize(), required_size);
        ob->SetMetatable(core_->kmt_pool()->kString, core_);
        return ob;
    }
    
    virtual NyString *NewUninitializedString(size_t capacity, bool old) override {
        void *chunk = ::malloc(NyString::RequiredSize(static_cast<uint32_t>(capacity)));
        auto ob = new (chunk) NyString(capacity);
        ob->SetMetatable(core_->kmt_pool()->kString, core_);
        return ob;
    }
    
    virtual NyMap *NewMap(NyObject *maybe, uint64_t kid, bool linear, bool old) override {
        auto ob = new NyMap(maybe, kid, linear, core_);
        ob->SetMetatable(core_->kmt_pool()->kMap, core_);
        return ob;
    }

    virtual NyTable *NewTable(size_t capacity, uint32_t seed, NyTable *base, bool old) override {
        DCHECK_LE(capacity, UINT32_MAX);
        void *chunk = ::malloc(NyTable::RequiredSize(static_cast<uint32_t>(capacity)));
        auto ob = new (chunk) NyTable(seed, static_cast<uint32_t>(capacity));
        DCHECK_NOTNULL(core_->kmt_pool());
        ob->SetMetatable(core_->kmt_pool()->kTable, core_);
        if (base) {
            DCHECK_GT(capacity, base->capacity());
            ob = ob->Rehash(base, core_);
        }
        DCHECK_EQ(ob->PlacedSize(), NyTable::RequiredSize(static_cast<uint32_t>(capacity)));
        return ob;
    }

    virtual NyByteArray *NewByteArray(size_t capacity, NyByteArray *base, bool old) override {
        DCHECK_LE(capacity, UINT32_MAX);
        void *chunk = ::malloc(NyByteArray::RequiredSize(static_cast<uint32_t>(capacity)));
        auto ob = new (chunk) NyByteArray(static_cast<uint32_t>(capacity));
        ob->SetMetatable(core_->kmt_pool()->kByteArray, core_);
        if (base) {
            DCHECK_GT(capacity, base->capacity());
            ob->Refill(base);
        }
        DCHECK_EQ(ob->PlacedSize(), NyByteArray::RequiredSize(static_cast<uint32_t>(capacity)));
        return ob;
    }
    
    virtual NyInt32Array *NewInt32Array(size_t capacity, NyInt32Array *base, bool old) override {
        DCHECK_LE(capacity, UINT32_MAX);
        void *chunk = ::malloc(NyInt32Array::RequiredSize(static_cast<uint32_t>(capacity)));
        auto ob = new (chunk) NyInt32Array(static_cast<uint32_t>(capacity));
        ob->SetMetatable(core_->kmt_pool()->kInt32Array, core_);
        if (base) {
            DCHECK_GT(capacity, base->capacity());
            ob->Refill(base);
        }
        DCHECK_EQ(ob->PlacedSize(), NyInt32Array::RequiredSize(static_cast<uint32_t>(capacity)));
        return ob;
    }
    
    virtual NyArray *NewArray(size_t capacity, NyArray *base, bool old) override {
        DCHECK_LE(capacity, UINT32_MAX);
        void *chunk = ::malloc(NyArray::RequiredSize(static_cast<uint32_t>(capacity)));
        auto ob = new (chunk) NyArray(static_cast<uint32_t>(capacity));
        ob->SetMetatable(core_->kmt_pool()->kArray, core_);
        if (base) {
            DCHECK_GT(capacity, base->capacity());
            ob->Refill(base, core_);
        }
        DCHECK_EQ(ob->PlacedSize(), NyArray::RequiredSize(static_cast<uint32_t>(capacity)));
        return ob;
    }
    
    virtual NyBytecodeArray *NewBytecodeArray(NyInt32Array *source_lines, Address bytecodes,
                                              size_t bytecode_bytes_size, bool old) override {
        // TODO:
        TODO();
        return nullptr;
    }
    
    virtual NyFunction *NewFunction(NyString *name, size_t n_params, bool vargs, size_t n_upvals,
                                    size_t max_stack, NyString *file_name, NyObject *exec,
                                    NyArray *proto_pool, NyArray *const_pool, bool old) override {
        DCHECK_LE(n_params, UINT8_MAX);
        DCHECK_LE(n_upvals, UINT32_MAX);
        DCHECK_LE(max_stack, UINT32_MAX);
        void *chunk = ::malloc(NyFunction::RequiredSize(static_cast<uint32_t>(n_upvals)));
        auto ob = new (chunk) NyFunction(name,
                                         static_cast<uint8_t>(n_params),
                                         vargs,
                                         static_cast<uint32_t>(n_upvals),
                                         static_cast<uint32_t>(max_stack),
                                         file_name,
                                         exec,
                                         proto_pool,
                                         const_pool,
                                         core_);
        ob->SetMetatable(core_->kmt_pool()->kFunction, core_);
        DCHECK_EQ(ob->PlacedSize(), NyFunction::RequiredSize(static_cast<uint32_t>(n_upvals)));
        return ob;
    }

    virtual NyCode *NewCode(int kind, NyInt32Array *source_lines, const uint8_t *instructions,
                            size_t instructions_byte_size) override {
        // TODO:
        TODO();
        return nullptr;
    }
    
    virtual NyClosure *NewClosure(NyFunction *proto, bool old) override {
        auto ob = new NyClosure(proto, core_);
        DCHECK_EQ(sizeof(NyClosure), ob->PlacedSize());
        return ob;
    }
    
    virtual NyDelegated *NewDelegated(DelegatedKind kind, Address fp, size_t n_upvals,
                                      bool old) override {
        DCHECK_LE(n_upvals, UINT32_MAX);
        void *chunk = ::malloc(NyDelegated::RequiredSize(static_cast<uint32_t>(n_upvals)));
        auto ob = new (chunk) NyDelegated(kind, fp, static_cast<uint32_t>(n_upvals));
        ob->SetMetatable(core_->kmt_pool()->kDelegated, core_);
        DCHECK_EQ(ob->PlacedSize(), NyDelegated::RequiredSize(static_cast<uint32_t>(n_upvals)));
        return ob;
    }
    
    virtual NyUDO *NewUninitializedUDO(size_t size, NyMap *clazz, bool ignore_managed,
                                       bool old) override {
        size_t n_fields = NyUDO::GetNFiedls(size);
        void *chunk = ::malloc(size);
        auto ob = new (chunk) NyUDO(n_fields, ignore_managed);
        ob->SetMetatable(clazz, core_);
        return ob;
    }
    
    virtual NyThread *NewThread(size_t initial_stack_size, bool old) override {
//        auto ob = new NyThread(core_);
//        ob->SetMetatable(core_->kmt_pool()->kThread, core_);
//        return ob;
        // TODO:
        TODO();
        return nullptr;
    }
        
private:
    NyaaCore *const core_;
}; // class MallocObjectFactory
    
/*static*/ ObjectFactory *ObjectFactory::NewMallocFactory(NyaaCore *core) {
    return new MallocObjectFactory(core);
}
    
} // namespace nyaa
    
} // namespace mai
