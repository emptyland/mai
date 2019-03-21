#include "nyaa/thread.h"
#include "nyaa/nyaa-core.h"
#include "nyaa/bytecode.h"
#include "mai-lang/call-info.h"
#include "mai-lang/isolate.h"
#include "mai/env.h"

namespace mai {
    
namespace nyaa {

NyThread::NyThread(NyaaCore *owns)
    : NyUDO(&UDOFinalizeDtor<NyThread>)
    , owns_(DCHECK_NOTNULL(owns))
    , env_(owns->g()) {
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
        DCHECK_LT(i, stack_tp_ - stack_bp());
        return stack_bp()[i];
    }
}
    
int NyThread::Run(NyScript *entry, NyTable *env) {
    entry_ = DCHECK_NOTNULL(entry);
    current_ = entry;
    if (env) {
        env_ = env;
    } else {
        env_ = owns_->g();
    }

    //ra_ = Object::kNil;
    Push(entry);           // current
    Push(entry->bcbuf());  // BC
    Push(NewPC(0));        // PC
    Push(NyInt32::New(0)); // Prev BP
    return Run(entry->bcbuf());
}

/** fp                                 bp         tp
 *  +--------+--------+--------+--------+---------+
 *  | callee | bcbuf  |  pc    | bp     | args...
 *  +--------+--------+--------+--------+---------+
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
                
            } break;
                
            case Bytecode::kPushGlobal: {
                int32_t k = ParseParam(bcbuf, 1, scale, &ok);
                DCHECK_GE(k, 0);
                NyString *name = static_cast<NyString *>(ConstPool()->Get(k));
                DCHECK(name->IsString(owns_));
                Push(env_->Get(name, owns_));
                
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
                return rvn;
            } break;
                
            case Bytecode::kCall: {
                uint32_t offset = 1;
                int32_t local = ParseParam(bcbuf, offset, scale, &ok);
                if (!ok) {
                    return -1;
                }
                offset += scale;
                int32_t n_params = ParseParam(bcbuf, offset, scale, &ok);
                if (!ok) {
                    return -1;
                }
                offset += scale;
                int32_t n_accept = ParseParam(bcbuf, offset, scale, &ok);
                if (!ok) {
                    return -1;
                }
                offset += scale;
                
                ptrdiff_t saved_bp = stack_tp_ - stack_bp() - 1 - n_params;
                DCHECK_GE(saved_bp, 0);
                Object *opd = Get(local); // callee
                if (opd->is_object() && static_cast<NyObject *>(opd)->IsRunnable(owns_)) {
                    if (static_cast<NyObject *>(opd)->IsDelegated(owns_)) {
                        int rv = CallDelegated(static_cast<NyDelegated *>(opd), saved_bp, n_params,
                                               n_accept);
                        if (rv < 0) {
                            return -1;
                        }
                    } else if (static_cast<NyObject *>(opd)->IsScript(owns_)) {
                        // TODO:
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
int NyThread::CallDelegated(NyDelegated *fn, ptrdiff_t stack_base, int32_t n_params,
                            int32_t n_accept) {
    HandleScope scope(owns_->isolate());

    Arguments args(reinterpret_cast<Value **>(stack_tp_ - n_params), n_params);
    args.SetCallee(Local<Value>::New(fn));
    auto saved_fn = current_;
    current_ = fn;
    int nret = fn->Call(&args, owns_);
    current_ = saved_fn;
    if (nret < 0 || has_raised_) {
        return -1;
    }
    Object **ret = stack_tp_ - nret;
    if (nret < n_accept) {
        for (int i = 0; i < nret; ++i) {
            stack_bp()[stack_base + i] = ret[i];
        }
        for (int i = nret; i < n_accept; ++i) {
            stack_bp()[stack_base + i] = Object::kNil;
        }
    } else { // >=
        for (int i = 0; i < n_accept; ++i) {
            stack_bp()[stack_base + i] = ret[i];
        }
    }
    stack_tp_ = stack_bp() + stack_base + n_accept;
    return nret;
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
    
const NyArray *NyThread::ConstPool() const {
    NyRunnable *o = Current();
    if (o->IsScript(owns_)) {
        return static_cast<NyScript *>(o)->const_pool();
    } else {
        DLOG(FATAL) << "Noreached!";
    }
    return nullptr;
}
    
} // namespace nyaa
    
} // namespace mai
