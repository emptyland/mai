#ifndef MAI_NYAA_BYTECODE_BUILDER_H_
#define MAI_NYAA_BYTECODE_BUILDER_H_

#include "nyaa/bytecode.h"
#include "nyaa/builtin.h"
#include "mai-lang/handles.h"
#include "glog/logging.h"
#include <unordered_map>

namespace mai {
namespace base {
class ArenaString;
} // namespace base
namespace nyaa {
namespace ast {
using String = base::ArenaString;
} // namespace ast
class NyaaCore;
class ObjectFactory;
class NyArray;
class NyByteArray;
class NyInt32Array;
class NyFunction;
class Object;
class BytecodeArrayBuilder;
class ConstPoolBuilder;
    
class BytecodeLable {
public:
    BytecodeLable() {}
    ~BytecodeLable() {}

    bool unbinded() const { return !binded_; }
    
    void SetBinded() {
        binded_ = true;
        subs_.clear();
    }
    
    void Reset() {
        pc_ = -1;
        kslot_ = -1;
        binded_ = false;
        subs_.clear();
    }
    
    DEF_VAL_GETTER(int, pc);
    DEF_VAL_GETTER(int, kslot);
    
    friend class BytecodeArrayBuilder;
    DISALLOW_IMPLICIT_CONSTRUCTORS(BytecodeLable);
private:
    struct Position {
        int pc = -1;
        int kslot = -1;
    };
    
    bool binded_ = false;
    int pc_ = -1;
    int kslot_ = -1;
    std::vector<Position> subs_;
}; // class BytecodeLable
    
class BytecodeArrayBuilder final {
public:
    BytecodeArrayBuilder() {}
    ~BytecodeArrayBuilder() {}

    std::tuple<Handle<NyByteArray>, Handle<NyInt32Array>> Build(NyaaCore *core);
    
    void Load(IVal a, IVal b, int line = 0);
    
    void LoadImm(IVal a, int32_t imm, int line = 0) {
        DCHECK_EQ(IVal::kLocal, a.kind);
        Emit(Bytecode::kLoadImm, a.index, imm, line);
    }
    
    void LoadNil(IVal a, int32_t b, int line = 0) {
        DCHECK_EQ(IVal::kLocal, a.kind);
        Emit(Bytecode::kLoadNil, a.index, b, line);
    }
    
    void Move(IVal a, IVal b, int line = 0) {
        DCHECK_EQ(IVal::kLocal, a.kind);
        DCHECK_EQ(IVal::kLocal, b.kind);
        if (a.index != b.index) {
            Emit(Bytecode::kMove, a.index, b.index, line);
        }
    }
    
    void Call(IVal a, int32_t b, int32_t c, int line = 0) {
        DCHECK_EQ(IVal::kLocal, a.kind);
        Emit(Bytecode::kCall, a.index, b, c, line);
    }
    
    void New(IVal a, IVal b, int32_t c, int line = 0) {
        DCHECK_EQ(IVal::kLocal, a.kind);
        DCHECK_EQ(IVal::kLocal, b.kind);
        Emit(Bytecode::kNew, a.index, b.index, c, line);
    }
    
    void Ret(IVal a, int32_t b, int line = 0) {
        DCHECK_EQ(IVal::kLocal, a.kind);
        Emit(Bytecode::kRet, a.index, b, line);
    }
    
    void Add(IVal a, IVal b, IVal c, int line = 0) { EmitArith(Bytecode::kAdd, a, b, c, line); }
    void Sub(IVal a, IVal b, IVal c, int line = 0) { EmitArith(Bytecode::kSub, a, b, c, line); }
    void Mul(IVal a, IVal b, IVal c, int line = 0) { EmitArith(Bytecode::kMul, a, b, c, line); }
    void Div(IVal a, IVal b, IVal c, int line = 0) { EmitArith(Bytecode::kDiv, a, b, c, line); }
    
    void Equal(IVal a, IVal b, IVal c, int line = 0) {
        EmitArith(Bytecode::kEqual, a, b, c, line);
    }
    void NotEqual(IVal a, IVal b, IVal c, int line = 0) {
        EmitArith(Bytecode::kNotEqual, a, b, c, line);
    }
    void LessThan(IVal a, IVal b, IVal c, int line = 0) {
        EmitArith(Bytecode::kLessThan, a, b, c, line);
    }
    void LessEqual(IVal a, IVal b, IVal c, int line = 0) {
        EmitArith(Bytecode::kLessEqual, a, b, c, line);
    }
    void GreaterThan(IVal a, IVal b, IVal c, int line = 0) {
        EmitArith(Bytecode::kGreaterThan, a, b, c, line);
    }
    void GreaterEqual(IVal a, IVal b, IVal c, int line = 0) {
        EmitArith(Bytecode::kGreaterEqual, a, b, c, line);
    }
    void Test(IVal a, int32_t b, int32_t c, int line = 0) {
        DCHECK_EQ(IVal::kLocal, a.kind);
        Emit(Bytecode::kTest, a.index, b, c, line);
    }
    void TestNil(IVal a, int32_t b, int32_t c, int line = 0) {
        DCHECK_EQ(IVal::kLocal, a.kind);
        Emit(Bytecode::kTestNil, a.index, b, c, line);
    }
    void Concat(IVal a, IVal b, int32_t c, int line = 0) {
        DCHECK_EQ(IVal::kLocal, a.kind);
        DCHECK_EQ(IVal::kLocal, b.kind);
        Emit(Bytecode::kConcat, a.index, b.index, c, line);
    }
    
    void Bind(BytecodeLable *lable, ConstPoolBuilder *kpool);
    
    void Jump(BytecodeLable *lable, ConstPoolBuilder *kpool, int line = 0);
    
    void GetField(IVal a, IVal b, IVal c, int line = 0) {
        DCHECK_EQ(IVal::kLocal, a.kind);
        DCHECK_EQ(IVal::kLocal, b.kind);
        Emit(Bytecode::kGetField, a.index, b.index, c.Encode(), line);
    }
    
    void SetField(IVal a, IVal b, IVal c, int line = 0) {
        DCHECK_EQ(IVal::kLocal, a.kind);
        Emit(Bytecode::kSetField, a.index, b.Encode(), c.Encode(), line);
    }
    
    void StoreGlobal(IVal a, IVal b, int line = 0) {
        DCHECK_EQ(IVal::kLocal, a.kind);
        DCHECK_EQ(IVal::kGlobal, b.kind);
        Emit(Bytecode::kStoreGlobal, a.index, b.index, line);
    }
    
    void StoreUp(IVal a, IVal b, int line = 0) {
        DCHECK_EQ(IVal::kLocal, a.kind);
        DCHECK_EQ(IVal::kUpval, b.kind);
        Emit(Bytecode::kStoreUp, a.index, b.index, line);
    }
    
    void Closure(IVal a, IVal b, int line = 0) {
        DCHECK_EQ(IVal::kLocal, a.kind);
        DCHECK_EQ(IVal::kFunction, b.kind);
        Emit(Bytecode::kClosure, a.index, b.index, line);
    }
    
    void NewMap(IVal a, int32_t b, int32_t c, int line = 0) {
        DCHECK_EQ(IVal::kLocal, a.kind);
        Emit(Bytecode::kNewMap, a.index, b, c, line);
    }
    
    void NewClass(IVal a, int32_t b, int32_t c, int line = 0) {
        DCHECK_EQ(IVal::kLocal, a.kind);
        Emit(Bytecode::kNewClass, a.index, b, c, line);
    }
    
    void Vargs(IVal a, int32_t b, int line = 0) {
        DCHECK_EQ(IVal::kLocal, a.kind);
        Emit(Bytecode::kVargs, a.index, b, line);
    }
    
    void Self(IVal a, IVal b, IVal c, int line = 0) {
        DCHECK_EQ(IVal::kLocal, a.kind);
        DCHECK_EQ(IVal::kLocal, b.kind);
        DCHECK_EQ(IVal::kConst, c.kind);
        Emit(Bytecode::kSelf, a.index, b.index, c.index, line);
    }

    DISALLOW_IMPLICIT_CONSTRUCTORS(BytecodeArrayBuilder);
private:
    void EmitArith(Bytecode::ID id, IVal a, IVal b, IVal c, int line) {
        DCHECK_EQ(IVal::kLocal, a.kind);
        Emit(id, a.index, b.Encode(), c.Encode(), line);
    }
    
    void Emit(Bytecode::ID id, int line) { AddID(id, 1, line); }
    void Emit(Bytecode::ID id, int32_t a, int line);
    void Emit(Bytecode::ID id, int32_t a, int32_t b, int line);
    void Emit(Bytecode::ID id, int32_t a, int32_t b, int32_t c, int line);
    
    int GetScale(int32_t a, int32_t b, int32_t c) {
        return std::max(GetScale(a, b), GetScale(b));
    }
    int GetScale(int32_t a, int32_t b) { return std::max(GetScale(a), GetScale(b)); }
    int GetScale(int32_t a);
    void AddID(Bytecode::ID id, int scale, int line);
    void AddParam(int32_t p, int scale, int line);

    void AddByte(Byte b, int line) {
        bcs_.push_back(b);
        line_info_.push_back(line);
    }
    
    std::vector<Byte> bcs_;
    std::vector<int>  line_info_;
}; // class BytecodeArrayBuilder

    
class ConstPoolBuilder {
public:
    ConstPoolBuilder(ObjectFactory *factory)
        : factory_(DCHECK_NOTNULL(factory)) {}
    
    Handle<NyArray> Build(NyaaCore *core);
    
    int32_t GetOrNewSmi(int64_t val) { return GetOrNew({.kind = kSmi, .smi_val = val}); }
    int32_t GetOrNewF64(f64_t val) { return GetOrNew({.kind = kF64, .f64_val = val}); }
    int32_t GetOrNewStr(const NyString *val);
    int32_t GetOrNewStr(const ast::String *val);
    int32_t GetOrNewInt(const ast::String *val);
    int32_t Add(Handle<Object> val) {
        int32_t idx = static_cast<int32_t>(const_pool_.size());
        const_pool_.push_back(val);
        return idx;
    }
    void Set(int32_t i, Handle<Object> val) {
        DCHECK_GE(i, 0);
        DCHECK_LT(i, const_pool_.size());
        const_pool_[i] = val;
    }
    
    DISALLOW_IMPLICIT_CONSTRUCTORS(ConstPoolBuilder);
private:
    enum Kind {
        kSmi,
        kInt,
        kF64,
        kStr,
    };
    struct Key {
        Kind kind;
        union {
            int64_t smi_val;
            f64_t f64_val;
            View<char> str_val;
            View<char> int_val; // big int literal string
        };
    }; // struct Key
    struct EqualTo : public std::binary_function<Key, Key, bool> {
        bool operator () (const Key &lhs, const Key &rhs) const;
    }; // struct EqualTo
    struct Hash : public std::unary_function<Key, size_t> {
        size_t operator () (const Key &key) const;
    }; // struct Hash
    
    int32_t GetOrNew(const Key &key);

    ObjectFactory *const factory_;
    std::unordered_map<Key, int32_t, Hash, EqualTo> const_to_p_;
    std::vector<Handle<Object>> const_pool_;
}; // class ConstPoolBuilder


class BytecodeArrayDisassembler {
public:
    BytecodeArrayDisassembler(Handle<NyByteArray> bcs, Handle<NyInt32Array> info, FILE *fp)
        : bcs_(bcs)
        , info_(info)
        , fp_(DCHECK_NOTNULL(fp)) {}
    
    void Disassembly();
    
    static void Disassembly(NyaaCore *core, Handle<NyFunction> script, std::string *buf,
                            size_t limit);

    static void Disassembly(NyaaCore *core, Handle<NyFunction> script, FILE *fp);

    static void Disassembly(Handle<NyByteArray> bcs, Handle<NyInt32Array> info, std::string *buf,
                            size_t limit);
    static void Disassembly(Handle<NyByteArray> bcs, Handle<NyInt32Array> info, FILE *fp);

    DISALLOW_IMPLICIT_CONSTRUCTORS(BytecodeArrayDisassembler);
private:
    struct BytecodeNode {
        Bytecode::ID id;
        int32_t n;
        int32_t a;
        int32_t b;
        int32_t c;
        uint32_t pc;
        int32_t line;
    };

    bool ReadNext(BytecodeNode *bc);
    bool IsEnd() const;
    void ParseInt32Params(int scale, int n, ...);

    Handle<NyByteArray> bcs_;
    Handle<NyInt32Array> info_;
    FILE *fp_;
    uint32_t pc_ = 0;
}; // class BytecodeArrayDisassembler
    
    

    
} // namespace nyaa
    
} // namespace mai


#endif // MAI_NYAA_BYTECODE_BUILDER_H_
