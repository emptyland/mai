#ifndef MAI_LANG_HIR_REDUCERS_H_
#define MAI_LANG_HIR_REDUCERS_H_

#include "lang/hir-reducer.h"
#include "lang/hir.h"

namespace mai {

namespace lang {

class SimplifiedElimination final : public AdvancedReducer {
public:
    SimplifiedElimination(Editor *editor, HGraph *graph, HOperatorFactory *ops)
        : AdvancedReducer(editor)
        , graph_(graph)
        , ops_(ops) {}
    ~SimplifiedElimination() override;
private:
    const char *GetReducerName() const final { return "simplified-elimination"; }
    Reduction Reduce(HNode *node) final;
    void Finalize() final;
    
    HGraph *const graph_;
    HOperatorFactory *const ops_;
}; // class SimplifiedElimination

class NilUncheckedLowering final : public AdvancedReducer {
    NilUncheckedLowering(Editor *editor, HGraph *graph, HOperatorFactory *ops);
    ~NilUncheckedLowering() override;
private:
    const char *GetReducerName() const final { return "nil-unchecked-lowering"; }
    Reduction Reduce(HNode *node) final;
    void Finalize() final;
    
    bool CanUncheckNil(const HNode *node) {
        auto iter = checked_record_.find(node->vid());
        if (iter == checked_record_.end()) {
            checked_record_[node->vid()] = 0;
            return false;
        }
        return iter->second++ > 0;
    }

    HGraph *const graph_;
    HOperatorFactory *const ops_;
    base::ArenaMap<int, int> checked_record_;
}; // class NilUncheckedLowering

} // namespace lang

} // namespace mai

#endif // MAI_LANG_HIR_REDUCERS_H_
