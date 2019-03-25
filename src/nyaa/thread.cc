#include "nyaa/thread.h"
#include "nyaa/nyaa-core.h"
#include "nyaa/bytecode.h"
#include "nyaa/nyaa-values.h"
#include "nyaa/object-factory.h"
#include "mai-lang/nyaa.h"
#include "mai/env.h"

namespace mai {
    
namespace nyaa {

NyThread::NyThread(NyaaCore *owns)
    : NyUDO(&UDOFinalizeDtor<NyThread>)
    , owns_(DCHECK_NOTNULL(owns)) {
}

NyThread::~NyThread() {
    delete[] stack_;
}
    
Error NyThread::Init() {
    stack_size_ = owns_->stub()->init_thread_stack_size();
    stack_ = new Object *[stack_size_];
    stack_last_ = stack_ + stack_size_;
    stack_fp_ = stack_;
    stack_tp_ = stack_fp_;
    
//    if (this == owns_->main_thd()) {
//        // TODO:
//    } else {
//        // TODO:
//    }
    return Error::OK();
}
    
void NyThread::Push(Object *value, size_t n) {
    if (stack_tp_ + n >= stack_last_) {
        // TODO:
        owns_->Raisef("Stack overflow!");
        return;
    }
    for (int i = 0; i < n; ++i) {
        stack_tp_[i] = value;
    }
    stack_tp_ += n;
}
    
Object *NyThread::Get(int i) {
    if (i < 0) {
        DCHECK_GE(stack_tp_ + i, stack_bp());
        return *(stack_tp_ + i);
    } else {
        DCHECK_LT(stack_bp() + i, stack_tp_);
        return stack_bp()[i];
    }
}
    
void NyThread::Set(int i, Object *value) {
    if (i < 0) {
        DCHECK_GE(stack_tp_ + i, stack_bp());
        *(stack_tp_ + i) = value;
    } else {
        DCHECK_LT(stack_bp() + i, stack_tp_);
        stack_bp()[i] = value;
    }
}
    
int NyThread::Run(NyScript *entry, NyMap *env) {
    if (!env) {
        env = owns_->g();
    }

    InitStack(entry, stack_tp_, 0, env);
    return Run(entry->bcbuf());
}
    
int NyThread::Run(NyFunction *fn, Arguments *args, int n_accepts, NyMap *env) {
    if (!env) {
        env = owns_->g();
    }
    
    InitStack(fn, stack_tp_, n_accepts, env);
    InitArgs(args, fn->n_params(), fn->vargs());
    return Run(fn->script()->bcbuf());
}
    
void NyThread::InitArgs(Arguments *args, int n_params, bool vargs) {
    if (vargs) {
        NyMap *params = owns_->factory()->NewMap(8, rand(), true /*linear*/);
        if (args->Length() < n_params) {
            for (size_t i = 0; i < args->Length(); ++i) {
                Push(*ApiWarpNoCheck<Object>(args->Get(i), owns_));
            }
            Push(Object::kNil, n_params - args->Length());
        } else { // >=
            for (size_t i = 0; i < n_params; ++i) {
                Push(*ApiWarpNoCheck<Object>(args->Get(i), owns_));
            }
            for (size_t i = n_params; i < args->Length(); ++i) {
                params->RawPut(NySmi::New(i), *ApiWarpNoCheck<Object>(args->Get(i), owns_), owns_);
            }
        }
        Push(params);
    } else {
        if (args->Length() < n_params) {
            for (size_t i = 0; i < args->Length(); ++i) {
                Push(*ApiWarpNoCheck<Object>(args->Get(i), owns_));
            }
            Push(Object::kNil, n_params - args->Length());
        } else { // >=
            for (size_t i = 0; i < n_params; ++i) {
                Push(*ApiWarpNoCheck<Object>(args->Get(i), owns_));
            }
        }
    }
}
    
int NyThread::Resume(Arguments *args, NyThread *save, NyMap *env) {
    DCHECK_EQ(kSuspended, state_);
    DCHECK_NE(this, save);
    save_ = DCHECK_NOTNULL(save);

    if (stack_tp_ == stack_) { // has not run yet.
        if (NyFunction *callee = DCHECK_NOTNULL(entry_)->ToFunction()) {
            return Run(callee, args, 0, env);
        }
        if (NyScript *callee = DCHECK_NOTNULL(entry_)->ToScript()) {
            return Run(callee, env);
        }
        if (NyDelegated *callee = entry_->ToDelegated()) {
            HandleScope scope(owns_->isolate());
            return callee->Call(args, owns_);
        }
        DLOG(FATAL) << "Noreached!";
    }
    
    const NyByteArray *bcbuf = CurrentBC();
    return Run(bcbuf);
}

/** fp                                           bp        tp
 *  +--------+--------+--------+--------+--------+---------+
 *  | callee | bcbuf  |   pc   |  rbp   |  rbs   | args...
 *  +--------+--------+--------+--------+--------+---------+
 */
int NyThread::Run(const NyByteArray *bcbuf) {
    if (has_raised_) {
        return -1;
    }
    
    state_ = kRunning;
    while (pc() < bcbuf->size()) {
        if (has_raised_) {
            break;
        }

        Bytecode::ID id = static_cast<Bytecode::ID>(bcbuf->Get(pc()));
        int scale = 0;
        switch (id) {
            case Bytecode::kDouble:
                scale = 2;
                (*pc_ptr()) ++;
                break;
                
            case Bytecode::kQuadruple:
                scale = 4;
                (*pc_ptr()) ++;
                break;
                
            default:
                scale = 1;
                break;
        }
        
        bool ok = true;
        id = static_cast<Bytecode::ID>(bcbuf->Get(pc()));
        // var a, b, c = foo(), 1
        // LoadGlobal 'foo'
        // Call 0, 1
        // PushImm 1
        // PushNil
        //
        // var a, b, c = foo()
        // LoadGlobal 'foo'
        // Call 0, 3
        //
        // var a, b, c = 1, 2, 3
        // PushImm 1
        // PushImm 2
        // PushImm 3
        switch (id) {
            case Bytecode::kPushImm: {
                int32_t imm = ParseInt32(bcbuf, 1, scale, &ok);
                if (!ok) {
                    return -1;
                }
                Push(NyInt32::New(imm));
                (*pc_ptr()) += 1 + scale;
            } break;
                
            case Bytecode::kPushConst: {
                int32_t k = ParseInt32(bcbuf, 1, scale, &ok);
                Push(ConstPool()->Get(k));
                (*pc_ptr()) += 1 + scale;
            } break;
                
            case Bytecode::kPushGlobal: {
                int32_t k = ParseInt32(bcbuf, 1, scale, &ok);
                DCHECK_GE(k, 0);
                NyString *name = static_cast<NyString *>(ConstPool()->Get(k));
                DCHECK(name->IsString());
                Push(Env()->RawGet(name, owns_));
                
                (*pc_ptr()) += 1 + scale;
            } break;
                
            case Bytecode::kPushNil: {
                int32_t n = ParseInt32(bcbuf, 1, scale, &ok);
                if (!ok) {
                    return -1;
                }
                Push(Object::kNil, n);
                (*pc_ptr()) += 1 + scale;
            } break;
                
            case Bytecode::kIndexConst:
            case Bytecode::kIndex: {
                
                Object *opd = nullptr, *key = nullptr;
                uint32_t offset = 1;
                if (id == Bytecode::kIndexConst) {
                    opd = Get(-1);
                    int32_t local = ParseInt32(bcbuf, offset, scale, &ok);
                    if (!ok) {
                        return -1;
                    }
                    offset += scale;
                    key = ConstPool()->Get(local);
                } else {
                    opd = Get(-2);
                    key = Get(-1);
                }
                
                if (!opd->IsObject()) {
                    owns_->Raisef("value has not field.");
                    return -1;
                }
                
                NyObject *ob = opd->ToHeapObject();
                if (NyMap *val = ob->ToMap()) {
                    if (val->GetMetatable() == owns_->kmt_pool()->kMap) {
                        Set(-1, val->RawGet(key, owns_));
                        break;
                    }
                    auto *index = val->GetMetatable()->RawGet(owns_->bkz_pool()->kInnerIndex,
                                                              owns_);
                    if (index == nullptr) {
                        Set(-1, val->RawGet(key, owns_));
                        break;
                    }
                }
                HandleScope scope(owns_->isolate());
                Arguments args(2);
                args.Set(0, Local<Value>::New(opd));
                args.Set(1, Local<Value>::New(key));
                Pop(id == Bytecode::kIndexConst ? 1: 2); // pop key
                        // pop opd

                int nret = CallMetaFunction(ob, owns_->bkz_pool()->kInnerIndex, &args,
                                            1,/*n_accepts*/ offset,/*offset*/ &bcbuf);
                if (owns_->curr_thd() != this) {
                    return nret;
                }
            } break;
                
            case Bytecode::kPop: {
                int32_t n = ParseInt32(bcbuf, 1, scale, &ok);
                if (!ok) {
                    return -1;
                }
                Pop(n);
                (*pc_ptr()) += 1 + scale;
            } break;
                
            case Bytecode::kAdd: {
                uint32_t offset = 1;
                int32_t lhs = ParseInt32(bcbuf, offset, scale, &ok);
                if (!ok) {
                    return -1;
                }
                offset += scale;
                int32_t rhs = ParseInt32(bcbuf, offset, scale, &ok);
                if (!ok) {
                    return -1;
                }
                offset += scale;
                
                Push(Object::Add(Get(lhs), Get(rhs), owns_));
                (*pc_ptr()) += offset;
            } break;
                
            case Bytecode::kReturn: {
                int32_t nret = ParseInt32(bcbuf, 1, scale, &ok);
                if (!ok) {
                    return -1;
                }
                (*pc_ptr()) += 1 + scale;

                Object **ret = stack_tp_ - nret;
                int n = n_accepts();
                size_t restore_rbp = rbp(), restore_rbs = rbs();

                stack_fp_ = stack_ + restore_rbp;
                stack_tp_ = stack_fp_ + restore_rbs;
                
                if (nret < n) {
                    for (int i = 0; i < nret; ++i) {
                        stack_tp_[i] = ret[i];
                    }
                    for (int i = nret; i < n; ++i) {
                        stack_tp_[i] = Object::kNil;
                    }
                } else { // >=
                    if (n < 0) {
                        n = nret;
                    }
                    for (int i = 0; i < n; ++i) {
                        stack_tp_[i] = ret[i];
                    }
                }
                stack_tp_ += n;
                DCHECK_LT(stack_tp_, stack_last_);
                
                bcbuf = CurrentBC();
            } break;
                
            case Bytecode::kNew: {
                uint32_t offset = 1;
                int32_t local = ParseInt32(bcbuf, offset, scale, &ok);
                if (!ok) {
                    return -1;
                }
                offset += scale;
                int32_t n_args = ParseInt32(bcbuf, offset, scale, &ok);
                if (!ok) {
                    return -1;
                }
                offset += scale;
                
                Object *opd = Get(local);
                if (opd->IsNil() || opd->IsSmi() || !opd->ToHeapObject()->IsMap()) {
                    owns_->Raisef("new non-class.");
                    return -1;
                }
                
                HandleScope scope(owns_->isolate());
                Handle<NyMap> clazz(opd->ToHeapObject()->ToMap());
                Handle<Object> ob_size(clazz->RawGet(owns_->bkz_pool()->kInnerSize, owns_));
                if (ob_size->IsNil() || !ob_size->IsSmi() || ob_size->ToSmi() < sizeof(NyUDO)) {
                    owns_->Raisef("incorrect object size.");
                    return -1;
                }
                Handle<NyUDO> thd(owns_->factory()->NewUninitializedUDO(ob_size->ToSmi(), *clazz));
                Handle<NyObject> init(clazz->RawGet(owns_->bkz_pool()->kInnerInit, owns_));
                
                Object **base = stack_tp_ - 1 - n_args;
                DCHECK_GE(base, stack_bp());
                if (init.is_null()) {
                    base[0] = *thd;
                    stack_tp_ = base + 1;
                    break;
                }
                
                if (init->IsSmi() || !init->ToHeapObject()->IsRunnable()) {
                    owns_->Raisef("incorrect __init__ function");
                    return -1;
                }
                base[0] = *init;
                base[1] = *thd;
                
                int nret = CallInternal(static_cast<NyRunnable *>(*init), base, n_args, 1, offset,
                                        &bcbuf);
                if (owns_->curr_thd() != this) {
                    return nret;
                }
            } break;
                
                // foo(bar())
                // push g['foo'] // 0
                // push g['bar'] // 1
                // call l[1], 0, -1
                // call l[0], -1, 0
            case Bytecode::kCall: {
                uint32_t offset = 1;
                int32_t local = ParseInt32(bcbuf, offset, scale, &ok);
                if (!ok) {
                    return -1;
                }
                offset += scale;
                int32_t n_args = ParseInt32(bcbuf, offset, scale, &ok);
                if (!ok) {
                    return -1;
                }
                offset += scale;
                int32_t n_accepts = ParseInt32(bcbuf, offset, scale, &ok);
                if (!ok) {
                    return -1;
                }
                offset += scale;

                Object **base = n_args < 0 ? stack_bp() + local : stack_tp_ - 1 - n_args;
                DCHECK_EQ(base, stack_bp() + local);
                
                NyObject *opd = static_cast<NyObject *>(Get(local)); // callee
                if (opd->IsNil()) {
                    owns_->Raisef("attempt to call nil value.");
                    return -1;
                }
                if (opd->IsSmi()) {
                    owns_->Raisef("attempt to call incorrect value.");
                    return -1;
                }
                
                if (n_args < 0) {
                    n_args = static_cast<int32_t>(stack_tp_ - base) - 1;
                }
                if (opd->IsRunnable()) {
                    int nret = CallInternal(static_cast<NyRunnable *>(opd), base, n_args, n_accepts,
                                            offset, &bcbuf);
                    if (nret < 0 || owns_->curr_thd() != this) {
                        return nret;
                    }
                } else {
                    HandleScope scope(owns_->isolate());
                    Arguments args(n_args);
                    for (int i = 0; i < n_args; ++i) {
                        args.Set(i, Local<Value>::New(base + 1 + i));
                    }
                    Pop(n_args + 1);
                    int nret = CallMetaFunction(opd, owns_->bkz_pool()->kInnerCall, &args,
                                                n_accepts, offset, &bcbuf);
                    if (nret < 0 || owns_->curr_thd() != this) {
                        return nret;
                    }
                }
            } break;
                
            // TODO:
                
            default:
                owns_->Raisef("Bad bytecode: %d", id);
                break;
        }
    }
    if (Current() == entry_) {
        state_ = kDead;
    } else {
        state_ = kSuspended;
    }
    return has_raised_ ? -1 : 0;
}
    
/**
 * bp                                  tp
 *  +--------+--------+--------+       +
 *  | callee | arg[0] | arg[1] | ..... |
 *  +--------+--------+--------+       +
 */
int NyThread::CallDelegated(NyDelegated *fn, Object **base, int32_t n_args,
                            int32_t n_accepts) {
    HandleScope scope(owns_->isolate());

    Arguments args(reinterpret_cast<Value **>(stack_tp_ - n_args), n_args);
    args.SetCallee(Local<Value>::New(fn));
    int nret = fn->Call(&args, owns_);
    if (nret < 0 || has_raised_) {
        return -1;
    }
    Object **ret = stack_tp_ - nret;
    if (nret < n_accepts) {
        for (int i = 0; i < nret; ++i) {
            base[i] = ret[i];
        }
        for (int i = nret; i < n_accepts; ++i) {
            base[i] = Object::kNil;
        }
    } else { // >=
        for (int i = 0; i < n_accepts; ++i) {
            base[i] = ret[i];
        }
    }
    stack_tp_ = base + n_accepts;
    DCHECK_LT(stack_tp_, stack_last_);
    return nret;
}
    
int NyThread::CallMetaFunction(NyObject *ob, NyString *name, Arguments *args, int n_accepts,
                               uint32_t offset, NyByteArray const **bcbuf) {
    NyMap *mt = DCHECK_NOTNULL(ob)->GetMetatable();
    Object *mf = mt->RawGet(name, owns_);
    if (mf == Object::kNil) {
        owns_->Raisef("attempt to call nil `%s' meta function.", name->bytes());
        return -1;
    }
    if (mf->IsSmi()) {
        owns_->Raisef("attempt to call incorrect `%s' meta function.", name->bytes());
        return -1;
    }
    NyRunnable *ro = static_cast<NyRunnable *>(mf);
    if (!ro->IsRunnable()) {
        owns_->Raisef("attempt to call incorrect `%s' meta function.", name->bytes());
        return -1;
    }
    
    Object **base = stack_tp_;
    Push(ro);
    for (size_t i = 0; i < args->Length(); ++i) {
        Push(*ApiWarpNoCheck<Object>(args->Get(i), owns_));
    }
    
    return CallInternal(ro, base, static_cast<int>(args->Length()), n_accepts, offset, bcbuf);
}
    
int NyThread::CallInternal(NyRunnable *ro, Object **base, int32_t n_args, int n_accepts,
                           uint32_t offset, NyByteArray const **bcbuf) {
    NyMap *env = Env();

    
    if (NyDelegated *callee = ro->ToDelegated()) {
        (*pc_ptr()) += offset; // NOTICE: add pc first!

        int nret = CallDelegated(callee, base, n_args, n_accepts);
        if (owns_->curr_thd() != this) {
            return nret;
        }
    } else if (NyScript *callee = ro->ToScript()) {
        (*pc_ptr()) += offset; // NOTICE: add pc first!
        
        InitStack(callee, base, n_accepts, env);
        *bcbuf = callee->bcbuf();
    } else if (NyFunction *callee = ro->ToFunction()) {
        (*pc_ptr()) += offset; // NOTICE: add pc first!
        
        Arguments args(reinterpret_cast<Value **>(stack_tp_ - n_args), n_args);
        InitStack(callee, base, n_accepts, env);
        InitArgs(&args, callee->n_params(), callee->vargs());
        *bcbuf = callee->script()->bcbuf();
    }
    return 0;
}
    
void NyThread::InitStack(NyRunnable *callee, Object **bp, int n_accepts, NyMap *env) {
    DCHECK_GE(stack_fp_, stack_);
    DCHECK_GE(bp, stack_fp_);
    ptrdiff_t rbp = stack_fp_ - stack_, rbs = bp - stack_fp_;
    
    stack_fp_ = stack_tp_;
    Push(callee);           // Callee
    Push(env);              // Env
    Push(NyInt32::New(n_accepts)); // Accepts
    Push(NewPC(0));         // PC
    Push(NySmi::New(rbp));  // RBP
    Push(NySmi::New(rbs));  // RBS
}
    
int32_t NyThread::ParseInt32(const NyByteArray *bcbuf, uint32_t offset, int scale, bool *ok) {
    int32_t param = 0;
    if (scale == 1) {
        int8_t val = static_cast<int8_t>(bcbuf->Get(pc() + offset));
        param = static_cast<int32_t>(val);
    } else if (scale == 2) {
        uint8_t lo = bcbuf->Get(pc() + offset + 0);
        uint8_t hi = bcbuf->Get(pc() + offset + 1);
        param = static_cast<int32_t>(static_cast<uint16_t>(hi) << 8 | lo);
    } else if (scale == 4) {
        uint8_t lo0 = bcbuf->Get(pc() + offset + 0);
        uint8_t lo1 = bcbuf->Get(pc() + offset + 1);
        uint8_t hi0 = bcbuf->Get(pc() + offset + 2);
        uint8_t hi1 = bcbuf->Get(pc() + offset + 3);
        param = static_cast<int32_t>(static_cast<uint32_t>(hi1) << 24 |
                                     static_cast<uint32_t>(hi0) << 16 |
                                     static_cast<uint32_t>(lo1) << 8 | lo0);
    } else {
        owns_->Raisef("unexpected scale: %d, requried(1, 2, 4)", scale);
        *ok = false;
    }
    return param;
}
    
NyRunnable *NyThread::Current() const {
    Object *val = *(stack_fp_ + kCalleeOffset);
    NyObject *o = val->ToHeapObject();
    DCHECK(o->IsRunnable());
    return static_cast<NyRunnable *>(o);
}
    
const NyByteArray *NyThread::CurrentBC() const {
    NyRunnable *val = Current();
    if (!val) {
        return nullptr;
    }
    if (val->IsScript()) {
        return static_cast<NyScript *>(val)->bcbuf();
    } else if (val->IsFunction()) {
        return static_cast<NyFunction *>(val)->script()->bcbuf();
    } else {
        return nullptr;
    }
}
    
NyMap *NyThread::Env() const {
    Object *val = *(stack_fp_ + kEnvOffset);
    NyObject *o = val->ToHeapObject();
    DCHECK(o->IsMap());
    return static_cast<NyMap *>(o);
}
    
const NyArray *NyThread::ConstPool() const {
    NyRunnable *o = Current();
    if (o->IsScript()) {
        return static_cast<NyScript *>(o)->const_pool();
    } else if (o->IsFunction()) {
        return static_cast<NyFunction *>(o)->script()->const_pool();
    } else {
        DLOG(FATAL) << "Noreached!";
    }
    return nullptr;
}
    
} // namespace nyaa
    
} // namespace mai
