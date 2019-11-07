#include "nyaa/function.h"
#include "nyaa/nyaa-core.h"
#include "nyaa/parser.h"
#include "nyaa/ast.h"
//#include "nyaa/code-gen.h"
#include "nyaa/object-factory.h"
//#include "nyaa/thread.h"
#include "base/arenas.h"
#include "mai-lang/isolate.h"
#include "mai/env.h"

namespace mai {
    
namespace nyaa {
    
////////////////////////////////////////////////////////////////////////////////////////////////////
/// class NyFunction:
////////////////////////////////////////////////////////////////////////////////////////////////////
    
const int32_t NyFunction::kOffsetCode = Template::OffsetOf(&NyFunction::code_);
const int32_t NyFunction::kOffsetBcbuf = Template::OffsetOf(&NyFunction::bcbuf_);

NyFunction::NyFunction(NyString *name, uint8_t n_params, bool vargs, uint32_t n_upvals,
                       uint32_t max_stack, NyString *file_name, NyObject *exec, NyArray *proto_pool,
                       NyArray *const_pool, NyaaCore *N)
    : name_(name)
    , n_params_(n_params)
    , vargs_(vargs)
    , n_upvals_(n_upvals)
    , max_stack_(max_stack)
    , file_name_(file_name)
    , exec_(exec)
    , proto_pool_(proto_pool)
    , const_pool_(const_pool) {
    DCHECK(exec->IsBytecodeArray() || exec->IsCode());
    N->BarrierWr(this, &name_, name);
    N->BarrierWr(this, &file_name_, file_name);
    N->BarrierWr(this, &exec_, exec);
    N->BarrierWr(this, &proto_pool_, proto_pool);
    N->BarrierWr(this, &const_pool_, const_pool);
    ::memset(upvals_, 0, n_upvals * sizeof(upvals_[0]));
}

void NyFunction::SetName(NyString *name, NyaaCore *N) {
    N->BarrierWr(this, &name_, name);
    name_ = name;
}

void NyFunction::SetUpval(size_t i, NyString *name, bool in_stack, int32_t index, NyaaCore *N) {
    DCHECK_LT(i, n_upvals_);
    N->BarrierWr(this, &upvals_[i].name, name);
    upvals_[i].name = name;
    upvals_[i].in_stack = in_stack;
    upvals_[i].index = index;
}
    
void NyFunction::SetPackedAST(NyString *packed, NyaaCore *N) {
    N->BarrierWr(this, &packed_ast_, packed);
    packed_ast_ = packed;
}

void NyFunction::Iterate(ObjectVisitor *visitor) {
    visitor->VisitPointer(this, reinterpret_cast<Object **>(&name_));
    visitor->VisitPointer(this, reinterpret_cast<Object **>(&exec_));
    visitor->VisitPointer(this, reinterpret_cast<Object **>(&file_name_));
    visitor->VisitPointer(this, reinterpret_cast<Object **>(&proto_pool_));
    visitor->VisitPointer(this, reinterpret_cast<Object **>(&const_pool_));
    visitor->VisitPointer(this, reinterpret_cast<Object **>(&packed_ast_));
    for (uint32_t i = 0; i < n_upvals_; ++i) {
        visitor->VisitPointer(this, reinterpret_cast<Object **>(&upvals_[i].name));
    }
}

/*static*/ Handle<NyFunction> NyFunction::Compile(const char *z, size_t n, NyaaCore *N) {
    base::StandaloneArena arena(N->isolate()->env()->GetLowLevelAllocator());
    Parser::Result result = Parser::Parse(z, n, N->max_trace_id(), &arena);
    if (result.error) {
        N->Raisef("[%d:%d] %s", result.error_line, result.error_column, result.error->data());
        return Handle<NyFunction>();
    }
    N->set_max_trace_id(result.next_trace_id);
    // TODO: return CodeGen::Generate(N->factory()->NewString(":memory:"), result.block, &arena, N);
    TODO();
    return Handle<NyFunction>::Empty();
}

/*static*/ Handle<NyFunction> NyFunction::Compile(const char *file_name, FILE *fp, NyaaCore *N) {
    base::StandaloneArena arena(N->isolate()->env()->GetLowLevelAllocator());
    Parser::Result result = Parser::Parse(fp, N->max_trace_id(), &arena);
    if (result.error) {
        N->Raisef("%s [%d:%d] %s", file_name, result.error_line, result.error_column,
                  result.error->data());
        return Handle<NyFunction>();
    }
    N->set_max_trace_id(result.next_trace_id);
    // TODO: return CodeGen::Generate(N->factory()->NewString(file_name), result.block, &arena, N);
    TODO();
    return Handle<NyFunction>::Empty();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
/// class NyCode:
////////////////////////////////////////////////////////////////////////////////////////////////////
NyBytecodeArray::NyBytecodeArray(NyInt32Array *source_lines, Address bytecodes,
                                 uint32_t bytecode_bytes_size, NyaaCore *N)
    : source_lines_(source_lines)
    , bytecode_bytes_size_(bytecode_bytes_size) {
    N->BarrierWr(this, &source_lines_, source_lines);
        
    ::memcpy(buf_, bytecodes, bytecode_bytes_size);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
/// class NyCode:
////////////////////////////////////////////////////////////////////////////////////////////////////
    
const int32_t NyCode::kOffsetKind = Template::OffsetOf(&NyCode::kind_);
const int32_t NyCode::kOffsetInstructionsBytesSize = Template::OffsetOf(&NyCode::instructions_bytes_size_);
const int32_t NyCode::kOffsetInstructions = Template::OffsetOf(&NyCode::instructions_);
    
NyCode::NyCode(Kind kind, NyInt32Array *source_lines, const uint8_t *instructions,
               uint32_t instructions_bytes_size, NyaaCore *N)
    : kind_(kind)
    , source_lines_(source_lines)
    , instructions_bytes_size_(instructions_bytes_size) {
    N->BarrierWr(this, &source_lines_, source_lines);
    ::memcpy(instructions_, instructions, instructions_bytes_size);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
/// class NyClosure:
////////////////////////////////////////////////////////////////////////////////////////////////////
    
const int32_t NyClosure::kOffsetProto = Template::OffsetOf(&NyClosure::proto_);
const int32_t NyClosure::kOffsetUpvals = Template::OffsetOf(&NyClosure::upvals_);

NyClosure::NyClosure(NyFunction *proto, NyaaCore *N)
: proto_(DCHECK_NOTNULL(proto)) {
    N->BarrierWr(this, &proto_, proto);
    ::memset(upvals_, 0, proto_->n_upvals() * sizeof(upvals_[0]));
}

void NyClosure::Bind(int i, Object *upval, NyaaCore *N) {
    DCHECK_GE(i, 0);
    DCHECK_LT(i, proto_->n_upvals());
    N->BarrierWr(this, upvals_ + i, upval);
    upvals_[i] = upval;
}

int NyClosure::Call(Object *argv[], int argc, int wanted, NyaaCore *N) {
    // TODO: return N->curr_thd()->TryRun(this, argv, argc, wanted);
    TODO();
    return -1;
}

/*static*/ Handle<NyClosure> NyClosure::Compile(const char *z, size_t n, NyaaCore *N) {
    // TODO:
    TODO();
    return Handle<NyClosure>::Empty();
}

/*static*/ Handle<NyClosure> NyClosure::Compile(const char *file_name, FILE *fp, NyaaCore *N) {
    // TODO:
    TODO();
    return Handle<NyClosure>::Empty();
}

/*static*/ int NyClosure::Do(const char *z, size_t n, int wanted, NyMap *env, NyaaCore *N) {
    Handle<NyClosure> script = Compile(z, n, N);
    if (script.is_not_valid()) {
        return -1;
    }
    // TODO: return N->curr_thd()->TryRun(*script, nullptr/*argv*/, 0/*argc*/, wanted, env);
    TODO();
    return -1;
}

/*static*/ int NyClosure::Do(const char *file_name, FILE *fp, int wanted, NyMap *env, NyaaCore *N) {
    Handle<NyClosure> script = Compile(file_name, fp, N);
    if (script.is_not_valid()) {
        return -1;
    }
    // TODO: return N->curr_thd()->TryRun(*script, nullptr/*argv*/, 0/*argc*/, wanted, env);
    TODO();
    return -1;
}

/*static*/ int NyClosure::DoFile(const char *file_name, int wanted, NyMap *env, NyaaCore *N) {
    FILE *fp = ::fopen(file_name, "r");
    if (!fp) {
        return -1;
    }
    int rv = Do(file_name, fp, wanted, env, N);
    ::fclose(fp);
    return rv;
}

} // namespace nyaa
    
    
} // namespace mai
