#include "lang/type-checker.h"
#include "lang/compiler.h"
#include "lang/token.h"
#include <deque>

namespace mai {

namespace lang {

static inline ASTVisitor::Result ResultWithType(TypeSign *type) {
    ASTVisitor::Result rv;
    rv.kind = 0;
    rv.sign = type;
    return rv;
}

TypeChecker::TypeChecker(base::Arena *arena, SyntaxFeedback *feedback)
    : arena_(arena)
    , error_feedback_(feedback)
    , all_units_(arena)
    , kVoid(new (arena) TypeSign(0, Token::kVoid))
    , kError(new (arena) TypeSign(0, Token::kError)) {
}


Error TypeChecker::AddBootFileUnits(const std::string &original_path,
                                    const std::vector<FileUnit *> &file_units,
                                    SourceFileResolve *resolve) {
    std::map<std::string, std::vector<FileUnit *>> external_units;
    for (auto unit : file_units) {
        auto key = unit->GetPathName(original_path.back() == '/' || original_path.back() == '\\' ?
                                     original_path.size() : original_path.size() + 1,
                                     Compiler::kFileExtName);
        external_units[key].push_back(unit);
    }
    
    while (!external_units.empty()) {
        std::vector<FileUnit *> units = std::move(external_units.begin()->second);
        std::string path = external_units.begin()->first;
        external_units.erase(external_units.begin());
        
        for (auto unit : units) {
            path_units_[path].push_back(unit);
            pkg_units_[unit->package_name()->ToString()].push_back(unit);
            all_units_.push_back(unit);
            
            for (auto import : unit->import_packages()) {
                auto iter = path_units_.find(import->original_path()->data());
                if (iter != path_units_.end()) {
                    continue;
                }
                std::vector<FileUnit *> import_units;
                if (auto rs = resolve->Resolve(import->original_path()->data(), &import_units);
                    rs.fail()) {
                    return rs;
                }
                auto key = unit->GetPathName(original_path.back() == '/' ||
                                             original_path.back() == '\\' ?
                                             original_path.size() : original_path.size() + 1,
                                             Compiler::kFileExtName);
                external_units[key] = std::move(import_units);
            }
        }
    }
    return Error::OK();
}

bool TypeChecker::Prepare() {
    int fail = 0;
    for (auto pair : pkg_units_) {
        const std::string &pkg_name = pair.first;
        
        for (auto unit : pair.second) {
            for (auto decl : unit->global_variables()) {
                std::string name(pkg_name);
                name.append(".").append(decl->identifier()->ToString());
                auto iter = symbols_.find(name);
                if (iter != symbols_.end()) {
                    error_feedback_->Printf(unit->FindSourceLocation(decl), "Duplicated symbol: %s",
                                            decl->identifier()->data());
                    fail++;
                    continue;
                }
                symbols_[name] = decl;
            }
            
            for (auto def : unit->definitions()) {
                std::string name(pkg_name);
                name.append(".").append(def->identifier()->ToString());
                
                if (def->IsFunctionDefinition()) {
                    auto iter = symbols_.find(name);
                    if (iter != symbols_.end()) {
                        error_feedback_->Printf(unit->FindSourceLocation(def),
                                                "Duplicated function: %s",
                                                def->identifier()->data());
                        fail++;
                        continue;
                    }
                    symbols_[name] = def;
                } else if (def->IsClassDefinition() || def->IsObjectDefinition() ||
                           def->IsInterfaceDefinition()) {
                    auto iter = symbols_.find(name);
                    if (iter != symbols_.end()) {
                        error_feedback_->Printf(unit->FindSourceLocation(def),
                                                "Duplicated class or object: %s",
                                                def->identifier()->data());
                        fail++;
                        continue;
                    }
                    symbols_[name] = def;
                }
            }
        }
    }
    
    class_any_ = DCHECK_NOTNULL(FindSymbolOrNull("lang.Any")->AsClassDefinition());
    class_exception_ = DCHECK_NOTNULL(FindSymbolOrNull("lang.Exception")->AsClassDefinition());

    for (auto pair : pkg_units_) {
        for (auto unit : pair.second) {
            if (!PrepareClassDefinition(unit)) {
                fail++;
            }
        }
    }
    return !fail;
}

bool TypeChecker::PrepareClassDefinition(FileUnit *unit) {
    // import mai.lang as *
    ImportStatement *base = new (arena_) ImportStatement(0, ASTString::New(arena_, "*"),
                                                         ASTString::New(arena_, "mai.lang"));
    unit->InsertImportStatement(base);

    FileScope file_scope(&symbols_, unit, &current_);
    file_scope.Initialize(path_units_);
    
    for (auto impl : unit->implements()) {
        Symbolize *sym = file_scope.FindOrNull(impl->prefix(), impl->name());
        if (!sym) {
            SourceLocation loc = unit->FindSourceLocation(impl);
            error_feedback_->Printf(loc, "Unresolve symbol: %s", impl->ToSymbolString().c_str());
            return false;
        }
        
        if (auto clazz = sym->AsClassDefinition()) {
            for (auto method : impl->methods()) {
                clazz->InsertMethod(method);
            }
            impl->set_owner(clazz);
        } else if (auto object = sym->AsObjectDefinition()) {
            for (auto method : impl->methods()) {
                object->InsertMethod(method);
            }
            impl->set_owner(object);
        } else {
            SourceLocation loc = unit->FindSourceLocation(impl);
            error_feedback_->Printf(loc, "Symbol: %s is not class or object",
                                    impl->ToSymbolString().c_str());
            return false;
        }
    }

    for (auto def : unit->definitions()) {
        if (auto ast = def->AsClassDefinition(); ast != nullptr && ast->base_name()) {
            Symbolize *sym = file_scope.FindOrNull(ast->base_prefix(), ast->base_name());
            if (!sym || !sym->IsClassDefinition()) {
                error_feedback_->Printf(FindSourceLocation(ast), "Base class not found: %s",
                                        ast->ToBaseSymbolString().c_str());
                return false;
            }
            ast->set_base(sym->AsClassDefinition());
        }
        
        if (auto ast = def->AsClassDefinition();
            ast != nullptr && !ast->base_name() && ast != class_any_) {
            ast->set_base(class_any_);
        }
    }
    return true;
}

bool TypeChecker::Check() {
    for (auto pair : pkg_units_) {
        for (auto unit : pair.second) {
            if (!CheckFileUnit(pair.first, unit)) {
                return false;
            }
            
            // TODO:
        }
    }
    return true;
}

bool TypeChecker::CheckFileUnit(const std::string &pkg_name, FileUnit *unit) {
    FileScope file_scope(&symbols_, unit, &current_);
    file_scope.Initialize(path_units_);
    
    error_feedback_->set_file_name(unit->file_name()->ToString());
    error_feedback_->set_package_name(unit->package_name()->ToString());

    for (auto decl : unit->global_variables()) {
        if (auto rv = decl->Accept(this); rv.sign == kError) {
            return false;
        }
    }

    for (auto def : unit->definitions()) {
        if (auto rv = def->Accept(this); rv.sign == kError) {
            return false;
        }
    }
    
    for (auto impl : unit->implements()) {
        if (auto rv = impl->Accept(this); rv.sign == kError) {
            return false;
        }
    }
    return true;
}

ASTVisitor::Result TypeChecker::VisitTypeSign(TypeSign *ast) /*override*/ {
    if (ast->id() == Token::kIdentifier) {
        Symbolize *sym = current_->GetFileScope()->FindOrNull(ast->prefix(),
                                                              ast->name());
        if (auto clazz = sym->AsClassDefinition()) {
            ast->set_id(Token::kClass);
            ast->set_clazz(clazz);
        } else if (auto object = sym->AsObjectDefinition()) {
            ast->set_id(Token::kObject);
            ast->set_object(object);
        } else if (auto interf = sym->AsInterfaceDefinition()) {
            ast->set_id(Token::kInterface);
            ast->set_interface(interf);
        } else {
            error_feedback_->Printf(FindSourceLocation(ast), "Type name: %s not found",
                                    ast->ToSymbolString().c_str());
            return ResultWithType(kError);
        }
    }
    return ResultWithType(ast);
}

ASTVisitor::Result TypeChecker::VisitBoolLiteral(BoolLiteral *ast) /*override*/ {
    return ResultWithType(ast->type());
}

ASTVisitor::Result TypeChecker::VisitI8Literal(I8Literal *ast) /*override*/ {
    return ResultWithType(ast->type());
}

ASTVisitor::Result TypeChecker::VisitU8Literal(U8Literal *ast) /*override*/ {
    return ResultWithType(ast->type());
}

ASTVisitor::Result TypeChecker::VisitI16Literal(I16Literal *ast) /*override*/ {
    return ResultWithType(ast->type());
}

ASTVisitor::Result TypeChecker::VisitU16Literal(U16Literal *ast) /*override*/ {
    return ResultWithType(ast->type());
}

ASTVisitor::Result TypeChecker::VisitI32Literal(I32Literal *ast) /*override*/ {
    return ResultWithType(ast->type());
}

ASTVisitor::Result TypeChecker::VisitU32Literal(U32Literal *ast) /*override*/ {
    return ResultWithType(ast->type());
}

ASTVisitor::Result TypeChecker::VisitIntLiteral(IntLiteral *ast) /*override*/ {
    return ResultWithType(ast->type());
}

ASTVisitor::Result TypeChecker::VisitUIntLiteral(UIntLiteral *ast) /*override*/ {
    return ResultWithType(ast->type());
}

ASTVisitor::Result TypeChecker::VisitI64Literal(I64Literal *ast) /*override*/ {
    return ResultWithType(ast->type());
}

ASTVisitor::Result TypeChecker::VisitU64Literal(U64Literal *ast) /*override*/ {
    return ResultWithType(ast->type());
}

ASTVisitor::Result TypeChecker::VisitF32Literal(F32Literal *ast) /*override*/ {
    return ResultWithType(ast->type());
}

ASTVisitor::Result TypeChecker::VisitF64Literal(F64Literal *ast) /*override*/ {
    return ResultWithType(ast->type());
}

ASTVisitor::Result TypeChecker::VisitNilLiteral(NilLiteral *ast) /*override*/ {
    return ResultWithType(ast->type());
}

ASTVisitor::Result TypeChecker::VisitStringLiteral(StringLiteral *ast) /*override*/ {
    return ResultWithType(ast->type());
}

ASTVisitor::Result TypeChecker::VisitLambdaLiteral(LambdaLiteral *ast) /*override*/ {
    std::vector<VariableDeclaration *> parameters;
    if (ClassScope *class_scope = current_->GetClassScope()) {
        TypeSign *type = new (arena_) TypeSign(ast->position(), Token::kRef, class_scope->clazz());
        VariableDeclaration *self = new (arena_) VariableDeclaration(type->position(),
                                                                     VariableDeclaration::PARAMETER,
                                                                     ASTString::New(arena_, "self"),
                                                                     type, nullptr);
        parameters.push_back(self);
    }
    
    if (ast->prototype()->vargs()) {
        TypeSign *any = new (arena_) TypeSign(ast->position(), Token::kAny);
        TypeSign *argv = new (arena_) TypeSign(arena_, ast->position(), Token::kArray, any);
        VariableDeclaration *vargs = new (arena_) VariableDeclaration(argv->position(),
                                                                      VariableDeclaration::PARAMETER,
                                                                      ASTString::New(arena_, "argv"),
                                                                      argv, nullptr);
        parameters.push_back(vargs);
    }
    
    for (auto param : ast->prototype()->parameters()) {
        VariableDeclaration *var = new (arena_) VariableDeclaration(param.type->position(),
                                                                   VariableDeclaration::PARAMETER,
                                                                   param.name, param.type, nullptr);
        parameters.push_back(var);
    }
    
    FunctionScope function_scope(ast, &current_);
    for (auto param : parameters) {
        if (auto rv = param->Accept(this); rv.sign == kError) {
            return ResultWithType(kError);
        }
    }

    for (auto stmt : ast->statements()) {
        if (auto rv = stmt->Accept(this); rv.sign == kError) {
            return ResultWithType(kError);
        }
    }
    return ResultWithType(new (arena_) TypeSign(ast->position(), ast->prototype()));
}

ASTVisitor::Result TypeChecker::VisitCallExpression(CallExpression *ast) /*override*/ {
    TypeSign *callee = nullptr;
    if (auto rv = ast->callee()->Accept(this); rv.sign == kError) {
        return ResultWithType(kError);
    } else {
        callee = rv.sign;
    }
    
    if (callee->id() == Token::kClass) { // Constructor
        if (ast->operands_size() != callee->clazz()->parameters_size()) {
            error_feedback_->Printf(FindSourceLocation(ast), "Unexpected class contructor: %s",
                                    callee->clazz()->identifier()->data());
            return ResultWithType(kError);
        }
        
        for (size_t i = 0; i < ast->operands_size(); i++) {
            TypeSign *argument_type = nullptr;
            if (auto rv = ast->operand(i)->Accept(this); rv.sign == kError) {
                return ResultWithType(kError);
            } else {
                argument_type = rv.sign;
            }
            
            TypeSign *parameter_type = nullptr;
            if (callee->clazz()->parameter(i).field_declaration) {
                parameter_type = callee->clazz()->field(i).declaration->type();
            } else {
                parameter_type = callee->clazz()->parameter(i).as_parameter->type();
            }
            if (!parameter_type->Convertible(argument_type)) {
                error_feedback_->Printf(FindSourceLocation(ast), "Unexpected class contructor: %s",
                                        callee->clazz()->identifier()->data());
                return ResultWithType(kError);
            }
        }
        return ResultWithType(new (arena_) TypeSign(callee->position(), Token::kRef,
                                                    callee->clazz()));
    } else if (callee->id() == Token::kFun) { // Function
        if (callee->prototype()->vargs()) {
            if (ast->operands_size() < callee->prototype()->parameters_size()) {
                error_feedback_->Printf(FindSourceLocation(ast), "Unexpected function calling: %s",
                                        callee->clazz()->identifier()->data());
                return ResultWithType(kError);
            }
        } else {
            if (ast->operands_size() != callee->prototype()->parameters_size()) {
                error_feedback_->Printf(FindSourceLocation(ast), "Unexpected function calling: %s",
                                        callee->clazz()->identifier()->data());
                return ResultWithType(kError);
            }
        }
        
        size_t n = std::min(ast->operands_size(), callee->parameters_size());
        for (size_t i = 0; i < n; i++) {
            TypeSign *parameter_type = nullptr;
            if (auto rv = ast->operand(i)->Accept(this); rv.sign == kError) {
                return ResultWithType(kError);
            } else {
                parameter_type = rv.sign;
            }
            if (!parameter_type->Convertible(callee->prototype()->parameter(i).type)) {
                error_feedback_->Printf(FindSourceLocation(ast), "Unexpected function calling: %s",
                                        callee->clazz()->identifier()->data());
                return ResultWithType(kError);
            }
        }
        return ResultWithType(callee->prototype()->return_type());
    }
    
    error_feedback_->Printf(FindSourceLocation(ast), "Incorrect calling type");
    return ResultWithType(kError);
}

ASTVisitor::Result TypeChecker::VisitPairExpression(PairExpression *ast) /*override*/ {
    TypeSign *key = nullptr;
    if (!ast->addition_key()) {
        if (auto rv = ast->key()->Accept(this); rv.sign == kError) {
            return ResultWithType(kError);
        } else {
            key = rv.sign;
        }
    }
    
    TypeSign *value = nullptr;
    if (auto rv = ast->value()->Accept(this); rv.sign == kError) {
        return ResultWithType(kError);
    } else {
        value = rv.sign;
    }
    
    return ResultWithType(new (arena_) TypeSign(arena_, ast->position(), Token::kPair, key, value));
}

ASTVisitor::Result TypeChecker::VisitIndexExpression(IndexExpression *) /*override*/ {
    // TODO:
    TODO();
}

ASTVisitor::Result TypeChecker::VisitUnaryExpression(UnaryExpression *ast) /*override*/ {
    Result rv;
    if (rv = ast->operand()->Accept(this); rv.sign == kError) {
        return ResultWithType(kError);
    }
    TypeSign *operand = rv.sign;
    switch (ast->op().kind) {
        case Operator::kNot:
            if (operand->id() != Token::kBool) {
                error_feedback_->Printf(FindSourceLocation(ast), "Incorrect type, need bool");
                return ResultWithType(kError);
            }
            break;
        case Operator::kBitwiseNot:
            if (!operand->IsIntegral()) {
                error_feedback_->Printf(FindSourceLocation(ast), "Incorrect type, need integral "
                                        "type");
                return ResultWithType(kError);
            }
            break;
        case Operator::kMinus:
            if (!operand->IsNumber()) {
                error_feedback_->Printf(FindSourceLocation(ast), "Incorrect type, need number "
                                        "type");
                return ResultWithType(kError);
            }
            break;
        case Operator::kRecv:
            if (operand->id() != Token::kChannel) {
                error_feedback_->Printf(FindSourceLocation(ast), "Incorrect type, need channel "
                                        "type");
                return ResultWithType(kError);
            }
            return ResultWithType(operand->parameter(0));
        case Operator::kIncrement:
        case Operator::kIncrementPost:
            if (!CheckModifitionAccess(ast)) {
                return ResultWithType(kError);
            }
            if (!operand->IsIntegral()) {
                error_feedback_->Printf(FindSourceLocation(ast), "Incorrect type, need integral "
                                        "type");
                return ResultWithType(kError);
            }
            break;
        case Operator::kDecrement:
        case Operator::kDecrementPost:
            if (!CheckModifitionAccess(ast)) {
                return ResultWithType(kError);
            }
            if (!operand->IsIntegral()) {
                error_feedback_->Printf(FindSourceLocation(ast), "Incorrect type, need integral "
                                        "type");
                return ResultWithType(kError);
            }
            break;
        default:
            NOREACHED();
            break;
    }
    return ResultWithType(operand);
}

ASTVisitor::Result TypeChecker::VisitBinaryExpression(BinaryExpression *ast) /*override*/ {
    Result rv;
    if (rv = ast->lhs()->Accept(this); rv.sign == kError) {
        return ResultWithType(kError);
    }
    TypeSign *lhs = rv.sign;
    if (rv = ast->rhs()->Accept(this); rv.sign == kError) {
        return ResultWithType(kError);
    }
    TypeSign *rhs = rv.sign;

    switch (ast->op().kind) {
        case Operator::kAdd:
            if (!CheckAddExpression(ast, lhs, rhs)) {
                return ResultWithType(kError);
            } else {
                return ResultWithType(lhs);
            }
            break;

        case Operator::kSub:
            if (!CheckSubExpression(ast, lhs, rhs)) {
                return ResultWithType(kError);
            } else {
                return ResultWithType(lhs);
            }
            break;
            
        case Operator::kMul:
        case Operator::kDiv:
            if (!lhs->IsNumber() || !lhs->Convertible(rhs)) {
                error_feedback_->Printf(FindSourceLocation(ast), "Incorrect type, need number");
                return ResultWithType(kError);
            }
            break;
            
        case Operator::kMod:
        case Operator::kBitwiseAnd:
        case Operator::kBitwiseOr:
        case Operator::kBitwiseXor:
            if (!lhs->IsIntegral() || !lhs->Convertible(rhs)) {
                error_feedback_->Printf(FindSourceLocation(ast), "Incorrect type, need integral "
                                        "number");
                return ResultWithType(kError);
            }
            break;
            
        case Operator::kBitwiseShl:
        case Operator::kBitwiseShr:
            if (!lhs->IsIntegral() || !rhs->IsIntegral()) {
                error_feedback_->Printf(FindSourceLocation(ast), "Incorrect type, need integral "
                                        "number");
                return ResultWithType(kError);
            }
            return ResultWithType(lhs);
            
        case Operator::kSend:
            if (lhs->id() != Token::kChannel) {
                error_feedback_->Printf(FindSourceLocation(ast), "Incorrect type, need channel");
                return ResultWithType(kError);
            }
            if (!lhs->parameter(0)->Convertible(rhs)) {
                error_feedback_->Printf(FindSourceLocation(ast), "Incorrect send type");
                return ResultWithType(kError);
            }
            return ResultWithType(new (arena_) TypeSign(ast->position(), Token::kBool));
            
        case Operator::kEqual:
        case Operator::kNotEqual:
        case Operator::kLess:
        case Operator::kLessEqual:
        case Operator::kGreater:
        case Operator::kGreaterEqual:
            if (lhs->IsNumber() && lhs->Convertible(rhs)) {
                // ok
            } else if (lhs->id() == Token::kString && lhs->Convertible(rhs)) {
                // ok
            } else {
                error_feedback_->Printf(FindSourceLocation(ast), "Incorrect type, need number or "
                                        "string");
                return ResultWithType(kError);
            }
            return ResultWithType(new (arena_) TypeSign(ast->position(), Token::kBool));
            
        case Operator::kAnd:
        case Operator::kOr:
            if (lhs->id() != Token::kBool || !lhs->Convertible(rhs)) {
                error_feedback_->Printf(FindSourceLocation(ast), "Incorrect type, need bool");
                return ResultWithType(kError);
            }
            break;

        default:
            NOREACHED();
            break;
    }
    DCHECK_EQ(lhs->id(), rhs->id());
    return ResultWithType(lhs);
}

ASTVisitor::Result TypeChecker::VisitArrayInitializer(ArrayInitializer *ast) /*override*/ {
    if (ast->reserve()) {
        if (auto rv = ast->reserve()->Accept(this); rv.sign == kError) {
            return ResultWithType(kError);
        } else {
            if (!rv.sign->IsIntegral()) {
                error_feedback_->Printf(FindSourceLocation(ast), "Incorrect array constructor "
                                        "parameter[0] type, need integral number");
                return ResultWithType(kError);
            }
        }
        return ResultWithType(new (arena_) TypeSign(arena_, ast->position(),
                                                    ast->mutable_container()
                                                    ? Token::kMutableArray : Token::kArray,
                                                    ast->element_type()));
    }

    TypeSign *defined_element_type = ast->element_type();
    for (auto element : ast->operands()) {
        auto rv = element->Accept(this);
        if (rv.sign == kError) {
            return ResultWithType(kError);
        }
        if (rv.sign->id() == Token::kVoid) {
            error_feedback_->Printf(FindSourceLocation(element), "Attempt void type element");
            return ResultWithType(kError);
        }
        if (!defined_element_type) {
            defined_element_type = rv.sign;
        }
        if (!defined_element_type->Convertible(rv.sign)) {
            defined_element_type->set_id(Token::kAny);
        }
    }
    
    if (ast->element_type()) {
        if (!ast->element_type()->Convertible(defined_element_type)) {
            error_feedback_->Printf(FindSourceLocation(ast), "Attempt unconvertible element type");
            return ResultWithType(kError);
        }
    } else {
        ast->set_element_type(defined_element_type);
    }

    return ResultWithType(new (arena_) TypeSign(arena_, ast->position(),
                                                ast->mutable_container()
                                                ? Token::kMutableArray : Token::kArray,
                                                ast->element_type()));
}

ASTVisitor::Result TypeChecker::VisitMapInitializer(MapInitializer *ast) /*override*/ {
    if (ast->reserve()) {
        Result rv = ast->reserve()->Accept(this);
        if (rv.sign == kError) {
            return ResultWithType(kError);
        }
        if (!rv.sign->IsIntegral()) {
            error_feedback_->Printf(FindSourceLocation(ast), "Incorrect map constructor "
                                    "parameter[0] type, need integral number");
            return ResultWithType(kError);
        }
        
        if (ast->load_factor()) {
            if (rv = ast->load_factor()->Accept(this); rv.sign == kError) {
                return ResultWithType(kError);
            }
            if (!rv.sign->IsFloating()) {
                error_feedback_->Printf(FindSourceLocation(ast), "Incorrect map constructor "
                                        "parameter[1] type, need floating number");
                return ResultWithType(kError);
            }
        }
        TypeSign *map = new (arena_) TypeSign(arena_, ast->position(), ast->mutable_container()
                                              ? Token::kMutableMap : Token::kMap,
                                              DCHECK_NOTNULL(ast->key_type()),
                                              DCHECK_NOTNULL(ast->value_type()));
        return ResultWithType(map);
    }
    
    TypeSign *defined_key_type = ast->key_type(), *defined_value_type = ast->value_type();
    for (auto pair : ast->operands()) {
        auto rv = DCHECK_NOTNULL(pair->AsPairExpression())->key()->Accept(this);
        if (rv.sign == kError) {
            return ResultWithType(kError);
        }
        if (rv.sign->id() == Token::kVoid) {
            error_feedback_->Printf(FindSourceLocation(pair), "Attempt void type key");
            return ResultWithType(kError);
        }
        if (!defined_key_type) {
            defined_key_type = rv.sign;
        }
        if (!defined_key_type->Convertible(rv.sign)) {
            defined_key_type->set_id(Token::kAny);
        }
        
        if (rv = DCHECK_NOTNULL(pair->AsPairExpression())->value()->Accept(this);
            rv.sign == kError) {
            return ResultWithType(kError);
        }
        if (rv.sign->id() == Token::kVoid) {
            error_feedback_->Printf(FindSourceLocation(pair), "Attempt void type value");
            return ResultWithType(kError);
        }
        if (!defined_value_type) {
            defined_value_type = rv.sign;
        }
        if (!defined_value_type->Convertible(rv.sign)) {
            defined_value_type->set_id(Token::kAny);
        }
    }
    
    if (ast->key_type()) {
        if (!ast->key_type()->Convertible(defined_key_type)) {
            error_feedback_->Printf(FindSourceLocation(ast), "Attempt unconvertible key type");
            return ResultWithType(kError);
        }
    } else {
        ast->set_key_type(defined_key_type);
    }
    if (ast->value_type()) {
        if (!ast->value_type()->Convertible(defined_value_type)) {
            error_feedback_->Printf(FindSourceLocation(ast), "Attempt unconvertible value type");
            return ResultWithType(kError);
        }
    } else {
        ast->set_value_type(defined_value_type);
    }

    TypeSign *map = new (arena_) TypeSign(arena_, ast->position(), ast->mutable_container()
                                          ? Token::kMutableMap : Token::kMap,
                                          DCHECK_NOTNULL(ast->key_type()),
                                          DCHECK_NOTNULL(ast->value_type()));
    return ResultWithType(map);
}

ASTVisitor::Result TypeChecker::VisitBreakableStatement(BreakableStatement *ast) /*override*/ {
    switch (ast->control()) {
        case BreakableStatement::BREAK:
            TODO(); // TODO
            break;
        case BreakableStatement::CONTINUE:
            TODO(); // TODO
            break;
        case BreakableStatement::THROW:
            TODO(); // TODO
            break;
        case BreakableStatement::RETURN: {
            TypeSign *ret_type = kVoid;
            if (ast->value()) {
                if (auto rv = ast->value()->Accept(this); rv.sign == kError) {
                    return ResultWithType(kError);
                } else {
                    ret_type = rv.sign;
                }
                if (ret_type->id() == Token::kVoid) {
                    error_feedback_->Printf(FindSourceLocation(ast), "Attempt return void type");
                    return ResultWithType(kError);
                }
            }

            FunctionScope *function_scope = current_->GetFunctionScope();
            if (!function_scope) {
                error_feedback_->Printf(FindSourceLocation(ast), "Attempt return in function "
                                        "outside");
                return ResultWithType(kError);
            }
            
            if (!function_scope->function()->prototype()->return_type()->Convertible(ret_type)) {
                error_feedback_->Printf(FindSourceLocation(ast), "Unconvertible return type");
                return ResultWithType(kError);
            }
        } break;
        default:
            NOREACHED();
            break;
    }
    return ResultWithType(kVoid);
}

ASTVisitor::Result TypeChecker::VisitAssignmentStatement(AssignmentStatement *ast) /*override*/ {
    TypeSign *rval = nullptr, *lval = nullptr;
    if (auto rv = ast->rval()->Accept(this); rv.sign == kError) {
        return ResultWithType(kError);
    } else {
        rval = rv.sign;
    }
    if (auto rv = ast->lval()->Accept(this); rv.sign == kError) {
        return ResultWithType(kError);
    } else {
        lval = rv.sign;
    }
    
    if (!CheckModifitionAccess(ast->lval())) {
        return ResultWithType(kError);
    }

    switch (ast->assignment_op().kind) {
        case Operator::kAdd: // +=
            if (!CheckAddExpression(ast, lval, rval)) {
                return ResultWithType(kError);
            }
            break;
        case Operator::kSub: // -=
            if (!CheckSubExpression(ast, lval, rval)) {
                return ResultWithType(kError);
            }
            break;
        case Operator::NOT_OPERATOR: // =
            if (!lval->Convertible(rval)) {
                error_feedback_->Printf(FindSourceLocation(ast), "Incorrect rval type in `=' "
                                        "expression");
                return ResultWithType(kError);
            }
            break;
        default:
            NOREACHED();
            break;
    }
    return ResultWithType(lval);
}

ASTVisitor::Result TypeChecker::VisitStringTemplateExpression(StringTemplateExpression *ast)
/*override*/ {
    for (auto operand : ast->operands()) {
        if (auto rv = operand->Accept(this); rv.sign == kError) {
            return ResultWithType(kError);
        }
    }
    return ResultWithType(new (arena_) TypeSign(ast->position(), Token::kString));
}

ASTVisitor::Result TypeChecker::VisitVariableDeclaration(VariableDeclaration *ast) /*override*/ {
    if (auto err = current_->Register(ast)) {
        error_feedback_->Printf(FindSourceLocation(ast), "Duplicated variable declaration: %s",
                                err->identifier()->data());
        return ResultWithType(kError);
    }

    if (ast->type() && ast->type()->id() == Token::kIdentifier) {
        if (auto rv = ast->type()->Accept(this); rv.sign == kError) {
            return ResultWithType(kError);
        }
    }
    
    if (ast->initializer()) {
        auto rv = ast->initializer()->Accept(this);
        if (rv.sign == kError) {
            return rv;
        }
        if (rv.sign->id() == Token::kVoid) {
            error_feedback_->Printf(FindSourceLocation(ast), "Void type for variable declaration");
            return ResultWithType(kError);
        }

        if (!ast->type()) {
            ast->set_type(rv.sign);
        } else {
            if (!ast->type()->Convertible(rv.sign)) {
                error_feedback_->Printf(FindSourceLocation(ast), "Unconvertible type");
                return ResultWithType(kError);
            }
        }
    }
    return ResultWithType(kVoid);
}

ASTVisitor::Result TypeChecker::VisitIdentifier(Identifier *ast) /*override*/ {
    auto [scope, sym] = current_->Resolve(ast->name());
    if (!sym) {
        error_feedback_->Printf(FindSourceLocation(ast), "Unresolve symbol: %s",
                                ast->name()->data());
        return ResultWithType(kError);
    }

    ast->set_original(sym);
    switch (scope->kind()) {
        case AbstractScope::kFileScope:
            ast->set_scope(static_cast<FileScope *>(scope)->file_unit());
            break;
        case AbstractScope::kClassScope:
            ast->set_scope(static_cast<ClassScope *>(scope)->clazz());
            break;
        case AbstractScope::kFunctionScope:
            ast->set_scope(scope->GetFunctionScope()->function());
            break;
        case AbstractScope::kBlockScope:
            // TODO:
        default:
            NOREACHED();
            break;
    }

    if (sym->IsClassDefinition()) {
        TypeSign *type = new (arena_) TypeSign(ast->position(), sym->AsClassDefinition());
        return ResultWithType(type);
    } else if (sym->IsObjectDefinition()) {
        TypeSign *type = new (arena_) TypeSign(ast->position(), sym->AsObjectDefinition());
        return ResultWithType(type);
    } else if (sym->IsInterfaceDefinition()) {
        TypeSign *type = new (arena_) TypeSign(ast->position(), sym->AsInterfaceDefinition());
        return ResultWithType(type);
    } else if (sym->IsFunctionDefinition()) {
        TypeSign *type = new (arena_) TypeSign(ast->position(),
                                               sym->AsFunctionDefinition()->prototype());
        return ResultWithType(type);
    }
    DCHECK(sym->IsVariableDeclaration());
    return ResultWithType(DCHECK_NOTNULL(sym->AsVariableDeclaration()->type()));
}

ASTVisitor::Result TypeChecker::VisitDotExpression(DotExpression *ast) {
    if (auto id = ast->primary()->AsIdentifier()) {
        Symbolize *sym = std::get<1>(current_->Resolve(id->name()));
        if (sym) {
            if (auto decl = sym->AsVariableDeclaration()) {
                return CheckDotExpression(DCHECK_NOTNULL(decl->type()), ast);
            } else if (auto object = sym->AsObjectDefinition()) {
                return CheckObjectFieldAccess(object, ast);
            } else {
                error_feedback_->Printf(FindSourceLocation(ast), "Incorrect type to get field");
                return ResultWithType(kError);
            }
        }
        DCHECK(sym == nullptr);
        FileScope *file_scope = current_->GetFileScope();
        sym = file_scope->FindOrNull(id->name(), ast->rhs());
        if (sym) {
            if (auto decl = sym->AsVariableDeclaration()) {
                return CheckDotExpression(DCHECK_NOTNULL(decl->type()), ast);
            } else if (auto object = sym->AsObjectDefinition()) {
                return CheckObjectFieldAccess(object, ast);
            } else {
                error_feedback_->Printf(FindSourceLocation(ast), "Incorrect type to get field");
                return ResultWithType(kError);
            }
        }
        error_feedback_->Printf(FindSourceLocation(ast), "Unresolve symbol: %s.%s",
                                id->name()->data(), ast->rhs()->data());
        return ResultWithType(kError);
    } else {
        auto rv = ast->primary()->Accept(this);
        if (rv.sign == kError) {
            return rv;
        }
        return CheckDotExpression(DCHECK_NOTNULL(rv.sign), ast);
    }
    return ResultWithType(kVoid);
}

ASTVisitor::Result TypeChecker::VisitInterfaceDefinition(InterfaceDefinition *ast) /*override*/ {
    for (auto method : ast->methods()) {
        if (auto rv = method->Accept(this); rv.sign == kError) {
            return ResultWithType(kVoid);
        }
    }
    return ResultWithType(kVoid);
}

ASTVisitor::Result TypeChecker::VisitObjectDefinition(ObjectDefinition *ast) /*override*/ {
    for (auto field : ast->fields()) {
        if (auto rv = field.declaration->Accept(this); rv.sign == kError) {
            return ResultWithType(kError);
        }
    }
    return ResultWithType(kVoid);
}

ASTVisitor::Result TypeChecker::VisitClassDefinition(ClassDefinition *ast) /*override*/ {
    for (auto param : ast->parameters()) {
        if (param.field_declaration) {
            continue;
        }
        if (auto rv = param.as_parameter->Accept(this); rv.sign == kError) {
            return ResultWithType(kError);
        }
    }
    
    for (auto field : ast->fields()) {
        if (auto rv = field.declaration->Accept(this); rv.sign == kError) {
            return ResultWithType(kError);
        }
    }
    
    if (ast->base()) {
        FunctionScope constructor_scope(ast, &current_);

        if (ast->arguments_size() != ast->base()->parameters_size()) {
            error_feedback_->Printf(FindSourceLocation(ast), "Unexpected base class constructor "
                                    "expected %ld parameters", ast->arguments_size());
            return ResultWithType(kError);
        }
        
        for (size_t i = 0; i < ast->arguments_size(); i++) {
            auto rv = ast->argument(i)->Accept(this);
            if (rv.sign == kError) {
                return ResultWithType(kError);
            }
            auto param = ast->base()->parameter(i);
            auto type = param.field_declaration
                ? ast->base()->field(param.as_field).declaration->type()
                : param.as_parameter->type();
            if (!type->Convertible(rv.sign)) {
                error_feedback_->Printf(FindSourceLocation(ast), "Unexpected base class "
                                        "constructor");
                return ResultWithType(kError);
            }
        }
    }
    return ResultWithType(kVoid);
}

ASTVisitor::Result TypeChecker::VisitFunctionDefinition(FunctionDefinition *ast) /*override*/ {
    for (size_t i = 0; i < ast->prototype()->parameters_size(); i++) {
        auto param = &ast->prototype()->mutable_parameters()->at(i);
        if (param->type->id() == Token::kIdentifier) {
            if (auto rv = param->type->Accept(this); rv.sign == kError) {
                return ResultWithType(kError);
            }
        }
    }
    if (ast->return_type()->id() == Token::kIdentifier) {
        if (auto rv = ast->return_type()->Accept(this); rv.sign == kError) {
            return ResultWithType(kError);
        }
    }

    if (ast->is_native() || ast->is_declare()) {
        return ResultWithType(kVoid);
    }

    if (auto rv = ast->body()->Accept(this); rv.sign == kError) {
        return ResultWithType(kError);
    }
    return ResultWithType(kVoid);
}

ASTVisitor::Result TypeChecker::VisitClassImplementsBlock(ClassImplementsBlock *ast) /*override*/ {
    ClassScope class_scope(ast->owner(), &current_);

    for (auto method : ast->methods()) {
        if (auto rv = method->Accept(this); rv.sign == kError) {
            return ResultWithType(kError);
        }
    }
    return ResultWithType(kVoid);
}

ASTVisitor::Result TypeChecker::CheckDotExpression(TypeSign *type, DotExpression *ast) {
    switch (static_cast<Token::Kind>(type->id())) {
        case Token::kI8:
        case Token::kU8:
        case Token::kI16:
        case Token::kU16:
        case Token::kI32:
        case Token::kU32:
        case Token::kI64:
        case Token::kU64:
        case Token::kInt:
        case Token::kUInt:
        case Token::kF32:
        case Token::kF64: {
            Token::Kind dest_type =
                static_cast<Token::Kind>(type->FindNumberCastHint(ast->rhs()->ToSlice()));
            if (dest_type == Token::kError) {
                error_feedback_->Printf(FindSourceLocation(ast), "Incorrect type(%s) to get field",
                                        Token::ToString(static_cast<Token::Kind>(type->id())).c_str());
                return ResultWithType(kError);
            }
            return ResultWithType(new (arena_) TypeSign(ast->position(), dest_type));
        } break;
        case Token::kArray:
        case Token::kMutableArray:
            TODO();
            break;
        case Token::kMap:
        case Token::kMutableMap:
            TODO();
            break;
        case Token::kChannel:
            TODO();
            break;
        case Token::kInterface:
        case Token::kRef:
        case Token::kObject:
            return CheckClassOrObjectFieldAccess(type, ast);
        default:
            error_feedback_->Printf(FindSourceLocation(ast), "Incorrect type(%s) to get field",
                                    Token::ToString(static_cast<Token::Kind>(type->id())).c_str());
            return ResultWithType(kError);
    }
}

ASTVisitor::Result TypeChecker::CheckClassOrObjectFieldAccess(TypeSign *type, DotExpression *ast) {
    if (auto clazz = type->definition()->AsClassDefinition()) {
        auto [layout, field] = clazz->ResolveField(ast->rhs()->ToSlice());
        if (field) {
            if (field->access == StructureDefinition::kPrivate) {
                if (auto scope = current_->GetClassScope(); !scope || layout != scope->clazz()) {
                    error_feedback_->Printf(FindSourceLocation(ast),
                                            "Attempt access private field: %s", ast->rhs()->data());
                    return ResultWithType(kError);
                }
            } else if (field->access == StructureDefinition::kProtected) {
                if (auto scope = current_->GetClassScope();
                    !scope || layout != scope->clazz()->base()) {
                    error_feedback_->Printf(FindSourceLocation(ast),
                                            "Attempt assign protected field: %s",
                                            ast->rhs()->data());
                    return ResultWithType(kError);
                }
            }
            return ResultWithType(DCHECK_NOTNULL(field->declaration->type()));
        }
        
        auto method = std::get<1>(clazz->ResolveMethod(ast->rhs()->ToSlice()));
        if (method) {
            return ResultWithType(new (arena_) TypeSign(method->position(), method->prototype()));
        }
    } else if (ObjectDefinition *object = type->definition()->AsObjectDefinition()) {
        return CheckObjectFieldAccess(object, ast);
    }

    auto method = type->interface()->FindMethodOrNull(ast->rhs()->ToSlice());
    if (method) {
        TypeSign *type = new (arena_) TypeSign(method->position(), method->prototype());
        return ResultWithType(type);
    }
    error_feedback_->Printf(FindSourceLocation(ast), "No field or method named: %s",
                            ast->rhs()->data());
    return ResultWithType(kError);
}

ASTVisitor::Result TypeChecker::CheckObjectFieldAccess(ObjectDefinition *object,
                                                       DotExpression *ast) {
    if (auto field = object->FindFieldOrNull(ast->rhs()->ToSlice())) {
        return ResultWithType(DCHECK_NOTNULL(field->declaration->type()));
    }
    
    auto method = object->FindMethodOrNull(ast->rhs()->ToSlice());
    if (method) {
        TypeSign *type = new (arena_) TypeSign(method->position(), method->prototype());
        return ResultWithType(type);
    }
    error_feedback_->Printf(FindSourceLocation(ast), "No field or method named: %s",
                            ast->rhs()->data());
    return ResultWithType(kError);
}

bool TypeChecker::CheckModifitionAccess(ASTNode *ast) {
    switch (ast->kind()) {
        case ASTNode::kIdentifier: {
            Symbolize *sym = std::get<1>(current_->Resolve(ast->AsIdentifier()->name()));
            if (!sym->IsVariableDeclaration()) {
                error_feedback_->Printf(FindSourceLocation(ast), "Incorrect lval type in "
                                        "assignment");
                return false;
            }
            if (sym->AsVariableDeclaration()->kind() != VariableDeclaration::VAR) {
                error_feedback_->Printf(FindSourceLocation(ast), "Val can not be assign");
                return false;
            }
        } break;

        case ASTNode::kIndexExpression: {
            TypeSign *primary = nullptr;
            if (auto rv = ast->AsIndexExpression()->primary()->Accept(this); rv.sign == kError) {
                return false;
            } else {
                primary = rv.sign;
            }

            if (primary->id() == Token::kArray || primary->id() == Token::kMap) {
                error_feedback_->Printf(FindSourceLocation(ast), "Attempt modify immutable "
                                        "containtor");
                return false;
            }
        } break;

        case ASTNode::kDotExpression: {
            TypeSign *primary = nullptr;
            DotExpression *dot = DCHECK_NOTNULL(ast->AsDotExpression());
            if (auto rv = dot->primary()->Accept(this); rv.sign == kError) {
                return false;
            } else {
                primary = rv.sign;
            }
            
            if (primary->id() == Token::kRef) {
                auto [clazz, field] = primary->clazz()->ResolveField(dot->rhs()->ToSlice());
                if (!field) {
                    error_feedback_->Printf(FindSourceLocation(ast), "Field not found: %s",
                                            dot->rhs()->data());
                    return false;
                }
                if (field->declaration->kind() != VariableDeclaration::VAR) {
                    error_feedback_->Printf(FindSourceLocation(ast),
                                            "Attempt modify val field: %s", dot->rhs()->data());
                    return false;
                }
            } else if (primary->id() == Token::kObject) {
                auto field = primary->object()->FindFieldOrNull(dot->rhs()->ToSlice());
                if (!field) {
                    error_feedback_->Printf(FindSourceLocation(ast), "Field not found: %s",
                                            dot->rhs()->data());
                    return false;
                }
                if (field->declaration->kind() != VariableDeclaration::VAR) {
                    error_feedback_->Printf(FindSourceLocation(ast),
                                            "Attempt modify val field: %s", dot->rhs()->data());
                    return false;
                }
            } else {
                NOREACHED();
            }
        } break;

        default:
            NOREACHED();
            break;
    }
    return true;
}

bool TypeChecker::CheckAddExpression(ASTNode *ast, TypeSign *lval, TypeSign *rval) {
    if (lval->id() == Token::kArray || lval->id() == Token::kMutableArray) {
        if (rval->id() != Token::kPair) {
            error_feedback_->Printf(FindSourceLocation(ast), "Incorrect rhs type, need "
                                    "pair(expr<-expr)");
            return false;
        }
        if (rval->parameter(0) && rval->parameter(0)->id() != Token::kInt) {
            error_feedback_->Printf(FindSourceLocation(ast), "Incorrect array index type");
            return false;
        }
        if (!lval->parameter(0)->Convertible(rval->parameter(1))) {
            error_feedback_->Printf(FindSourceLocation(ast), "Incorrect array element type");
            return false;
        }
        return true;
    } else if (lval->id() == Token::kMap || lval->id() == Token::kMutableMap) {
        if (rval->id() != Token::kPair) {
            error_feedback_->Printf(FindSourceLocation(ast), "Incorrect rhs type, need "
                                    "pair(expr<-expr)");
            return false;
        }
        if (!rval->parameter(0) || !lval->parameter(0)->Convertible(rval->parameter(0))) {
            error_feedback_->Printf(FindSourceLocation(ast), "Incorrect map key type");
            return false;
        }
        if (!lval->parameter(1)->Convertible(rval->parameter(1))) {
            error_feedback_->Printf(FindSourceLocation(ast), "Incorrect map value type");
            return false;
        }
        return true;
    }
    if (!lval->IsNumber() || !lval->Convertible(rval)) {
        error_feedback_->Printf(FindSourceLocation(ast), "Operand is not a number");
        return false;
    }
    return true;
}

bool TypeChecker::CheckSubExpression(ASTNode *ast, TypeSign *lval, TypeSign *rval) {
    if (lval->id() == Token::kArray || lval->id() == Token::kMutableArray) {
        if (!lval->parameter(0)->Convertible(rval)) {
            error_feedback_->Printf(FindSourceLocation(ast), "Incorrect array element type");
            return false;
        }
        return true;
    } else if (lval->id() == Token::kMap || lval->id() == Token::kMutableMap) {
        if (!lval->parameter(0)->Convertible(rval)) {
            error_feedback_->Printf(FindSourceLocation(ast), "Incorrect map value type");
            return false;
        }
        return true;
    }
    if (!lval->IsNumber() || !lval->Convertible(rval)) {
        error_feedback_->Printf(FindSourceLocation(ast), "Operand is not a number");
        return false;
    }
    return true;
}

void FileScope::Initialize(const std::map<std::string, std::vector<FileUnit *>> &path_units) {
    for (auto stmt : file_unit()->import_packages()) {
        auto iter = path_units.find(stmt->original_path()->ToString());
        DCHECK(iter != path_units.end());
        auto pkg = iter->second.front()->package_name()->ToString();
        
        if (stmt->alias() && stmt->alias()->Equal("*")) {
            all_name_space_.insert(pkg);
        }
        
        if (stmt->alias()) {
            alias_name_space_[stmt->alias()->ToString()] = pkg;
        } else {
            alias_name_space_[pkg] = pkg;
        }
    }
    all_name_space_.insert(file_unit_->package_name()->ToString());
}

Symbolize *FileScope::FindOrNull(const ASTString *name) {
    for (auto full_name : all_name_space_) {
        full_name.append(".").append(name->ToString());
        if (auto iter = all_symbols_->find(full_name); iter != all_symbols_->end()) {
            return iter->second;
        }
    }
    return nullptr;
}

Symbolize *FileScope::FindOrNull(const ASTString *prefix, const ASTString *name) {
    if (!prefix) {
        return FindOrNull(name);
    }
    if (auto iter = alias_name_space_.find(prefix->ToString()); iter == alias_name_space_.end()) {
        return nullptr;
    }
    std::string full_name(prefix->ToString());
    full_name.append(".").append(name->ToString());
    auto iter = all_symbols_->find(full_name);
    return iter == all_symbols_->end() ? nullptr : iter->second;
}

FunctionScope::FunctionScope(ClassDefinition *constructor, AbstractScope **current)
    : BlockScope(kFunctionScope, kFunctionBlock, constructor, current)
    , function_(nullptr)
    , constructor_(DCHECK_NOTNULL(constructor)) {
    for (auto param : constructor_->parameters()) {
        VariableDeclaration *var = nullptr;
        if (param.field_declaration) {
            var = constructor_->field(param.as_field).declaration;
        } else {
            var = param.as_parameter;
        }
        locals_[var->identifier()->ToSlice()] = var;
    }
}

} // namespace lang

} // namespace mai
