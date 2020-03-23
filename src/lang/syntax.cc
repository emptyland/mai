#include "lang/syntax.h"

namespace mai {

namespace lang {

extern void InitializeASTNumberCastHintTable();
extern void FreeASTNumberCastHintTable();

extern void InitializeTokenKeywordTable();
extern void FreeTokenKeywordTable();

void InitializeSyntaxLibrary() {
    InitializeASTNumberCastHintTable();
    InitializeTokenKeywordTable();
}

void FreeSyntaxLibrary() {
    FreeTokenKeywordTable();
    FreeASTNumberCastHintTable();
}

} // namespace lang

} // namespace mai
