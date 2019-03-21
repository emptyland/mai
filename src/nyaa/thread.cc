#include "nyaa/thread.h"
#include "nyaa/nyaa-core.h"
#include "nyaa/bytecode.h"
#include "nyaa/nyaa-values.h"
#include "nyaa/object-factory.h"
#include "mai-lang/call-info.h"
#include "mai-lang/isolate.h"
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
    stack_size_ = owns_->isolate()->init_thread_stack_size();
    stack_ = new Object *[stack_size_];
    stack_last_ = stack_ + stack_size_;
    stack_fp_ = stack_;
    stack_tp_ = stack_fp_;
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
    
int NyThread::Run(NyScript *entry, NyMap *env) {
    entry_ = DCHECK_NOTNULL(entry);
    if (!env) {
        env = owns_->g();
    }

    InitStack(entry, 0, env);
    return Run(entry->bcbuf());
}
    
int NyThread::Run(NyFunction *fn, Arguments *args, NyMap *env) {
    entry_ = DCHECK_NOTNULL(fn);
    if (!env) {
        env = owns_->g();
    }
    
    InitStack(fn, 0, env);
    return Run(fn->script()->bcbuf());
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
    
    while (pc() < bcbuf->size()) {
        if (has_raised_) {
            return -1;
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
                DCHECK(name->IsString(owns_));
                Push(Env()->Get(name, owns_));
                
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
                
            case Bytecode::kReturn: {
                int32_t rvn = ParseParam(bcbuf, 1, scale, &ok);
                if (!ok) {
                    return -1;
                }
                (*pc_ptr()) += 1 + scale;
                if (Current() == entry_) {
                    return rvn;
                }
                // TODO: process n_accepts.
                
                size_t restore_rbp = rbp(), restore_rbs = rbs();
                stack_fp_ = stack_ + restore_rbp;
                stack_tp_ = stack_fp_ + restore_rbs;
            } break;
                
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
                
                ptrdiff_t base = stack_tp_ - stack_bp() - 1 - n_args;
                DCHECK_GE(base, 0);
                Object *opd = Get(local); // callee
                if (opd->is_object() && static_cast<NyObject *>(opd)->IsRunnable(owns_)) {
                    if (static_cast<NyObject *>(opd)->IsDelegated(owns_)) {
                        CallDelegated(static_cast<NyDelegated *>(opd), base, n_args, n_accepts);
                    } else if (static_cast<NyObject *>(opd)->IsScript(owns_)) {
                        NyScript *script = static_cast<NyScript *>(opd);
                        NyMap *env = Env();
                        InitStack(script, 0, env);
                    } else if (static_cast<NyObject *>(opd)->IsFunction(owns_)) {
                        CallFunction(static_cast<NyFunction *>(opd), base, n_args, n_accepts);
                    }
                }
                
                (*pc_ptr()) += offset;
            } break;
                
            // TODO:
                
            default:
                owns_->Raisef("Bad bytecode: %d", id);
                break;
        }
    }
    return 0;
}
    
/**
 * bp                                  tp
 *  +--------+--------+--------+       +
 *  | callee | arg[0] | arg[1] | ..... |
 *  +--------+--------+--------+       +
 */
int NyThread::CallDelegated(NyDelegated *fn, ptrdiff_t stack_base, int32_t n_args,
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
            stack_bp()[stack_base + i] = ret[i];
        }
        for (int i = nret; i < n_accepts; ++i) {
            stack_bp()[stack_base + i] = Object::kNil;
        }
    } else { // >=
        for (int i = 0; i < n_accepts; ++i) {
            stack_bp()[stack_base + i] = ret[i];
        }
    }
    stack_tp_ = stack_bp() + stack_base + n_accepts;
    return nret;
}
    
int NyThread::CallFunction(NyFunction *fn, ptrdiff_t stack_base, int32_t n_args,
                           int32_t n_accepts) {
    HandleScope scope(owns_->isolate());

    Object **argv = stack_tp_ - n_args;
    NyMap *env = Env();
    InitStack(fn, n_accepts, env);
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
                vargs->Put(NyInt32::New(i), argv[i], owns_);
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
    
void NyThread::InitStack(NyRunnable *callee, int n_accepts, NyMap *env) {
    Push(callee);           // Callee
    Push(env);              // Env
    Push(NyInt32::New(n_accepts)); // Accepts
    Push(NewPC(0));         // PC
    Push(NyInt64::New(stack_tp_ - stack_));  // RBP
    Push(NyInt64::New(stack_tp_ - stack_fp_));  // RBS
}
    
int32_t NyThread::ParseParam(const NyByteArray *bcbuf, uint32_t offset, int scale, bool *ok) {
    int32_t param = 0;
    if (scale == 1) {
        param = static_cast<int32_t>(bcbuf->Get(pc() + offset));
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
    NyObject *o = val->heap_object();
    DCHECK(o->IsRunnable(owns_));
    return static_cast<NyRunnable *>(o);
}
    
NyMap *NyThread::Env() const {
    Object *val = *(stack_fp_ + kEnvOffset);
    NyObject *o = val->heap_object();
    DCHECK(o->IsMap(owns_));
    return static_cast<NyMap *>(o);
}
    
const NyArray *NyThread::ConstPool() const {
    NyRunnable *o = Current();
    if (o->IsScript(owns_)) {
        return static_cast<NyScript *>(o)->const_pool();
    } else if (o->IsFunction(owns_)) {
        return static_cast<NyFunction *>(o)->script()->const_pool();
    } else {
        DLOG(FATAL) << "Noreached!";
    }
    return nullptr;
}
    
} // namespace nyaa
    
} // namespace mai
