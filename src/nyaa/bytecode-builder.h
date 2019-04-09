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
class NyScript;
class Object;
    
class BytecodeArrayBuilder {
public:
    BytecodeArrayBuilder() {}
    ~BytecodeArrayBuilder() {}

    std::tuple<Handle<NyByteArray>, Handle<NyInt32Array>> Build(NyaaCore *core);
    
    void Load(IVal a, IVal b, int line = 0);
    
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
    
    void Ret(IVal a, int32_t b, int line = 0) {
        DCHECK_EQ(IVal::kLocal, a.kind);
        Emit(Bytecode::kRet, a.index, b, line);
    }
    
    void Add(IVal a, IVal b, IVal c, int line = 0) { EmitArith(Bytecode::kAdd, a, b, c, line); }
    void Sub(IVal a, IVal b, IVal c, int line = 0) { EmitArith(Bytecode::kSub, a, b, c, line); }
    void Mul(IVal a, IVal b, IVal c, int line = 0) { EmitArith(Bytecode::kMul, a, b, c, line); }
    void Div(IVal a, IVal b, IVal c, int line = 0) { EmitArith(Bytecode::kDiv, a, b, c, line); }

    DISALLOW_IMPLICIT_CONSTRUCTORS(BytecodeArrayBuilder);
private:
    void EmitArith(Bytecode::ID id, IVal a, IVal b, IVal c, int line) {
        DCHECK_EQ(IVal::kLocal, a.kind);
        Emit(id, a.index,
             b.kind == IVal::kConst ? -b.index-1 : b.index,
             c.kind == IVal::kConst ? -c.index-1 : c.index, line);
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
    
    int32_t GerOrNewSmi(int64_t val) { return GetOrNew({.kind = kSmi, .smi_val = val}); }
    int32_t GerOrNewF64(f64_t val) { return GetOrNew({.kind = kF64, .f64_val = val}); }
    int32_t GerOrNewStr(const ast::String *val) {
        return GetOrNew({.kind = kStr, .str_val = val});
    }
    int32_t GerOrNewInt(const ast::String *val) {
        return GetOrNew({.kind = kInt, .int_val = val});
    }
    int32_t Add(Handle<Object> val) {
        int32_t idx = static_cast<int32_t>(const_pool_.size());
        const_pool_.push_back(val);
        return idx;
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
            const ast::String *str_val;
            const ast::String *int_val; // big int literal string
        };
    }; // struct Key
    struct EqualTo : public std::binary_function<Key, Key, bool> {
        bool operator () (const Key &lhs, const Key &rhs) const;
    }; // struct EqualTo
    struct Hash : public std::unary_function<Key, size_t> {
        bool operator () (const Key &key) const;
    }; // struct Hash
    
    int32_t GetOrNew(const Key &key);
    
    //NyaaCore *const core_;
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
    
    static void Disassembly(NyaaCore *core, Handle<NyScript> script, std::string *buf,
                            size_t limit);

    static void Disassembly(NyaaCore *core, Handle<NyScript> script, FILE *fp);

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
