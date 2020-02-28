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

TypeSign::TypeSign(int position, int kind, Definition *def)
    : ASTNode(position, kTypeSign)
    , kind_(kind)
    , definition_(DCHECK_NOTNULL(def)) {
    DCHECK((kind == Token::kInterface && def->IsInterfaceDefinition()) ||
           (kind == Token::kClass && def->IsClassDefinition()) ||
           (kind == Token::kObject && def->IsObjectDefinition()) ||
           kind == Token::kRef);
}

#if defined(DEBUG) || defined(_DEBUG)
StructureDefinition *TypeSign::structure() const {
    DCHECK(kind_ == Token::kClass ||
           kind_ == Token::kObject ||
           kind_ == Token::kRef)
        << Token::ToString(static_cast<Token::Kind>(kind_));
    DCHECK(structure_->IsClassDefinition() || structure_->IsObjectDefinition());
    return structure_;
}

Definition *TypeSign::definition() const {
    DCHECK(kind_ == Token::kClass ||
           kind_ == Token::kObject ||
           kind_ == Token::kInterface ||
           kind_ == Token::kRef) << Token::ToString(static_cast<Token::Kind>(kind_));
    DCHECK(definition_->IsClassDefinition() || definition_->IsObjectDefinition() ||
           definition_->IsInterfaceDefinition());
    return definition_;
}
#endif // defined(DEBUG) || defined(_DEBUG)

bool TypeSign::IsSignedIntegral() const {
    switch (id()) {
        case Token::kI8:
        case Token::kI16:
        case Token::kI32:
        case Token::kInt:
        case Token::kI64:
            return true;
            
        default:
            return false;
    }
}

bool TypeSign::IsUnsignedIntegral() const {
    switch (id()) {
        case Token::kU8:
        case Token::kU16:
        case Token::kU32:
        case Token::kUInt:
        case Token::kU64:
            return true;
            
        default:
            return false;
    }
}

bool TypeSign::IsFloating() const { return id() == Token::kF32 || id() == Token::kF64; }

bool TypeSign::Convertible(TypeSign *rhs) const {
    if (id() == Token::kAny) { // Any type!
        return true;
    }
    switch (static_cast<Token::Kind>(id())) {
        case Token::kIdentifier:
            return id() == rhs->id() && ToSymbolString() == rhs->ToSymbolString();
        case Token::kInterface:
            if (id() == rhs->id() && interface_ == rhs->interface_) {
                return true;
            }
            return (rhs->id() == Token::kObject || rhs->id() == Token::kRef) &&
                   interface_->HasImplement(rhs->structure());
        case Token::kObject:
            return id() == rhs->id() && object_ == rhs->object_;
        case Token::kRef:
        case Token::kClass:
            return id() == rhs->id() && clazz()->BaseOf(rhs->clazz());
        case Token::kFun:
            return id() == rhs->id() && prototype()->IsAccept(rhs->prototype());
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
                                 const ASTString *base_prefix,
                                 const ASTString *base_name,
                                 base::ArenaVector<Expression *> &&arguments)
    : StructureDefinition(arena, position, kClassDefinition, name, std::move(fields))
    , parameters_(constructor)
    , named_parameters_(arena)
    , base_prefix_(base_prefix)
    , base_name_(base_name)
    , base_(nullptr)
    , arguments_(arguments) {
}

int ClassDefinition::MakeParameterLookupTable() {
    for (size_t i = 0; i < parameters_size(); i++) {
        Parameter param = parameter(i);
        const ASTString *name = nullptr;
        if (param.field_declaration) {
            name = field(param.as_field).declaration->identifier();
        } else {
            name = param.as_parameter->identifier();
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
