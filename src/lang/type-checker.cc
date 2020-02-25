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
        //DCHECK(unit->package_name()->ToString().find(original_path) == 0);
        auto key = unit->GetPathName(original_path.back() == '/' || original_path.back() == '\\' ?
                                     original_path.size() : original_path.size() + 1,
                                     Compiler::kFileExtName);
        //printf("key = %s\n", key.c_str());
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
                auto key = unit->GetPathName(original_path.back() == '/' || original_path.back() == '\\' ?
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
                        error_feedback_->Printf(unit->FindSourceLocation(def), "Duplicated symbol: %s",
                                                def->identifier()->data());
                        fail++;
                        continue;
                    }
                    symbols_[name] = def;
                } else if (def->IsClassDefinition()) {
                    auto iter = symbols_.find(name);
                    if (iter != symbols_.end()) {
                        error_feedback_->Printf(unit->FindSourceLocation(def),
                                                "Duplicated class or object: %s",
                                                def->identifier()->data());
                        fail++;
                        continue;
                    }
                    symbols_[name] = def;
                    //classes_objects_[name] = def;
                }
            }
        }
    }

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
                // TODO:
            }
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

    for (auto decl : unit->global_variables()) {
        auto rv = decl->Accept(this);
        if (rv.sign == kError) {
            return false;
        }
    }

    for (auto def : unit->definitions()) {
        auto rv = def->Accept(this);
        if (rv.sign == kError) {
            return false;
        }
    }
    return true;
}

ASTVisitor::Result TypeChecker::CheckDotExpression(TypeSign *type, DotExpression *ast) {
    switch (type->id()) {
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
        case Token::kInterface: {
            auto method = type->interface()->FindMethodOrNull(ast->rhs()->ToSlice());
            if (!method) {
                error_feedback_->Printf(FindSourceLocation(ast), "No field or method named: %s",
                                        ast->rhs()->data());
                return ResultWithType(kError);
            }
            TypeSign *type = new (arena_) TypeSign(method->position(), method->prototype());
            return ResultWithType(type);
        } break;
        case Token::kClass:
        case Token::kObject: {
            auto field = type->structure()->FindFieldOrNull(ast->rhs()->ToSlice());
            if (field) {
                return ResultWithType(DCHECK_NOTNULL(field->declaration->type()));
            }
            auto method = type->structure()->FindMethodOrNull(ast->rhs()->ToSlice());
            if (method) {
                TypeSign *type = new (arena_) TypeSign(method->position(), method->prototype());
                return ResultWithType(type);
            }
            error_feedback_->Printf(FindSourceLocation(ast), "No field or method named: %s",
                                    ast->rhs()->data());
            return ResultWithType(kError);
        } break;
        default: {
            error_feedback_->Printf(FindSourceLocation(ast), "Incorrect type to get field");
            return ResultWithType(kError);
        } break;
    }
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

ASTVisitor::Result TypeChecker::VisitVariableDeclaration(VariableDeclaration *ast) /*override*/ {
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
                error_feedback_->Printf(FindSourceLocation(ast), "Un-convertible type");
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
    if (scope->kind() == AbstractScope::kFileScope) {
        ast->set_scope(static_cast<FileScope *>(scope)->file_unit());
    } else if (scope->kind() == AbstractScope::kBlockScope) {
        ast->set_scope(scope->GetFunctionScope()->function());
    } else if (scope->kind() == AbstractScope::kFunctionScope) {
        ast->set_scope(static_cast<FunctionScope *>(scope)->function());
    } else if (scope->kind() == AbstractScope::kClassScope) {
        ast->set_scope(static_cast<ClassScope *>(scope)->clazz());
    } else {
        NOREACHED();
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
            // TODO: Object
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
            // TODO: Object
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
    TODO();
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
        ClassScope class_scope(ast, &current_);
        
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
                error_feedback_->Printf(FindSourceLocation(ast), "Unexpected base class constructor");
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

    // TODO();
    return ResultWithType(kVoid);
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

ClassScope::ClassScope(ClassDefinition *clazz, AbstractScope **current)
    : AbstractScope(kClassScope, current)
    , clazz_(clazz) {
    for (auto param : clazz_->parameters()) {
        if (param.field_declaration) {
            // Ignore
        } else {
            parameters_[param.as_parameter->identifier()->ToSlice()] = param.as_parameter;
        }
    }
}

Symbolize *ClassScope::FindOrNull(const ASTString *name) /*override*/ {
    auto iter = parameters_.find(name->ToSlice());
    return iter == parameters_.end() ? nullptr : iter->second;
}

} // namespace lang

} // namespace mai
