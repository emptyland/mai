#include "nyaa/high-level-ir.h"

namespace mai {
    
namespace nyaa {

namespace hir {

const char *Type::kNames[] = {
#define DEFINE_NAME(name, literal) literal,
    DECL_DAG_TYPES(DEFINE_NAME)
#undef DEFINE_NAME
};
    
void Function::PrintTo(std::string *buf, size_t limit) const {
    buf->resize(limit);
    FILE *fp = ::fmemopen(&(*buf)[0], limit, "w");
    if (!fp) {
        return;
    }
    PrintTo(fp);
    buf->resize(::ftell(fp));
    ::fclose(fp);
}
    
void BasicBlock::PrintTo(FILE *fp) const {
    fprintf(fp, "l%d:\n", label_);
    if (!in_edges_.empty()) {
        fprintf(fp, "[in]: ");
        for (auto edge : in_edges_) {
            fprintf(fp, "l%d ", edge->label_);
        }
        putc('\n', fp);
    }
    if (!out_edges_.empty()) {
        fprintf(fp, "[out]: ");
        for (auto edge : out_edges_) {
            fprintf(fp, "l%d ", edge->label_);
        }
        putc('\n', fp);
    }
    
    Value *inst = root_;
    while (inst) {
        fprintf(fp, "    ");
        inst->PrintTo(fp);
        fprintf(fp, "; line = %d\n", inst->line());
        inst = inst->next();
    }
}
    
/*virtual*/ void Constant::PrintOperator(FILE *fp) const {
    ::fprintf(fp, "%s ", Type::kNames[type()]);
    switch (type()) {
        case Type::kInt:
            ::fprintf(fp, "%lld", smi_);
            break;
        case Type::kFloat:
            ::fprintf(fp, "%f", f64_);
            break;
        case Type::kString:
            ::fprintf(fp, "\'%s\'", str_->data());
            break;
        default:
            DLOG(FATAL) << "Noreached!" << Type::kNames[type()];
            break;
    }
}
    
/*virtual*/ void Branch::PrintOperator(FILE *fp) const {
    fprintf(fp, "br ");
    cond_->PrintValue(fp);
    fprintf(fp, " then l%d", if_true_->label());
    if (if_false_) {
        fprintf(fp, " else l%d", if_false_->label());
    }
}
    
/*virtual*/ void Phi::PrintOperator(FILE *fp) const {
    fprintf(fp, "phi ");
    for (const auto &path : incoming_) {
        fprintf(fp, "[l%d ", path.incoming_bb->label());
        path.incoming_value->PrintValue(fp);
        fprintf(fp, "] ");
    }
}

} // namespace hir

} // namespace nyaa
    
} // namespace mai
