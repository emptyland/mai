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

bool TypeSign::Convertible(TypeSign *rhs) const {
    if (id() == Token::kAny) { // Any type!
        return true;
    }
    switch (id()) {
        case Token::kIdentifier:
            return id() == rhs->id() && symbol_->Equal(rhs->symbol_->data());
        case Token::kClass:
            return id() == rhs->id() && clazz() == rhs->clazz();
        case Token::kFun: {
            if (!prototype()->return_type()->Convertible(rhs->prototype()->return_type()) ||
                prototype()->parameters_size() != rhs->prototype()->parameters_size() ||
                prototype()->vargs() != rhs->prototype()->vargs()) {
                return false;
            }
            for (size_t i = 0; i < prototype()->parameters_size(); i++) {
                if (!prototype()->parameter(i).type->Convertible(rhs->prototype()->parameter(i).type)) {
                    return false;
                }
            }
            return true;
        } break;
        case Token::kArray:
        case Token::kMutableArray:
            return rhs->id() == id() && parameter(0)->Convertible(rhs->parameter(0));
        case Token::kMap:
        case Token::kMutableMap:
            return rhs->id() == id() && parameter(0)->Convertible(rhs->parameter(0)) &&
                   parameter(1)->Convertible(rhs->parameter(1));
        case Token::kInt:
            return rhs->id() == Token::kInt || rhs->id() == Token::kI32;
        case Token::kUInt:
            return rhs->id() == Token::kUInt || rhs->id() == Token::kU32;
        default:
            return id() == rhs->id();
    }
}

ClassDefinition::ClassDefinition(base::Arena *arena,
                                 int position,
                                 const ASTString *name,
                                 base::ArenaVector<Parameter> &&constructor,
                                 base::ArenaVector<Field> &&fields,
                                 const ASTString *base_name,
                                 base::ArenaVector<Expression *> &&arguments)
    : Definition(position, kClassDefinition, name)
    , parameters_(constructor)
    , named_parameters_(arena)
    , fields_(fields)
    , named_fields_(arena)
    , base_name_(base_name)
    , arguments_(arguments)
    , methods_(arena)
    , named_methods_(arena) {
}

int ClassDefinition::MakeParameterLookupTable() {
    for (size_t i = 0; i < parameters_size(); i++) {
        Parameter param = parameter(i);
        const ASTString *name = nullptr;
        if (param.field_declaration) {
            name = field(param.as_field).declaration->identifier();
        } else {
            name = param.as_parameter.name;
        }
        if (auto iter = named_parameters_.find(name->ToSlice()); iter != named_parameters_.end()) {
            return -static_cast<int>(i + 1);
        }
        named_parameters_[name->ToSlice()] = i;
    }
    return 0;
}

} // namespace lang

} // namespace mai
