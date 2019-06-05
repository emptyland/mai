#include "nyaa/function.h"
#include "nyaa/nyaa-core.h"
#include "nyaa/parser.h"
#include "nyaa/ast.h"
#include "nyaa/code-gen.h"
#include "nyaa/object-factory.h"
#include "nyaa/thread.h"
#include "base/arenas.h"
#include "mai-lang/isolate.h"
#include "mai/env.h"

namespace mai {
    
namespace nyaa {
    
////////////////////////////////////////////////////////////////////////////////////////////////////
/// class NyFunction:
////////////////////////////////////////////////////////////////////////////////////////////////////

NyFunction::NyFunction(NyString *name, uint8_t n_params, bool vargs, uint32_t n_upvals,
                       uint32_t max_stack, NyString *file_name, NyInt32Array *file_info,
                       NyObject *exec, NyArray *proto_pool, NyArray *const_pool, NyaaCore *N)
    : name_(name)
    , n_params_(n_params)
    , vargs_(vargs)
    , n_upvals_(n_upvals)
    , max_stack_(max_stack)
    , file_name_(file_name)
    , file_info_(file_info)
    , exec_(exec)
    , proto_pool_(proto_pool)
    , const_pool_(const_pool) {
    DCHECK(exec->IsByteArray() || exec->IsCode());
    N->BarrierWr(this, &name_, name);
    N->BarrierWr(this, &file_name_, file_name);
    N->BarrierWr(this, &file_info_, file_info);
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

void NyFunction::Iterate(ObjectVisitor *visitor) {
    visitor->VisitPointer(this, reinterpret_cast<Object **>(&name_));
    visitor->VisitPointer(this, reinterpret_cast<Object **>(&exec_));
    visitor->VisitPointer(this, reinterpret_cast<Object **>(&file_name_));
    visitor->VisitPointer(this, reinterpret_cast<Object **>(&file_info_));
    visitor->VisitPointer(this, reinterpret_cast<Object **>(&proto_pool_));
    visitor->VisitPointer(this, reinterpret_cast<Object **>(&const_pool_));
    for (uint32_t i = 0; i < n_upvals_; ++i) {
        visitor->VisitPointer(this, reinterpret_cast<Object **>(&upvals_[i].name));
    }
}

/*static*/ Handle<NyFunction> NyFunction::Compile(const char *z, size_t n, NyaaCore *N) {
    base::StandaloneArena arena(N->isolate()->env()->GetLowLevelAllocator());
    Parser::Result result = Parser::Parse(z, n, &arena);
    if (result.error) {
        N->Raisef("[%d:%d] %s", result.error_line, result.error_column, result.error->data());
        return Handle<NyFunction>();
    }
    return CodeGen::Generate(N->factory()->NewString(":memory:"), result.block, &arena, N);
}

/*static*/ Handle<NyFunction> NyFunction::Compile(const char *file_name, FILE *fp, NyaaCore *N) {
    base::StandaloneArena arena(N->isolate()->env()->GetLowLevelAllocator());
    Parser::Result result = Parser::Parse(fp, &arena);
    if (result.error) {
        N->Raisef("%s [%d:%d] %s", file_name, result.error_line, result.error_column,
                  result.error->data());
        return Handle<NyFunction>();
    }
    return CodeGen::Generate(N->factory()->NewString(file_name), result.block, &arena, N);
}
    
////////////////////////////////////////////////////////////////////////////////////////////////////
/// class NyCode:
////////////////////////////////////////////////////////////////////////////////////////////////////
    
const int32_t NyCode::kOffsetKind = Template::OffsetOf(&NyCode::kind_);
const int32_t NyCode::kOffsetInstructionsBytesSize = Template::OffsetOf(&NyCode::instructions_bytes_size_);
const int32_t NyCode::kOffsetInstructions = Template::OffsetOf(&NyCode::instructions_);
    
NyCode::NyCode(Kind kind, const uint8_t *instructions, uint32_t instructions_bytes_size)
    : kind_(kind)
    , instructions_bytes_size_(instructions_bytes_size) {
    ::memcpy(instructions_, instructions, instructions_bytes_size);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
/// class NyClosure:
////////////////////////////////////////////////////////////////////////////////////////////////////

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
    return N->curr_thd()->TryRun(this, argv, argc, wanted);
}

/*static*/ Handle<NyClosure> NyClosure::Compile(const char *z, size_t n, NyaaCore *N) {
    try {
        Handle<NyFunction> script = NyFunction::Compile(z, n, N);
        if (script.is_valid()) {
            return N->factory()->NewClosure(*script);
        }
    } catch (CallFrame::ExceptionId e) {
        // ignore
    }
    return Handle<NyClosure>::Empty();
}

/*static*/ Handle<NyClosure> NyClosure::Compile(const char *file_name, FILE *fp, NyaaCore *N) {
    try {
        Handle<NyFunction> script = NyFunction::Compile(file_name, fp, N);
        if (script.is_valid()) {
            return N->factory()->NewClosure(*script);
        }
    } catch (CallFrame::ExceptionId e) {
        // ignore
    }
    return Handle<NyClosure>::Empty();
}

/*static*/ int NyClosure::Do(const char *z, size_t n, int wanted, NyMap *env, NyaaCore *N) {
    Handle<NyClosure> script = Compile(z, n, N);
    if (script.is_not_valid()) {
        return -1;
    }
    return N->curr_thd()->TryRun(*script, nullptr/*argv*/, 0/*argc*/, wanted, env);
}

/*static*/ int NyClosure::Do(const char *file_name, FILE *fp, int wanted, NyMap *env, NyaaCore *N) {
    Handle<NyClosure> script = Compile(file_name, fp, N);
    if (script.is_not_valid()) {
        return -1;
    }
    return N->curr_thd()->TryRun(*script, nullptr/*argv*/, 0/*argc*/, wanted, env);
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
