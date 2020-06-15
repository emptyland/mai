#include "lang/compiler.h"
#include "lang/hir.h"
#include "lang/mm.h"
#include "lang/metadata.h"
#include "base/arena-utils.h"
#include <stack>

namespace mai {

namespace lang {

class HIRGraphGenerator final {
public:
    class Environment;
    
    HIRGraphGenerator(const CompilationInfo *compilation_info, base::Arena *arena);

    void Initialize() {
        for (BytecodeInstruction bc : compilation_info_->linear_path()) {
            BytecodeNode *node = BytecodeNode::From(arena_, bc);
            bc_sequnce_.push_back(node);
        }
        fun_env_.push(compilation_info_->start_fun());
    }
    
    DEF_PTR_GETTER(HGraph, graph);

    void Generate();
private:
    const Function *current_fun() const {
        DCHECK(!fun_env_.empty());
        return fun_env_.top();
    }
    
    BytecodeNode *BC() const { return bc_sequnce_[bc_position_]; }
    
    void VisitBytecode();
    
    int NextNodeId() { return graph_->NextNodeId(); }
    
    HNode *NewNode(const HOperator *op, HType type) {
        return HNode::New(arena_, NextNodeId(), type, op, 0, 0, nullptr);
    }
    
    HNode *NewNode(const HOperator *op, HType type, HNode *a) {
        HNode *inputs[] = {a};
        return HNode::New(arena_, NextNodeId(), type, op, arraysize(inputs), arraysize(inputs),
                          inputs);
    }
    
    HNode *NewNode(const HOperator *op, HType type, HNode *a, HNode *b) {
        HNode *inputs[] = {a, b};
        return HNode::New(arena_, NextNodeId(), type, op, arraysize(inputs), arraysize(inputs),
                          inputs);
    }
    
    HNode *NewNode(const HOperator *op, HType type, HNode *a, HNode *b, HNode *c) {
        HNode *inputs[] = {a, b, c};
        return HNode::New(arena_, NextNodeId(), type, op, arraysize(inputs), arraysize(inputs),
                          inputs);
    }
    
    HNode *NewNode(const HOperator *op, HType type, HNode **inputs, int inputs_size) {
        return HNode::New(arena_, NextNodeId(), type, op, inputs_size, inputs_size, inputs);
    }
    
    const CompilationInfo *const compilation_info_;
    base::Arena *const arena_;
    base::ArenaVector<BytecodeNode *> bc_sequnce_;
    size_t bc_position_ = 0;
    int next_node_id_ = 0;
    std::stack<const Function *> fun_env_;
    HOperatorFactory ops_;
    HGraph *graph_;
    base::ArenaVector<HNode *> exit_control_;
    Environment *env_ = nullptr;
}; // class HIRGraphGenerator


class HIRGraphGenerator::Environment {
public:
    Environment(HIRGraphGenerator *owns, HNode *start)
        : owns_(owns)
        , values_(owns_->arena_)
        , control_dependency_(start)
        , effect_dependency_(start)
        , arguments_(owns_->arena_) {
    }

    DEF_PTR_PROP_RW(HNode, accumulator);
    
    void SetAccumulator(HNode *node) {
        DCHECK(accumulator() == nullptr);
        set_accumulator(node);
    }

    HNode *TakeAccumulator() {
        HNode *acc = DCHECK_NOTNULL(accumulator());
        set_accumulator(nullptr);
        return acc;
    }
    
    void BindRegister(int reg, HNode *value) {
        values_[reg] = value;
        
        // Record arguments
        int off = (reg - kParameterSpaceOffset) * kStackOffsetGranularity;
        if (off > owns_->current_fun()->stack_size()) {
            arguments_[reg] = value;
        }
    }
    
    HNode *FindValue(int reg) {
        auto iter = values_.find(reg);
        DCHECK(iter != values_.end());
        return iter->second;
    }
    
    void ClearArguments() { arguments_.clear(); }
private:
    HIRGraphGenerator *const owns_;
    HNode *control_dependency_;
    HNode *effect_dependency_;
    HNode *accumulator_;
    base::ArenaMap<int, HNode *> values_;
    base::ArenaMap<int, HNode *> arguments_;
}; // class HIRGraphGenerator::Environment


HIRGraphGenerator::HIRGraphGenerator(const CompilationInfo *compilation_info, base::Arena *arena)
    : compilation_info_(compilation_info)
    , arena_(arena)
    , bc_sequnce_(arena)
    , ops_(arena)
    , exit_control_(arena)
    , graph_(new (arena) HGraph(arena)){
}

void HIRGraphGenerator::Generate() {
    HNode *start = NewNode(ops_.Start(), HTypes::Void);
    graph_->set_start(start);

    Environment env(this, start);
    env_ = &env;
    for (bc_position_ = 0; bc_position_ < bc_sequnce_.size(); bc_position_++) {
        if (BC()->id() == kCheckStack) {
            auto iter = compilation_info_->invoke_info().find(bc_position_);
            DCHECK(iter != compilation_info_->invoke_info().end());
            fun_env_.push(iter->second.fun);
        }
        VisitBytecode();
        if (BC()->id() == kReturn) {
            fun_env_.pop();
        }
    }
    
    int exit_control_in = static_cast<int>(exit_control_.size());
    HNode **exit_control = &exit_control_[0];
    HNode *end = NewNode(ops_.End(exit_control_in), HTypes::Void, exit_control, exit_control_in);
    graph_->set_end(end);
}

void HIRGraphGenerator::VisitBytecode() {
    switch (BC()->id()) {
        case kCheckStack:
            NewNode(ops_.CheckStack(), HTypes::Void);
            break;
        
        case kLdaTrue: {
            const HOperator *op = ops_.ConstantWord32(1);
            HNode *node = NewNode(op, HTypes::Word32);
            env_->SetAccumulator(node);
        } break;

        case kLdaFalse: {
            const HOperator *op = ops_.ConstantWord32(0);
            HNode *node = NewNode(op, HTypes::Word32);
            env_->SetAccumulator(node);
        } break;
            
        case kLdaZero: {
            const HOperator *op = ops_.ConstantWord64(0);
            HNode *node = NewNode(op, HTypes::Word64);
            env_->SetAccumulator(node);
        } break;
        
        case kStar32:
        case kStar64:
        case kStarPtr: {
            int slot = BC()->param(0);
            env_->BindRegister(slot, env_->TakeAccumulator());
        } break;
            
        case kMove32:
        case kMove64:
        case kMovePtr: {
            HNode *value = env_->FindValue(BC()->param(1));
            env_->BindRegister(BC()->param(0), value);
        } break;
            
        default:
            NOREACHED();
            break;
    }
}


HGraph *GenerateHIRGraph(const CompilationInfo *compilation_info, base::Arena *arena) {
    HIRGraphGenerator g(compilation_info, arena);
    g.Initialize();
    g.Generate();
    return g.graph();
}

} // namespace lang

} // namespace mai

