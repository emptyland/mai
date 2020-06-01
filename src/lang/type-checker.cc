#include "lang/type-checker.h"
#include "lang/compiler.h"
#include "lang/token.h"
#include <deque>

namespace mai {

namespace lang {

class TypeChecker::Scope : public AbstractScope {
public:
    enum Kind {
        kFileScope,
        kClassScope,
        kFunctionScope,
        kLoopBlockScope,
        kIfBlockScope,
        kPlainBlockScope,
    };
    ~Scope() override = default;

    DEF_VAL_GETTER(Kind, kind);
    DEF_PTR_GETTER(AbstractScope, prev);

    bool is_file_scope() const { return kind_ == kFileScope; }
    bool is_class_scope() const { return kind_ == kClassScope; }
    bool is_function_scope() const { return kind_ == kFunctionScope; }
    bool is_block_scope() const {
        return kind_ == kLoopBlockScope || kind_ == kIfBlockScope || kind_ == kPlainBlockScope;
    }
    
    bool has_broke() const { return broke_ > 0; }
    
    int MarkBroke() { return broke_++; }

    virtual Symbolize *FindOrNull(const ASTString *name) { return nullptr; }
    virtual Symbolize *Register(Symbolize *sym) { return nullptr; }
    
    AbstractScope *Enter() override {
        DCHECK_NE(this, *current_);
        prev_ = *current_;
        *current_ = this;
        return prev_;
    }
    
    AbstractScope *Exit() override {
        DCHECK_EQ(this, *current_);
        *current_ = prev_;
        return prev_;
    }

    std::tuple<Scope *, Symbolize *> Resolve(const ASTString *name) {
        for (auto i = this; i != nullptr; i = i->prev_) {
            if (Symbolize *sym = i->FindOrNull(name); sym != nullptr) {
                return {i, sym};
            }
        }
        return {nullptr, nullptr};
    }

    Scope *GetScope(Kind kind) {
        for (Scope *scope = this; scope != nullptr; scope = scope->prev_) {
            if (scope->kind_ == kind) {
                return scope;
            }
        }
        return nullptr;
    }

    FileScope *GetFileScope() { return down_cast<FileScope>(GetScope(kFileScope)); }
    ClassScope *GetClassScope() { return down_cast<ClassScope>(GetScope(kClassScope)); }
    FunctionScope *GetFunctionScope() { return down_cast<FunctionScope>(GetScope(kFunctionScope)); }
    BlockScope *GetLoopScope() {
        for (Scope *scope = this; scope != nullptr; scope = scope->prev_) {
            if (scope->kind_ == kFunctionScope) {
                break;
            }
            if (scope->kind_ == kLoopBlockScope) {
                return down_cast<BlockScope>(scope);
            }
        }
        return nullptr;
    }

    DISALLOW_IMPLICIT_CONSTRUCTORS(Scope);
protected:
    Scope(Kind kind, Scope **current)
        : kind_(kind)
        , current_(current) {
    }
    
    Kind kind_;
    Scope **current_;
    Scope *prev_ = nullptr;
    int broke_ = 0;
}; // class Scope

class TypeChecker::FileScope : public Scope {
public:
    FileScope(const std::map<std::string, std::vector<FileUnit *>> &path_units,
              const std::map<std::string, Symbolize *> *symbols, FileUnit *file_unit,
              Scope **current);

    DEF_PTR_GETTER(FileUnit, file_unit);
    
    Symbolize *FindOrNull(const ASTString *name) override {
        std::string_view exists;
        return FindExcludePackageNameOrNull(name, "", &exists);
    }
    Symbolize *FindExcludePackageNameOrNull(const ASTString *name, std::string_view exclude,
                                            std::string_view *exists);
    Symbolize *FindOrNull(const ASTString *prefix, const ASTString *name);

    DISALLOW_IMPLICIT_CONSTRUCTORS(FileScope);
private:
    FileUnit *file_unit_;
    const std::map<std::string, Symbolize *> *all_symbols_;
    std::set<std::string> all_name_space_;
    std::map<std::string, std::string> alias_name_space_;
}; // class FileScope

TypeChecker::FileScope::FileScope(const std::map<std::string, std::vector<FileUnit *>> &path_units,
                                  const std::map<std::string, Symbolize *> *symbols,
                                  FileUnit *file_unit, Scope **current)
    : Scope(kFileScope, current)
    , file_unit_(file_unit)
    , all_symbols_(symbols) {
    for (auto stmt : file_unit_->import_packages()) {
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

Symbolize *
TypeChecker::FileScope::FindExcludePackageNameOrNull(const ASTString *name,
                                                     std::string_view exclude,
                                                     std::string_view *exists) {
    for (const auto &pkg : all_name_space_) {
        if (exclude.compare(pkg) == 0) {
            continue;
        }
        std::string full_name(pkg);
        full_name.append(".").append(name->ToString());
        if (auto iter = all_symbols_->find(full_name); iter != all_symbols_->end()) {
            *exists = pkg;
            return iter->second;
        }
    }
    return nullptr;
}

Symbolize *TypeChecker::FileScope::FindOrNull(const ASTString *prefix, const ASTString *name) {
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

class TypeChecker::ClassScope : public Scope {
public:
    ClassScope(StructureDefinition *clazz, Scope **current)
        : Scope(kClassScope, current)
        , clazz_(clazz) {
        Enter();
    }
    ~ClassScope() override { Exit(); }
    
    ClassDefinition *clazz() { return DCHECK_NOTNULL(clazz_->AsClassDefinition()); }
    ObjectDefinition *object() { return DCHECK_NOTNULL(clazz_->AsObjectDefinition()); }

    DISALLOW_IMPLICIT_CONSTRUCTORS(ClassScope);
private:
    StructureDefinition *clazz_;
}; // class ClassScope

class TypeChecker::BlockScope : public Scope {
public:
    BlockScope(Kind kind, Statement *stmt, Scope **current)
        : Scope(kind, current)
        , stmt_(stmt) {
        Enter();
    }
    
    ~BlockScope() override { Exit(); }
    
    DEF_PTR_GETTER(Statement, stmt);

    Symbolize *FindOrNull(const ASTString *name) override {
        auto iter = locals_.find(name->ToSlice());
        return iter == locals_.end() ? nullptr : iter->second;
    }

    Symbolize *Register(Symbolize *sym) override {
        if (auto iter = locals_.find(sym->identifier()->ToSlice()); iter != locals_.end()) {
            return iter->second;
        }
        locals_[sym->identifier()->ToSlice()] = sym;
        return nullptr;
    }
    
protected:
    Statement *stmt_;
    std::map<std::string_view, Symbolize *> locals_;
}; // class BlockScope


class TypeChecker::FunctionScope : public BlockScope {
public:
    FunctionScope(LambdaLiteral *function, Scope **current)
        : BlockScope(kFunctionScope, function, current)
        , function_(function)
        , constructor_(nullptr) {
    }
    
    FunctionScope(StructureDefinition *constructor, Scope **current);
    
    DEF_PTR_GETTER(LambdaLiteral, function);
    DEF_PTR_GETTER(StructureDefinition, constructor);
private:
    LambdaLiteral *function_;
    StructureDefinition *constructor_;
}; // class FunctionScope

TypeChecker::FunctionScope::FunctionScope(StructureDefinition *constructor, Scope **current)
    : BlockScope(kFunctionScope, constructor, current)
    , function_(nullptr)
    , constructor_(DCHECK_NOTNULL(constructor)) {
    if (constructor->IsClassDefinition()) {
        for (auto param : constructor_->AsClassDefinition()->parameters()) {
            VariableDeclaration *var = nullptr;
            if (param.field_declaration) {
                var = constructor_->field(param.as_field).declaration;
            } else {
                var = param.as_parameter;
            }
            locals_[var->identifier()->ToSlice()] = var;
        }
    }
}

inline SourceLocation TypeChecker::FindSourceLocation(const ASTNode *ast) {
    return current_->GetFileScope()->file_unit()->FindSourceLocation(ast);
}

namespace {

#define VISIT_CHECK(node) if (rv = (node)->Accept(this); rv.sign == kError) { \
        return ResultWithType(kError); \
    }(void)0

#define VISIT_CHECK_GET(type, node) if (auto rv = (node)->Accept(this); rv.sign == kError) { \
        return ResultWithType(kError); \
    } else { \
        type = rv.sign; \
    }(void)0

#define VISIT_CHECK_JUST(node) if (auto rv = (node)->Accept(this); rv.sign == kError) { \
        return ResultWithType(kError); \
    }(void)0

static inline ASTVisitor::Result ResultWithType(TypeSign *type) {
    ASTVisitor::Result rv;
    rv.kind = 0;
    rv.sign = type;
    return rv;
}

class SymbolTrackScope {
public:
    SymbolTrackScope(Symbolize *sym, std::unordered_set<void *> *symbol_track)
        : symbol_(sym)
        , symbol_track_(symbol_track)
        , has_tracked_(symbol_track_->find(sym) != symbol_track_->end()) {
        if (!has_tracked_) { symbol_track_->insert(symbol_); }
    }
    
    ~SymbolTrackScope() {
        if (!has_tracked_) { symbol_track_->erase(symbol_); }
    }

    DEF_VAL_GETTER(bool, has_tracked);
    
private:
    Symbolize *symbol_;
    std::unordered_set<void *> *symbol_track_;
    bool has_tracked_;
}; // class SymbolTrackScope

} // namespace

TypeChecker::TypeChecker(base::Arena *arena, SyntaxFeedback *feedback)
    : arena_(arena)
    , error_feedback_(feedback)
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
            //all_units_.push_back(unit);
            
            for (auto import : unit->import_packages()) {
                auto it1 = path_units_.find(import->original_path()->data());
                if (it1 != path_units_.end()) {
                    continue;
                }
                auto it2 = external_units.find(import->original_path()->data());
                if (it2 != external_units.end()) {
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
            error_feedback_->set_file_name(unit->file_name()->ToString());
            error_feedback_->set_package_name(unit->package_name()->ToString());
            for (auto decl : unit->global_variables()) {
                std::string name(pkg_name);
                name.append(".").append(decl->identifier()->ToString());
                auto iter = symbols_.find(name);
                if (iter != symbols_.end()) {
                    error_feedback_->Printf(unit->FindSourceLocation(decl), "Duplicated symbol: %s",
                                            name.c_str());
                    fail++;
                    continue;
                }
                symbols_[name] = decl;
                decl->set_file_unit(unit);
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
                def->set_file_unit(unit);
            }
        }
    }
    
    class_object_ = DCHECK_NOTNULL(FindSymbolOrNull("lang.Object")->AsClassDefinition());
    class_exception_ = DCHECK_NOTNULL(FindSymbolOrNull("lang.Exception")->AsClassDefinition());

    for (auto pair : pkg_units_) {
        for (auto unit : pair.second) {
            error_feedback_->set_file_name(unit->file_name()->ToString());
            error_feedback_->set_package_name(unit->package_name()->ToString());
            
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
    unit->set_scope(new FileScope(path_units_, &symbols_, unit, &current_));

    FileScope::Holder holder(unit->scope());
    FileScope *file_scope = down_cast<FileScope>(unit->scope());
    
    for (auto impl : unit->implements()) {
        Symbolize *sym = file_scope->FindOrNull(impl->prefix(), impl->name());
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
        
        for (auto method : impl->methods()) {
            std::string full_name =
                (impl->ToSymbolString(!impl->prefix() ? unit->package_name() : impl->prefix()))
                .append("::")
                .append(method->identifier()->ToString());
            symbols_[full_name] = method;
            method->prototype()->set_owner(impl->owner());
        }
    }

    for (auto def : unit->definitions()) {
        if (auto ast = def->AsClassDefinition(); ast != nullptr && ast->base_name()) {
            Symbolize *sym = file_scope->FindOrNull(ast->base_prefix(), ast->base_name());
            if (!sym || !sym->IsClassDefinition()) {
                error_feedback_->Printf(FindSourceLocation(ast), "Base class not found: %s",
                                        ast->ToBaseSymbolString().c_str());
                return false;
            }
            ast->set_base(sym->AsClassDefinition());
        }
        
        if (auto ast = def->AsClassDefinition();
            ast != nullptr && !ast->base_name() && ast != class_object_) {
            ast->set_base(class_object_);
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
        }
    }

    for (auto pair : pkg_units_) {
        for (auto unit : pair.second) {
            delete unit->scope();
            unit->set_scope(nullptr);
        }
    }
    return true;
}

bool TypeChecker::CheckFileUnit(const std::string &pkg_name, FileUnit *unit) {
    error_feedback_->set_file_name(unit->file_name()->ToString());
    error_feedback_->set_package_name(unit->package_name()->ToString());
    FileScope::Holder holder(unit->scope());
    FileScope *file_scope = down_cast<FileScope>(unit->scope());

    std::string exits_pkg_name;
    for (auto decl : unit->global_variables()) {
        if (!CheckDuplicatedSymbol(pkg_name, decl, file_scope)) {
            return false;
        }
        if (HasSymbolChecked(decl)) {
            continue;
        }
        if (auto rv = decl->Accept(this); rv.sign == kError) {
            return false;
        }
    }

    for (auto def : unit->definitions()) {
        if (!CheckDuplicatedSymbol(pkg_name, def, file_scope)) {
            return false;
        }
        if (HasSymbolChecked(def)) {
            continue;
        }
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
    switch (static_cast<Token::Kind>(ast->id())) {
        case Token::kIdentifier: {
            Symbolize *sym = current_->GetFileScope()->FindOrNull(ast->prefix(), ast->name());

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
        } break;
        case Token::kArray:
        case Token::kMap:
        case Token::kChannel:
            for (size_t i = 0; i < ast->parameters_size(); i++) {
                VISIT_CHECK_JUST(ast->parameter(i));
            }
            break;
        case Token::kFun: {
            VISIT_CHECK_JUST(ast->prototype()->return_type());
            for (auto param : ast->prototype()->parameters()) {
                VISIT_CHECK_JUST(param.type);
            }
        }
        default:
            break;
    }
    return ResultWithType(ast);
}

ASTVisitor::Result TypeChecker::VisitBoolLiteral(BoolLiteral *ast) /*override*/ {
    return ResultWithType(new (arena_) TypeSign(ast->position(), Token::kBool));
}

ASTVisitor::Result TypeChecker::VisitI8Literal(I8Literal *ast) /*override*/ {
    return ResultWithType(new (arena_) TypeSign(ast->position(), Token::kI8));
}

ASTVisitor::Result TypeChecker::VisitU8Literal(U8Literal *ast) /*override*/ {
    return ResultWithType(new (arena_) TypeSign(ast->position(), Token::kU8));
}

ASTVisitor::Result TypeChecker::VisitI16Literal(I16Literal *ast) /*override*/ {
    return ResultWithType(new (arena_) TypeSign(ast->position(), Token::kI16));
}

ASTVisitor::Result TypeChecker::VisitU16Literal(U16Literal *ast) /*override*/ {
    return ResultWithType(new (arena_) TypeSign(ast->position(), Token::kU16));
}

ASTVisitor::Result TypeChecker::VisitI32Literal(I32Literal *ast) /*override*/ {
    return ResultWithType(new (arena_) TypeSign(ast->position(), Token::kI32));
}

ASTVisitor::Result TypeChecker::VisitU32Literal(U32Literal *ast) /*override*/ {
    return ResultWithType(new (arena_) TypeSign(ast->position(), Token::kU32));
}

ASTVisitor::Result TypeChecker::VisitIntLiteral(IntLiteral *ast) /*override*/ {
    return ResultWithType(new (arena_) TypeSign(ast->position(), Token::kInt));
}

ASTVisitor::Result TypeChecker::VisitUIntLiteral(UIntLiteral *ast) /*override*/ {
    return ResultWithType(new (arena_) TypeSign(ast->position(), Token::kUInt));
}

ASTVisitor::Result TypeChecker::VisitI64Literal(I64Literal *ast) /*override*/ {
    return ResultWithType(new (arena_) TypeSign(ast->position(), Token::kI64));
}

ASTVisitor::Result TypeChecker::VisitU64Literal(U64Literal *ast) /*override*/ {
    return ResultWithType(new (arena_) TypeSign(ast->position(), Token::kU64));
}

ASTVisitor::Result TypeChecker::VisitF32Literal(F32Literal *ast) /*override*/ {
    return ResultWithType(new (arena_) TypeSign(ast->position(), Token::kF32));
}

ASTVisitor::Result TypeChecker::VisitF64Literal(F64Literal *ast) /*override*/ {
    return ResultWithType(new (arena_) TypeSign(ast->position(), Token::kF64));
}

ASTVisitor::Result TypeChecker::VisitNilLiteral(NilLiteral *ast) /*override*/ {
    return ResultWithType(new (arena_) TypeSign(ast->position(), Token::kNil));
}

ASTVisitor::Result TypeChecker::VisitStringLiteral(StringLiteral *ast) /*override*/ {
    return ResultWithType(new (arena_) TypeSign(ast->position(), Token::kString));
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
    Result rv;
    for (auto param : parameters) {
        VISIT_CHECK(param);
    }

    for (auto stmt : ast->statements()) {
        VISIT_CHECK(stmt);
    }
    
    if (ast->prototype()->return_type()->id() != Token::kVoid && !function_scope.has_broke()) {
        error_feedback_->Printf(FindSourceLocation(ast), "Return statement not found");
        return ResultWithType(kError);
    }
    return ResultWithType(new (arena_) TypeSign(ast->position(), ast->prototype()));
}

ASTVisitor::Result TypeChecker::VisitCallExpression(CallExpression *ast) /*override*/ {
    TypeSign *callee = nullptr;
    VISIT_CHECK_GET(callee, ast->callee());

    if (callee->id() == Token::kClass) { // Constructor
        if (ast->operands_size() != callee->clazz()->parameters_size()) {
            error_feedback_->Printf(FindSourceLocation(ast), "Unexpected class contructor: %s",
                                    callee->clazz()->identifier()->data());
            return ResultWithType(kError);
        }
        
        for (size_t i = 0; i < ast->operands_size(); i++) {
            TypeSign *argument_type = nullptr;
            VISIT_CHECK_GET(argument_type, ast->operand(i));
            
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
        ast->set_prototype(callee->prototype());
        if (callee->prototype()->vargs()) {
            if (ast->operands_size() < callee->prototype()->parameters_size()) {
                error_feedback_->Printf(FindSourceLocation(ast), "Unexpected function prototype: %s",
                                        callee->ToString().c_str());
                return ResultWithType(kError);
            }
        } else {
            if (ast->operands_size() != callee->prototype()->parameters_size()) {
                error_feedback_->Printf(FindSourceLocation(ast), "Unexpected function prototype: %s",
                                        callee->ToString().c_str());
                return ResultWithType(kError);
            }
        }

        size_t n = std::min(ast->operands_size(), callee->parameters_size());
        for (size_t i = 0; i < n; i++) {
            TypeSign *argument_type = nullptr;
            VISIT_CHECK_GET(argument_type, ast->operand(i));
            if (callee->prototype()->vargs() && i >= callee->prototype()->parameters_size()) {
                continue;
            }
            TypeSign *parameter_type = nullptr;
            VISIT_CHECK_GET(parameter_type, callee->prototype()->parameter(i).type);
            if (!parameter_type->Convertible(argument_type)) {
                error_feedback_->Printf(FindSourceLocation(ast), "Unexpected function prototype: "
                                        " %s at parameter[%ld]: %s", callee->ToString().c_str(), i,
                                        argument_type->ToString().c_str());
                return ResultWithType(kError);
            }
        }
        return ResultWithType(callee->prototype()->return_type());
    }
    
    error_feedback_->Printf(FindSourceLocation(ast), "Incorrect calling type(%s)",
                            callee->ToString().c_str());
    return ResultWithType(kError);
}

ASTVisitor::Result TypeChecker::VisitPairExpression(PairExpression *ast) /*override*/ {
    TypeSign *key = nullptr;
    if (!ast->addition_key()) {
        VISIT_CHECK_GET(key, ast->key());
    }

    TypeSign *value = nullptr;
    VISIT_CHECK_GET(value, ast->value());
    return ResultWithType(new (arena_) TypeSign(arena_, ast->position(), Token::kPair, key, value));
}

ASTVisitor::Result TypeChecker::VisitIndexExpression(IndexExpression *ast) /*override*/ {
    TypeSign *primary = nullptr, *index = nullptr;
    VISIT_CHECK_GET(primary, ast->primary());
    VISIT_CHECK_GET(index, ast->index());

    switch (static_cast<Token::Kind>(primary->id())) {
        case Token::kArray:
            if (!index->IsIntegral()) {
                error_feedback_->Printf(FindSourceLocation(index), "Incorrect type(%s) for array "
                                        "index, need integral type", index->ToString().c_str());
                return ResultWithType(kError);
            }
            ast->set_hint(primary->parameter(0)->Clone(arena_));
            return ResultWithType(ast->hint());
        case Token::kMap:
        case Token::kMutableMap:
            if (!primary->parameter(0)->Convertible(index)) {
                error_feedback_->Printf(FindSourceLocation(index), "Incorrect type for map "
                                        "key, %s vs %s", primary->parameter(0)->ToString().c_str(),
                                        index->ToString().c_str());
                return ResultWithType(kError);
            }
            ast->set_hint(primary->parameter(1)->Clone(arena_));
            return ResultWithType(ast->hint());
        default:
            error_feedback_->Printf(FindSourceLocation(ast), "Incorrect type(%s) for `[]' operator",
                                    primary->ToString().c_str());
            return ResultWithType(kError);
    }
    //return ResultWithType(kError);
}

ASTVisitor::Result TypeChecker::VisitUnaryExpression(UnaryExpression *ast) /*override*/ {
    TypeSign *operand = nullptr;
    VISIT_CHECK_GET(operand, ast->operand());
    switch (ast->op().kind) {
        case Operator::kNot:
            if (operand->id() != Token::kBool) {
                error_feedback_->Printf(FindSourceLocation(ast), "Incorrect type(%s), need bool",
                                        operand->ToString().c_str());
                return ResultWithType(kError);
            }
            break;
        case Operator::kBitwiseNot:
            if (!operand->IsIntegral()) {
                error_feedback_->Printf(FindSourceLocation(ast), "Incorrect type(%s), need integral"
                                        " type", operand->ToString().c_str());
                return ResultWithType(kError);
            }
            break;
        case Operator::kMinus:
            if (!operand->IsNumber()) {
                error_feedback_->Printf(FindSourceLocation(ast), "Incorrect type(%s), need number "
                                        "type", operand->ToString().c_str());
                return ResultWithType(kError);
            }
            break;
        case Operator::kRecv:
            if (operand->id() != Token::kChannel) {
                error_feedback_->Printf(FindSourceLocation(ast), "Incorrect type(%s), need channel "
                                        " type", operand->ToString().c_str());
                return ResultWithType(kError);
            }
            ast->set_hint(operand->parameter(0));
            return ResultWithType(operand->parameter(0));
        case Operator::kIncrement:
        case Operator::kIncrementPost:
            if (!CheckModifitionAccess(ast->operand())) {
                return ResultWithType(kError);
            }
            if (!operand->IsIntegral()) {
                error_feedback_->Printf(FindSourceLocation(ast), "Incorrect type(%s), need integral"
                                        " type", operand->ToString().c_str());
                return ResultWithType(kError);
            }
            break;
        case Operator::kDecrement:
        case Operator::kDecrementPost:
            if (!CheckModifitionAccess(ast->operand())) {
                return ResultWithType(kError);
            }
            if (!operand->IsIntegral()) {
                error_feedback_->Printf(FindSourceLocation(ast), "Incorrect type(%s), need integral"
                                        " type", operand->ToString().c_str());
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
    TypeSign *lhs = nullptr, *rhs = nullptr;
    VISIT_CHECK_GET(lhs, ast->lhs());
    VISIT_CHECK_GET(rhs, ast->rhs());

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
                error_feedback_->Printf(FindSourceLocation(ast), "Incorrect type, need number;"
                                        "%s and %s", lhs->ToString().c_str(),
                                        rhs->ToString().c_str());
                return ResultWithType(kError);
            }
            break;
            
        case Operator::kMod:
        case Operator::kBitwiseAnd:
        case Operator::kBitwiseOr:
        case Operator::kBitwiseXor:
            if (!lhs->IsIntegral() || !lhs->Convertible(rhs)) {
                error_feedback_->Printf(FindSourceLocation(ast), "Incorrect type, need integral "
                                        "number; %s and %s", lhs->ToString().c_str(),
                                        rhs->ToString().c_str());
                return ResultWithType(kError);
            }
            break;
            
        case Operator::kBitwiseShl:
        case Operator::kBitwiseShr:
            if (!lhs->IsIntegral() || !rhs->IsIntegral()) {
                error_feedback_->Printf(FindSourceLocation(ast), "Incorrect type, need integral "
                                        "number; %s and %s", lhs->ToString().c_str(),
                                        rhs->ToString().c_str());
                return ResultWithType(kError);
            }
            return ResultWithType(lhs);
            
        case Operator::kSend:
            if (lhs->id() != Token::kChannel) {
                error_feedback_->Printf(FindSourceLocation(ast), "Incorrect type(%s), need channel",
                                        lhs->ToString().c_str());
                return ResultWithType(kError);
            }
            if (!lhs->parameter(0)->Convertible(rhs)) {
                error_feedback_->Printf(FindSourceLocation(ast), "Incorrect send type, %s vs %s",
                                        lhs->parameter(0)->ToString().c_str(),
                                        rhs->ToString().c_str());
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
                // Number is ok
            } else if (lhs->id() == Token::kString && lhs->Convertible(rhs)) {
                // String is ok
            } else {
                error_feedback_->Printf(FindSourceLocation(ast), "Incorrect type: %s vs %s",
                                        lhs->ToString().c_str(), rhs->ToString().c_str());
                return ResultWithType(kError);
            }
            return ResultWithType(new (arena_) TypeSign(ast->position(), Token::kBool));
            
        case Operator::kAnd:
        case Operator::kOr:
            if (lhs->id() != Token::kBool || !lhs->Convertible(rhs)) {
                error_feedback_->Printf(FindSourceLocation(ast), "Incorrect type(%s), need bool",
                                        rhs->ToString().c_str());
                return ResultWithType(kError);
            }
            break;

        default:
            NOREACHED();
            break;
    }
    //DCHECK_EQ(lhs->id(), rhs->id());
    return ResultWithType(lhs);
}

ASTVisitor::Result TypeChecker::VisitTypeCastExpression(TypeCastExpression *ast) /*override*/ {
    TypeSign *operand = nullptr;
    VISIT_CHECK_GET(operand, ast->operand());
    TypeSign *dest = nullptr;
    VISIT_CHECK_GET(dest, ast->type());
    //VISIT_CHECK_JUST(ast->type());
    if (!operand->Castable(dest)) {
        error_feedback_->Printf(FindSourceLocation(ast), "Impossible cast %s to %s",
                                operand->ToString().c_str(), dest->ToString().c_str());
        return ResultWithType(kError);
    }
    return ResultWithType(dest->Clone(arena_));
}

ASTVisitor::Result TypeChecker::VisitTypeTestExpression(TypeTestExpression *ast) /*override*/ {
    TypeSign *operand = nullptr;
    VISIT_CHECK_GET(operand, ast->operand());
    TypeSign *dest = nullptr;
    VISIT_CHECK_GET(dest, ast->type());
    if (operand->IsNumber() || operand->id() == Token::kBool) {
        error_feedback_->Printf(FindSourceLocation(ast), "Impossible testing");
        return ResultWithType(kError);
    }
    return ResultWithType(new (arena_) TypeSign(ast->position(), Token::kBool));
}

ASTVisitor::Result TypeChecker::VisitArrayInitializer(ArrayInitializer *ast) /*override*/ {
    Result rv;
    if (ast->reserve()) {
        VISIT_CHECK(ast->element_type());
        VISIT_CHECK(ast->reserve());
        if (!rv.sign->IsIntegral()) {
            error_feedback_->Printf(FindSourceLocation(ast), "Incorrect array constructor "
                                    "parameter[0] type(%s), need integral number",
                                    rv.sign->ToString().c_str());
            return ResultWithType(kError);
        }
        return ResultWithType(new (arena_) TypeSign(arena_, ast->position(), Token::kArray,
                                                    ast->element_type()));
    }

    TypeSign *defined_element_type = ast->element_type();
    for (auto element : ast->operands()) {
        VISIT_CHECK(element);
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
            error_feedback_->Printf(FindSourceLocation(ast), "Attempt unconvertible element type, "
                                    "%s vs %s", ast->element_type()->ToString().c_str(),
                                    defined_element_type->ToString().c_str());
            return ResultWithType(kError);
        }
    } else {
        ast->set_element_type(defined_element_type);
    }

    VISIT_CHECK(ast->element_type());
    return ResultWithType(new (arena_) TypeSign(arena_, ast->position(), Token::kArray,
                                                ast->element_type()));
}

ASTVisitor::Result TypeChecker::VisitMapInitializer(MapInitializer *ast) /*override*/ {
    Result rv;
    if (ast->reserve()) {
        VISIT_CHECK(ast->reserve());
        if (!rv.sign->IsIntegral()) {
            error_feedback_->Printf(FindSourceLocation(ast), "Incorrect map constructor "
                                    "parameter[0] type(%s), need integral number",
                                    rv.sign->ToString().c_str());
            return ResultWithType(kError);
        }
        TypeSign *map = new (arena_) TypeSign(arena_, ast->position(), ast->mutable_container()
                                              ? Token::kMutableMap : Token::kMap,
                                              DCHECK_NOTNULL(ast->key_type()),
                                              DCHECK_NOTNULL(ast->value_type()));
        return ResultWithType(map);
    }

    TypeSign *defined_key_type = ast->key_type(), *defined_value_type = ast->value_type();
    for (auto pair : ast->operands()) {
        VISIT_CHECK(DCHECK_NOTNULL(pair->AsPairExpression())->key());
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

        VISIT_CHECK(DCHECK_NOTNULL(pair->AsPairExpression())->value());
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
            error_feedback_->Printf(FindSourceLocation(ast), "Attempt unconvertible key type, "
                                    "%s vs %s", ast->key_type()->ToString().c_str(),
                                    defined_key_type->ToString().c_str());
            return ResultWithType(kError);
        }
    } else {
        ast->set_key_type(defined_key_type);
    }
    if (ast->value_type()) {
        if (!ast->value_type()->Convertible(defined_value_type)) {
            error_feedback_->Printf(FindSourceLocation(ast), "Attempt unconvertible value type, "
                                    "%s vs %s", ast->value_type()->ToString().c_str(),
                                    defined_key_type->ToString().c_str());
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
        case BreakableStatement::CONTINUE: {
            BlockScope *scope = current_->GetLoopScope();
            if (!scope) {
                error_feedback_->Printf(FindSourceLocation(ast), "Break statement outside loop");
                return ResultWithType(kError);
            }
            if (current_->MarkBroke() > 0) {
                error_feedback_->Printf(FindSourceLocation(ast), "Useless break statement");
                return ResultWithType(kError);
            }
        } break;
        case BreakableStatement::THROW: {
            TypeSign *expect = nullptr;
            VISIT_CHECK_GET(expect, ast->value());
            if (expect->id() != Token::kRef) {
                error_feedback_->Printf(FindSourceLocation(ast->value()), "Incorrect throw "
                                        "type(%s)", expect->ToString().c_str());
                return ResultWithType(kError);
            }
            if (!expect->clazz()->SameOrBaseOf(class_exception_)) {
                error_feedback_->Printf(FindSourceLocation(ast->value()), "Incorrect throw "
                                        "type(%s), need base of Expected class",
                                        expect->ToString().c_str());
                return ResultWithType(kError);
            }
            if (current_->MarkBroke() > 0) {
                error_feedback_->Printf(FindSourceLocation(ast), "Useless throw statement");
                return ResultWithType(kError);
            }
        } break;
        case BreakableStatement::RETURN: {
            FunctionScope *scope = current_->GetFunctionScope();
            if (!scope) {
                error_feedback_->Printf(FindSourceLocation(ast), "Attempt return in function "
                                        "outside");
                return ResultWithType(kError);
            }
            
            TypeSign *ret_type = kVoid;
            if (ast->value()) {
                VISIT_CHECK_GET(ret_type, ast->value());
                if (ret_type->id() == Token::kVoid) {
                    error_feedback_->Printf(FindSourceLocation(ast), "Attempt return void type");
                    return ResultWithType(kError);
                }
            }

            TypeSign *decl_type = scope->function()->prototype()->return_type();
            if (!decl_type->Convertible(ret_type)) {
                error_feedback_->Printf(FindSourceLocation(ast), "Unconvertible return type, "
                                        "%s vs %s", decl_type->ToString().c_str(),
                                        ret_type->ToString().c_str());
                return ResultWithType(kError);
            }
            
            if (current_->MarkBroke() > 0) {
                error_feedback_->Printf(FindSourceLocation(ast), "Useless return statement");
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
    VISIT_CHECK_GET(rval, ast->rval());
    VISIT_CHECK_GET(lval, ast->lval());
    
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
                error_feedback_->Printf(FindSourceLocation(ast), "Incorrect rval type(%s) in `=' "
                                        "expression, lval is %s", rval->ToString().c_str(),
                                        lval->ToString().c_str());
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
        VISIT_CHECK_JUST(operand);
    }
    return ResultWithType(new (arena_) TypeSign(ast->position(), Token::kString));
}

ASTVisitor::Result TypeChecker::VisitVariableDeclaration(VariableDeclaration *ast) /*override*/ {
    if (auto err = current_->Register(ast)) {
        error_feedback_->Printf(FindSourceLocation(ast), "Duplicated variable declaration: %s",
                                err->identifier()->data());
        return ResultWithType(kError);
    }

    SymbolTrackScope track_scope(ast, &symbol_track_);
    if (track_scope.has_tracked()) {
        error_feedback_->Printf(FindSourceLocation(ast), "Dependence circle reference: %s",
                                ast->identifier()->data());
        return ResultWithType(kError);
    }

    if (ast->type() && ast->type()->id() == Token::kIdentifier) {
        VISIT_CHECK_JUST(ast->type());
    }

    if (ast->initializer()) {
        TypeSign *init = nullptr;
        VISIT_CHECK_GET(init, ast->initializer());
        if (init->id() == Token::kVoid) {
            error_feedback_->Printf(FindSourceLocation(ast), "Void type for variable declaration");
            return ResultWithType(kError);
        }

        if (!ast->type()) {
            ast->set_type(init);
        } else {
            if (!ast->type()->Convertible(init)) {
                error_feedback_->Printf(FindSourceLocation(ast), "Unconvertible type");
                return ResultWithType(kError);
            }
        }
    }

    if (ast->type()->id() == Token::kClass) {
        ast->type()->set_id(Token::kRef);
    }
    symbol_trace_.insert(ast);
    return ResultWithType(kVoid);
}

ASTVisitor::Result TypeChecker::VisitChannelInitializer(ChannelInitializer *ast) /*override*/ {
    // val ch = channel[int](1)
    // ch <- 100
    // ch <- 200
    Result rv;
    VISIT_CHECK(ast->value_type());
    if (ast->buffer_size()) {
        VISIT_CHECK(ast->buffer_size());
        if (rv.sign->id() != Token::kInt) {
            error_feedback_->Printf(FindSourceLocation(ast->buffer_size()), "Unexpected channel "
                                    "buffer-size type(%s), need int", rv.sign->ToString().c_str());
            return ResultWithType(kError);
        }
    }

    TypeSign *type = new (arena_) TypeSign(arena_, ast->position(), Token::kChannel, ast->value_type());
    return ResultWithType(type);
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
        case Scope::kFileScope:
            ast->set_scope(static_cast<FileScope *>(scope)->file_unit());
            break;
        case Scope::kClassScope:
            ast->set_scope(static_cast<ClassScope *>(scope)->clazz());
            break;
        case Scope::kFunctionScope:
            ast->set_scope(scope->GetFunctionScope()->function());
            break;
        case Scope::kPlainBlockScope:
        case Scope::kIfBlockScope:
        case Scope::kLoopBlockScope:
            ast->set_scope(down_cast<BlockScope>(scope)->stmt());
            break;
        default:
            NOREACHED();
            break;
    }

    if (!HasSymbolChecked(sym) && !CheckSymbolDependence(sym)) {
        return ResultWithType(kError);
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
    return ResultWithType(DCHECK_NOTNULL(sym->AsVariableDeclaration()->type())->Clone(arena_));
}

ASTVisitor::Result TypeChecker::VisitDotExpression(DotExpression *ast) {
    if (auto id = ast->primary()->AsIdentifier()) {
        FileScope *file_scope = current_->GetFileScope();
        Symbolize *sym = file_scope->FindOrNull(id->name(), ast->rhs());
        if (sym) {
            if (!HasSymbolChecked(sym) && !CheckSymbolDependence(sym)) {
                return ResultWithType(kError);
            }
            if (auto decl = sym->AsVariableDeclaration()) {
                return ResultWithType(DCHECK_NOTNULL(decl->type())->Clone(arena_));
            } else if (auto object = sym->AsObjectDefinition()) {
                return ResultWithType(new (arena_) TypeSign(ast->position(), object));
            } else if (auto clazz = sym->AsClassDefinition()) {
                return ResultWithType(new (arena_) TypeSign(ast->position(), clazz));
            } else if (auto fun = sym->AsFunctionDefinition()) {
                return ResultWithType(new (arena_) TypeSign(ast->position(), fun->prototype()));
            } else {
                error_feedback_->Printf(FindSourceLocation(ast), "Incorrect type to get field");
                return ResultWithType(kError);
            }
        }

        DCHECK(sym == nullptr);
        sym = std::get<1>(current_->Resolve(id->name()));
        if (sym) {
            if (!HasSymbolChecked(sym) && !CheckSymbolDependence(sym)) {
                return ResultWithType(kError);
            }
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
        TypeSign *primary = nullptr;
        VISIT_CHECK_GET(primary, ast->primary());
        return CheckDotExpression(DCHECK_NOTNULL(primary), ast);
    }
    return ResultWithType(kVoid);
}

ASTVisitor::Result TypeChecker::VisitInterfaceDefinition(InterfaceDefinition *ast) /*override*/ {
    Result rv;
    for (auto method : ast->methods()) {
        VISIT_CHECK(method);
    }
    return ResultWithType(kVoid);
}

ASTVisitor::Result TypeChecker::VisitObjectDefinition(ObjectDefinition *ast) /*override*/ {
    FunctionScope constructor_scope(ast, &current_);
    TypeSign *type = new (arena_) TypeSign(ast->position(), Token::kRef, ast);
    VariableDeclaration *self = new (arena_) VariableDeclaration(type->position(),
                                                                 VariableDeclaration::PARAMETER,
                                                                 ASTString::New(arena_, "self"),
                                                                 type, nullptr);
    constructor_scope.Register(self);
    symbol_trace_.insert(self);
    
    Result rv;
    for (auto field : ast->fields()) {
        VISIT_CHECK(field.declaration);
    }
    symbol_trace_.insert(ast);
    return ResultWithType(kVoid);
}

ASTVisitor::Result TypeChecker::VisitClassDefinition(ClassDefinition *ast) /*override*/ {
    SymbolTrackScope track_scope(ast, &symbol_track_);
    if (track_scope.has_tracked()) {
        error_feedback_->Printf(FindSourceLocation(ast), "Dependence circle reference: %s",
                                ast->identifier()->data());
        return ResultWithType(kError);
    }

    Result rv;
    for (auto param : ast->parameters()) {
        if (param.field_declaration) {
            continue;
        }
        VISIT_CHECK(param.as_parameter);
    }
    
    for (auto field : ast->fields()) {
        VISIT_CHECK(field.declaration);
    }
    
    if (ast->base()) {
        FunctionScope constructor_scope(ast, &current_);

        if (ast->arguments_size() != ast->base()->parameters_size()) {
            error_feedback_->Printf(FindSourceLocation(ast), "Unexpected base class constructor "
                                    "expected %ld parameters", ast->arguments_size());
            return ResultWithType(kError);
        }

        for (size_t i = 0; i < ast->arguments_size(); i++) {
            TypeSign *arg_type = nullptr;
            VISIT_CHECK_GET(arg_type, ast->argument(i));
            auto param = ast->base()->parameter(i);
            auto param_type = param.field_declaration
                ? ast->base()->field(param.as_field).declaration->type()
                : param.as_parameter->type();
            VISIT_CHECK(param_type);
            if (!param_type->Convertible(arg_type)) {
                error_feedback_->Printf(FindSourceLocation(ast), "Unexpected base class "
                                        "constructor, parameter[%zd] %s vs %s", i,
                                        param_type->ToString().c_str(), rv.sign->ToString().c_str());
                return ResultWithType(kError);
            }
        }
    }
    symbol_trace_.insert(ast);
    return ResultWithType(kVoid);
}

ASTVisitor::Result TypeChecker::VisitFunctionDefinition(FunctionDefinition *ast) /*override*/ {
    SymbolTrackScope track_scope(ast, &symbol_track_);
    if (track_scope.has_tracked()) {
        return ResultWithType(kVoid); // Recursive calling
    }
    if (!current_->is_file_scope()) { // embed function
        current_->Register(ast);
    }

    Result rv;
    for (size_t i = 0; i < ast->prototype()->parameters_size(); i++) {
        auto param = &ast->prototype()->mutable_parameters()->at(i);
        if (param->type->id() == Token::kIdentifier) {
            VISIT_CHECK(param->type);
        }
    }
    if (DCHECK_NOTNULL(ast->return_type())->id() == Token::kIdentifier) {
        VISIT_CHECK(ast->return_type());
    }

    if (ast->is_native() || ast->is_declare()) {
        return ResultWithType(kVoid);
    }

    VISIT_CHECK(ast->body());
    symbol_trace_.insert(ast);
    return ResultWithType(kVoid);
}

ASTVisitor::Result TypeChecker::VisitClassImplementsBlock(ClassImplementsBlock *ast) /*override*/ {
    ClassScope class_scope(ast->owner(), &current_);

    for (auto method : ast->methods()) {
        if (HasSymbolChecked(method)) {
            continue;
        }
        VISIT_CHECK_JUST(method);
    }
    return ResultWithType(kVoid);
}

ASTVisitor::Result TypeChecker::VisitWhileLoop(WhileLoop *ast) /*override*/ {
    BlockScope block_scope(Scope::kLoopBlockScope, ast, &current_);
    Result rv;

    VISIT_CHECK(ast->condition());
    if (rv.sign->id() != Token::kBool) {
        error_feedback_->Printf(FindSourceLocation(ast->condition()), "Incorrect condition type, "
                                "need bool");
        return ResultWithType(kError);
    }

    for (auto stmt : ast->statements()) {
        VISIT_CHECK(stmt);
    }
    return ResultWithType(kVoid);
}

ASTVisitor::Result TypeChecker::VisitForLoop(ForLoop *ast) /*override*/ {
    BlockScope block_scope(Scope::kLoopBlockScope, ast, &current_);
    Result rv;

    switch (ast->control()) {
        case ForLoop::STEP:
            rv = CheckForStep(ast);
            break;
        case ForLoop::EACH:
            rv = CheckForeach(ast);
            break;
        default:
            NOREACHED();
            break;
    }
    if (rv.sign == kError) {
        return ResultWithType(kError);
    }
    for (auto stmt : ast->statements()) {
        VISIT_CHECK(stmt);
    }
    return ResultWithType(kVoid);
}

ASTVisitor::Result TypeChecker::VisitWhenExpression(WhenExpression *ast) /*override*/ {
    std::unique_ptr<BlockScope> block_scope;
    TypeSign *type = nullptr, *primary = nullptr;
    Result rv;
    if (ast->primary()) {
        if (ast->primary()->IsVariableDeclaration()) {
            block_scope.reset(new BlockScope(Scope::kPlainBlockScope, ast, &current_));
            VISIT_CHECK(ast->primary());
            primary = ast->primary()->AsVariableDeclaration()->type();
        } else {
            VISIT_CHECK(ast->primary());
            primary = rv.sign;
        }
    }

    for (const auto &clause : ast->clauses()) {
        VariableDeclaration *decl = nullptr;
        if (!clause.is_multi && clause.single_case->IsVariableDeclaration()) {
            decl = clause.single_case->AsVariableDeclaration();
            if (!primary) {
                error_feedback_->Printf(FindSourceLocation(decl), "Invalid type cast case, miss "
                                        "primary expression");
                return ResultWithType(kError);
            }
            
            VISIT_CHECK(decl->type());
            if (!primary->Castable(decl->type())) {
                error_feedback_->Printf(FindSourceLocation(decl), "Invalid type cast case, "
                                        "impossible cast %s to %s", primary->ToString().c_str(),
                                        decl->type()->ToString().c_str());
                return ResultWithType(kError);
            }
        } else {
            //VISIT_CHECK(expect);
#define CHECK_EXPECT(expect) \
            if (primary) { \
                if (!primary->Convertible(rv.sign)) { \
                    error_feedback_->Printf(FindSourceLocation(expect), "Invalid when case, " \
                                            "incorrect type %s vs %s", primary->ToString().c_str(), \
                                            rv.sign->ToString().c_str()); \
                    return ResultWithType(kError); \
                } \
            } else { \
                if (rv.sign->id() != Token::kBool) { \
                    error_feedback_->Printf(FindSourceLocation(expect), "Invalid condition case" \
                                            "unexpected bool type, expected %s", \
                                            rv.sign->ToString().c_str()); \
                    return ResultWithType(kError); \
                } \
            } (void)0
            
            if (clause.is_multi) {
                for (auto expect : *clause.multi_cases) {
                    VISIT_CHECK(expect);
                    CHECK_EXPECT(expect);
                }
            } else {
                VISIT_CHECK(clause.single_case);
                CHECK_EXPECT(clause.single_case);
            }
        }
        BlockScope clause_scope(Scope::kPlainBlockScope, clause.body, &current_);
        if (decl) {
            clause_scope.Register(decl);
            symbol_trace_.insert(decl);
        }
        VISIT_CHECK(clause.body);
        if (!type) {
            type = rv.sign;
        }
        if (rv.sign->id() == Token::kVoid) {
            type->set_id(Token::kVoid);
        } else if (!type->Convertible(rv.sign)) {
            type->set_id(Token::kAny);
        }
    }
    
    VISIT_CHECK(DCHECK_NOTNULL(ast->else_clause()));
    DCHECK(type != nullptr);
    if (rv.sign->id() == Token::kVoid) {
        type->set_id(Token::kVoid);
    } else if (!type->Convertible(rv.sign)) {
        type->set_id(Token::kAny);
    }
    ast->set_hint(type);
    return ResultWithType(type);
}

ASTVisitor::Result TypeChecker::VisitIfExpression(IfExpression *ast) /*override*/ {
    BlockScope block_scope(Scope::kIfBlockScope, ast, &current_);
    TypeSign *type = nullptr;
    Result rv;
    do {
        if (ast->extra_statement()) {
            VISIT_CHECK(ast->extra_statement());
        }

        VISIT_CHECK(ast->branch_true());
        if (!type) {
            type = rv.sign;
        }
        if (type->id() == Token::kVoid || rv.sign->id() == Token::kVoid) {
            type->set_id(Token::kVoid);
        } else if (!type->Convertible(rv.sign)) {
            type->set_id(Token::kAny);
        }

        if (!ast->branch_false()) {
            type->set_id(Token::kVoid);
            break;
        }
        if (ast->branch_false()) {
            IfExpression *next = ast->branch_false()->AsIfExpression(); // else if
            if (!next) {
                VISIT_CHECK(ast->branch_false());
                if (type->id() == Token::kVoid || rv.sign->id() == Token::kVoid) {
                    type->set_id(Token::kVoid);
                } else if (!type->Convertible(rv.sign)) {
                    type = new (arena_) TypeSign(ast->position(), Token::kAny);
                }
            }
            ast = next;
        }
    } while (ast);
    return ResultWithType(DCHECK_NOTNULL(type));
}

ASTVisitor::Result TypeChecker::VisitStatementBlock(StatementBlock *ast) /*override*/ {
    BlockScope block_scope(Scope::kPlainBlockScope, ast, &current_);
    Result rv;
    for (auto stmt : ast->statements()) {
        VISIT_CHECK(stmt);
    }
    return ResultWithType(kVoid);
}

ASTVisitor::Result TypeChecker::VisitTryCatchFinallyBlock(TryCatchFinallyBlock *ast) /*override*/ {
    Result rv;
    {
        BlockScope block_scope(Scope::kPlainBlockScope, ast, &current_);
        for (auto stmt : ast->try_statements()) {
            VISIT_CHECK(stmt);
        }
    }

    for (auto block : ast->catch_blocks()) {
        BlockScope block_scope(Scope::kPlainBlockScope, ast, &current_);
        VISIT_CHECK(block->expected_declaration());
        TypeSign *expect = block->expected_declaration()->type();
        if (expect->id() != Token::kRef) {
            error_feedback_->Printf(FindSourceLocation(block->expected_declaration()), "Incorrect "
                                    "catch block expected type(%s)", expect->ToString().c_str());
            return ResultWithType(kError);
        }
        if (!expect->clazz()->SameOrBaseOf(class_exception_)) {
            error_feedback_->Printf(FindSourceLocation(block->expected_declaration()), "Incorrect "
                                    "catch block expected type(%s), need base of Expected class",
                                    expect->ToString().c_str());
            return ResultWithType(kError);
        }

        for (auto stmt : block->statements()) {
            VISIT_CHECK(stmt);
        }
    }
    
    BlockScope block_scope(Scope::kPlainBlockScope, ast, &current_);
    for (auto stmt : ast->finally_statements()) {
        VISIT_CHECK(stmt);
    }
    return ResultWithType(kVoid);
}

ASTVisitor::Result TypeChecker::VisitRunStatement(RunStatement *ast) /*override*/ {
    VISIT_CHECK_JUST(ast->calling());
    return ResultWithType(kVoid);
}

ASTVisitor::Result TypeChecker::CheckForStep(ForLoop *ast) {
    TypeSign *subject = nullptr;
    VISIT_CHECK_GET(subject, ast->subject());
    if (!subject->IsNumber()) {
        error_feedback_->Printf(FindSourceLocation(ast->subject()), "Incorrect subject type(%s), "
                                "need number", subject->ToString().c_str());
        return ResultWithType(kError);
    }
    TypeSign *limit = nullptr;
    VISIT_CHECK_GET(limit, ast->limit());
    if (subject->id() != limit->id()) {
        error_feedback_->Printf(FindSourceLocation(ast->limit()), "Different limit type(%s), but "
                                "subject is %s", limit->ToString().c_str(),
                                subject->ToString().c_str());
        return ResultWithType(kError);
    }
    
    if (ast->value()->type() && ast->value()->type()->id() != subject->id()) {
        error_feedback_->Printf(FindSourceLocation(ast->limit()), "Different value type(%s), but "
                                "subject is %s", ast->value()->type()->ToString().c_str(),
                                subject->ToString().c_str());
        return ResultWithType(kError);
    } else {
        ast->value()->set_type(subject);
    }
    if (ast->value()) {
        VISIT_CHECK_JUST(ast->value());
    }
    return ResultWithType(kVoid);
}

ASTVisitor::Result TypeChecker::CheckForeach(ForLoop *ast) {
    TypeSign *subject = nullptr;
    VISIT_CHECK_GET(subject, ast->subject());
    if (subject->id() != Token::kArray && subject->id() != Token::kMap &&
        subject->id() != Token::kMutableMap) {
        error_feedback_->Printf(FindSourceLocation(ast->subject()), "Incorrect subject type(%s), "
                                "need array/mutable_array/map/mutable_map",
                                subject->ToString().c_str());
        return ResultWithType(kError);
    }
    
    if (subject->id() == Token::kArray) {
        if (ast->key()) {
            if (ast->key()->type() && ast->key()->type()->id() != Token::kInt) {
                error_feedback_->Printf(FindSourceLocation(ast->limit()), "Incorrect index type(%s)"
                                        ", need int", ast->key()->type()->ToString().c_str());
                return ResultWithType(kError);
            } else {
                ast->key()->set_type(new (arena_) TypeSign(ast->key()->position(),
                                                           Token::kInt));
            }
        }
        if (ast->value()) {
            if (ast->value()->type() && !ast->value()->type()->Convertible(subject->parameter(0))) {
                error_feedback_->Printf(FindSourceLocation(ast->limit()), "Incorrect value type(%s)"
                                        ", need %s", ast->value()->type()->ToString().c_str(),
                                        subject->parameter(0)->ToString().c_str());
                return ResultWithType(kError);
            } else {
                ast->value()->set_type(subject->parameter(0));
            }
        }
    } else if (subject->id() == Token::kChannel) {
        // TODO:
        TODO();
    } else { // is map
        if (ast->key()) {
            if (ast->key()->type() && !ast->key()->type()->Convertible(subject->parameter(0))) {
                error_feedback_->Printf(FindSourceLocation(ast->limit()), "Incorrect index type(%s)"
                                        ", need %s", ast->key()->type()->ToString().c_str(),
                                        subject->parameter(0)->ToString().c_str());
                return ResultWithType(kError);
            } else {
                ast->key()->set_type(subject->parameter(0));
            }
        }
        if (ast->value()) {
            if (ast->value()->type() && !ast->value()->type()->Convertible(subject->parameter(1))) {
                error_feedback_->Printf(FindSourceLocation(ast->limit()), "Incorrect value type(%s)"
                                        ", need %s", ast->value()->type()->ToString().c_str(),
                                        subject->parameter(0)->ToString().c_str());
                return ResultWithType(kError);
            } else {
                ast->value()->set_type(subject->parameter(1));
            }
        }
    }
    Result rv;
    if (ast->key()) {
        VISIT_CHECK(ast->key());
    }
    if (ast->value()) {
        VISIT_CHECK(ast->value());
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
            if (!::strncmp("length", ast->rhs()->data(), ast->rhs()->size())) {
                return ResultWithType(new (arena_) TypeSign(ast->position(), Token::kU32));
            } else if (!::strncmp("capacity", ast->rhs()->data(), ast->rhs()->size())) {
                return ResultWithType(new (arena_) TypeSign(ast->position(), Token::kU32));
            } else if (!::strncmp("resize", ast->rhs()->data(), ast->rhs()->size())) {
                FunctionPrototype *proto = new (arena_) FunctionPrototype(arena_);
                proto->InsertParameter(nullptr, new (arena_) TypeSign(ast->position(), Token::kU32));
                proto->set_return_type(type);
                return ResultWithType(new (arena_) TypeSign(ast->position(), proto));
            } else {
                error_feedback_->Printf(FindSourceLocation(ast), "No field/method named %s in %s",
                                        ast->rhs()->data(), type->ToString().c_str());
                return ResultWithType(kError);
            }
            break;
        case Token::kMap:
            if (!::strncmp("length", ast->rhs()->data(), ast->rhs()->size())) {
                return ResultWithType(new (arena_) TypeSign(ast->position(), Token::kU32));
            } else if (!::strncmp("put", ast->rhs()->data(), ast->rhs()->size())) {
                FunctionPrototype *proto = new (arena_) FunctionPrototype(arena_);
                proto->InsertParameter(nullptr, type->parameter(0));
                proto->InsertParameter(nullptr, type->parameter(1));
                proto->set_return_type(type);
                return ResultWithType(new (arena_) TypeSign(ast->position(), proto));
            } else if (!::strncmp("remove", ast->rhs()->data(), ast->rhs()->size())) {
                FunctionPrototype *proto = new (arena_) FunctionPrototype(arena_);
                proto->InsertParameter(nullptr, type->parameter(0));
                proto->set_return_type(type);
                return ResultWithType(new (arena_) TypeSign(ast->position(), proto));
            } else if (!::strncmp("containsKey", ast->rhs()->data(), ast->rhs()->size())) {
                FunctionPrototype *proto = new (arena_) FunctionPrototype(arena_);
                proto->InsertParameter(nullptr, type->parameter(0));
                proto->set_return_type(new (arena_) TypeSign(ast->position(), Token::kBool));
                return ResultWithType(new (arena_) TypeSign(ast->position(), proto));
            } else {
                error_feedback_->Printf(FindSourceLocation(ast), "No field/method named %s in %s",
                                        ast->rhs()->data(), type->ToString().c_str());
                return ResultWithType(kError);
            }
            break;
        case Token::kChannel: {
            if (!::strncmp("hasClose", ast->rhs()->data(), ast->rhs()->size())) {
                return ResultWithType(new (arena_) TypeSign(ast->position(), Token::kBool));
            } else if (!::strncmp("length", ast->rhs()->data(), ast->rhs()->size())) {
                return ResultWithType(new (arena_) TypeSign(ast->position(), Token::kU32));
            } else if (!::strncmp("capacity", ast->rhs()->data(), ast->rhs()->size())) {
                return ResultWithType(new (arena_) TypeSign(ast->position(), Token::kU32));
            } else if (!::strncmp("close", ast->rhs()->data(), ast->rhs()->size())) {
                FunctionPrototype *proto = new (arena_) FunctionPrototype(arena_);
                proto->set_return_type(kVoid);
                return ResultWithType(new (arena_) TypeSign(ast->position(), proto));
            } else {
                error_feedback_->Printf(FindSourceLocation(ast), "No field/method named %s in %s",
                                       ast->rhs()->data(), type->ToString().c_str());
                return ResultWithType(kError);
            }
        } break;
        case Token::kInterface:
        case Token::kRef:
        case Token::kObject:
        case Token::kClass:
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
            return ResultWithType(DCHECK_NOTNULL(field->declaration->type())->Clone(arena_));
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
        return ResultWithType(DCHECK_NOTNULL(field->declaration->type())->Clone(arena_));
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
                error_feedback_->Printf(FindSourceLocation(ast), "Attempt modify val variable: %s",
                                        sym->identifier()->data());
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
    if (lval->id() == Token::kArray) {
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
    if (lval->id() == Token::kArray) {
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

bool TypeChecker::CheckSymbolDependence(Symbolize *sym) {
    if (sym->file_unit() && current_->GetFileScope()->file_unit() != sym->file_unit()) {
        auto saved = current_;
        current_ = nullptr;
        {
            FileScope::Holder holder(sym->file_unit()->scope());
            if (auto rv = sym->Accept(this); rv.sign == kError) {
                return false;
            }
        }
        DCHECK(current_ == nullptr);
        current_ = saved;
    } else {
        if (auto rv = sym->Accept(this); rv.sign == kError) {
            return false;
        }
    }
    return true;
}

bool TypeChecker::CheckDuplicatedSymbol(const std::string &pkg_name, const Symbolize *sym,
                                        FileScope *file_scope) {
    std::string_view exits_pkg_name;
    if (file_scope->FindExcludePackageNameOrNull(sym->identifier(), pkg_name, &exits_pkg_name)) {
        error_feedback_->Printf(FindSourceLocation(sym), "Duplicated symbol '%s', already exist at "
                                "package %s", sym->identifier()->data(),
                                exits_pkg_name.data());
        return false;
    }
    return true;
}

} // namespace lang

} // namespace mai
