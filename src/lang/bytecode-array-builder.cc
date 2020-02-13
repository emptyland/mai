#include "lang/bytecode-array-builder.h"
#include "base/slice.h"

namespace mai {

namespace lang {

void BytecodeArrayBuilder::Print(base::AbstractPrinter *output, bool ownership) const {
    for (auto node : nodes_) {
        output->Append("    ");
        node->Print(output);
        output->Append(";\n");
    }
    if (ownership) {
        delete output;
    }
}

} // namespace lang

} // namespace mai
