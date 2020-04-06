#include "lang/syntax.h"

namespace mai {

namespace lang {

extern void InitializeASTNumberCastHintTable();
extern void FreeASTNumberCastHintTable();

extern void InitializeASTCompilingAttributesTable();
extern void FreeASTCompilingAttributesTable();

extern void InitializeTokenKeywordTable();
extern void FreeTokenKeywordTable();

void InitializeSyntaxLibrary() {
    InitializeASTNumberCastHintTable();
    InitializeASTCompilingAttributesTable();
    InitializeTokenKeywordTable();
}

void FreeSyntaxLibrary() {
    FreeTokenKeywordTable();
    FreeASTCompilingAttributesTable();
    FreeASTNumberCastHintTable();
}

} // namespace lang

} // namespace mai
