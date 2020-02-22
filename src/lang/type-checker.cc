#include "lang/type-checker.h"
#include "lang/compiler.h"
#include "lang/token.h"

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


Error TypeChecker::AddBootFileUnits(const std::vector<FileUnit *> &file_units,
                                    SourceFileResolve *resolve) {
    for (auto unit : file_units) {
        path_units_[unit->GetPathName(Compiler::kFileExtName)].push_back(unit);
        pkg_units_[unit->package_name()->ToString()].push_back(unit);
        all_units_.push_back(unit);
    }
    
    for (auto unit : file_units) {
        for (auto import : unit->import_packages()) {
            auto iter = path_units_.find(import->original_path()->data());
            if (iter != path_units_.end()) {
                continue;
            }

            std::vector<FileUnit *> import_units;
            resolve->Resolve(import->original_path()->data(), &import_units);
            for (auto unit : file_units) {
                path_units_[unit->GetPathName(Compiler::kFileExtName)].push_back(unit);
                pkg_units_[unit->package_name()->ToString()].push_back(unit);
                all_units_.push_back(unit);
            }
        }
    }
    return Error::OK();
}

bool TypeChecker::ResolveSymbols() {
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
                    auto iter = classes_objects_.find(name);
                    if (iter != classes_objects_.end()) {
                        error_feedback_->Printf(unit->FindSourceLocation(def),
                                                "Duplicated class or object: %s",
                                                def->identifier()->data());
                        fail++;
                        continue;
                    }
                    classes_objects_[name] = def;
                }
            }
        }
    }
    return !fail;
}

bool TypeChecker::Check() {
    for (auto pair : pkg_units_) {
        //const std::string &pkg_name = pair.first;
        
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
    // import mai.lang as *
    ImportStatement *base = new (arena_) ImportStatement(0, ASTString::New(arena_, "*"),
                                                         ASTString::New(arena_, "mai.lang"));
    unit->InsertImportStatement(base);
    
    FileScope file_scope(&symbols_, unit, &current_);
    file_scope.Initialize(path_units_);
    
    for (auto impl : unit->implements()) {
        Symbolize *sym = file_scope.ResolveOrNull(impl->prefix(), impl->name());
        if (!sym) {
            SourceLocation loc = unit->FindSourceLocation(impl);
            if (impl->prefix()) {
                error_feedback_->Printf(loc, "Unresolve symbol: %s.%s", impl->prefix()->data(),
                                        impl->name()->data());
            } else {
                error_feedback_->Printf(loc, "Unresolve symbol: %s", impl->name()->data());
            }
            return false;
        }
        
        if (auto clazz = sym->AsClassDefinition()) {
            for (auto method : impl->methods()) {
                clazz->InsertMethod(method);
                // TODO:
            }
        } else {
            SourceLocation loc = unit->FindSourceLocation(impl);
            if (impl->prefix()) {
                error_feedback_->Printf(loc, "Symbol: %s.%s is not class or object",
                                        impl->prefix()->data(), impl->name()->data());
            } else {
                error_feedback_->Printf(loc, "Symbol: %s is not class or object",
                                        impl->name()->data());
            }
            return false;
        }
    }

    for (auto decl : unit->global_variables()) {
        auto rv = decl->Accept(this);
        if (rv.sign == kError) {
            return false;
        }
    }
    return false;
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
        case Token::kObject: {
            auto field = type->clazz()->FindFieldOrNull(ast->rhs()->ToSlice());
            if (field) {
                return ResultWithType(DCHECK_NOTNULL(field->declaration->type()));
            }
            auto method = type->clazz()->FindMethodOrNull(ast->rhs()->ToSlice());
            if (method) {
                TypeSign *type = new (arena_) TypeSign(method->position(), method->prototype());
                return ResultWithType(type);
            }
            SourceLocation loc = current_->GetFileScope()->file_unit()->FindSourceLocation(ast);
            error_feedback_->Printf(loc, "No field or method named: %s", ast->rhs()->data());
            return ResultWithType(kError);
        } break;
        default: {
            SourceLocation loc = current_->GetFileScope()->file_unit()->FindSourceLocation(ast);
            error_feedback_->Printf(loc, "Incorrect type to get field");
            return ResultWithType(kError);
        } break;
    }
}

ASTVisitor::Result TypeChecker::VisitVariableDeclaration(VariableDeclaration *ast) /*override*/ {
    if (ast->initializer()) {
        auto rv = ast->initializer()->Accept(this);
        if (rv.sign == kError) {
            return rv;
        }
        if (rv.sign->id() == Token::kVoid) {
            
            error_feedback_->Printf({}, "Void type for variable declaration");
            TODO();
            return ResultWithType(kError);
        }
        
        if (!ast->type()) {
            ast->set_type(rv.sign);
        } else {
            if (ast->type()->id() == Token::kClass && rv.sign->id() == Token::kObject) {
                ast->set_type(rv.sign);
            }
            if (!ast->type()->Convertible(rv.sign)) {
                error_feedback_->Printf({}, "Un-convertible type");
                TODO();
                return ResultWithType(kError);
            }
        }
    }
    return ResultWithType(kVoid);
}

ASTVisitor::Result TypeChecker::VisitDotExpression(DotExpression *ast) {
    if (auto id = ast->primary()->AsIdentifier()) {
        Symbolize *sym = current_->ResolveOrNull(id->name());
        if (sym) {
            if (auto decl = sym->AsVariableDeclaration()) {
                return CheckDotExpression(DCHECK_NOTNULL(decl->type()), ast);
            // TODO: Object
            } else {
                SourceLocation loc = current_->GetFileScope()->file_unit()->FindSourceLocation(ast);
                error_feedback_->Printf(loc, "Incorrect type to get field");
                return ResultWithType(kError);
            }
        }
        DCHECK(sym == nullptr);
        FileScope *file_scope = current_->GetFileScope();
        sym = file_scope->ResolveOrNull(id->name(), ast->rhs());
        if (sym) {
            if (auto decl = sym->AsVariableDeclaration()) {
                return CheckDotExpression(DCHECK_NOTNULL(decl->type()), ast);
            // TODO: Object
            } else {
                SourceLocation loc = current_->GetFileScope()->file_unit()->FindSourceLocation(ast);
                error_feedback_->Printf(loc, "Incorrect type to get field");
                return ResultWithType(kError);
            }
        }
        SourceLocation loc = file_scope->file_unit()->FindSourceLocation(ast);
        error_feedback_->Printf(loc, "Unresolve symbol: %s.%s", id->name()->data(), ast->rhs()->data());
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
}

Symbolize *FileScope::ResolveOrNull(const ASTString *name) {
    for (auto full_name : all_name_space_) {
        full_name.append(".").append(name->ToString());
        if (auto iter = all_symbols_->find(full_name); iter != all_symbols_->end()) {
            return iter->second;
        }
    }
    return nullptr;
}

Symbolize *FileScope::ResolveOrNull(const ASTString *prefix, const ASTString *name) {
    if (!prefix) {
        return ResolveOrNull(name);
    }
    if (auto iter = alias_name_space_.find(prefix->ToString()); iter == alias_name_space_.end()) {
        return nullptr;
    }
    std::string full_name(prefix->ToString());
    full_name.append(".").append(name->ToString());
    auto iter = all_symbols_->find(full_name);
    return iter == all_symbols_->end() ? nullptr : iter->second;
}

} // namespace lang

} // namespace mai
