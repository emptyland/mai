#pragma once
#ifndef MAI_LANG_BYTECODE_ARRAY_BUILDER_H_
#define MAI_LANG_BYTECODE_ARRAY_BUILDER_H_

#include "lang/bytecode.h"
#include "lang/mm.h"
#include "base/arena-utils.h"

namespace mai {

namespace lang {

class BytecodeLabel {
public:
    BytecodeLabel() {}
    ~BytecodeLabel() { /*DCHECK(!unlinked_nodes_.empty() || !is_bind_);*/ }
    
    DEF_VAL_PROP_RW(bool, is_bind);
    DEF_VAL_PROP_RW(int, pc);
    const std::vector<BytecodeNode *> &unlinked_nodes() const { return unlinked_nodes_; }
    
    void Associate(BytecodeNode *bc) { unlinked_nodes_.push_back(bc); }
    
    void Bind(int pc) {
        pc_ = pc;
        unlinked_nodes_.clear();
        is_bind_ = true;
    }
    
    void Clear() {
        pc_ = -1;
        is_bind_ = false;
        unlinked_nodes_.clear();
    }

    DISALLOW_IMPLICIT_CONSTRUCTORS(BytecodeLabel);
private:
    int pc_ = -1;
    bool is_bind_ = false;
    std::vector<BytecodeNode *> unlinked_nodes_;
}; // class BytecodeLabel

class BytecodeArrayBuilder final {
public:
    BytecodeArrayBuilder(base::Arena *arena)
        : arena_(DCHECK_NOTNULL(arena))
        , nodes_(arena)
        , incomplete_(arena) {}

    template<BytecodeID ID>
    inline void Add() { nodes_.push_back(Bytecodes<ID>::New(arena_)); }
   
    template<BytecodeID ID>
    inline void Add(int a) {
        DCHECK_GE(a, 0);
        nodes_.push_back(Bytecodes<ID>::New(arena_, a));
    }

    template<BytecodeID ID>
    inline void Add(int a, int b) {
        DCHECK_GE(a, 0);
        DCHECK_GE(b, 0);
        nodes_.push_back(Bytecodes<ID>::New(arena_, a, b));
    }
    
    template<BytecodeID ID>
    inline void Incomplete(int a) {
        BytecodeNode *node = Bytecodes<ID>::New(arena_, a);
        nodes_.push_back(node);
        incomplete_.push_back(node);
    }

    template<BytecodeID ID>
    inline void Incomplete(int a, int b) {
        BytecodeNode *node = Bytecodes<ID>::New(arena_, a, b);
        nodes_.push_back(node);
        incomplete_.push_back(node);
    }

//    void Goto(BytecodeLabel *label, int slot) {
//        if (label->is_bind()) {
//            Add<kGoto>(slot, label->pc());
//            return;
//        }
//        BytecodeNode *node = Bytecodes<kGoto>::New(arena_, slot, pc());
//        label->Associate(node);
//        nodes_.push_back(node);
//    }
//
//    void GotoIfTrue(BytecodeLabel *label, int slot) {
//        if (label->is_bind()) {
//            Add<kGotoIfTrue>(slot, label->pc());
//            return;
//        }
//        BytecodeNode *node = Bytecodes<kGotoIfTrue>::New(arena_, slot, pc());
//        label->Associate(node);
//        nodes_.push_back(node);
//    }
//
//    void GotoIfFalse(BytecodeLabel *label, int slot) {
//        if (label->is_bind()) {
//            Add<kGotoIfFalse>(slot, label->pc());
//            return;
//        }
//        BytecodeNode *node = Bytecodes<kGotoIfFalse>::New(arena_, slot, pc());
//        label->Associate(node);
//        nodes_.push_back(node);
//    }

    void Jump(BytecodeLabel *label, int slot) {
        if (label->is_bind()) {
            DCHECK_GT(pc(), label->pc());
            Add<kBackwardJump>(slot, pc() - label->pc());
            return;
        }
        BytecodeNode *node = Bytecodes<kForwardJump>::New(arena_, slot, pc());
        label->Associate(node);
        nodes_.push_back(node);
    }

    void JumpIfTrue(BytecodeLabel *label, int slot) {
        if (label->is_bind()) {
            Add<kBackwardJumpIfTrue>(slot, label->pc());
            return;
        }
        BytecodeNode *node = Bytecodes<kForwardJumpIfTrue>::New(arena_, slot, pc());
        label->Associate(node);
        nodes_.push_back(node);
    }

    void JumpIfFalse(BytecodeLabel *label, int slot) {
        if (label->is_bind()) {
            Add<kBackwardJumpIfFalse>(slot, label->pc());
            return;
        }
        BytecodeNode *node = Bytecodes<kForwardJumpIfFalse>::New(arena_, slot, pc());
        label->Associate(node);
        nodes_.push_back(node);
    }

    void Bind(BytecodeLabel *label) {
        DCHECK(!label->is_bind());
        for (BytecodeNode *node : label->unlinked_nodes()) {
            Link(node, pc());
        }
        label->Bind(pc());
    }
    
    int pc() const { return static_cast<int>(nodes_.size()); }

    std::vector<BytecodeInstruction> Build() const {
        DCHECK(incomplete_.empty());
        std::vector<BytecodeInstruction> instrs;
        instrs.reserve(nodes_.size());
        for (BytecodeNode *node : nodes_) {
            instrs.push_back(node->To());
        }
        return instrs;
    }
    
    std::vector<BytecodeInstruction> BuildWithRewrite(std::function<int(int)> callback) {
        for (auto node : incomplete_) {
            for (int i = 0; i < 4; i++) {
                node->set_param(i, callback(node->param(i)));
            }
        }
        incomplete_.clear();
        return Build();
    }
    
    void Abandon() { nodes_.clear(); }
    
    void Print(base::AbstractPrinter *output, bool ownership = false) const;
    
    DISALLOW_IMPLICIT_CONSTRUCTORS(BytecodeArrayBuilder);
private:
    void Link(BytecodeNode *bc, int pc) {
        switch (bc->id()) {
            case kForwardJump:
            case kForwardJumpIfTrue:
            case kForwardJumpIfFalse:
                // bc->params_[0]: slot
                DCHECK_GT(pc, bc->params_[1]);
                bc->params_[1] = pc - bc->params_[1];
                break;
            default:
                NOREACHED();
                break;
        }
    }
    
    base::ArenaVector<BytecodeNode *> nodes_;
    base::ArenaVector<BytecodeNode *> incomplete_;
    base::Arena *arena_;
}; // class BytecodeArrayBuilder

} // namespace lang

} // namespace mai

#endif // MAI_LANG_BYTECODE_ARRAY_BUILDER_H_
