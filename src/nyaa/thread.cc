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
    
    if (this == owns_->main_thd()) {
        // TODO:
    } else {
        // TODO:
    }
    return Error::OK();
}
    
void NyThread::Push(Object *value, int n) {
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
    
int NyThread::Run(NyFunction *fn, Arguments *args, NyMap *env) {
    if (!env) {
        env = owns_->g();
    }
    
    InitStack(fn, stack_tp_, 0, env);
    return Run(fn->script()->bcbuf());
}
    
int NyThread::Resume(Arguments *args, NyThread *prev) {
    DCHECK_EQ(kSuspended, state_);
    prev_ = prev;
    
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
                int32_t imm = ParseParam(bcbuf, 1, scale, &ok);
                if (!ok) {
                    return -1;
                }
                Push(NyInt32::New(imm));
                (*pc_ptr()) += 1 + scale;
            } break;
                
            case Bytecode::kPushConst: {
                int32_t k = ParseParam(bcbuf, 1, scale, &ok);
                Push(ConstPool()->Get(k));
                (*pc_ptr()) += 1 + scale;
            } break;
                
            case Bytecode::kPushGlobal: {
                int32_t k = ParseParam(bcbuf, 1, scale, &ok);
                DCHECK_GE(k, 0);
                NyString *name = static_cast<NyString *>(ConstPool()->Get(k));
                DCHECK(name->IsString());
                Push(Env()->RawGet(name, owns_));
                
                (*pc_ptr()) += 1 + scale;
            } break;
                
            case Bytecode::kPushNil: {
                int32_t n = ParseParam(bcbuf, 1, scale, &ok);
                if (!ok) {
                    return -1;
                }
                Push(Object::kNil, n);
                (*pc_ptr()) += 1 + scale;
            } break;
                
            case Bytecode::kAdd: {
                uint32_t offset = 1;
                int32_t lhs = ParseParam(bcbuf, offset, scale, &ok);
                if (!ok) {
                    return -1;
                }
                offset += scale;
                int32_t rhs = ParseParam(bcbuf, offset, scale, &ok);
                if (!ok) {
                    return -1;
                }
                offset += scale;
                
                Push(NySmi::New(Get(lhs)->ToSmi() + Get(rhs)->ToSmi()));
                (*pc_ptr()) += offset;
            } break;
                
            case Bytecode::kReturn: {
                int32_t nret = ParseParam(bcbuf, 1, scale, &ok);
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
                int32_t local = ParseParam(bcbuf, offset, scale, &ok);
                if (!ok) {
                    return -1;
                }
                offset += scale;
                int32_t n_args = ParseParam(bcbuf, offset, scale, &ok);
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
                
                (*pc_ptr()) += offset; // NOTICE: farward pc first!
                
                if (init->IsDelegated()) {
                    CallDelegated(init->ToDelegated(), base, n_args, 1);
                } else if (init->IsScript()) {
                    NyScript *script = init->ToScript();
                    NyMap *env = Env();
                    InitStack(script, base, 0, env);
                    bcbuf = script->bcbuf();
                } else if (init->IsFunction()) {
                    NyFunction *callee = init->ToFunction();
                    CallFunction(callee, base, n_args, 1);
                    bcbuf = callee->script()->bcbuf();
                }
            } break;
                
                // foo(bar())
                // push g['foo'] // 0
                // push g['bar'] // 1
                // call l[1], 0, -1
                // call l[0], -1, 0
            case Bytecode::kCall: {
                uint32_t offset = 1;
                int32_t local = ParseParam(bcbuf, offset, scale, &ok);
                if (!ok) {
                    return -1;
                }
                offset += scale;
                int32_t n_args = ParseParam(bcbuf, offset, scale, &ok);
                if (!ok) {
                    return -1;
                }
                offset += scale;
                int32_t n_accepts = ParseParam(bcbuf, offset, scale, &ok);
                if (!ok) {
                    return -1;
                }
                offset += scale;

                //printf("%d\n", stack_tp_ - stack_bp());
                Object **base = n_args < 0 ? stack_bp() + local : stack_tp_ - 1 - n_args;
                //DCHECK_GE(base, stack_bp());
                DCHECK_EQ(base, stack_bp() + local);
                Object *opd = Get(local); // callee
                if (opd->IsObject() && static_cast<NyObject *>(opd)->IsRunnable()) {
                    (*pc_ptr()) += offset; // NOTICE: farward pc first!
                    
                    if (n_args < 0) {
                        n_args = static_cast<int32_t>(stack_tp_ - base) - 1;
                    }
                    if (static_cast<NyObject *>(opd)->IsDelegated()) {
                        int nret = CallDelegated(static_cast<NyDelegated *>(opd), base, n_args,
                                                 n_accepts);
                        if (state_ == kSuspended) {
                            return nret;
                        }
                    } else if (static_cast<NyObject *>(opd)->IsScript()) {
                        NyScript *script = static_cast<NyScript *>(opd);
                        NyMap *env = Env();
                        InitStack(script, base, 0, env);
                        bcbuf = script->bcbuf();
                    } else if (static_cast<NyObject *>(opd)->IsFunction()) {
                        NyFunction *callee = static_cast<NyFunction *>(opd);
                        CallFunction(callee, base, n_args, n_accepts);
                        bcbuf = callee->script()->bcbuf();
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
    
int NyThread::CallFunction(NyFunction *fn, Object **base, int32_t n_args,
                           int32_t n_accepts) {
    //HandleScope scope(owns_->isolate());

    Object **argv = stack_tp_ - n_args;
    NyMap *env = Env();
    InitStack(fn, base, n_accepts, env);
    if (fn->vargs()) {
        if (n_args < fn->n_params()) {
            for (int i = 0; i < n_args; ++i) {
                Push(argv[i]);
            }
            Push(Object::kNil, fn->n_params() - n_args);
        } else { // n_args >= n_params
            for (int i = 0; i < fn->n_params(); ++i) {
                Push(argv[i]);
            }
            Handle<NyMap> vargs(owns_->factory()->NewMap(16, rand(), true/*linear*/));
            for (int i = fn->n_params(); i < n_args; ++i) {
                vargs->RawPut(NyInt32::New(i), argv[i], owns_);
            }
            Push(*vargs);
        }
    } else {
        // def foo(a, b, c)
        // foo(1, 2)
        // foo(1, 2, 3, 4)
        if (n_args < fn->n_params()) {
            for (int i = 0; i < n_args; ++i) {
                Push(argv[i]);
            }
            Push(Object::kNil, fn->n_params() - n_args);
        } else { // n_args >= n_params
            for (int i = 0; i < fn->n_params(); ++i) {
                Push(argv[i]);
            }
        }
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
    
int32_t NyThread::ParseParam(const NyByteArray *bcbuf, uint32_t offset, int scale, bool *ok) {
    int32_t param = 0;
    if (scale == 1) {
        int8_t val = static_cast<int8_t>(bcbuf->Get(pc() + offset));
        param = static_cast<int32_t>(val);
    } else if (scale == 2) {
        // TODO:
    } else if (scale == 4) {
        // TODO:
    } else {
        owns_->Raisef("Bad scale: %d", scale);
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
