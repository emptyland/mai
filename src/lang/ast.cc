#include "lang/ast.h"
#include "lang/token.h"

namespace mai {

namespace lang {

TypeSign::TypeSign(int position, const ASTString *symbol)
    : ASTNode(position, kTypeSign)
    , kind_(Token::kIdentifier)
    , symbol_(symbol) {
}

TypeSign::TypeSign(int position, FunctionPrototype *prototype)
    : ASTNode(position, kTypeSign)
    , kind_(Token::kFun)
    , prototype_(prototype) {
}

} // namespace lang

} // namespace mai
