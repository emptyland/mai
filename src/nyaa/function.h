#ifndef MAI_NYAA_FUNCTION_H_
#define MAI_NYAA_FUNCTION_H_

#include "nyaa/nyaa-values.h"
#include "asm/utils.h"
#include <type_traits>

namespace mai {
    
namespace nyaa {

class NyFunction final : public NyObject {
public:
    using Template = arch::ObjectTemplate<NyFunction, int32_t>;
    using UpvalDesc = mai::nyaa::UpvalDesc;
    static const int32_t kOffsetCode;
    static const int32_t kOffsetBcbuf;
    static const int32_t kOffsetConstPool;
    
    NyFunction(NyString *name,
               uint8_t n_params,
               bool vargs,
               uint32_t n_upvals,
               uint32_t max_stack_size,
               NyString *file_name,
               NyObject *exec,
               NyArray *proto_pool,
               NyArray *const_pool,
               NyaaCore *N);
    
    DEF_VAL_GETTER(uint8_t, n_params);
    DEF_VAL_GETTER(bool, vargs);
    DEF_VAL_GETTER(uint32_t, n_upvals);
    DEF_VAL_GETTER(uint32_t, called_count);
    DEF_VAL_GETTER(uint32_t, max_stack);
    DEF_PTR_GETTER(NyString, name);
    DEF_PTR_GETTER(NyObject, exec);
    DEF_PTR_GETTER(NyString, file_name);
    DEF_PTR_GETTER(NyArray, proto_pool);
    DEF_PTR_GETTER(NyArray, const_pool);
    DEF_PTR_GETTER(NyString, packed_ast);
    
    NyBytecodeArray *bcbuf() const {
        DCHECK(IsInterpretationExec());
        return bcbuf_;
    }
    
    NyCode *code() const {
        DCHECK(IsNativeExec());
        return code_;
    }
    
    void SetName(NyString *name, NyaaCore *N);
    
    bool IsNativeExec() const {
        DCHECK(exec_->IsCode() || exec_->IsBytecodeArray());
        return exec_->IsCode();
    }
    
    bool IsInterpretationExec() const {
        DCHECK(exec_->IsCode() || exec_->IsBytecodeArray());
        return exec_->IsBytecodeArray();
    }
    
    const UpvalDesc &upval(size_t i) const {
        DCHECK_LT(i, n_upvals_);
        return upvals_[i];
    }
    
    const UpvalDesc *upvals() const { return upvals_; }
    
    uint32_t IncreaseCalledCount() { return ++called_count_; }
    
    void SetUpval(size_t i, NyString *name, bool in_stack, int32_t index,
                  NyaaCore *N);
    
    void SetPackedAST(NyString *packed, NyaaCore *N);
    
    static Handle<NyFunction> Compile(const char *z, NyaaCore *N) {
        return Compile(z, !z ? 0 : ::strlen(z), N);
    }
    static Handle<NyFunction> Compile(const char *z, size_t n, NyaaCore *N);
    static Handle<NyFunction> Compile(const char *file_name, FILE *fp, NyaaCore *N);
    
    void Iterate(ObjectVisitor *visitor);
    
    size_t PlacedSize() const { return RequiredSize(n_upvals_); }
    
    static size_t RequiredSize(uint32_t n_upvals) {
        return sizeof(NyFunction) + n_upvals * sizeof(upvals_[0]);
    }
    
    DEF_HEAP_OBJECT(Function);
private:
    uint8_t n_params_;
    bool vargs_;
    uint32_t n_upvals_;
    uint32_t max_stack_;
    uint32_t called_count_ = 0;
    NyString *name_; // [strong ref]
    NyString *file_name_; // [strong ref]
    NyArray *proto_pool_; // [strong ref] internal defined functions.
    NyArray *const_pool_; // [strong ref]
    union {
        NyBytecodeArray *bcbuf_; // [strong ref]
        NyCode *code_; // [strong ref]
        NyObject *exec_; // [strong ref] stub
    };
    NyString *packed_ast_ = nullptr; // [strong ref] serialized ast data.
    UpvalDesc upvals_[0]; // elements [strong ref]
}; // class NyCallable
    
class NyBytecodeArray : public NyObject {
public:
    NyBytecodeArray(NyInt32Array *source_lines, Address bytecodes, uint32_t bytecode_bytes_size,
                    NyaaCore *N);
    
    DEF_PTR_GETTER(NyInt32Array, source_lines);
    DEF_VAL_GETTER(uint32_t, bytecode_bytes_size);
    
    uint8_t byte(size_t i) const {
        DCHECK_GE(i, 0);
        DCHECK_LT(i, bytecode_bytes_size_);
        return buf_[i];
    }
    
    inline View<Byte> GetView(uint32_t begin, uint32_t len) const {
        DCHECK_LE(begin + len, bytecode_bytes_size_);
        return MakeView(buf_ + begin, len);
    }
    
    void Iterate(ObjectVisitor *visitor) {
        visitor->VisitPointer(this, reinterpret_cast<Object **>(&source_lines_));
    }
    
    size_t PlacedSize() const { return RequiredSize(bytecode_bytes_size_); }
    
    static size_t RequiredSize(uint32_t bytecode_bytes_size) {
        return sizeof(NyBytecodeArray) + bytecode_bytes_size * sizeof(buf_[0]);
    }
    
    DEF_HEAP_OBJECT(BytecodeArray);
private:
    NyInt32Array *source_lines_; // [strong ref]
    uint32_t bytecode_bytes_size_;
    Byte buf_[0];
}; // class NyBytecodeArray
    
// This object only in code area of heap
class NyCode final : public NyObject {
public:
    enum Kind {
        kStub,
        kHandler,
        kFunction,
        kOptimizedFunction,
    };
    
    using Template = arch::ObjectTemplate<NyCode, int32_t>;
    
    static const int32_t kOffsetKind;
    static const int32_t kOffsetInstructionsBytesSize;
    static const int32_t kOffsetInstructions;

    NyCode(Kind kind, NyInt32Array *source_lines, const uint8_t *instructions,
           uint32_t instructions_bytes_size, NyaaCore *N);
    
    DEF_PTR_GETTER(NyInt32Array, source_lines);
    DEF_VAL_GETTER(Kind, kind);
    DEF_VAL_GETTER(uint32_t, instructions_bytes_size);
    Address entry_address() { return instructions_; }

    void Iterate(ObjectVisitor *visitor) {
        visitor->VisitPointer(this, reinterpret_cast<Object **>(&source_lines_));
    }
    
    size_t PlacedSize() const { return RequiredSize(instructions_bytes_size_); }
    
    static size_t RequiredSize(uint32_t instructions_bytes_size) {
        return sizeof(NyCode) + instructions_bytes_size * sizeof(instructions_[0]);
    }
private:
    NyInt32Array *source_lines_; // [strong ref]
    Kind kind_;
    uint32_t instructions_bytes_size_;
    Byte instructions_[0];
}; // class NyCode
    

template<class T>
class CallStub final {
public:
    static_assert(std::is_function<T>::value, "only call function");
    
    CallStub(NyCode *code) : entry_fn_(reinterpret_cast<T*>(code->entry_address())) {}
    
    T *entry_fn() const { return entry_fn_; }
private:
    T *entry_fn_;
}; // class CallStub
    
class NyClosure final : public NyRunnable {
public:
    using Template = arch::ObjectTemplate<NyClosure, int32_t>;
    
    static const int32_t kOffsetProto;
    static const int32_t kOffsetUpvals;
    
    NyClosure(NyFunction *proto, NyaaCore *N);
    
    DEF_PTR_GETTER(NyFunction, proto);
    Object *upval(int i) const {
        DCHECK_GE(i, 0);
        DCHECK_LT(i, proto_->n_upvals());
        return upvals_[i];
    }
    
    void Bind(int i, Object *upval, NyaaCore *N);
    
    int Call(Object *argv[], int argc, int wanted, NyaaCore *N);
    
    static Handle<NyClosure> Compile(const char *z, NyaaCore *N) {
        return Compile(z, !z ? 0 : ::strlen(z), N);
    }
    static Handle<NyClosure> Compile(const char *z, size_t n, NyaaCore *N);
    static Handle<NyClosure> Compile(const char *file_name, FILE *fp, NyaaCore *N);
    
    static int Do(const char *z, size_t n, int wanted, NyMap *env, NyaaCore *N);
    static int Do(const char *file_name, FILE *fp, int wanted, NyMap *env, NyaaCore *N);
    static int DoFile(const char *file_name, int wanted, NyMap *env, NyaaCore *N);
    
    void Iterate(ObjectVisitor *visitor) {
        visitor->VisitPointer(this, reinterpret_cast<Object **>(&proto_));
        visitor->VisitPointers(this, upvals_, upvals_ + proto_->n_upvals());
    }
    
    size_t PlacedSize() const { return RequiredSize(proto_->n_upvals()); }
    
    static size_t RequiredSize(uint32_t n_upvals) {
        return sizeof(NyClosure) + n_upvals * sizeof(Object *);
    }
    
    DEF_HEAP_OBJECT(Closure);
private:
    NyFunction *proto_; // [strong ref]
    Object *upvals_[0]; // [strong ref]
}; // class NyClosure

class NyDelegated final : public NyRunnable {
public:
    using Kind = DelegatedKind;
    using FunctionCallbackFP = void (*)(const FunctionCallbackInfo<Object> &);
    
    NyDelegated(Kind kind, Address fp, uint32_t n_upvals)
    : kind_(kind)
    , stub_(static_cast<void *>(fp))
    , n_upvals_(n_upvals) {
        ::memset(upvals_, 0, n_upvals * sizeof(Object *));
    }
    
    DEF_VAL_GETTER(uint32_t, n_upvals);
    
    Object *upval(int i) const {
        DCHECK_GE(i, 0);
        DCHECK_LT(i, n_upvals_);
        return upvals_[i];
    }
    
    void Bind(int i, Object *upval, NyaaCore *N);
    
    int Call(Object *argv[], int argc, int nrets, NyaaCore *N);
    
    int Apply(const FunctionCallbackInfo<Object> &info);
    
    DEF_VAL_GETTER(Kind, kind);
    
    Address fp_addr() { return static_cast<Address>(stub_); }
    
    void Iterate(ObjectVisitor *visitor) {
        visitor->VisitPointers(this, upvals_, upvals_ + n_upvals_);
    }
    
    size_t PlacedSize() const { return RequiredSize(n_upvals_); }
    
    static size_t RequiredSize(uint32_t n_upvals) {
        return sizeof(NyDelegated) + n_upvals * sizeof(Object *);
    }
    
    DEF_HEAP_OBJECT(Delegated);
private:
    Kind kind_;
    uint32_t n_upvals_;
    union {
        FunctionCallbackFP fn_fp_;
        void *stub_;
    };
    Object *upvals_[0]; // [strong ref], MUST BE TAIL
}; // class NyDelegated

    
} // namespace nyaa
    
    
} // namespace mai


#endif // MAI_NYAA_FUNCTION_H_
