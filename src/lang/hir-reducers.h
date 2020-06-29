#ifndef MAI_LANG_HIR_REDUCERS_H_
#define MAI_LANG_HIR_REDUCERS_H_

#include "lang/hir-reducer.h"
#include "lang/hir.h"

namespace mai {

namespace lang {

class SimplifiedElimination final : public AdvancedReducer {
public:
    explicit SimplifiedElimination(Editor *editor, HGraph *graph, HOperatorFactory *ops)
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

} // namespace lang

} // namespace mai

#endif // MAI_LANG_HIR_REDUCERS_H_
