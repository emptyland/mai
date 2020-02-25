#include "lang/ast.h"
#include "lang/token.h"

namespace mai {

namespace lang {

TypeSign::TypeSign(int position, const ASTString *prefix, const ASTString *name)
    : ASTNode(position, kTypeSign)
    , kind_(Token::kIdentifier)
    , prefix_(prefix)
    , name_(name) {
}

TypeSign::TypeSign(int position, FunctionPrototype *prototype)
    : ASTNode(position, kTypeSign)
    , kind_(Token::kFun)
    , prototype_(DCHECK_NOTNULL(prototype)) {
}

TypeSign::TypeSign(int position, ClassDefinition *clazz)
    : ASTNode(position, kTypeSign)
    , kind_(Token::kClass)
    , clazz_(DCHECK_NOTNULL(clazz)) {
}

TypeSign::TypeSign(int position, ObjectDefinition *object)
    : ASTNode(position, kTypeSign)
    , kind_(Token::kObject)
    , object_(DCHECK_NOTNULL(object)) {
}

TypeSign::TypeSign(int position, InterfaceDefinition *interface)
    : ASTNode(position, kTypeSign)
    , kind_(Token::kInterface)
    , interface_(DCHECK_NOTNULL(interface)) {
}

bool TypeSign::Convertible(TypeSign *rhs) const {
    if (id() == Token::kAny) { // Any type!
        return true;
    }
    switch (id()) {
        case Token::kIdentifier:
            return id() == rhs->id() && ToSymbolString() == rhs->ToSymbolString();
        case Token::kClass: {
            if (id() != rhs->id()) {
                return false;
            }
            return clazz()->BaseOf(rhs->clazz());
        } break;
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

std::string TypeSign::ToSymbolString() const {
    DCHECK_EQ(Token::kIdentifier, kind_);
    std::string buf(!prefix_ ? "" : prefix_->ToString());
    if (prefix_) { buf.append(1, '.'); }
    return buf.append(DCHECK_NOTNULL(name_)->ToString());
}

ClassDefinition::ClassDefinition(base::Arena *arena,
                                 int position,
                                 const ASTString *name,
                                 base::ArenaVector<Parameter> &&constructor,
                                 base::ArenaVector<Field> &&fields,
                                 const ASTString *base_name,
                                 base::ArenaVector<Expression *> &&arguments)
    : StructureDefinition(arena, position, kClassDefinition, name, std::move(fields))
    , parameters_(constructor)
    , named_parameters_(arena)
    , base_name_(base_name)
    , arguments_(arguments) {
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
