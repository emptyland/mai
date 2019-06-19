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
    
using EntryTrampolineCallStub =
    CallStub<int (NyThread *, NyCode *code, NyaaCore *, CodeContextBundle *, Address)>;
    
const int32_t CallFrame::kOffsetCallee = Template::OffsetOf(&CallFrame::callee_);
const int32_t CallFrame::kOffsetEnv = Template::OffsetOf(&CallFrame::env_);
const int32_t CallFrame::kOffsetConstPool = Template::OffsetOf(&CallFrame::const_pool_);
const int32_t CallFrame::kOffsetStackBE = Template::OffsetOf(&CallFrame::stack_be_);
const int32_t CallFrame::kOffsetStackBP = Template::OffsetOf(&CallFrame::stack_bp_);
const int32_t CallFrame::kOffsetStackTP = Template::OffsetOf(&CallFrame::stack_tp_);
const int32_t CallFrame::kOffsetEntry = Template::OffsetOf(&CallFrame::entry_);
const int32_t CallFrame::kOffsetPC = Template::OffsetOf(&CallFrame::pc_);
    
const int32_t NyThread::kOffsetOwns = Template::OffsetOf(&NyThread::owns_);
const int32_t NyThread::kOffsetInterruptionPending =
    Template::OffsetOf(&NyThread::interruption_pending_);
const int32_t NyThread::kOffsetSavePoint = Template::OffsetOf(&NyThread::save_point_);
const int32_t NyThread::kOffsetFrame = Template::OffsetOf(&NyThread::frame_);
const int32_t NyThread::kOffsetStack = Template::OffsetOf(&NyThread::stack_);
const int32_t NyThread::kOffsetNaStBK = Template::OffsetOf(&NyThread::nast_bk_);
const int32_t NyThread::kOffsetNaStBKSize = Template::OffsetOf(&NyThread::nast_bk_size_);

const int32_t CodeContextBundle::kOffsetNaStTP = Template::OffsetOf(&CodeContextBundle::nast_tp_);
const int32_t CodeContextBundle::kOffsetNaStBP = Template::OffsetOf(&CodeContextBundle::nast_bp_);
    
void CallFrame::Enter(NyThread *owns, NyRunnable *callee, NyByteArray *bcbuf, NyArray *kpool,
                      int wanted, size_t bp, size_t tp, NyMap *env) {
    DCHECK_NE(this, owns->frame_);

    level_ = (owns->frame_ ? owns->frame_->level_ + 1 : 0);
    prev_ = owns->frame_;
    owns->frame_ = this;
    
    owns->CheckStack(tp);

    compact_file_info_ = owns->owns_->stub()->compact_source_line_info();
    callee_     = DCHECK_NOTNULL(callee);
    bcbuf_      = bcbuf;
    const_pool_ = kpool;
    env_        = DCHECK_NOTNULL(env);
    wanted_     = wanted;
    DCHECK(bp >= 0 && bp < owns->stack_size_);
    stack_be_   = bp;
    stack_bp_   = stack_be_;
    DCHECK(tp >= bp && tp < owns->stack_size_);
    stack_tp_   = tp;
    if (NyClosure *fn = NyClosure::Cast(callee)) {
        if (fn->proto()->IsNativeExec()) {
            entry_ = reinterpret_cast<intptr_t>(fn->proto()->code()->entry_address());
        } else {
            entry_ = 0;
        }
    } else {
        entry_ = 0;
    }
    
    //printf("enter: %p\n", callee_);
}

void CallFrame::Exit(NyThread *owns) {
    //printf("exit: %p\n", callee_);
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
    visitor->VisitRootPointer(reinterpret_cast<Object **>(&const_pool_));
    visitor->VisitRootPointer(reinterpret_cast<Object **>(&env_));
}
    
int CallFrame::GetFileLine(const NyInt32Array *line_info) {
    if (!compact_file_info_) {
        return line_info->Get(pc_);
    }
    
    for (int i = 0; i < line_info->size(); i += 3) {
        if (pc_ >= line_info->Get(i) && pc_ < line_info->Get(i + 1)) {
            return line_info->Get(i + 2);
        }
    }
    DLOG(FATAL) << "Noreached!";
    return 0;
}
    
TryCatchCore::TryCatchCore(NyaaCore *core) : TryCatchCore(core, core->curr_thd()) {}

TryCatchCore::TryCatchCore(NyaaCore *core, NyThread *thread)
    : core_(DCHECK_NOTNULL(core)) {
    prev_ = thread->catch_point_;
    thread->catch_point_ = this;

    obs_ = reinterpret_cast<Object **>(core_->AdvanceHandleSlots(kSlotSize));
    obs_[kThread] = thread;
    Reset();
}

TryCatchCore::~TryCatchCore() {
    auto thrd = thread();
    DCHECK_EQ(thrd->catch_point_, this);
    thrd->catch_point_ = prev_;
}
    
void TryCatchCore::Catch(NyString *message, Object *exception, NyArray *stack_trace) {
    has_caught_ = true;
    obs_[kMessage] = message;
    obs_[kException] = exception;
    obs_[kStackTrace] = stack_trace;
}

std::string TryCatchCore::ToString() const {
    std::string buf;
    buf.append(message()->bytes(), message()->size());
    buf.append("\n");
    if (stack_trace()) {
        buf.append("stack strace:\n");
        for (int i = 0; i < stack_trace()->size(); ++i) {
            buf.append("    ");
            const NyString *line = static_cast<NyString *>(stack_trace()->Get(i));
            buf.append(line->bytes(), line->size());
            buf.append("\n");
        }
    }
    return buf;
}

NyThread::NyThread(NyaaCore *owns)
    : NyUDO(NyUDO::GetNFiedls(sizeof(NyThread)), true /* ignore_managed */)
    , owns_(DCHECK_NOTNULL(owns)) {
    SetFinalizer(&UDOFinalizeDtor<NyThread>, owns);
    //::memset(&save_point_, 0, sizeof(save_point_));
}

NyThread::~NyThread() {
    ::free(stack_);
    ::free(nast_bk_);
    while (frame_) {
        CallFrame *ci = frame_;
        frame_ = frame_->prev();
        delete ci;
    }
    if (this != owns_->main_thd()) {
        owns_->RemoveThread(this);
    }
}

Error NyThread::Init() {
    stack_size_ = owns_->stub()->init_thread_stack_size();
    stack_ = static_cast<Object **>(::malloc(sizeof(Object *) * stack_size_)); //new Object *[stack_size_];
    stack_last_ = stack_ + stack_size_;
    stack_tp_ = stack_;

    return Error::OK();
}
    
void NyThread::Vraisef(const char *fmt, va_list ap) {
    NyString *msg = owns_->factory()->Vsprintf(fmt, ap);
    Raise(msg, nullptr);
}
    
void NyThread::Raise(NyString *msg, Object *ex) {
    std::vector<NyString *> stack_trace;
    CallFrame *x = frame_;
    while (x) {
        NyInt32Array *file_info;
        NyString *file_name;
        std::tie(file_name, file_info) = x->FileInfo();

        NyString *line = nullptr;
        if (file_info) {
            line = owns_->factory()->Sprintf("%s:%d", !file_name ? "unknown" : file_name->bytes(),
                                             GetLine(file_info, x->pc()));
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
        interruption_pending_ = CallFrame::kException;
        switch (owns_->stub()->exec()) {
            case Nyaa::kAOT:
                break; // Ingore: DO NOT throw anything.
            case Nyaa::kInterpreter:
                throw interruption_pending_;
            default:
                DLOG(FATAL) << "Noreached!";
                break;
        }
    } else {
        DLOG(FATAL) << msg->bytes();
    }
}
    
int NyThread::TryRun(NyRunnable *fn, Object *argv[], int argc, int wanted, NyMap *env) {
    int rv = 0;
    CallFrame *ci = frame_;
    interruption_pending_ = CallFrame::kNormal; // clear interruption pending flag
    try {
        rv = Run(fn, argv, argc, wanted, env);
    } catch (CallFrame::ExceptionId which) {
        if (which == CallFrame::kException) {
            DCHECK_EQ(which, interruption_pending_);
            // Ingore
        } else {
            throw which;
        }
    }
    if (interruption_pending_ == CallFrame::kException) {
        Unwind(ci);
        rv = -1;
    }
    return rv;
}
    
int NyThread::Resume(Object *argv[], int argc, int wanted, NyMap *env) {
    int rv = 0;
    CallFrame *ci = frame_;
    
    DCHECK_NE(this, owns_->curr_thd());
    save_ = owns_->curr_thd();
    owns_->set_curr_thd(this);

    try {
        if (ci) {
            CallFrame *frame = frame_;
            CopyResult(stack_ + frame->stack_bp() - 1, argv, argc, frame->wanted());
            frame->Exit(this);
            delete frame;
            
            if (frame_->proto()->IsNativeExec()) {
                interruption_pending_ = CallFrame::kNormal;
                EntryTrampolineCallStub stub(owns_->code_pool()->kEntryTrampoline);
                CodeContextBundle bundle(this);
                rv = stub.entry_fn()(this, frame_->proto()->code(), owns_, &bundle,
                                     owns_->GetSuspendPointAddress());
            } else {
                frame_->AddPC(ParseBytecodeSize(frame_->pc()));
                rv = Run();
            }
        } else {
            rv = Run(entry_, argv, argc, wanted, env);
        }
    } catch (CallFrame::ExceptionId which) {
        // ignore
        DCHECK_EQ(which, interruption_pending_);
        if (which != CallFrame::kException && which != CallFrame::kYield) {
            throw which;
        }
    }
    switch (interruption_pending_) {
        case CallFrame::kException:
            Unwind(ci);
            rv = -1;
            state_ = kSuspended;
            break;
        case CallFrame::kYield:
            rv = frame_->nrets();
            DCHECK_GE(rv, 0);
            state_ = kSuspended;
            break;
        default:
            break;
    }
    DCHECK_EQ(this, owns_->curr_thd());
    owns_->set_curr_thd(save_);
    save_ =  nullptr;
    return rv;
}

void NyThread::Yield() {
    interruption_pending_ = CallFrame::kYield;
    switch (owns_->stub()->exec()) {
        case Nyaa::kAOT:
            break;
        case Nyaa::kInterpreter:
            throw interruption_pending_;
        default:
            DLOG(FATAL) << "Noreached!";
            break;
    }
}

int NyThread::Run(NyRunnable *rb, Object *argv[], int argc, int wanted, NyMap *env) {
    if (!env) {
        env = owns_->g();
    }
    Push(rb);
    DCHECK_GE(stack_tp_, stack_);
    size_t top = stack_tp_ - stack_;
    
    int rv = -1;
    CallFrame *frame = new CallFrame;
    if (NyClosure *fn = rb->ToClosure()) {
        NyByteArray *bcbuf = fn->proto()->IsInterpretationExec() ? fn->proto()->bcbuf() : nullptr;
        frame->Enter(this, fn, bcbuf, fn->proto()->const_pool(),
                     wanted, /* wanted */
                     top, /* frame_bp */
                     top + fn->proto()->max_stack() /* frame_tp */,
                     env);
        int adjust = CopyArgs(argv, argc, fn->proto()->n_params(), fn->proto()->vargs());
        frame_->AdjustBP(adjust);
        if (fn->proto()->IsNativeExec()) {
            EntryTrampolineCallStub stub(owns_->code_pool()->kEntryTrampoline);
            CodeContextBundle bundle(this);
            rv = stub.entry_fn()(this, fn->proto()->code(), owns_, &bundle, nullptr);
        } else {
            rv = Run();
        }
    } else if (NyDelegated *fn = rb->ToDelegated()) {
        frame->Enter(this, fn,
                     nullptr, /* bc buf */
                     nullptr, /* const pool */
                     wanted, /* wanted */
                     top, /*frame_bp*/
                     top + 20, /* frame_tp */
                     env);
        HandleScope handle_scope(owns_->stub());
        FunctionCallbackInfo<Object> info(fn, argv, argc, owns_->stub());
        fn->Apply(info);
        rv = frame->nrets();
        if (rv >= 0) {
            CopyResult(stack_ + frame->stack_bp() - 1, rv, wanted);
        }
        frame->Exit(this);
        delete frame;
    } else {
        DLOG(FATAL) << "noreached!";
    }
    owns_->GarbageCollectionSafepoint(__FILE__, __LINE__);
    return rv;
}
    
int NyThread::CopyArgs(Object **args, int n_args, int n_params, bool vargs) {
    int adjust = 0;
    if (vargs) {
        if (n_args >= n_params) {
            adjust = n_args - n_params;
        }
    }
    if (n_args < n_params) {
        for (size_t i = 0; i < n_args; ++i) {
            Push(args[i]);
        }
        Push(Object::kNil, n_params - n_args);
    } else { // >=
        // FIXME:
        for (int i = n_params; i < n_args; ++i) {
            Push(args[i]);
        }
        for (int i = 0; i < n_params; ++i) {
            Push(args[i]);
        }
    }
    return adjust;
}
    
void NyThread::PrintStack() {
    printf("<%d>--------------------------------------------------\n", frame_->pc());
    int i = 0;
    for (Object **p = frame_bp(); p < stack_tp_; ++p) {
        NyString *s = (*p)->ToString(owns_);
        printf("[%d] %p, %s\n", i++, *p,  s->bytes());
    }
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
    
void NyThread::CheckStack(size_t size) {
    size_t required_size = 0;
    size_t frame_limit = 0;
    if (frame_) {
        frame_limit = frame_->stack_tp();
    } else {
        frame_limit = 0;
    }
    required_size = size > frame_limit ? size : frame_limit;
    DCHECK_GE(stack_tp_, stack_);
    size_t tp = stack_tp_ - stack_;

    if (required_size > stack_size_) {
        required_size = (required_size << 1);
    } else if (required_size < stack_size_ / 2) {
        required_size = stack_size_ / 2;
        if (required_size < owns_->stub()->init_thread_stack_size()) {
            return;
        }
    } else {
        return;
    }
    stack_      = static_cast<Object **>(::realloc(stack_, sizeof(Object *) * required_size));
    stack_size_ = required_size;
    stack_last_ = stack_ + stack_size_;
    stack_tp_   = stack_ + tp;
}

int NyThread::Run() {
    if (catch_point_) {
        catch_point_->Reset();
    }

    state_ = kRunning;
    while (frame_->pc() < frame_->bcbuf()->size()) {
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
        switch (id) {
            case Bytecode::kLoadImm: {
                int32_t ra, imm;
                int delta = ParseBytecodeInt32Params(1, scale, 2, &ra, &imm);
                Set(ra, NyInt32::New(imm));
                frame_->AddPC(delta);
            } break;

            case Bytecode::kLoadNil: {
                int32_t ra, n;
                int delta = ParseBytecodeInt32Params(1, scale, 2, &ra, &n);
                for (int i = 0; i < n; ++i) {
                    Set(ra + i, Object::kNil);
                }
                frame_->AddPC(delta);
            } break;

            case Bytecode::kLoadGlobal: {
                int32_t ra, rb;
                int delta = ParseBytecodeInt32Params(1, scale, 2, &ra, &rb);
                Object *key = frame_->const_pool()->Get(rb);
                Object *val = frame_->env()->RawGet(key, owns_);
                Set(ra, val);
                frame_->AddPC(delta);
            } break;

            case Bytecode::kLoadConst: {
                int32_t ra, rb;
                int delta = ParseBytecodeInt32Params(1, scale, 2, &ra, &rb);
                Object *k = frame_->const_pool()->Get(rb);
                Set(ra, k);
                frame_->AddPC(delta);
            } break;

            case Bytecode::kLoadUp: {
                int32_t ra, ub;
                int delta = ParseBytecodeInt32Params(1, scale, 2, &ra, &ub);
                Object *uv = frame_->upval(ub);
                Set(ra, uv);
                frame_->AddPC(delta);
            } break;

            case Bytecode::kStoreUp: {
                int32_t ra, ub;
                int delta = ParseBytecodeInt32Params(1, scale, 2, &ra, &ub);
                Object *val = Get(ra);
                frame_->SetUpval(ub, val, owns_);
                frame_->AddPC(delta);
            } break;

            case Bytecode::kStoreGlobal: {
                int32_t ra, kb;
                int delta = ParseBytecodeInt32Params(1, scale, 2, &ra, &kb);
                Object *val = Get(ra);
                Object *idx = frame_->const_pool()->Get(kb);
                frame_->env()->RawPut(idx, val, owns_);
                frame_->AddPC(delta);
            } break;

            case Bytecode::kGetField: {
                int32_t ra, rb, rkc;
                int delta = ParseBytecodeInt32Params(1, scale, 3, &ra, &rb, &rkc);
                Object *key = nullptr;
                if (rkc < 0) {
                    key = frame_->const_pool()->Get(-rkc - 1);
                } else {
                    key = Get(rkc);
                }
                Object *value = InternalGetField(frame_bp() + ra, Get(rb), key);
                Set(ra, value);
                frame_->AddPC(delta);
            } break;

            case Bytecode::kSetField: {
                int32_t ra, rkb, rkc;
                int delta = ParseBytecodeInt32Params(1, scale, 3, &ra, &rkb, &rkc);
                Object *key = nullptr;
                if (rkb < 0) {
                    key = frame_->const_pool()->Get(-rkb - 1);
                } else {
                    key = Get(rkb);
                }

                Object *value = nullptr;
                if (rkc < 0) {
                    value = frame_->const_pool()->Get(-rkc - 1);
                } else {
                    value = Get(rkc);
                }
                InternalSetField(frame_bp() + ra, key, value);
                frame_->AddPC(delta);
            } break;

            case Bytecode::kMove: {
                int32_t ra, rb;
                int delta = ParseBytecodeInt32Params(1, scale, 2, &ra, &rb);
                Object *val = Get(rb);
                Set(ra, val);
                frame_->AddPC(delta);
            } break;

            case Bytecode::kRet: {
                int32_t ra, n;
                int delta = ParseBytecodeInt32Params(1, scale, 2, &ra, &n);
                if (n < 0) {
                    n = static_cast<int32_t>(stack_tp_ - (frame_bp() + ra));
                } else {
                    stack_tp_ = (frame_bp() + ra) + n;
                }
                frame_->set_nrets(n);
                CopyResult(stack_ + frame_->stack_be() - 1, n, frame_->wanted());
                frame_->AddPC(delta);
                auto outter = frame_;
                outter->Exit(this);
                delete outter;
                owns_->GarbageCollectionSafepoint(__FILE__, __LINE__);
                return n;
            } break;

            case Bytecode::kTest: {
                int32_t ra, neg, none;
                int delta = ParseBytecodeInt32Params(1, scale, 3, &ra, &neg, &none);
                bool cond = false;
                Object *ob = Get(ra);
                if (ob == Object::kNil) {
                    cond = false;
                } else {
                    cond = neg ? ob->IsFalse() : ob->IsTrue();
                }
                if (cond) {
                    delta += ParseBytecodeSize(frame_->pc() + delta);
                }
                frame_->AddPC(delta);
            } break;

            case Bytecode::kTestNil: {
                int32_t ra, neg, none;
                int delta = ParseBytecodeInt32Params(1, scale, 3, &ra, &neg, &none);
                bool cond = false;
                Object *ob = Get(ra);
                if (ob == Object::kNil) {
                    cond = !neg ? true : false;
                } else {
                    cond = !neg ? false : true;
                }
                if (cond) {
                    delta += ParseBytecodeSize(frame_->pc() + delta);
                }
                frame_->AddPC(delta);
            } break;

            case Bytecode::kTestSet: {
                int32_t ra, rb, c;
                int delta = ParseBytecodeInt32Params(1, scale, 3, &ra, &rb, &c);
                bool cond = false;
                Object *ob = Get(rb);
                if (ob == Object::kNil) {
                    cond = false;
                } else {
                    cond = c ? ob->IsFalse() : ob->IsTrue();
                }
                if (cond) {
                    Set(ra, ob);
                } else {
                    delta += ParseBytecodeSize(frame_->pc() + delta);
                }
                frame_->AddPC(delta);
            } break;

            case Bytecode::kJumpImm: {
                int32_t offset;
                ParseBytecodeInt32Params(1, scale, 1, &offset);
                frame_->AddPC(offset);
            } break;

            case Bytecode::kJumpConst: {
                int32_t k;
                ParseBytecodeInt32Params(1, scale, 1, &k);
                Object *ob = frame_->const_pool()->Get(k);
                DCHECK(ob->IsSmi());
                frame_->AddPC(static_cast<int32_t>(ob->ToSmi()));
            } break;

            case Bytecode::kClosure: {
                int32_t ra, pb;
                int delta = ParseBytecodeInt32Params(1, scale, 2, &ra, &pb);
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

            case Bytecode::kNewMap: {
                int32_t ra, n, p;
                int delta = ParseBytecodeInt32Params(1, scale, 3, &ra, &n, &p);
                RuntimeNewMap(ra, n, p, 0);
                owns_->GarbageCollectionSafepoint(__FILE__, __LINE__);
                frame_->AddPC(delta);
            } break;

            case Bytecode::kSelf: {
                int32_t ra, rb, kc;
                int delta = ParseBytecodeInt32Params(1, scale, 3, &ra, &rb, &kc);
                Object *key = frame_->const_pool()->Get(kc);
                Object *method = InternalGetField(frame_bp() + ra, Get(rb), key);
                Set(ra + 1, Get(rb));
                Set(ra, method);
                frame_->AddPC(delta);
            } break;

                // foo(bar())
            case Bytecode::kCall: {
                int32_t ra, argc, wanted;
                int delta = ParseBytecodeInt32Params(1, scale, 3, &ra, &argc, &wanted);
                Object *val = Get(ra);
                if (val->IsSmi()) {
                    owns_->Raisef("can not call number.");
                    return -1;
                }
                Object **base = frame_bp() + ra;
                if (argc >= 0) {
                    stack_tp_ = base + 1 + argc;
                }
                int rv = InternalCall(base, argc, wanted);
//                if (interruption_pending_ != CallFrame::kNormal) {
//                    return rv;
//                }
                frame_->AddPC(delta);
            } break;

            case Bytecode::kNew: {
                int32_t ra, rb, argc;
                int delta = ParseBytecodeInt32Params(1, scale, 3, &ra, &rb, &argc);
                NyMap *clazz = NyMap::Cast(Get(rb));
                if (!clazz) {
                    owns_->Raisef("new non-class.");
                    return -1;
                }
                Object *n_fields = clazz->RawGet(owns_->bkz_pool()->kInnerSize, owns_);
                if (n_fields == Object::kNil || !n_fields->IsSmi()) {
                    owns_->Raisef("incorrect class table: error __size__.");
                    return -1;
                }
                NyUDO *udo = InternalNewUdo(frame_bp() + rb, argc, n_fields->ToSmi(), clazz);
                Set(ra, udo);
                frame_->AddPC(delta);
            } break;

            case Bytecode::kVargs: {
                int32_t ra, wanted;
                int delta = ParseBytecodeInt32Params(1, scale, 2, &ra, &wanted);
                int32_t nvargs = static_cast<int32_t>(frame_->GetNVargs());
                if (wanted < 0) {
                    CheckStack(frame_->stack_bp() + nvargs);
                    Object **a = frame_bp() + ra;
                    for (size_t i = 0; i < nvargs; ++i) {
                        a[i] = frame_be()[i];
                    }
                    stack_tp_ = a + nvargs;
                } else {
                    Object **a = frame_bp() + ra;
                    if (wanted > nvargs) {
                        for (int i = 0; i < nvargs; ++i) {
                            a[i] = frame_be()[i];
                        }
                        for (int i = nvargs; i < wanted; ++i) {
                            a[i] = Object::kNil;
                        }
                    } else {
                        for (int i = 0; i < wanted; ++i) {
                            a[i] = frame_be()[i];
                        }
                    }
                    stack_tp_ = a + wanted;
                }
                frame_->AddPC(delta);
            } break;

            case Bytecode::kConcat: {
                int32_t ra, rb, n;
                int delta = ParseBytecodeInt32Params(1, scale, 3, &ra, &rb, &n);
                NyString *str = Get(rb)->ToString(owns_);
                NyString *rv = owns_->factory()->NewUninitializedString(str->size() + 64);
                Set(ra, rv->Add(str, owns_)); // protect for gc
                for (int i = 1; i < n; ++i) {
                    str = Get(rb + i)->ToString(owns_);
                    rv = DCHECK_NOTNULL(NyString::Cast(Get(ra)));
                    Set(ra, rv->Add(str, owns_)); // protect for gc
                }

                rv = DCHECK_NOTNULL(NyString::Cast(Get(ra)));
                rv->Done(owns_);
                frame_->AddPC(delta);
            } break;

            #define PROCESS_ARITH(op) \
                int32_t ra, rkb, rkc; \
                int delta = ParseBytecodeInt32Params(1, scale, 3, &ra, &rkb, &rkc); \
                Object *lhs = rkb < 0 ? frame_->const_pool()->Get(-rkb - 1) : Get(rkb); \
                Object *rhs = rkc < 0 ? frame_->const_pool()->Get(-rkc - 1) : Get(rkc); \
                if (!InternalCallMetaFunction(frame_bp() + ra, owns_->bkz_pool()->kInner##op, \
                                              1, lhs, 1, rhs)) { \
                    Set(ra, Object::op(lhs, rhs, owns_)); \
                } \
                frame_->AddPC(delta)

            case Bytecode::kAdd: {
                PROCESS_ARITH(Add);
            } break;
            case Bytecode::kSub: {
                PROCESS_ARITH(Sub);
            } break;
            case Bytecode::kMul: {
                PROCESS_ARITH(Mul);
            } break;
            case Bytecode::kDiv: {
                PROCESS_ARITH(Div);
            } break;
            case Bytecode::kMod: {
                PROCESS_ARITH(Mod);
            } break;

            case Bytecode::kEqual: {
                int32_t ra, rkb, rkc;
                int delta = ParseBytecodeInt32Params(1, scale, 3, &ra, &rkb, &rkc);
                Object *lhs = rkb < 0 ? frame_->const_pool()->Get(-rkb - 1) : Get(rkb);
                Object *rhs = rkc < 0 ? frame_->const_pool()->Get(-rkc - 1) : Get(rkc);
                Set(ra, NySmi::New(Object::Equal(lhs, rhs, owns_)));
                frame_->AddPC(delta);
            } break;

            case Bytecode::kLessThan: {
                int32_t ra, rkb, rkc;
                int delta = ParseBytecodeInt32Params(1, scale, 3, &ra, &rkb, &rkc);
                Object *lhs = rkb < 0 ? frame_->const_pool()->Get(-rkb - 1) : Get(rkb);
                Object *rhs = rkc < 0 ? frame_->const_pool()->Get(-rkc - 1) : Get(rkc);
                Set(ra, NySmi::New(Object::LessThan(lhs, rhs, owns_)));
                frame_->AddPC(delta);
            } break;

            case Bytecode::kLessEqual: {
                int32_t ra, rkb, rkc;
                int delta = ParseBytecodeInt32Params(1, scale, 3, &ra, &rkb, &rkc);
                Object *lhs = rkb < 0 ? frame_->const_pool()->Get(-rkb - 1) : Get(rkb);
                Object *rhs = rkc < 0 ? frame_->const_pool()->Get(-rkc - 1) : Get(rkc);
                Set(ra, NySmi::New(Object::LessEqual(lhs, rhs, owns_)));
                frame_->AddPC(delta);
            } break;

            case Bytecode::kGreaterThan: {
                int32_t ra, rkb, rkc;
                int delta = ParseBytecodeInt32Params(1, scale, 3, &ra, &rkb, &rkc);
                Object *lhs = rkb < 0 ? frame_->const_pool()->Get(-rkb - 1) : Get(rkb);
                Object *rhs = rkc < 0 ? frame_->const_pool()->Get(-rkc - 1) : Get(rkc);
                Set(ra, NySmi::New(!Object::LessEqual(lhs, rhs, owns_)));
                frame_->AddPC(delta);
            } break;

            case Bytecode::kGreaterEqual: {
                int32_t ra, rkb, rkc;
                int delta = ParseBytecodeInt32Params(1, scale, 3, &ra, &rkb, &rkc);
                Object *lhs = rkb < 0 ? frame_->const_pool()->Get(-rkb - 1) : Get(rkb);
                Object *rhs = rkc < 0 ? frame_->const_pool()->Get(-rkc - 1) : Get(rkc);
                Set(ra, NySmi::New(!Object::LessThan(lhs, rhs, owns_)));
                frame_->AddPC(delta);
            } break;

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
    return 0;
}
    
Object *NyThread::InternalGetField(Object **base, Object *mm, Object *key) {
    if (mm == Object::kNil) {
        Raisef("attempt to nil field.");
        return Object::kNil;
    }
    if (mm->IsSmi()) {
        Raisef("attempt to smi field.");
        return Object::kNil;
    }
    
    NyObject *ob = mm->ToHeapObject();
    if (NyMap *map = ob->ToMap()) {
        if (map->GetMetatable() == owns_->kmt_pool()->kMap) {
            return map->RawGet(key, owns_);
        }
        Object *mindex = map->GetMetatable()->RawGet(owns_->bkz_pool()->kInnerIndex, owns_);
        if (NyMap *index_map = NyMap::Cast(mindex)) {
            return index_map->RawGet(key, owns_);
        } else if (InternalCallMetaFunction(base, owns_->bkz_pool()->kInnerIndex, 1, mm, 1, key)) {
            return Get(-1);
        } else {
            return map->RawGet(key, owns_);
        }
    } else if (NyUDO *udo = ob->ToUDO()) {
        if (InternalCallMetaFunction(base, owns_->bkz_pool()->kInnerIndex, 1, mm, 1, key)) {
            return Get(-1);
        } else {
            return udo->RawGet(key, owns_);
        }
    } else {
        Raisef("incorrect type for getfield.");
        return Object::kNil;
    }
    return Object::kNil;
}
    
int NyThread::InternalSetField(Object **base, Object *key, Object *value) {
    Object *mm = *base;
    if (mm == Object::kNil) {
        Raisef("attempt to nil field.");
        return -1;
    }
    if (mm->IsSmi()) {
        Raisef("attempt to smi field. %d", mm->ToSmi());
        return -1;
    }
    
    NyObject *ob = mm->ToHeapObject();
    if (NyMap *map = ob->ToMap()) {
        if (map->GetMetatable() == owns_->kmt_pool()->kMap) {
            map->RawPut(key, value, owns_);
            return 0;
        }
        Object *mnewindex = map->GetMetatable()->RawGet(owns_->bkz_pool()->kInnerNewindex, owns_);
        if (NyMap *newindex_map = NyMap::Cast(mnewindex)) {
            newindex_map->RawPut(key, value, owns_);
        } else {
            // base + 1 for skip map position
            if (!InternalCallMetaFunction(base + 1, owns_->bkz_pool()->kInnerNewindex, 0, mm, 2,
                                          key, value)) {
                map->RawPut(key, value, owns_);
            }
        }
    } else if (NyUDO *udo = ob->ToUDO()) {
        // base + 1 for skip map position
        if (!InternalCallMetaFunction(base + 1, owns_->bkz_pool()->kInnerNewindex, 0, mm, 2, key,
                                      value)) {
            udo->RawPut(key, value, owns_);
        }
    } else {
        Raisef("incorrect type for setfield. %s", mm->ToString(owns_)->bytes());
        return -1;
    }
    return 0;
}
    
NyUDO *NyThread::InternalNewUdo(Object **args, int32_t n_args, size_t n_fields, NyMap *clazz) {
    size_t size = NyUDO::RequiredSize(n_fields);
    NyUDO *udo = owns_->factory()->NewUninitializedUDO(size, clazz, false);
    ::memset(udo->data(), 0, size - sizeof(NyUDO));
    *args = udo; // protected for gc.
    
    Object *vv = clazz->RawGet(owns_->bkz_pool()->kInnerInit, owns_);
    if (NyRunnable *init = NyRunnable::Cast(vv)) {
        Object **base = args;
        if (n_args >= 0) {
            stack_tp_ = base + 1 + n_args;
        }
        if (n_args < 0) {
            n_args = static_cast<int32_t>(stack_tp_ - base - 1);
        }
        size_t base_p = base - stack_;
        CheckStack(base_p + n_args + 2);
        base = stack_ + base_p;
        Run(init, base, n_args + 1, 0/*nrets*/, frame_->env());
    }
    return udo;
}
    
int NyThread::InternalCallMetaFunction(Object **base, Object *a1, Object *a2, int wanted,
                                       NyString *name, bool *has) {
    NyRunnable *fn = nullptr;
    switch (a1->GetType()) {
        case kTypeMap:
        case kTypeUdo:
            fn = a1->ToHeapObject()->GetValidMetaFunction(name, owns_);
            break;
        default:
            *has = false;
            break;
    }
    if (!fn) {
        return 0;
    }
    *has = true;
    stack_tp_ = base;
    size_t tp = stack_tp_ - stack_;
    Push(fn);
    Push(a1);
    Push(a2);
    return InternalCall(stack_ + tp, 2, wanted);
}

bool NyThread::InternalCallMetaFunction(Object **base, NyString *name, int wanted, Object *a1,
                                        int n, ...) {
    DCHECK_GE(wanted, 0);
    NyRunnable *fn = nullptr;
    switch (a1->GetType()) {
        case kTypeMap:
        case kTypeUdo:
            fn = a1->ToHeapObject()->GetValidMetaFunction(name, owns_);
            break;
        default:
            break;
    }
    if (!fn) {
        return false;
    }
    stack_tp_ = base;
    size_t tp = base - stack_;
    CheckStack(base - stack_ + n + 1);
    stack_tp_[0] = fn;
    stack_tp_[1] = a1;
    va_list ap;
    va_start(ap, n);
    for (int i = 0; i < n; ++i) {
        stack_tp_[i + 2] = va_arg(ap, Object *);
    }
    va_end(ap);
    stack_tp_ += n + 2;
    InternalCall(stack_ + tp, n + 1, wanted);
    return true;
}

#if 0
int NyThread::InternalCall(Object **base, int32_t nargs, int32_t wanted) {
    DCHECK((*base)->IsObject());
    //PrintStack();
    size_t base_p = base - stack_;
    NyObject *ob = static_cast<NyObject *>(*base);
    
    if (n_args < 0) {
        n_args = static_cast<int32_t>(stack_tp_ - base - 1);
    }
    switch (ob->GetType()) {
        case kTypeClosure: {
            NyClosure *callee = ob->ToClosure();
            int adjust = 0;
            if (callee->proto()->vargs()) {
                int n_params = callee->proto()->n_params();
                if (n_args > n_params) {
                    adjust = n_args - n_params;
                }
                // a0, a1, a2, a3, a4: n_params = 2, adjust = 3;
                // a2, a3, a4, a0, a1
                if (adjust > 0 && n_params > 0) {
                    int fixed_size = n_params;
                    Object **fixed = new Object *[fixed_size];
                    ::memcpy(fixed, base + 1, sizeof(Object *) * fixed_size);
                    ::memmove(base + 1, base + 1 + fixed_size, sizeof(Object *) * adjust);
                    ::memcpy(base + 1 + adjust, fixed, sizeof(Object *) * fixed_size);
                    delete[] fixed;
                }
            }

            CallFrame *frame = new CallFrame;
            frame->Enter(this, callee,
                         callee->proto()->bcbuf(),
                         callee->proto()->const_pool(),
                         wanted,
                         base_p + 1, /*frame_bp*/
                         base_p + 1 + callee->proto()->max_stack(), /*frame_tp*/
                         frame_->env());
            frame->AdjustBP(adjust);
            return Run();
        } break;

        case kTypeDelegated: {
            NyDelegated *callee = ob->ToDelegated();
            CallFrame *frame = new CallFrame;
            frame->Enter(this, callee,
                         nullptr, /* bc buf */
                         nullptr, /* const pool */
                         wanted,
                         base_p + 1, /*frame_bp*/
                         base_p + 20, /* frame_tp */
                         frame_->env());
            FunctionCallbackInfo<Object> info(base, n_args, owns_->stub());
            //PrintStack();
            callee->Apply(info);
            int rv = frame->nrets();
            if (rv >= 0) {
                CopyResult(stack_ + frame->stack_be() - 1, rv, wanted);
            }
            frame->Exit(this);
            delete frame;

            owns_->GarbageCollectionSafepoint(__FILE__, __LINE__);
            //PrintStack();
            return rv;
        } break;

        default: {
            // TODO: call metatable method.
            //DLOG(FATAL) << "TODO:" << ob->GetType();
            
            NyString *s = ob->ToString(owns_);
            Raisef("bad calling: %s", s->bytes());
            
        } break;
    }
    return 0;
}
#endif
    
void NyThread::RuntimeNewMap(int32_t ra, int32_t n, int32_t p, uint32_t seed) {
    bool clazz = p < 0;
    bool linear = clazz ? false : p;
    uint32_t capacity = (linear ? n : n / 2) + 4;
    if (capacity < 8) {
        capacity = 8;
    }
    // linear > 0 : linear map
    //        < 0 : class metatable
    //       == 0 : normal map
    uint64_t kid = p < 0 ? owns_->GenerateUdoKid() : 0;
    NyMap *ob = owns_->factory()->NewMap(capacity, seed, kid/*kid*/, linear, false/*old*/);
    Object **base = frame_bp() + ra;
    if (linear) {
        for (int i = 0; i < n; ++i) {
            ob->RawPut(NyInt32::New(i), base[i], owns_);
        }
    } else {
        for (int i = 0; i < n; i += 2) {
            ob->RawPut(base[i], base[i + 1], owns_);
        }
    }
    if (clazz) { ProcessClass(ob); }
    Set(ra, ob);
}

NyUDO *NyThread::RuntimePrepareNew(int32_t val, int32_t type, int32_t *nargs, NyRunnable **init) {
    NyMap *clazz = NyMap::Cast(Get(type));
    if (!clazz) {
        owns_->Raisef("new non-class.");
        return nullptr;
    }
    Object *n_fields = clazz->RawGet(owns_->bkz_pool()->kInnerSize, owns_);
    if (n_fields == Object::kNil || !n_fields->IsSmi()) {
        owns_->Raisef("incorrect class table: error __size__.");
        return nullptr;
    }
    
    size_t size = NyUDO::RequiredSize(n_fields->ToSmi());
    NyUDO *udo = owns_->factory()->NewUninitializedUDO(size, clazz, false);
    ::memset(udo->data(), 0, size - sizeof(NyUDO));
    Set(type, udo); // protected for gc.
    
    Object **base = frame_bp() + type;
    if (*nargs >= 0) {
        stack_tp_ = base + 1 + *nargs;
    }
    if (*nargs < 0) {
        *nargs = static_cast<int32_t>(stack_tp_ - base - 1);
    }
    CheckStack((base - stack_) + *nargs + 2);
    *init = NyRunnable::Cast(clazz->RawGet(owns_->bkz_pool()->kInnerInit, owns_));
    return udo;
}
    
int NyThread::RuntimePrepareCall(int32_t callee, int32_t argc, int wanted) {
    //PrintStack();
    //printf("frame: %p\n", frame_bp());
    Object *val = Get(callee);
    if (val->IsSmi()) {
        owns_->Raisef("can not call number.");
        return -1;
    }
    Object **base = frame_bp() + callee;
    if (argc >= 0) {
        stack_tp_ = base + 1 + argc;
    }
    return PrepareCall(base, argc, wanted);
}

int NyThread::RuntimeRet(int32_t base, int32_t nrets) {
    if (nrets < 0) {
        nrets = static_cast<int32_t>(stack_tp_ - (frame_bp() + base));
    } else {
        stack_tp_ = (frame_bp() + base) + nrets;
    }
    frame_->set_nrets(nrets);
    CopyResult(stack_ + frame_->stack_be() - 1, nrets, frame_->wanted());
    auto outter = frame_;
    outter->Exit(this);
    delete outter;
    return nrets;
}

void NyThread::RuntimeSaveNativeStack(Address nast_tp) {
    //nast_tp += 16;
    if (save_point_->nast_tp()) {
        DCHECK_EQ(save_point_->nast_tp(), nast_tp);
    } else {
        save_point_->set_nast_tp(DCHECK_NOTNULL(nast_tp));
    }
    DCHECK_LT(save_point_->nast_tp(), save_point_->nast_bp());
    nast_bk_size_ = save_point_->nast_size();
    //DCHECK_EQ(0, nast_bk_size_ % arch::kNativeStackAligment);
    ::free(nast_bk_);
    nast_bk_ = static_cast<Address>(::malloc(nast_bk_size_));
    ::memcpy(nast_bk_, save_point_->nast_tp(), nast_bk_size_);

//    for (Address i = save_point_->nast_tp(); i < save_point_->nast_bp(); i += kPointerSize) {
//        printf("[nast:%p] %p\n", i, *reinterpret_cast<void **>(i));
//    }

    std::vector<size_t> off_vec;
    Address tp = save_point_->nast_tp();
    while(tp < save_point_->nast_bp()) {
        Address p = *reinterpret_cast<Address *>(tp);
        size_t offset = p - save_point_->nast_tp();
        //size_t index = (save_point_->nast_bp() - p) / kPointerSize - 1;
        //printf("<find> offset: 0x%zx\n", offset);
        tp = p;
        off_vec.push_back(offset);
    }
    
    uintptr_t *bk = reinterpret_cast<uintptr_t *>(nast_bk_);
    bk[0] = off_vec[0];
    for (size_t i = 1; i < off_vec.size(); ++i) {
        bk = reinterpret_cast<uintptr_t *>(nast_bk_ + off_vec[i - 1]);
        *bk = off_vec[i] - off_vec[i - 1];
    }

//    for (Address i = nast_bk_; i < nast_bk_ + nast_bk_size_; i += kPointerSize) {
//        printf("[bk:%p] %p\n", i, *reinterpret_cast<void **>(i));
//    }
}

int NyThread::PrepareCall(Object **base, int32_t nargs, int32_t wanted) {
    DCHECK((*base)->IsObject());
    size_t base_p = base - stack_;
    NyObject *ob = static_cast<NyObject *>(*base);
    if (nargs < 0) {
        nargs = static_cast<int32_t>(stack_tp_ - base - 1);
    }
    switch (ob->GetType()) {
        case kTypeClosure: {
            NyClosure *callee = ob->ToClosure();
            int adjust = 0;
            if (callee->proto()->vargs()) {
                int nparams = callee->proto()->n_params();
                if (nargs > nparams) {
                    adjust = nargs - nparams;
                }
                // a0, a1, a2, a3, a4: n_params = 2, adjust = 3;
                // a2, a3, a4, a0, a1
                if (adjust > 0 && nparams > 0) {
                    int fixed_size = nparams;
                    Object **fixed = new Object *[fixed_size];
                    ::memcpy(fixed, base + 1, sizeof(Object *) * fixed_size);
                    ::memmove(base + 1, base + 1 + fixed_size, sizeof(Object *) * adjust);
                    ::memcpy(base + 1 + adjust, fixed, sizeof(Object *) * fixed_size);
                    delete[] fixed;
                }
            }
            
            CallFrame *frame = new CallFrame;
            NyByteArray *bcbuf =
                callee->proto()->IsInterpretationExec() ? callee->proto()->bcbuf(): nullptr;
            frame->Enter(this, callee,
                         bcbuf,
                         callee->proto()->const_pool(),
                         wanted,
                         base_p + 1, /*frame_bp*/
                         base_p + 1 + callee->proto()->max_stack(), /*frame_tp*/
                         frame_->env());
            frame->AdjustBP(adjust);
        } break;
            
        case kTypeDelegated: {
            NyDelegated *callee = ob->ToDelegated();
            CallFrame *frame = new CallFrame;
            frame->Enter(this, callee,
                         nullptr, /* bc buf */
                         nullptr, /* const pool */
                         wanted,
                         base_p + 1, /*frame_bp*/
                         base_p + 20, /* frame_tp */
                         frame_->env());
        } break;

        default:
            DLOG(FATAL) << "incorrect type: " << kBuiltinTypeName[ob->GetType()];
            break;
    }
    return nargs;
}
    
int NyThread::FinializeCall(Object **base, int32_t nargs, int32_t wanted) {
    Object *ob = *base;
    switch (ob->GetType()) {
        case kTypeClosure: {
            NyClosure *callee = NyClosure::Cast(ob);
            DCHECK_EQ(callee, frame_->callee());
            if (callee->proto()->IsNativeExec()) {
                EntryTrampolineCallStub stub(owns_->code_pool()->kEntryTrampoline);
                CodeContextBundle bundle(this);
                return stub.entry_fn()(this, callee->proto()->code(), owns_, &bundle, nullptr);
            } else {
                DCHECK(callee->proto()->IsInterpretationExec());
                return Run();
            }
        } break;
        case kTypeDelegated: {
            NyDelegated *callee = NyDelegated::Cast(ob);
            DCHECK_EQ(callee, frame_->callee());
            FunctionCallbackInfo<Object> info(base, nargs, owns_->stub());
            //PrintStack();
            callee->Apply(info);
            if (owns_->stub()->exec() == Nyaa::kAOT) {
                if (interruption_pending_ == CallFrame::kYield) {
                    return frame_->nrets();
                }
            }

            CallFrame *outter = frame_;
            int rv = outter->nrets();
            if (rv >= 0) {
                CopyResult(stack_ + outter->stack_be() - 1, rv, wanted);
            }
            outter->Exit(this);
            delete outter;
            
            owns_->GarbageCollectionSafepoint(__FILE__, __LINE__);
            return rv;
        } break;
        default: {
            
        } break;
    }
    return 0;
}

void NyThread::CopyResult(Object **ret, int nrets, int wanted) {
    switch (wanted) {
        case -1:
            wanted = nrets;
            break;
        case 0:
            stack_tp_ = ret;
            return;
        case 1:
            if (nrets == 0) {
                ret[0] = Object::kNil;
            } else {
                ret[0] = *(stack_tp_ - nrets);
            }
            stack_tp_ = ret + 1;
            return;
        default:
            break;
    }
    Object **from = stack_tp_ - nrets;
    for (int i = 0; i < nrets && i < wanted; ++i) {
        ret[i] = from[i];
    }
    for (int i = nrets; i < wanted; ++i) {
        ret[i] = Object::kNil;
    }
    stack_tp_ = ret + wanted;
}

void NyThread::ProcessClass(NyMap *clazz) {
    int64_t self_offset = 0;
    int64_t self_size = 0;

    auto base = clazz->RawGet(owns_->bkz_pool()->kInnerBase, owns_);
    if (NyMap *base_map = NyMap::Cast(base)) {
        auto size = base_map->RawGet(owns_->bkz_pool()->kInnerSize, owns_);
        self_size = size->ToSmi();
        self_offset = self_size;
    }
    
    NyMap::Iterator iter(clazz);
    for (iter.SeekFirst(); iter.Valid(); iter.Next()) {
        DCHECK_NE(Object::kNil, iter.key());
        NyString *key = NyString::Cast(iter.key());
        //printf("field:%s\n", key->bytes());
        if (key && key->bytes()[0] != '_' && key->bytes()[0] != '$') {
            if (!iter.value()->IsSmi()) {
                continue;
            }

            int64_t access_mask = iter.value()->ToSmi() & 0x3;
            int64_t offset = (iter.value()->ToSmi() & ~0x3) >> 2;
            Object *tag = NySmi::New(((self_offset + offset) << 2) | access_mask);
            clazz->RawPut(key, tag, owns_);
            
            ++self_size;
        }
    }
    
    clazz->RawPut(owns_->bkz_pool()->kInnerOffset, NySmi::New(self_offset), owns_);
    clazz->RawPut(owns_->bkz_pool()->kInnerSize, NySmi::New(self_size), owns_);
}

int NyThread::ParseBytecodeInt32Params(int offset, int scale, int n, ...) {
    va_list ap;
    va_start(ap, n);
    for (int i = 0; i < n; ++i) {
        int32_t *p = va_arg(ap, int32_t *);
        View<Byte> slice = frame_->bcbuf()->GetView(frame_->pc() + offset, scale);
        *p = Bytecode::ParseInt32Param(slice, scale);
        offset += scale;
    }
    va_end(ap);
    return offset;
}

int NyThread::ParseBytecodeSize(int offset) {
    Bytecode::ID first = static_cast<Bytecode::ID>(frame_->bcbuf()->Get(offset));
    int scale = 0;
    int size = 0;
    switch (first) {
        case Bytecode::kDouble:
            scale = 2;
            size = 2;
            first = static_cast<Bytecode::ID>(frame_->bcbuf()->Get(offset + 1));
            break;
        case Bytecode::kQuadruple:
            scale = 4;
            size = 2;
            first = static_cast<Bytecode::ID>(frame_->bcbuf()->Get(offset + 1));
            break;
        default:
            scale = 1;
            size = 1;
            break;
    }
#define DEFINE_SIZE(name, n, ...) \
    case Bytecode::k##name: size += (n) * scale; break;
    switch (first) {
        DECL_BYTECODES(DEFINE_SIZE)
        default:
            DLOG(FATAL) << "incorrect bytecode: " << first;
            break;
    }
#undef DEFINE_SIZE
    return size;
}
    
int NyThread::GetLine(const NyInt32Array *line_info, int pc) const {
    if (!owns_->stub()->compact_source_line_info()) {
        return line_info->Get(pc);
    }
    for (int i = 0; i < line_info->size(); i += 3) {
        if (pc >= line_info->Get(i) && pc < line_info->Get(i + 1)) {
            return line_info->Get(i + 2);
        }
    }
    DLOG(FATAL) << "Noreached!";
    return -1;
}

} // namespace nyaa
    
} // namespace mai
