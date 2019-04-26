#include "nyaa/bytecode-builder.h"
#include "nyaa/nyaa-values.h"
#include "nyaa/nyaa-core.h"
#include "nyaa/object-factory.h"
#include "base/arena-utils.h"
#include "base/hash.h"

namespace mai {
    
namespace nyaa {
    
std::tuple<Handle<NyByteArray>, Handle<NyInt32Array>> BytecodeArrayBuilder::Build(NyaaCore *core) {
    Handle<NyByteArray> bcs = core->factory()->NewByteArray(bcs_.size());
    if (!bcs) {
        return {};
    }
    bcs->Add(&bcs_[0], bcs_.size(), core);
    
    Handle<NyInt32Array> info = core->factory()->NewInt32Array(line_info_.size());
    if (!info) {
        return {};
    }
    info->Add(&line_info_[0], line_info_.size(), core);
    return {bcs,info};
}
    
void BytecodeArrayBuilder::Load(IVal a, IVal b, int line) {
    DCHECK_EQ(IVal::kLocal, a.kind);
    switch (b.kind) {
        case IVal::kLocal:
            Emit(Bytecode::kMove, a.index, b.index, line);
            break;
        case IVal::kUpval:
            Emit(Bytecode::kLoadUp, a.index, b.index, line);
            break;
        case IVal::kGlobal:
            Emit(Bytecode::kLoadGlobal, a.index, b.index, line);
            break;
        case IVal::kFunction:
            Emit(Bytecode::kClosure, a.index, b.index, line);
            break;
        case IVal::kConst:
            Emit(Bytecode::kLoadConst, a.index, b.index, line);
            break;
        default:
            DLOG(FATAL) << "noreached!";
            break;
    }
}

void BytecodeArrayBuilder::Emit(Bytecode::ID id, int32_t a, int line) {
    int s = GetScale(a);
    AddID(id, s, line);
    AddParam(a, s, line);
}

void BytecodeArrayBuilder::Emit(Bytecode::ID id, int32_t a, int32_t b, int line) {
    int s = GetScale(a, b);
    AddID(id, s, line);
    AddParam(a, s, line);
    AddParam(b, s, line);
}

void BytecodeArrayBuilder::Emit(Bytecode::ID id, int32_t a, int32_t b, int32_t c, int line) {
    int s = GetScale(a, b, c);
    AddID(id, s, line);
    AddParam(a, s, line);
    AddParam(b, s, line);
    AddParam(c, s, line);
}
    
int BytecodeArrayBuilder::GetScale(int32_t p) {
    if (p <= INT8_MAX && p >= INT8_MIN) {
        return 1;
    } else if (p <= INT16_MAX && p >= INT16_MIN) {
        return 2;
    } else {
        return 4;
    }
}

void BytecodeArrayBuilder::AddID(Bytecode::ID id, int scale, int line) {
    switch (scale) {
        case 1:
            AddByte(id, line);
            break;
        case 2:
            AddByte(Bytecode::kDouble, line);
            AddByte(id, line);
            break;
        case 4:
            AddByte(Bytecode::kQuadruple, line);
            AddByte(id, line);
            break;
        default:
            break;
    }
}

void BytecodeArrayBuilder::AddParam(int32_t p, int scale, int line) {
    switch (scale) {
        case 1:
            AddByte(static_cast<Byte>(p), line);
            break;
        case 2:
            AddByte(static_cast<Byte>(p & 0xff), line);
            AddByte(static_cast<Byte>((p & 0xff00) >> 8), line);
            break;
        case 4:
            AddByte(static_cast<Byte>(p & 0xff), line);
            AddByte(static_cast<Byte>((p & 0xff00) >> 8), line);
            AddByte(static_cast<Byte>((p & 0xff0000) >> 16), line);
            AddByte(static_cast<Byte>((p & 0xff000000) >> 24), line);
            break;
        default:
            break;
    }
}
    
void BytecodeArrayBuilder::Bind(BytecodeLable *lable, ConstPoolBuilder *kpool) {
    if (lable->unbinded()) {
        int pc = static_cast<int>(bcs_.size());
        if (lable->subs_.empty()) {
            lable->pc_ = pc;
            lable->kslot_ = kpool->Add(NyInt32::New(0));
        }

        for (auto sub : lable->subs_) {
            int delta = pc - sub.pc;
            kpool->Set(sub.kslot, NyInt32::New(delta));
        }
        lable->SetBinded();
    } else {
        DCHECK_NE(-1, lable->pc());
        DCHECK_NE(-1, lable->kslot());
    }
}

void BytecodeArrayBuilder::Jump(BytecodeLable *lable, ConstPoolBuilder *kpool, int line) {
    if (lable->unbinded()) {
        int pc = static_cast<int>(bcs_.size());
        int kslot = kpool->Add(NyInt32::New(0));
        lable->subs_.push_back(BytecodeLable::Position{pc, kslot});
        Emit(Bytecode::kJumpConst, kslot, line);
    } else {
        int pc = static_cast<int>(bcs_.size());
        int delta = lable->pc() - pc;
        Emit(Bytecode::kJumpImm, delta, line);
    }
}
    
int32_t ConstPoolBuilder::GetOrNewStr(const NyString *val) {
    View<char> view = MakeView(val->bytes(), val->size());
    return GetOrNew({.kind = kStr, .str_val = view});
}
    
int32_t ConstPoolBuilder::GetOrNewStr(const ast::String *val) {
    View<char> view = MakeView(val->data(), val->size());
    return GetOrNew({.kind = kStr, .str_val = view});
}

int32_t ConstPoolBuilder::GetOrNewInt(const ast::String *val) {
    View<char> view = MakeView(val->data(), val->size());
    return GetOrNew({.kind = kStr, .int_val = view});
}
    
Handle<NyArray> ConstPoolBuilder::Build(NyaaCore *core) {
    if (const_pool_.empty()) {
        return Handle<NyArray>::Null();
    }
    Handle<NyArray> pool(factory_->NewArray(const_pool_.size()));
    for (auto val : const_pool_) {
        pool->Add(*val, core);
    }
    return pool;
}
    
int32_t ConstPoolBuilder::GetOrNew(const Key &key) {
    auto iter = const_to_p_.find(key);
    if (iter != const_to_p_.end()) {
        return iter->second;
    }
    
    int32_t p = -1;
    switch (key.kind) {
        case kF64:
            p = Add(factory_->NewFloat64(key.f64_val));
            break;
        case kSmi:
            p = Add(NySmi::New(key.smi_val));
            break;
        case kStr:
            p = Add(factory_->NewString(key.str_val.z, key.str_val.n));
            break;
        case kInt:
            // TODO:
        default:
            DLOG(FATAL) << "TODO:";
            break;
    }
    const_to_p_.insert({key, p});
    return p;
}


bool ConstPoolBuilder::EqualTo::operator()(const Key &lhs, const Key &rhs) const {
    if (lhs.kind != rhs.kind) {
        return false;
    }
    switch (lhs.kind) {
        case kSmi:
            return lhs.smi_val == rhs.smi_val;
        case kF64:
            return lhs.f64_val == rhs.f64_val;
        case kInt:
            return ::strncmp(lhs.str_val.z, rhs.str_val.z, lhs.str_val.n) == 0;
        case kStr:
            return ::strncmp(lhs.int_val.z, rhs.int_val.z, lhs.int_val.n) == 0;
        default:
            break;
    }
    DLOG(FATAL) << "noreached!";
    return false;
}

size_t ConstPoolBuilder::Hash::operator()(const Key &key) const {
    switch (key.kind) {
        case kSmi:
            return key.smi_val;
        case kF64:
            return *reinterpret_cast<const uint64_t *>(&key.f64_val);
        case kInt:
            return base::Hash::Js(key.int_val.z, key.int_val.n);
        case kStr:
            return base::Hash::Js(key.str_val.z, key.str_val.n);
        default:
            break;
    }
    DLOG(FATAL) << "noreached!";
    return 0;
}
    
void BytecodeArrayDisassembler::Disassembly() {
    BytecodeNode bc;
    while (ReadNext(&bc)) {
        fprintf(fp_, "[%03u] %s ", bc.pc, Bytecode::kNames[bc.id]);
        for (int i = 0; i < bc.n; ++i) {
            fprintf(fp_, "%d ", (&bc.a)[i]);
        }
        if (info_.is_valid()) {
            fprintf(fp_, "; line: %d", bc.line);
        }
        fputc('\n', fp_);
    }
}
    
/*static*/
void BytecodeArrayDisassembler::Disassembly(NyaaCore *core, Handle<NyFunction> script,
                                            std::string *buf, size_t limit) {
    buf->resize(limit);
    FILE *fp = ::fmemopen(&(*buf)[0], limit, "w");
    if (!fp) {
        return;
    }
    Disassembly(core, script, fp);
    buf->resize(::ftell(fp));
    ::fclose(fp);
}
    
static void PrintFunctionDesc(NyaaCore *core, Handle<NyFunction> script, FILE *fp) {
    fprintf(fp, "function: %s, params: %u, vargs: %d, max-stack: %u, upvals: %u\n",
            !script->name() ? "[unnamed]" : script->name()->bytes(),
            script->n_params(), script->vargs(), script->max_stack(), script->n_upvals());
    if (script->file_name()) {
        fprintf(fp, "file name: %s\n", script->file_name()->bytes());
    }
    if (script->const_pool()) {
        fprintf(fp, "const pool: %u\n", script->const_pool()->size());
        for (int i = 0; i < script->const_pool()->size(); ++i) {
            Handle<NyString> str(script->const_pool()->Get(i)->ToString(core));
            fprintf(fp, " [%u] (%d) %s\n", i, -i-1, str->bytes());
        }
    }
}
    
/*static*/
void BytecodeArrayDisassembler::Disassembly(NyaaCore *core, Handle<NyFunction> script, FILE *fp) {
    PrintFunctionDesc(core, script, fp);
    fprintf(fp, ".............................\n");
    Disassembly(script->bcbuf(), script->file_info(), fp);
    
    if (script->proto_pool()) {
        for (size_t i = 0; i < script->proto_pool()->size(); ++i) {
            Handle<NyFunction> proto(script->proto_pool()->Get(i));
            fprintf(fp, "-----------------------------\n");
            PrintFunctionDesc(core, proto, fp);
            fprintf(fp, ".............................\n");
            Disassembly(proto->bcbuf(), proto->file_info(), fp);
        }
    }
}
    
/*static*/
void BytecodeArrayDisassembler::Disassembly(Handle<NyByteArray> bcs, Handle<NyInt32Array> info,
                                            std::string *buf, size_t limit) {
    buf->resize(limit);
    FILE *fp = ::fmemopen(&(*buf)[0], limit, "w");
    if (!fp) {
        return;
    }
    Disassembly(bcs, info, fp);
    buf->resize(::ftell(fp));
    ::fclose(fp);
}

/*static*/
void BytecodeArrayDisassembler::Disassembly(Handle<NyByteArray> bcs, Handle<NyInt32Array> info,
                                            FILE *fp) {
    BytecodeArrayDisassembler dis(bcs, info, fp);
    dis.Disassembly();
}
    
bool BytecodeArrayDisassembler::ReadNext(BytecodeNode *bc) {
    if (IsEnd()) {
        return false;
    }
    int scale = 0;
    bc->pc = pc_;
    Bytecode::ID maybe_prefix = static_cast<Bytecode::ID>(bcs_->Get(pc_++));
    switch (maybe_prefix) {
        case Bytecode::kDouble:
            scale = 2;
            break;
        case Bytecode::kQuadruple:
            scale = 4;
            break;
        default:
            scale = 1;
            pc_--;
            break;
    }
    
    if (info_.is_valid()) {
        bc->line = info_->Get(pc_);
    }
    bc->id = static_cast<Bytecode::ID>(bcs_->Get(pc_++));
    switch (bc->id) {
#define DEFINE_READ_PARAMS(name, argc, ...) \
        case Bytecode::k##name: \
            ParseInt32Params(scale, argc, &bc->a, &bc->b, &bc->c); \
            bc->n = argc; \
            break;
        DECL_BYTECODES(DEFINE_READ_PARAMS)
#undef DEFINE_READ_PARAMS
        default:
            DLOG(FATAL) << "incorrect bytecode: " << bc->id;
            break;
    }
    return true;
}
    
bool BytecodeArrayDisassembler::IsEnd() const {
    return pc_ >= bcs_->size();
}
    
void BytecodeArrayDisassembler::ParseInt32Params(int scale, int n, ...) {
    va_list ap;
    va_start(ap, n);
    for (int i = 0; i < n; ++i) {
        int32_t *p = va_arg(ap, int32_t *);
        View<Byte> slice = bcs_->GetView(pc_, scale);
        *p = Bytecode::ParseInt32Param(slice, scale);
        pc_ += scale;
    }
    va_end(ap);
}
    
} // namespace nyaa
    
} // namespace mai
