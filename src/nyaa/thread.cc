#include "nyaa/thread.h"
#include "nyaa/nyaa-core.h"
#include "nyaa/bytecode.h"
#include "nyaa/nyaa-values.h"
#include "nyaa/object-factory.h"
#include "nyaa/visitors.h"
#include "mai-lang/nyaa.h"
#include "mai/env.h"

namespace mai {
    
namespace nyaa {
    
void CallFrame::Enter(NyThread *owns, NyRunnable *callee, NyByteArray *bcbuf, NyArray *kpool,
                      int wanted, Object **bp, Object **tp, NyMap *env) {
    DCHECK_NE(this, owns->frame_);
    
    level_ = (owns->frame_ ? owns->frame_->level_ + 1 : 0);
    prev_ = owns->frame_;
    owns->frame_ = this;
    
    callee_     = DCHECK_NOTNULL(callee);
    bcbuf_      = bcbuf;
    const_poll_ = kpool;
    env_        = DCHECK_NOTNULL(env);
    wanted_      = wanted;
    DCHECK(bp >= owns->stack_ && bp < owns->stack_last_);
    stack_bp_   = bp - owns->stack_;
    DCHECK(tp >= bp && tp < owns->stack_last_);
    stack_tp_   = tp - owns->stack_;
}

void CallFrame::Exit(NyThread *owns) {
    DCHECK_EQ(this, owns->frame_);
    owns->frame_ = prev_;
}

void CallFrame::IterateRoot(RootVisitor *visitor) {
    visitor->VisitRootPointer(reinterpret_cast<Object **>(&callee_));
    visitor->VisitRootPointer(reinterpret_cast<Object **>(&bcbuf_));
    visitor->VisitRootPointer(reinterpret_cast<Object **>(&const_poll_));
    visitor->VisitRootPointer(reinterpret_cast<Object **>(&env_));
}

    
NyThread::NyThread(NyaaCore *owns)
    : owns_(DCHECK_NOTNULL(owns)) {
}

NyThread::~NyThread() {
    delete[] stack_;
}
    
Error NyThread::Init() {
    stack_size_ = owns_->stub()->init_thread_stack_size();
    stack_ = new Object *[stack_size_];
    stack_last_ = stack_ + stack_size_;
    stack_tp_ = stack_;

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
        DCHECK_GE(frame_tp() + i, frame_bp());
        return *(frame_tp() + i);
    } else {
        DCHECK_LT(frame_bp() + i, frame_tp());
        return frame_bp()[i];
    }
}
    
void NyThread::Set(int i, Object *value) {
    if (i < 0) {
        DCHECK_GE(frame_tp() + i, frame_bp());
        *(frame_tp() + i) = value;
    } else {
        DCHECK_LT(frame_bp() + i, frame_tp());
        frame_bp()[i] = value;
    }
}
    
int NyThread::Run(NyScript *entry, int n_result, NyMap *env) {
    if (!env) {
        env = owns_->g();
    }

    CallFrame frame;
    frame.Enter(this, entry, entry->bcbuf(), entry->const_pool(),
                n_result, /* wanted */
                stack_tp_, /* frame_bp */
                stack_tp_ + entry->max_stack_size() /* frame_tp */,
                env);
    Push(entry);
    int rv = Run();
    frame.Exit(this);
    return rv;
}
    
int NyThread::Run(NyFunction *fn, Arguments *args, int n_result, NyMap *env) {
    if (!env) {
        env = owns_->g();
    }
    
    CallFrame frame;
    frame.Enter(this, fn, fn->script()->bcbuf(), fn->script()->const_pool(),
                n_result, /* wanted */
                stack_tp_, /* frame_bp */
                stack_tp_ + fn->script()->max_stack_size() /* frame_tp */,
                env);
    Push(fn);
    CopyArgs(args, static_cast<int>(args->Length()), fn->vargs());
    int rv = Run();
    frame.Exit(this);
    return rv;
}
    
void NyThread::CopyArgs(Arguments *args, int n_params, bool vargs) {
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
            return Run(callee, 0, env);
        }
        if (NyDelegated *callee = entry_->ToDelegated()) {
            HandleScope scope(owns_->isolate());
            return callee->Call(args, owns_);
        }
        DLOG(FATAL) << "Noreached!";
    }
    
    //const NyByteArray *bcbuf = CurrentBC();
    return Run();
}
    
void NyThread::IterateRoot(RootVisitor *visitor) {
    visitor->VisitRootPointer(reinterpret_cast<Object **>(&entry_));
    visitor->VisitRootPointer(reinterpret_cast<Object **>(&save_));
    visitor->VisitRootPointer(reinterpret_cast<Object **>(&next_));
    visitor->VisitRootPointer(reinterpret_cast<Object **>(&prev_));

    visitor->VisitRootPointers(reinterpret_cast<Object **>(stack_),
                               reinterpret_cast<Object **>(stack_tp_));
    
    CallFrame *f = frame_;
    while (f) {
        f->IterateRoot(visitor);
        f = f->prev_;
    }
}
    

/** fp                                           bp        tp
 *  +--------+--------+--------+--------+--------+---------+
 *  | callee | bcbuf  |   pc   |  rbp   |  rbs   | args...
 *  +--------+--------+--------+--------+--------+---------+
 */
int NyThread::Run() {
    if (has_raised_) {
        return -1;
    }
    
    state_ = kRunning;
    while (frame_->pc() < frame_->bcbuf()->size()) {
        if (has_raised_) {
            break;
        }

        Bytecode::ID id = static_cast<Bytecode::ID>(frame_->BC());
        int scale = 0;
        switch (id) {
            case Bytecode::kDouble:
                scale = 2;
                frame_->AddPC(1);
                break;
                
            case Bytecode::kQuadruple:
                scale = 4;
                frame_->AddPC(1);
                break;
                
            default:
                scale = 1;
                break;
        }
    
        id = static_cast<Bytecode::ID>(frame_->BC());
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
                
            case Bytecode::kLoadImm: {
                int32_t ra, imm;
                int delta = 1;
                if ((delta = ParseBytecodeInt32Params(delta, scale, 2, &ra, &imm)) < 0) {
                    return -1;
                }
                Set(ra, NyInt32::New(imm));
                frame_->AddPC(delta);
            } break;
                
            case Bytecode::kLoadGlobal: {
                int32_t ra, rb;
                int delta = 1;
                if ((delta = ParseBytecodeInt32Params(delta, scale, 2, &ra, &rb)) < 0) {
                    return -1;
                }
                Object *key = frame_->const_poll()->Get(rb);
                Set(ra, frame_->env()->RawGet(key, owns_));
                frame_->AddPC(delta);
            } break;

            case Bytecode::kRet: {
                int32_t ra, n;
                int delta = 1;
                if ((delta = ParseBytecodeInt32Params(delta, scale, 2, &ra, &n)) < 0) {
                    return -1;
                }
                if (n < 0) {
                    n = static_cast<int32_t>(stack_tp_ - (frame_bp() + ra));
                } else {
                    stack_tp_ = (frame_bp() + ra) + n;
                }
                CopyResult(frame_bp(), n, frame_->wanted());
                frame_->AddPC(delta);
                return n;
            } break;
                
            case Bytecode::kNew: {
            } break;
                
                // foo(bar())
            case Bytecode::kCall: {
                int32_t ra, n_args, n_rets;
                int delta = 1;
                if ((delta = ParseBytecodeInt32Params(delta, scale, 3, &ra, &n_args, &n_rets)) < 0) {
                    return -1;
                }
                Object *val = Get(ra);
                if (val->IsSmi()) {
                    owns_->Raisef("can not call number.");
                    return -1;
                }
                Object **base = frame_bp() + ra;
                if (n_args >= 0) {
                    stack_tp_ = base + 1 + n_args;
                }
                InternalCall(base, n_args, n_rets);
                frame_->AddPC(delta);
            } break;
                
            // TODO:
                
            default:
                owns_->Raisef("Bad bytecode: %d", id);
                break;
        }
    }
    if (frame_->callee() == entry_) {
        state_ = kDead;
    } else {
        state_ = kSuspended;
    }
    return has_raised_ ? -1 : 0;
}
    
int NyThread::InternalCall(Object **base, int32_t n_args, int32_t wanted) {
    DCHECK((*base)->IsObject());

    NyObject *ob = static_cast<NyObject *>(*base);
    switch (ob->GetMetatable()->kid()) {
        case kTypeFunction: {
            NyFunction *callee = ob->ToFunction();
            if (n_args < 0) {
                n_args = static_cast<int32_t>(stack_tp_ - base - 1);
            }

            CallFrame frame;
            frame.Enter(this, callee,
                        callee->script()->bcbuf(),
                        callee->script()->const_pool(),
                        wanted,
                        base, /*frame_bp*/
                        base + 1 + callee->script()->max_stack_size(), /*frame_tp*/
                        frame_->env());
            int rv = Run();
            frame.Exit(this);
            return rv;
        } break;
            
        case kTypeScript: {
            //NyScript *callee = ob->ToScript();
            // TODO:
        } break;

        case kTypeDelegated: {
            NyDelegated *callee = ob->ToDelegated();

            if (n_args < 0) {
                n_args = static_cast<int32_t>(stack_tp_ - base - 1);
            }
            HandleScope scope(owns_->isolate());
            Arguments args(reinterpret_cast<Value **>(base + 1), n_args);
            args.SetCallee(Local<Value>::New(callee));
            CallFrame frame;
            frame.Enter(this, callee,
                        nullptr, /* bc buf */
                        nullptr, /* const pool */
                        wanted,
                        base, /*frame_bp*/
                        base + 20, /* frame_tp */
                        frame_->env());
            int rv = callee->Call(&args, owns_);
            if (rv >= 0) {
                CopyResult(stack_ + frame.stack_bp(), rv, wanted);
            }
            frame.Exit(this);
            return rv;
        } break;

        default: {
            // TODO: call metatable method.
            DLOG(FATAL) << "TODO";
        } break;
    }
    return 0;
}
    
/**
 * bp                                  tp
 *  +--------+--------+--------+       +
 *  | callee | arg[0] | arg[1] | ..... |
 *  +--------+--------+--------+       +
 */
//int NyThread::CallDelegated(NyDelegated *fn, Object **base, int32_t n_args,
//                            int32_t n_receive) {
//    HandleScope scope(owns_->isolate());
//
//    Arguments args(reinterpret_cast<Value **>(stack_tp_ - n_args), n_args);
//    args.SetCallee(Local<Value>::New(fn));
//    int nret = fn->Call(&args, owns_);
//    if (nret < 0 || has_raised_) {
//        return -1;
//    }
//    Object **ret = stack_tp_ - nret;
//    if (nret < n_receive) {
//        for (int i = 0; i < nret; ++i) {
//            base[i] = ret[i];
//        }
//        for (int i = nret; i < n_receive; ++i) {
//            base[i] = Object::kNil;
//        }
//    } else { // >=
//        for (int i = 0; i < n_receive; ++i) {
//            base[i] = ret[i];
//        }
//    }
//    stack_tp_ = base + n_receive;
//    DCHECK_LT(stack_tp_, stack_last_);
//    return nret;
//}
    
//int NyThread::CallMetaFunction(NyObject *ob, NyString *name, Arguments *args, int n_accepts,
//                               uint32_t offset, NyByteArray const **bcbuf) {
//    NyMap *mt = DCHECK_NOTNULL(ob)->GetMetatable();
//    Object *mf = mt->RawGet(name, owns_);
//    if (mf == Object::kNil) {
//        owns_->Raisef("attempt to call nil `%s' meta function.", name->bytes());
//        return -1;
//    }
//    if (mf->IsSmi()) {
//        owns_->Raisef("attempt to call incorrect `%s' meta function.", name->bytes());
//        return -1;
//    }
//    NyRunnable *ro = static_cast<NyRunnable *>(mf);
//    if (!ro->IsRunnable()) {
//        owns_->Raisef("attempt to call incorrect `%s' meta function.", name->bytes());
//        return -1;
//    }
//
//    Object **base = stack_tp_;
//    Push(ro);
//    for (size_t i = 0; i < args->Length(); ++i) {
//        Push(*ApiWarpNoCheck<Object>(args->Get(i), owns_));
//    }
//
//    return CallInternal(ro, base, static_cast<int>(args->Length()), n_accepts, offset, bcbuf);
//}
    
//int NyThread::CallInternal(NyRunnable *ro, Object **base, int32_t n_args, int n_accepts,
//                           uint32_t offset, NyByteArray const **bcbuf) {
//    NyMap *env = Env();
//
//    if (NyDelegated *callee = ro->ToDelegated()) {
//        (*pc_ptr()) += offset; // NOTICE: add pc first!
//
//        int nret = CallDelegated(callee, base, n_args, n_accepts);
//        if (owns_->curr_thd() != this) {
//            return nret;
//        }
//    } else if (NyScript *callee = ro->ToScript()) {
//        (*pc_ptr()) += offset; // NOTICE: add pc first!
//
//        InitStack(callee, base, n_accepts, env);
//        *bcbuf = callee->bcbuf();
//    } else if (NyFunction *callee = ro->ToFunction()) {
//        (*pc_ptr()) += offset; // NOTICE: add pc first!
//
//        Arguments args(reinterpret_cast<Value **>(stack_tp_ - n_args), n_args);
//        InitStack(callee, base, n_accepts, env);
//        CopyArgs(&args, callee->n_params(), callee->vargs());
//        *bcbuf = callee->script()->bcbuf();
//    }
//    return 0;
//}
    
void NyThread::CopyResult(Object **ret, int n_rets, int wanted) {
    switch (wanted) {
        case -1:
            wanted = n_rets;
            break;
        case 0:
            stack_tp_ = ret;
            return;
        case 1:
            if (n_rets == 0) {
                ret[0] = Object::kNil;
            } else {
                ret[0] = *(stack_tp_ - 1);
            }
            stack_tp_ = ret;
            return;
        default:
            break;
    }
    Object **from = stack_tp_ - n_rets;
    for (int i = 0; i < n_rets && i < wanted; ++i) {
        ret[i] = from[i];
    }
    for (int i = n_rets; i < wanted; ++i) {
        ret[i] = Object::kNil;
    }
    stack_tp_ = ret + wanted;
}
    
int NyThread::ParseBytecodeInt32Params(int offset, int scale, int n, ...) {
    bool ok = true;
    va_list ap;
    va_start(ap, n);
    for (int i = 0; i < n; ++i) {
        int32_t *p = va_arg(ap, int32_t *);
        *p = ParseInt32(frame_->bcbuf(), frame_->pc() + offset, scale, &ok);
        if (!ok) {
            return -1;
        }
        offset += scale;
    }
    va_end(ap);
    return offset;
}
    
int32_t NyThread::ParseInt32(const NyByteArray *bcbuf, int offset, int scale, bool *ok) {
    int32_t param = 0;
    if (scale == 1) {
        int8_t val = static_cast<int8_t>(bcbuf->Get(offset));
        param = static_cast<int32_t>(val);
    } else if (scale == 2) {
        uint8_t lo = bcbuf->Get(offset + 0);
        uint8_t hi = bcbuf->Get(offset + 1);
        param = static_cast<int32_t>(static_cast<uint16_t>(hi) << 8 | lo);
    } else if (scale == 4) {
        uint8_t lo0 = bcbuf->Get(offset + 0);
        uint8_t lo1 = bcbuf->Get(offset + 1);
        uint8_t hi0 = bcbuf->Get(offset + 2);
        uint8_t hi1 = bcbuf->Get(offset + 3);
        param = static_cast<int32_t>(static_cast<uint32_t>(hi1) << 24 |
                                     static_cast<uint32_t>(hi0) << 16 |
                                     static_cast<uint32_t>(lo1) << 8 | lo0);
    } else {
        owns_->Raisef("unexpected scale: %d, requried(1, 2, 4)", scale);
        *ok = false;
    }
    return param;
}
    
} // namespace nyaa
    
} // namespace mai
