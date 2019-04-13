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
    
std::tuple<NyString *, NyInt32Array *> CallFrame::FileInfo() const {
    if (NyClosure *ob = callee_->ToClosure()) {
        return { ob->proto()->file_name(), ob->proto()->file_info() };
    } else {
        return { nullptr, nullptr };
    }
}

void CallFrame::IterateRoot(RootVisitor *visitor) {
    visitor->VisitRootPointer(reinterpret_cast<Object **>(&callee_));
    visitor->VisitRootPointer(reinterpret_cast<Object **>(&bcbuf_));
    visitor->VisitRootPointer(reinterpret_cast<Object **>(&const_poll_));
    visitor->VisitRootPointer(reinterpret_cast<Object **>(&env_));
}
    
    
TryCatchCore::TryCatchCore(NyaaCore *core)
    : core_(DCHECK_NOTNULL(core))
    , thrd_(DCHECK_NOTNULL(core_->curr_thd())) {
    prev_ = thrd_->catch_point_;
    thrd_->catch_point_ = this;
}

TryCatchCore::~TryCatchCore() {
    DCHECK_EQ(thrd_->catch_point_, this);
    thrd_->catch_point_ = prev_;
}
    
void TryCatchCore::Catch(NyString *message, Object *exception, NyArray *stack_trace) {
    has_caught_ = true;
    message_ = message;
    exception_ = exception;
    stack_trace_ = stack_trace;
}

void TryCatchCore::IterateRoot(RootVisitor *visitor) {
    visitor->VisitRootPointer(reinterpret_cast<Object **>(&thrd_));
    visitor->VisitRootPointer(reinterpret_cast<Object **>(&message_));
    visitor->VisitRootPointer(reinterpret_cast<Object **>(&exception_));
    visitor->VisitRootPointer(reinterpret_cast<Object **>(&stack_trace_));
}

    
NyThread::NyThread(NyaaCore *owns)
    : NyUDO(true /* ignore_managed */)
    , owns_(DCHECK_NOTNULL(owns)) {
    SetFinalizer(&UDOFinalizeDtor<NyThread>, owns);
}

NyThread::~NyThread() {
    delete[] stack_;
    
    if (this != owns_->main_thd()) {
        owns_->RemoveThread(this);
    }
}
    
Error NyThread::Init() {
    stack_size_ = owns_->stub()->init_thread_stack_size();
    stack_ = new Object *[stack_size_];
    stack_last_ = stack_ + stack_size_;
    stack_tp_ = stack_;

    return Error::OK();
}

void NyThread::Raisef(const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    Vraisef(fmt, ap);
    va_end(ap);
}

void NyThread::Vraisef(const char *fmt, va_list ap) {
    has_raised_ = true;
    NyString *msg = owns_->factory()->Vsprintf(fmt, ap);
    Raise(msg, nullptr);
}
    
void NyThread::Raise(NyString *msg, Object *ex) {
    has_raised_ = true;
    
    std::vector<NyString *> stack_trace;
    CallFrame *x = frame_;
    while (x) {
        NyInt32Array *file_info;
        NyString *file_name;
        std::tie(file_name, file_info) = x->FileInfo();
        
        NyString *line = nullptr;
        if (file_info) {
            line = owns_->factory()->Sprintf("%s:%d", !file_name ? "unknown" : file_name->bytes(),
                                             file_info->Get(x->pc()));
        } else {
            line = owns_->factory()->Sprintf("%s:[%p]", !file_name ? "" : file_name->bytes(),
                                             x->callee());
        }
        stack_trace.push_back(line);
        x = x->prev();
    }
    
    if (catch_point_) {
        NyArray *bt = nullptr;
        if (!stack_trace.empty()) {
            bt = owns_->factory()->NewArray(stack_trace.size());
            for (auto line : stack_trace) {
                bt = bt->Add(line, owns_);
            }
        }
        catch_point_->Catch(msg, ex, bt);
    } else {
        DLOG(FATAL) << msg->bytes();
    }
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
    
int NyThread::Run(NyClosure *fn, Arguments *args, int n_result, NyMap *env) {
    if (!env) {
        env = owns_->g();
    }
    Push(fn);
    
    CallFrame frame;
    frame.Enter(this, fn, fn->proto()->bcbuf(), fn->proto()->const_pool(),
                n_result, /* wanted */
                stack_tp_, /* frame_bp */
                stack_tp_ + fn->proto()->max_stack() /* frame_tp */,
                env);
    //Push(fn);
    CopyArgs(args, static_cast<int>(args->Length()), fn->proto()->vargs());
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
        if (NyClosure *callee = DCHECK_NOTNULL(entry_)->ToClosure()) {
            return Run(callee, args, 0);
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
    TryCatchCore *c = catch_point_;
    while (c) {
        c->IterateRoot(visitor);
        c = c->prev();
    }
}
    

/** fp                                           bp        tp
 *  +--------+--------+--------+--------+--------+---------+
 *  | callee | bcbuf  |   pc   |  rbp   |  rbs   | args...
 *  +--------+--------+--------+--------+--------+---------+
 */
int NyThread::Run() {
    has_raised_ = false;
    if (catch_point_) {
        catch_point_->Reset();
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
                
            case Bytecode::kLoadConst: {
                int32_t ra, rb;
                int delta = 1;
                if ((delta = ParseBytecodeInt32Params(delta, scale, 2, &ra, &rb)) < 0) {
                    return -1;
                }
                Object *k = frame_->const_poll()->Get(rb);
                Set(ra, k);
                frame_->AddPC(delta);
            } break;
                
            case Bytecode::kLoadUp: {
                int32_t ra, ub;
                int delta = 1;
                if ((delta = ParseBytecodeInt32Params(delta, scale, 2, &ra, &ub)) < 0) {
                    return -1;
                }
                Object *uv = frame_->upval(ub);
                Set(ra, uv);
                frame_->AddPC(delta);
            } break;
                
            case Bytecode::kStoreUp: {
                int32_t ra, ub;
                int delta = 1;
                if ((delta = ParseBytecodeInt32Params(delta, scale, 2, &ra, &ub)) < 0) {
                    return -1;
                }
                Object *val = Get(ra);
                frame_->SetUpval(ub, val, owns_);
                frame_->AddPC(delta);
            } break;
                
            case Bytecode::kStoreGlobal: {
                int32_t ra, kb;
                int delta = 1;
                if ((delta = ParseBytecodeInt32Params(delta, scale, 2, &ra, &kb)) < 0) {
                    return -1;
                }
                Object *val = Get(ra);
                Object *idx = frame_->const_poll()->Get(kb);
                frame_->env()->RawPut(idx, val, owns_);
                frame_->AddPC(delta);
            } break;
                
            case Bytecode::kMove: {
                int32_t ra, rb;
                int delta = 1;
                if ((delta = ParseBytecodeInt32Params(delta, scale, 2, &ra, &rb)) < 0) {
                    return -1;
                }
                Object *val = Get(rb);
                Set(ra, val);
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
                CopyResult(frame_bp() - 1, n, frame_->wanted());
                frame_->AddPC(delta);
                return n;
            } break;
                
            case Bytecode::kNew: {
                // TODO:
            } break;
                
            case Bytecode::kClosure: {
                int32_t ra, pb;
                int delta = 1;
                if ((delta = ParseBytecodeInt32Params(delta, scale, 2, &ra, &pb)) < 0) {
                    return -1;
                }
                NyFunction *proto = static_cast<NyFunction *>(frame_->proto()->proto_pool()->Get(pb));
                //printf("proto:%p, %d\n", proto, proto->IsArray());
                DCHECK(proto->IsObject() && proto->ToHeapObject()->IsFunction());
                NyClosure *closure = owns_->factory()->NewClosure(proto);
                for (int i = 0; i < proto->n_upvals(); ++i) {
                    Bind(i, closure, proto->upval(i));
                }
                Set(ra, closure);
                frame_->AddPC(delta);
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
                owns_->Raisef("Bad bytecode: %s(%d)", Bytecode::kNames[id], id);
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
        case kTypeClosure: {
            NyClosure *callee = ob->ToClosure();
            if (n_args < 0) {
                n_args = static_cast<int32_t>(stack_tp_ - base - 1);
            }

            CallFrame frame;
            frame.Enter(this, callee,
                        callee->proto()->bcbuf(),
                        callee->proto()->const_pool(),
                        wanted,
                        base + 1, /*frame_bp*/
                        base + 1 + callee->proto()->max_stack(), /*frame_tp*/
                        frame_->env());
            int rv = Run();
            frame.Exit(this);
            return rv;
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
                        base + 1, /*frame_bp*/
                        base + 20, /* frame_tp */
                        frame_->env());
            int rv = callee->Call(&args, owns_);
            if (rv >= 0) {
                CopyResult(stack_ + frame.stack_bp() - 1, rv, wanted);
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
        View<Byte> slice = frame_->bcbuf()->GetView(frame_->pc() + offset, scale);
        *p = Bytecode::ParseInt32Param(slice, scale);
        if (!ok) {
            return -1;
        }
        offset += scale;
    }
    va_end(ap);
    return offset;
}
    
} // namespace nyaa
    
} // namespace mai
