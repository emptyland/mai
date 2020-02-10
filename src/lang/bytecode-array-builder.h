#pragma once
#ifndef MAI_LANG_BYTECODE_ARRAY_BUILDER_H_
#define MAI_LANG_BYTECODE_ARRAY_BUILDER_H_

#include "lang/bytecode.h"
#include "base/arena-utils.h"

namespace mai {

namespace lang {

class BytecodeLabel {
public:
    BytecodeLabel() {}
    ~BytecodeLabel() { DCHECK(is_bind_) << "Not bind yet!"; }
    
    DEF_VAL_PROP_RW(bool, is_bind);
    DEF_VAL_PROP_RW(int, pc);
    const std::vector<BytecodeNode *> &unlinked_nodes() const { return unlinked_nodes_; }
    
    void Associate(BytecodeNode *bc) { unlinked_nodes_.push_back(bc); }
    
    void Bind(int pc) {
        pc_ = pc;
        unlinked_nodes_.clear();
        is_bind_ = true;
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
        , nodes_(arena) {}

    template<BytecodeID ID>
    inline void Add() { nodes_.push_back(Bytecodes<ID>::New(arena_)); }
   
    template<BytecodeID ID>
    inline void Add(int a) { nodes_.push_back(Bytecodes<ID>::New(arena_, a)); }

    template<BytecodeID ID>
    inline void Add(int a, int b) { nodes_.push_back(Bytecodes<ID>::New(arena_, a, b)); }
    
    void Goto(BytecodeLabel *label) {
        if (label->is_bind()) {
            Add<kGoto>(label->pc());
            return;
        }
        BytecodeNode *node = Bytecodes<kGoto>::New(arena_, pc());
        label->Associate(node);
        nodes_.push_back(node);
    }
    
    void GotoIfTrue(BytecodeLabel *label) {
        if (label->is_bind()) {
            Add<kGotoIfTrue>(label->pc());
            return;
        }
        BytecodeNode *node = Bytecodes<kGotoIfTrue>::New(arena_, pc());
        label->Associate(node);
        nodes_.push_back(node);
    }
    
    void GotoIfFalse(BytecodeLabel *label) {
        if (label->is_bind()) {
            Add<kGotoIfFalse>(label->pc());
            return;
        }
        BytecodeNode *node = Bytecodes<kGotoIfFalse>::New(arena_, pc());
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
        std::vector<BytecodeInstruction> instrs;
        instrs.reserve(nodes_.size());
        for (BytecodeNode *node : nodes_) {
            instrs.push_back(node->To());
        }
        return instrs;
    }
    
    void Abandon() { nodes_.clear(); }
    
    DISALLOW_IMPLICIT_CONSTRUCTORS(BytecodeArrayBuilder);
private:
    void Link(BytecodeNode *bc, int pc) {
        switch (bc->id()) {
            case kGoto:
            case kGotoIfTrue:
            case kGotoIfFalse:
                bc->params_[0] = pc;
                break;
            default:
                NOREACHED();
                break;
        }
    }
    
    base::ArenaVector<BytecodeNode *> nodes_;
    base::Arena *arena_;
}; // class BytecodeArrayBuilder

} // namespace lang

} // namespace mai

#endif // MAI_LANG_BYTECODE_ARRAY_BUILDER_H_
