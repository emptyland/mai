#include "lang/bytecode-generator.h"
#include "lang/token.h"
#include "lang/bytecode-array-builder.h"
#include "lang/machine.h"
#include "lang/isolate-inl.h"
#include "lang/metadata-space.h"
#include "lang/metadata.h"
#include "lang/stack-frame.h"
#include "base/arenas.h"

namespace mai {

namespace lang {

class BytecodeGenerator::Scope : public AbstractScope {
public:
    enum Kind {
        kFileScope,
        kClassScope,
        kFunctionScope,
        kLoopBlockScope,
        kPlainBlockScope,
    };
    
    DEF_VAL_GETTER(Kind, kind);
    DEF_PTR_GETTER(AbstractScope, prev);
    
    bool is_file_scope() const { return kind_ == kFileScope; }
    bool is_class_scope() const { return kind_ == kClassScope; }
    bool is_function_scope() const { return kind_ == kFunctionScope; }
    
    virtual Value Find(const ASTString *name) { return {Value::kError}; }

    virtual void Register(const std::string &name, Value value) {}

    std::tuple<Scope *, Value> Resolve(const ASTString *name) {
        for (auto i = this; i != nullptr; i = i->prev_) {
            if (Value value = i->Find(name); value.linkage != Value::kError) {
                return {i, value};
            }
        }
        return {nullptr, {Value::kError}};
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

    FunctionScope *GetFunctionScope() {
        return down_cast<FunctionScope>(GetScope(kFunctionScope));
    }
    
    BlockScope *GetLocalBlockScope(Kind kind) {
        for (Scope *scope = this; scope != nullptr; scope = scope->prev_) {
            if (scope->kind_ == kFunctionScope) {
                break;
            }
            if (scope->kind_ == kind) {
                return down_cast<BlockScope>(scope);
            }
        }
        return nullptr;
    }

    DISALLOW_IMPLICIT_CONSTRUCTORS(Scope);
protected:
    Scope(Kind kind, Scope **current)
        : kind_(kind)
        , current_(current) {}
    
    ~Scope() {}
    
    virtual AbstractScope *Enter() override {
        prev_ = *current_;
        DCHECK_NE(*current_, this);
        *current_ = this;
        return prev_;
    }

    virtual AbstractScope *Exit() override {
        DCHECK_EQ(*current_, this);
        *current_ = prev_;
        return prev_;
    }

    Kind kind_;
    Scope **current_;
    Scope *prev_ = nullptr;
}; // class BytecodeGenerator::AbstractScope

class BytecodeGenerator::FileScope : public Scope {
public:
    FileScope(const std::map<std::string, std::vector<FileUnit *>> &path_units,
              std::unordered_map<std::string, Value> *symbols,
              FileUnit *file_unit, Scope **current, FileScope **current_file);
    ~FileScope() override = default;
    
    DEF_PTR_GETTER(FileUnit, file_unit);
    
    void Register(const std::string &name, Value value) override {
        DCHECK(symbols_->find(name) == symbols_->end());
        (*symbols_)[name] = value;
    }
    
    Value Find(const ASTString *prefix, const ASTString *name);
    Value Find(const ASTString *name) override {
        std::string_view exists;
        return FindExcludePackageName(name, "", &exists);
    }
    Value FindExcludePackageName(const ASTString *name, std::string_view exclude,
                                 std::string_view *exists);
    
    std::string LocalFileFullName(const ASTString *name) const {
        return LocalFileFullName(file_unit_, name);
    }
    
    static std::string LocalFileFullName(const FileUnit *unit, const ASTString *name) {
        return unit->package_name()->ToString() + "." + name->ToString();
    }

    DISALLOW_IMPLICIT_CONSTRUCTORS(FileScope);
private:
    AbstractScope *Enter() override {
        auto old = Scope::Enter();
        DCHECK(*current_file_ == nullptr);
        *current_file_ = this;
        return old;
    }

    AbstractScope *Exit() override {
        DCHECK_EQ(this, *current_file_);
        *current_file_ = nullptr;
        return Scope::Exit();
    }

    FileUnit *const file_unit_;
    std::unordered_map<std::string, Value> *symbols_;
    std::set<std::string> all_name_space_;
    std::map<std::string, std::string> alias_name_space_;
    FileScope **current_file_;
}; // class BytecodeGenerator::FileScope

BytecodeGenerator::FileScope::FileScope(const std::map<std::string, std::vector<FileUnit *>> &path_units,
                                        std::unordered_map<std::string, Value> *symbols,
                                        FileUnit *file_unit,
                                        Scope **current,
                                        FileScope **current_file)
    : Scope(kFileScope, current)
    , file_unit_(file_unit)
    , symbols_(symbols)
    , current_file_(current_file) {
    for (auto stmt : file_unit->import_packages()) {
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
    all_name_space_.insert(file_unit->package_name()->ToString());
    
}

BytecodeGenerator::Value BytecodeGenerator::FileScope::Find(const ASTString *prefix,
                                                            const ASTString *name) {
    if (!prefix) {
        return Find(name);
    }
    if (auto iter = alias_name_space_.find(prefix->ToString()); iter == alias_name_space_.end()) {
        return {Value::kError};
    }
    std::string full_name(prefix->ToString());
    full_name.append(".").append(name->ToString());
    if (auto iter = symbols_->find(full_name); iter == symbols_->end()) {
        return {Value::kError};
    } else {
        return iter->second;
    }
}

BytecodeGenerator::Value
BytecodeGenerator::FileScope::FindExcludePackageName(const ASTString *name, std::string_view exclude,
                                                     std::string_view *exists) {
    for (const auto &pkg : all_name_space_) {
        if (exclude.compare(pkg) == 0) {
            continue;
        }
        std::string full_name(pkg);
        full_name.append(".").append(name->ToString());
        if (auto iter = symbols_->find(full_name); iter != symbols_->end()) {
            *exists = pkg;
            return iter->second;
        }
    }
    return {Value::kError};
}

class BytecodeGenerator::ClassScope : public Scope {
public:
    ClassScope(StructureDefinition *clazz, Scope **current, ClassScope **current_class)
        : Scope(kClassScope, current)
        , clazz_(clazz)
        , current_class_(current_class) {
        Enter();
        prev_class_ = *current_class_;
        DCHECK_NE(this, *current_class_);
        *current_class_ = this;
    }

    ~ClassScope() {
        DCHECK_EQ(this, *current_class_);
        *current_class_ = prev_class_;
        Exit();
    }
    
    DEF_PTR_GETTER(StructureDefinition, clazz);
    
    DISALLOW_IMPLICIT_CONSTRUCTORS(ClassScope);
private:
    StructureDefinition *clazz_;
    ClassScope *prev_class_;
    ClassScope **current_class_;
}; // class BytecodeGenerator::ClassScope


class BytecodeGenerator::BlockScope : public Scope {
public:
    inline BlockScope(Kind kind, Scope **current);
    ~BlockScope() override;
    
    DEF_VAL_PROP_RM(BytecodeLabel, retry_label);
    DEF_VAL_PROP_RM(BytecodeLabel, exit_label);
    
    Value Find(const ASTString *name) override {
        if (auto iter = symbols_.find(name->ToString()); iter == symbols_.end()) {
            return {Value::kError};
        } else {
            return iter->second;
        }
    }

    void Register(const std::string &name, Value value) override {
        DCHECK(symbols_.find(name) == symbols_.end());
        symbols_[name] = value;
    }
    
    DISALLOW_IMPLICIT_CONSTRUCTORS(BlockScope);
private:
    std::map<std::string, Value> symbols_;
    BytecodeLabel retry_label_;
    BytecodeLabel exit_label_;
    StackSpaceAllocator::Level stack_level_{0, 0};
}; // class BytecodeGenerator::BlockScope

class BytecodeGenerator::FunctionScope : public BlockScope {
public:
    struct CapturedVarDesc {
        std::string name;
        const Class *type;
        Function::CapturedVarKind kind;
        int32_t index;
    };

    FunctionScope(base::Arena *arena, FileUnit *file_unit, Scope **current,
                  FunctionScope **current_fun)
        : BlockScope(kFunctionScope, current)
        , file_unit_(file_unit)
        , builder_(arena)
        , current_fun_(current_fun) {
        prev_fun_ = *current_fun_;
        DCHECK_NE(this, *current_fun_);
        *current_fun_ = this;
    }
    ~FunctionScope() override {
        DCHECK_EQ(this, *current_fun_);
        *current_fun_ = prev_fun_;
    }
    
    int AddCapturedVarDesc(const std::string &name, const Class *type,
                           Function::CapturedVarKind kind, int index) {
        int cidx = static_cast<int>(captured_vars_.size());
        CapturedVarDesc desc{name, type, kind, index};
        captured_vars_.push_back(desc);
        return cidx;
    }
    
    void AddExceptionHandler(const Class *expect_type, intptr_t start_pc, intptr_t stop_pc,
                             intptr_t handler_pc) {
        exception_handlers_.push_back({expect_type, start_pc, stop_pc, handler_pc});
    }

    DEF_PTR_PROP_RW(FileUnit, file_unit);
    DEF_VAL_PROP_RM(std::vector<int>, source_lines);
    DEF_VAL_PROP_RM(std::vector<CapturedVarDesc>, captured_vars);
    DEF_VAL_PROP_RM(std::vector<Function::ExceptionHandlerDesc>, exception_handlers);
    DEF_VAL_PROP_RM(std::vector<Function::InvokingHint>, invoking_hint);
    DEF_VAL_PROP_RM(std::vector<uint32_t>, proto_params);
    DEF_VAL_PROP_RW(uint32_t, proto_ret_type);
    DEF_VAL_PROP_RW(bool, proto_vargs);

    BytecodeArrayBuilder *builder() { return &builder_; }
    ConstantPoolBuilder *constants() { return &constants_; }
    StackSpaceAllocator *stack() { return &stack_; }
    
    int StackReserve(const Class *clazz) {
        if (clazz->is_reference()) {
            return stack_.ReserveRef();
        } else {
            return stack_.Reserve(clazz->reference_size());
        }
    }
    
    void StackFallback(const Class *clazz, int index) {
        if (clazz->is_reference()) {
            stack_.FallbackRef(index);
        } else {
            stack_.Fallback(index, clazz->reference_size());
        }
    }

    void EmitYield(ASTNode *ast, int code) {
        invoking_hint_.push_back({builder_.pc(), 0, stack_.GetTopRef()});
        Incoming(ast)->Add<kYield>(code);
    }

    void EmitContact(ASTNode *ast, int param_size) {
        invoking_hint_.push_back({builder_.pc(), 0, stack_.GetTopRef()});
        Incoming(ast)->Add<kContact>(0, param_size);
    }

    void EmitNewObject(ASTNode *ast, int type) {
        invoking_hint_.push_back({builder_.pc(), 0, stack_.GetTopRef()});
        Incoming(ast)->Add<kNewObject>(0, type);
    }
    
    void EmitArray(ASTNode *ast, int len, int type) {
        invoking_hint_.push_back({builder_.pc(), 0, stack_.GetTopRef()});
        Incoming(ast)->Add<kArray>(len, type);
    }
    
    void EmitArrayWith(ASTNode *ast, int param_size, int type) {
        invoking_hint_.push_back({builder_.pc(), 0, stack_.GetTopRef()});
        Incoming(ast)->Add<kArrayWith>(param_size, type);
    }

    void EmitDirectlyCallFunction(ASTNode *ast, bool native, int slot, int params_size) {
        if (native) {
            invoking_hint_.push_back({builder_.pc(), 0, stack_.GetTopRef()});
            Incoming(ast)->Add<kCallNativeFunction>(slot, params_size);
        } else {
            invoking_hint_.push_back({builder_.pc(), 1, stack_.GetTopRef()});
            Incoming(ast)->Add<kCallBytecodeFunction>(slot, params_size);
        }
    }

    void EmitDirectlyCallFunctionWithLine(int line, bool native, int slot, int params_size) {
        if (native) {
            invoking_hint_.push_back({builder_.pc(), 0, stack_.GetTopRef()});
            IncomingWithLine(line)->Add<kCallNativeFunction>(slot, params_size);
        } else {
            invoking_hint_.push_back({builder_.pc(), 1, stack_.GetTopRef()});
            IncomingWithLine(line)->Add<kCallBytecodeFunction>(slot, params_size);
        }
    }

    void EmitVirtualCallFunction(ASTNode *ast, int slot, int params_size) {
        invoking_hint_.push_back({builder_.pc(), 2, stack_.GetTopRef()});
        Incoming(ast)->Add<kCallFunction>(slot, params_size);
    }

    BytecodeArrayBuilder *Incoming(ASTNode *ast) {
        SourceLocation loc = file_unit_->FindSourceLocation(ast);
        source_lines_.push_back(loc.begin_line);
        return &builder_;
    }
    
    BytecodeArrayBuilder *IncomingWithLine(int line) {
        source_lines_.push_back(line);
        return &builder_;
    }
    
    void AddInvokingHint(uint32_t flags) {
        invoking_hint_.push_back({builder_.pc(), flags, stack_.GetTopRef()});
    }
    
    DISALLOW_IMPLICIT_CONSTRUCTORS(FunctionScope);
private:
    FileUnit *file_unit_;
    BytecodeArrayBuilder builder_;
    ConstantPoolBuilder constants_;
    StackSpaceAllocator stack_;
    FunctionScope **current_fun_;
    FunctionScope *prev_fun_;
    std::vector<int> source_lines_;
    std::vector<CapturedVarDesc> captured_vars_;
    std::vector<Function::ExceptionHandlerDesc> exception_handlers_;
    std::vector<Function::InvokingHint> invoking_hint_;
    std::vector<uint32_t> proto_params_;
    uint32_t proto_ret_type_ = kType_void;
    bool proto_vargs_ = false;
}; // class BytecodeGenerator::FunctionScope


inline BytecodeGenerator::BlockScope::BlockScope(Scope::Kind kind, Scope **current)
    : Scope(kind, current) {
    Enter();
    if (kind != kFunctionScope) {
        stack_level_ = GetFunctionScope()->stack()->level();
    }
}

BytecodeGenerator::BlockScope::~BlockScope() /*override*/ {
    if (kind() != kFunctionScope) {
        GetFunctionScope()->stack()->set_level(stack_level_);
    }
    Exit();
}

using BValue = BytecodeGenerator::Value;

namespace {

#define TOP_REF (current_fun_->stack()->GetTopRef())
#define EMIT(ast, action) current_fun_->Incoming(ast)->action

#define VISIT_CHECK(node) if (rv = (node)->Accept(this); rv.kind == Value::kError) { \
        return ResultWithError(); \
    }(void)0

#define VISIT_CHECK_JUST(node) if (auto rv = (node)->Accept(this); rv.kind == Value::kError) { \
        return ResultWithError(); \
    }(void)0

inline ASTVisitor::Result ResultWith(BValue::Linkage linkage, int type, int index) {
    ASTVisitor::Result rv;
    rv.kind = linkage;
    rv.bundle.index = index;
    rv.bundle.type  = type;
    return rv;
}

inline ASTVisitor::Result ResultWith(const BValue &value) {
    ASTVisitor::Result rv;
    rv.kind = value.linkage;
    rv.bundle.index = value.index;
    rv.bundle.type  = value.type->id();
    rv.bundle.flags = value.flags;
    return rv;
}

inline ASTVisitor::Result ResultWithError() {
    ASTVisitor::Result rv;
    rv.kind = BValue::kError;
    rv.bundle.index = 0;
    rv.bundle.type  = 0;
    return rv;
}

inline ASTVisitor::Result ResultWithVoid() { return ResultWith(BValue::kStack, kType_void, 0); }

inline int GetParameterOffset(int param_size, int index) {
    return -(param_size - index) - kPointerSize * 2;
}

// off = -idx - k = -(idx + k)
// -off = idx + k
// -off -k
inline int GetStackOffset(int off) {
    if (off < 0) {
        off = -off - kPointerSize * 2;
        DCHECK_LT(off, kParameterSpaceOffset - kPointerSize * 2);
        DCHECK_EQ(0, off % kStackOffsetGranularity);
        return (kParameterSpaceOffset - off - kPointerSize * 2) / kStackOffsetGranularity;
    } else {
        DCHECK_LT(off, 0x1000 - kParameterSpaceOffset);
        DCHECK_EQ(0, off % kStackOffsetGranularity);
        return (kParameterSpaceOffset + off) / kStackOffsetGranularity;
    }
}

inline int GetConstOffset(int off) {
    DCHECK_GE(off, 0);
    DCHECK_EQ(0, off % kConstPoolOffsetGranularity);
    return off / kConstPoolOffsetGranularity;
}

inline int GetGlobalOffset(int off) {
    DCHECK_GE(off, 0);
    DCHECK_EQ(0, off % kGlobalSpaceOffsetGranularity);
    return off / kGlobalSpaceOffsetGranularity;
}

inline bool NeedInbox(const Class *lval, const Class *rval) {
    switch (static_cast<BuiltinType>(rval->id())) {
        case kType_bool:
        case kType_i8:
        case kType_u8:
        case kType_i16:
        case kType_u16:
        case kType_i32:
        case kType_u32:
        case kType_int:
        case kType_uint:
        case kType_i64:
        case kType_u64:
        case kType_f32:
        case kType_f64:
            return lval->id() == kType_any;
        default:
            return false;
    }
}

} // namespace

BytecodeGenerator::BytecodeGenerator(Isolate *isolate,
                                     SyntaxFeedback *feedback,
                                     ClassDefinition *class_exception,
                                     ClassDefinition *class_object,
                                     std::map<std::string, std::vector<FileUnit *>> &&path_units,
                                     std::map<std::string, std::vector<FileUnit *>> &&pkg_units,
                                     base::Arena *arena)
    : isolate_(isolate)
    , metadata_space_(isolate->metadata_space())
    , error_feedback_(DCHECK_NOTNULL(feedback))
    , class_exception_(class_exception)
    , class_object_(class_object)
    , path_units_(std::move(path_units))
    , pkg_units_(std::move(pkg_units))
    , arena_(arena) {
    GetStackOffset(0);
    GetParameterOffset(0, 0);
    GetConstOffset(0);
    GetGlobalOffset(0);
}

BytecodeGenerator::BytecodeGenerator(Isolate *isolate, SyntaxFeedback *feedback, base::Arena *arena)
    : isolate_(isolate)
    , error_feedback_(DCHECK_NOTNULL(feedback))
    , class_exception_(nullptr)
    , class_object_(nullptr)
    , arena_(arena) {
}

BytecodeGenerator::~BytecodeGenerator() {}

bool BytecodeGenerator::Prepare() {
    //symbol_trace_.insert(class_any_); // ignore class any

    for (auto pair : pkg_units_) {
        const std::string &pkg_name = pair.first;

        for (auto unit : pair.second) {
            error_feedback_->set_file_name(unit->file_name()->ToString());
            error_feedback_->set_package_name(unit->package_name()->ToString());
            unit->set_scope(new FileScope(path_units_, &symbols_, unit, &current_, &current_file_));
            if (!PrepareUnit(pkg_name, unit)) {
                return false;
            }
        }
    }
    
    for (auto pair : pkg_units_) {
        const std::string &pkg_name = pair.first;
        
        for (auto unit : pair.second) {
            FileScope::Holder file_holder(unit->scope());

            error_feedback_->set_file_name(unit->file_name()->ToString());
            error_feedback_->set_package_name(unit->package_name()->ToString());
            for (auto ast : unit->global_variables()) {
                std::string name = pkg_name + "." + ast->identifier()->ToString();

                if (auto decl_ast = ast->AsVariableDeclaration()) {
                    int index = LinkGlobalVariable(decl_ast);
                    Result rv;
                    if (rv = decl_ast->type()->Accept(this); rv.kind == Value::kError) {
                        return false;
                    }
                    DCHECK_EQ(Value::kMetadata, rv.kind);
                    const Class *clazz = metadata_space_->type(rv.bundle.index);
                    current_->Register(name, {Value::kGlobal, clazz, index, ast});
                }
            }
        }
    }
    return true;
}

bool BytecodeGenerator::Generate() {
    FunctionScope function_scope(arena_, nullptr, &current_, &current_fun_);
    current_init0_fun_ = &function_scope;
    function_scope.IncomingWithLine(0)->Add<kCheckStack>();
    
    for (auto pair : pkg_units_) {
        const std::string &pkg_name = pair.first;
        
        for (auto unit : pair.second) {
            error_feedback_->set_file_name(unit->file_name()->ToString());
            error_feedback_->set_package_name(unit->package_name()->ToString());
            function_scope.set_file_unit(unit);

            if (!GenerateUnit(pkg_name, unit)) {
                current_init0_fun_ = nullptr;
                return false;
            }
        }
    }

    Value fun_main_main = FindValue("main.main");
    if (fun_main_main.linkage == Value::kError) {
        error_feedback_->DidFeedback("Unservole 'main.main' entery function");
        current_init0_fun_ = nullptr;
        return false;
    }
    DCHECK_EQ(Value::kGlobal, fun_main_main.linkage);
    Closure *main = *global_space_.offset<Closure *>(fun_main_main.index);
    function_scope.IncomingWithLine(1)->Add<kLdaGlobalPtr>(GetGlobalOffset(fun_main_main.index));
    function_scope.EmitDirectlyCallFunctionWithLine(1, main->is_cxx_function(), 0/*slot*/,
                                                    0/*args_size*/);
    function_scope.IncomingWithLine(1)->Add<kReturn>();
    generated_init0_fun_ = BuildFunction("init0", &function_scope);
    isolate_->SetGlobalSpace(global_space_.TakeSpans(), global_space_.TakeBitmap(),
                             global_space_.capacity(), global_space_.length());
    isolate_->metadata_space()->InvalidateAllLookupTables();
    isolate_->metadata_space()->InvalidateLanguageClasses();
    
    for (auto &pair : path_units_) {
        for (auto unit : pair.second) {
            delete unit->scope();
            unit->set_scope(nullptr);
        }
    }
    current_init0_fun_ = nullptr;
    return true;
}

bool BytecodeGenerator::PrepareUnit(const std::string &pkg_name, FileUnit *unit) {
    FileScope::Holder file_holder(unit->scope());

    for (auto ast : unit->definitions()) {
        std::string name = pkg_name + "." + ast->identifier()->ToString();
        
        if (auto clazz_ast = ast->AsClassDefinition()) {
            Class *clazz = metadata_space_->PrepareClass(name, Type::kReferenceTag, kPointerSize);
            clazz_ast->set_clazz(clazz);
            current_->Register(name, {Value::kMetadata, clazz, static_cast<int>(clazz->id()), ast});
        } else if (auto object_ast = ast->AsObjectDefinition()) {
            Class *clazz = metadata_space_->PrepareClass(name, Type::kReferenceTag, kPointerSize);
            object_ast->set_clazz(clazz);
            // object foo ==> main.foo$class
            current_->Register(name + "$class", {Value::kMetadata, clazz,
                                                 static_cast<int>(clazz->id()), ast});
            int index = global_space_.ReserveRef();
            current_->Register(name, {Value::kGlobal, clazz, index, ast});
        } else if (auto fun_ast = ast->AsFunctionDefinition()) {
            const Class *clazz = metadata_space_->builtin_type(kType_closure);
            if (fun_ast->is_native()) {
                Closure *closure = isolate_->FindExternalLinkageOrNull(name);
                if (!closure) {
                    error_feedback_->Printf(unit->FindSourceLocation(ast),
                                            "Unresolve external linkage function: %s",
                                            name.c_str());
                    return false;
                }
                int index = global_space_.AppendAny(closure);
                current_->Register(name, {Value::kGlobal, clazz, index, ast, fun_ast->attributes()});
                symbol_trace_.insert(ast);
            } else {
                int index = global_space_.ReserveRef();
                current_->Register(name, {Value::kGlobal, clazz, index, ast, fun_ast->attributes()});
            }
        }
    }
    
    for (auto impl : unit->implements()) {
        for (auto ast : impl->methods()) {
            std::string name = pkg_name + "." + impl->owner()->identifier()->ToString() + "::" +
                ast->identifier()->ToString();
            const Class *clazz = metadata_space_->builtin_type(kType_closure);
            if (ast->is_native()) {
                Closure *closure = isolate_->FindExternalLinkageOrNull(name);
                if (!closure) {
                    error_feedback_->Printf(unit->FindSourceLocation(ast),
                                            "Unresolve external linkage function: %s",
                                            name.c_str());
                    return false;
                }
                int index = global_space_.AppendAny(closure);
                current_->Register(name, {Value::kGlobal, clazz, index, ast, ast->attributes()});
                symbol_trace_.insert(ast);
            } else {
                int index = global_space_.ReserveRef();
                current_->Register(name, {Value::kGlobal, clazz, index, ast, ast->attributes()});
            }
        }
    }
    return true;
}

bool BytecodeGenerator::GenerateUnit(const std::string &pkg_name, FileUnit *unit) {
    FileScope::Holder file_holder(unit->scope());
    
    for (auto ast : unit->global_variables()) {
        if (HasGenerated(ast)) {
            continue;
        }

        if (auto decl_ast = ast->AsVariableDeclaration()) {
            if (auto rv = decl_ast->Accept(this); rv.kind == Value::kError) {
                return false;
            }
        }
    }
    
    for (auto ast : unit->definitions()) {
        if (HasGenerated(ast)) {
            continue;
        }
        if (auto rv = ast->Accept(this); rv.kind == Value::kError) {
            return false;
        }
    }
    return true;
}

bool BytecodeGenerator::GenerateSymbolDependence(Value value) {
    if (Value::kGlobal != value.linkage && Value::kMetadata != value.linkage) {
        return true;
    }
    DCHECK(value.ast != nullptr);
    if (!value.ast->file_unit()) {
        return true; // ignore
    }
    return GenerateSymbolDependence(value.ast);
}

bool BytecodeGenerator::GenerateSymbolDependence(Symbolize *ast) {
    if (!ast->file_unit()) {
        return true; // ignore
    }
    if (ast->file_unit() == current_file_->file_unit()) {
        return ast->Accept(this).kind != Value::kError;
    }
    
    bool ok = true;
    FileScope *saved_file = current_file_;
    Scope *saved_current = current_;
    FunctionScope *saved_fun = current_fun_;
    current_fun_ = current_init0_fun_;
    current_file_ = nullptr;
    current_ = nullptr;
    {
        FileScope::Holder file_holder(ast->file_unit()->scope());
        DCHECK_NOTNULL(current_fun_)->set_file_unit(ast->file_unit());
        if (auto rv = ast->Accept(this); rv.kind == Value::kError) {
            ok = false;
        }
    }
    current_ = saved_current;
    current_file_ = saved_file;
    current_fun_ = saved_fun;
    DCHECK_NOTNULL(current_fun_)->set_file_unit(saved_file->file_unit());
    return ok;
}

Function *BytecodeGenerator::BuildFunction(const std::string &name, FunctionScope *scope) {
    const char *file_name = scope->file_unit() ? scope->file_unit()->file_name()->data() : "init0";
    SourceLineInfo *source_info = metadata_space_->NewSourceLineInfo(file_name,
                                                                     scope->source_lines());
    ConstantPoolBuilder *const_pool = scope->constants();
    StackSpaceAllocator *stack = scope->stack();
    uint32_t stack_size = RoundUp(stack->GetMaxStackSize(), kStackAligmentSize);
    std::vector<BytecodeInstruction> instrs =
        scope->builder()->BuildWithRewrite([stack_size](int index) {
            if (index >= 0) {
                return index;
            }
            return GetStackOffset(stack_size - index);
        });
    BytecodeArray *bytecodes = metadata_space_->NewBytecodeArray(instrs);

    std::vector<uint32_t> stack_bitmap(stack->bitmap());
    if (stack_bitmap.empty()) {
        stack_bitmap.resize(1); // Placeholder
    }

    FunctionBuilder builder(name);
    for (auto var : scope->captured_vars()) {
        builder.AddCapturedVar(var.name, var.type, var.kind, var.index);
    }
    return builder
        // TODO:
        .stack_size(stack_size)
        .stack_bitmap(stack_bitmap)
        .prototype(scope->proto_params(), scope->proto_vargs(), scope->proto_ret_type())
        .source_line_info(source_info)
        .movable_exception_table(std::move(*scope->mutable_exception_handlers()))
        .movable_invoking_hint(std::move(*scope->mutable_invoking_hint()))
        .bytecode(bytecodes)
        .const_pool(const_pool->spans(), const_pool->length())
        .const_pool_bitmap(const_pool->bitmap(), (const_pool->length() + 31) / 32)
        .Build(metadata_space_);
}

ASTVisitor::Result BytecodeGenerator::VisitTypeSign(TypeSign *ast) /*override*/ {
    switch (static_cast<Token::Kind>(ast->id())) {
        case Token::kRef:
        case Token::kObject:
        case Token::kClass: {
            if (current_fun_ && !HasGenerated(ast->structure()) &&
                !GenerateSymbolDependence(ast->structure())) {
                return ResultWithError();
            }
            int type_id = DCHECK_NOTNULL(ast->structure()->clazz())->id();
            return ResultWith(Value::kMetadata, type_id, type_id);
        }
        default:
            break;
    }
    return ResultWith(Value::kMetadata, ast->ToBuiltinType(), ast->ToBuiltinType());
}

ASTVisitor::Result BytecodeGenerator::VisitClassDefinition(ClassDefinition *ast) /*override*/ {
    std::string name(DCHECK_NOTNULL(current_file_)->file_unit()->package_name()->ToString());
    name.append(".").append(ast->identifier()->ToString());
    
    if (ast->base() && !HasGenerated(ast->base())) {
        if (!GenerateSymbolDependence(ast->base())) {
            return ResultWithError();
        }
    }

    ClassScope class_scope(ast, &current_, &current_class_);
    ClassBuilder builder(name);

    uint32_t offset = 0;
    const Class *base = nullptr;
    if (ast == class_object_) {
        base = metadata_space_->builtin_type(kType_any);
        offset = metadata_space_->builtin_type(kType_any)->instrance_size();
    } else if (ast == class_exception_) {
        base = metadata_space_->builtin_type(kType_Throwable);
        offset = metadata_space_->builtin_type(kType_Throwable)->instrance_size();
    } else {
        base = DCHECK_NOTNULL(ast->base())->clazz();
        offset = ast->base()->clazz()->instrance_size();
    }
    symbol_trace_.insert(ast);

    std::vector<FieldDesc> fields_desc;
    uint32_t size = ProcessStructure(ast, offset, &builder, &fields_desc);
    if (!size) {
        return ResultWithError();
    }

    builder
        .tags(Type::kReferenceTag)
        .base(base)
        .reference_size(kPointerSize)
        .instrance_size(size)
        .BuildWithPreparedClass(metadata_space_, ast->clazz());
    Function *ctor = GenerateClassConstructor(fields_desc, base, ast);
    if (!ctor) {
        return ResultWithError();
    }
    if (!ProcessMethodsPost(ctor, ast)) {
        return ResultWithError();
    }
    return ResultWithVoid();
}

ASTVisitor::Result BytecodeGenerator::VisitObjectDefinition(ObjectDefinition *ast) /*override*/ {
    std::string name(DCHECK_NOTNULL(current_file_)->file_unit()->package_name()->ToString());
    name.append(".").append(ast->identifier()->ToString());
    std::string class_name = name + "$class";
    
    ClassScope class_scope(ast, &current_, &current_class_);
    ClassBuilder builder(class_name);
    const Class *base = metadata_space_->builtin_type(kType_any);
    std::vector<FieldDesc> fields_desc;
    uint32_t size = ProcessStructure(ast, base->instrance_size(), &builder, &fields_desc);
    if (!size) {
        return ResultWithError();
    }
    Class *clazz = builder
        .tags(Type::kReferenceTag)
        .base(base)
        .reference_size(kPointerSize)
        .instrance_size(size)
        .BuildWithPreparedClass(metadata_space_, ast->clazz());
    Function *ctor = GenerateObjectConstructor(fields_desc, ast);
    if (!ctor) {
        return ResultWithError();
    }
    if (!ProcessMethodsPost(ctor, ast)) {
        return ResultWithError();
    }
    
    int kidx = current_fun_->constants()->FindOrInsertMetadata(clazz);
    current_fun_->EmitNewObject(ast, GetConstOffset(kidx));
    EMIT(ast, Incomplete<kStarPtr>(-kPointerSize)); // argv[0] = self

    kidx = current_fun_->constants()->FindOrInsertClosure(clazz->init()->fn());
    LdaConst(metadata_space_->builtin_type(kType_closure), kidx, ast);
    current_fun_->EmitDirectlyCallFunction(ast, false/*native*/, 0/*slot*/,
                                           kPointerSize/*argument_size*/);
    Value value = EnsureFindValue(name);
    StaGlobal(clazz, value.index, ast);

    symbol_trace_.insert(ast);
    return ResultWithVoid();
}

bool BytecodeGenerator::ProcessMethodsPost(Function *ctor, StructureDefinition *ast) {
    if (ctor) {
        ast->clazz()->set_init_function(Machine::This()->NewClosure(ctor, 0, Heap::kMetadata));
    }

    for (uint32_t i = 0; i < ast->methods_size(); i++) {
        FunctionDefinition *method = ast->method(i);
        auto full_name = current_file_->LocalFileFullName(ast->identifier()) + "::" +
            method->identifier()->ToString();
        Value value = EnsureFindValue(full_name);
        DCHECK_EQ(Value::kGlobal, value.linkage);
        if (method->is_native()) {
            ast->clazz()->set_method_function(i, *global_space_.offset<Closure *>(value.index));
            continue;
        }
        auto rv = method->Accept(this);
        if (rv.kind == Value::kError) {
            return false;
        }
        Closure *closure = Machine::This()->NewClosure(rv.fun, 0/*captured_var_size*/, Heap::kOld);
        ast->clazz()->set_method_function(i, closure);
        *global_space_.offset<Closure *>(value.index) = closure;
    }
    return true;
}

uint32_t BytecodeGenerator::ProcessStructure(StructureDefinition *ast, uint32_t offset,
                                             ClassBuilder *builder,
                                             std::vector<FieldDesc> *fields_desc) {
    int tag = 1;
    for (auto field : ast->fields()) {
        auto &field_builder = builder->field(field.declaration->identifier()->ToString());
        Result rv = field.declaration->type()->Accept(this);
        if (rv.kind == Value::kError) {
            return 0;
        }
        uint32_t flags = 0;
        if (field.access == StructureDefinition::kPublic) {
            flags |= Field::kPublic;
        } else {
            flags |= Field::kPrivate;
        }
        if (field.declaration->kind() == VariableDeclaration::VAR) {
            flags |= Field::kRdWr;
        } else {
            flags |= Field::kRead;
        }
        
        DCHECK_EQ(rv.kind, Value::kMetadata);
        uint32_t field_offset = 0;
        const Class *field_class = metadata_space_->type(rv.bundle.index);
        if ((offset + field_class->reference_size()) % 2 == 0) {
            field_offset = offset;
        } else {
            field_offset = RoundUp(offset, 2);
        }
        field_builder
            .type(field_class)
            .offset(field_offset)
            .tag(tag++)
            .flags(flags)
        .End();
        fields_desc->push_back({field.declaration->identifier(), field_class, field_offset});
        if ((offset + field_class->reference_size()) % 2 == 0) {
            offset += field_class->reference_size();
        } else {
            offset = field_offset + field_class->reference_size();
        }
    }
    
    for (auto method : ast->methods()) {
        // Placeholder:
        uint32_t tags = 0;
        if (method->is_native()) {
            tags = Method::kNative;
        } else {
            tags = Method::kBytecode;
        }
        builder->method(method->identifier()->ToString())
            .fn(nullptr)
            .tags(tags)
        .End();
    }
    
    return offset;
}

ASTVisitor::Result BytecodeGenerator::VisitFunctionDefinition(FunctionDefinition *ast)
/*override*/ {
    if (ast->file_unit()) {
        symbol_trace_.insert(ast);
    }
    if (ast->is_native()) {
        return ResultWithVoid();
    }

    const bool is_method = current_->is_class_scope();
    std::string name;
    if (is_method) {
        name = current_file_->LocalFileFullName(current_class_->clazz()->identifier()) + "::" +
            ast->identifier()->ToString();
    } else {
        name = ast->file_unit() ?
            current_file_->LocalFileFullName(ast->identifier()) : ast->identifier()->ToString();
    }
    const Class *clazz = metadata_space_->builtin_type(kType_closure);
    Function *fun = nullptr;
    {
        FunctionScope function_scope(arena_, current_file_->file_unit(), &current_, &current_fun_);
        current_->Register(ast->identifier()->ToString(),
                           {Value::kStack, clazz, -BytecodeStackFrame::kOffsetCallee});
        fun = GenerateLambdaLiteral(name, is_method, ast->body());
    }
    if (!fun) {
        return ResultWithError();
    }
    if (is_method) {
        Result rv;
        rv.kind = Value::kMetadata;
        rv.fun  = fun;
        return rv;
    }
    if (ast->file_unit()) { // Global var
        Closure *closure = Machine::This()->NewClosure(fun, 0/*captured_var_size*/, 0/*flags*/);
        DCHECK_EQ(ast->file_unit(), current_file_->file_unit());
        Value value = EnsureFindValue(name);
        DCHECK_EQ(Value::kGlobal, value.linkage);
        *global_space_.offset<Closure *>(value.index) = closure;
        symbol_trace_.insert(ast);
        return ResultWithVoid();
    }

    // Local var
    int local = current_fun_->stack()->ReserveRef();
    current_->Register(ast->identifier()->ToString(), {Value::kStack, clazz, local, ast});

    if (fun->captured_var_size() > 0) {
        int kidx = current_fun_->constants()->FindOrInsertMetadata(fun);
        EMIT(ast, Add<kClose>(GetConstOffset(kidx)));
        StaStack(clazz, local, ast);
    } else {
        Closure *closure = Machine::This()->NewClosure(fun, 0/*captured_var_size*/, 0/*flags*/);
        int kidx = current_fun_->constants()->FindOrInsertClosure(closure);
        LdaConst(clazz, kidx, ast);
        StaStack(clazz, local, ast);
    }
    return ResultWithVoid();
}

ASTVisitor::Result BytecodeGenerator::VisitLambdaLiteral(LambdaLiteral *ast) /*override*/ {
    Function *fun = nullptr;
    {
        FunctionScope function_scope(arena_, current_file_->file_unit(), &current_, &current_fun_);
        fun = GenerateLambdaLiteral("$lambda$"/*name*/, false/*is_method*/, ast);
    }
    if (!fun) {
        return ResultWithError();
    }
    if (fun->captured_var_size() > 0) {
        int kidx = current_fun_->constants()->FindOrInsertMetadata(fun);
        //current_fun_->Incoming(ast)->Add<kClose>(GetConstOffset(kidx));
        EMIT(ast, Add<kClose>(GetConstOffset(kidx)));
        return ResultWith(Value::kACC, kType_closure, 0);
    } else {
        Closure *closure = Machine::This()->NewClosure(fun, 0/*captured_var_size*/, 0/*flags*/);
        int kidx = current_fun_->constants()->FindOrInsertClosure(closure);
        return ResultWith(Value::kConstant, kType_closure, kidx);
    }
}

ASTVisitor::Result BytecodeGenerator::VisitCallExpression(CallExpression *ast) /*override*/ {
    CallingReceiver receiver;
    auto rv = GenerateCalling(ast, &receiver);
    if (rv.kind == Value::kError) {
        return ResultWithError();
    }
    
    switch (receiver.kind) {
        case CallingReceiver::kCtor:
            current_fun_->EmitDirectlyCallFunction(receiver.ast, false/*native*/, 0/*slot*/,
                                                   receiver.arguments_size);
            break;
        case CallingReceiver::kVtab:
            current_fun_->EmitVirtualCallFunction(receiver.ast, 0/*slot*/, receiver.arguments_size);
            break;
        case CallingReceiver::kNative:
            current_fun_->EmitDirectlyCallFunction(receiver.ast, true/*native*/, 0/*slot*/,
                                                   receiver.arguments_size);
            break;
        case CallingReceiver::kBytecode:
            current_fun_->EmitDirectlyCallFunction(receiver.ast, false/*native*/, 0/*slot*/,
                                                   receiver.arguments_size);
            break;
        default:
            NOREACHED();
            break;
    }
    if (receiver.attributes & kAttrYield) {
        current_fun_->EmitYield(ast, YIELD_PROPOSE);
    }
    return rv;
}

ASTVisitor::Result BytecodeGenerator::GenerateMethodCalling(Value primary, CallExpression *ast,
                                                            CallingReceiver *receiver) {
    DotExpression *dot = DCHECK_NOTNULL(ast->callee()->AsDotExpression());
    const Class *clazz = primary.type;
    DCHECK(clazz->is_reference());
    StackSpaceAllocator::Scope stack_scope(current_fun_->stack());
    
    int self = current_fun_->stack()->ReserveRef();
    MoveToStackIfNeeded(clazz, primary.index, primary.linkage, self, dot);
    
    auto proto = ast->prototype();
    std::vector<const Class *> params;
    Result rv;
    for (auto param : proto->parameters()) {
        VISIT_CHECK(param.type);
        params.push_back(metadata_space_->type(rv.bundle.index));
    }
    int arg_size = GenerateArguments(ast->operands(), params, dot->primary(), self, proto->vargs());
    if (arg_size < 0) {
        return ResultWithError();
    }

    auto [owns, method] = metadata_space_->FindClassMethod(clazz, dot->rhs()->data());
    if (!method) {
        auto key = dot->rhs()->ToSlice();
        int kidx = current_fun_->constants()->FindString(key);
        if (kidx < 0) {
            String *name = Machine::This()->NewUtf8String(key.data(), key.size(), Heap::kMetadata);
            kidx = current_fun_->constants()->FindOrInsertString(name);
        }
        EMIT(ast, Add<kLdaVtableFunction>(self, GetConstOffset(kidx)));
        receiver->kind = CallingReceiver::kVtab;
        receiver->attributes = kAttrYield;
    } else {
        std::string name = std::string(owns->name()) + "::" + dot->rhs()->ToString();
        Value value = FindValue(name);
        if (value.linkage == Value::kError) {
            value = FindOrInsertExternalFunction(name);
        }
        DCHECK_EQ(Value::kGlobal, value.linkage);
        LdaGlobal(metadata_space_->builtin_type(kType_closure), value.index, ast);
        receiver->kind = method->is_native()? CallingReceiver::kNative : CallingReceiver::kBytecode;
        receiver->attributes = value.flags;
    }
    receiver->arguments_size = arg_size;
    receiver->ast = ast;

    VISIT_CHECK(proto->return_type());
    return ResultWith(Value::kACC, rv.bundle.type, 0);
}

Function *BytecodeGenerator::GenerateLambdaLiteral(const std::string &name, bool is_method,
                                                   LambdaLiteral *ast) {
    EMIT(ast, Add<kCheckStack>());

    int param_size = 0;
    for (auto param : ast->prototype()->parameters()) {
        param_size += RoundUp(param.type->GetReferenceSize(), kStackSizeGranularity);
    }
    if (ast->prototype()->vargs()) {
        param_size += kPointerSize;
    }
    if (is_method) {
        param_size += kPointerSize;
    }
    param_size = RoundUp(param_size, kStackAligmentSize);
    int param_offset = 0;
    if (is_method) { // Method!
        param_offset += kPointerSize;
        current_->Register("self", {
            Value::kStack,
            current_class_->clazz()->clazz(),
            GetParameterOffset(param_size, param_offset)
        });
        current_fun_->mutable_proto_params()->push_back(current_class_->clazz()->clazz()->id());
    }
    for (auto param : ast->prototype()->parameters()) {
        auto rv = param.type->Accept(this);
        if (rv.kind == Value::kError) {
            return nullptr;
        }
        const Class *clazz = DCHECK_NOTNULL(metadata_space_->type(rv.bundle.type));
        param_offset += RoundUp(clazz->reference_size(), kStackSizeGranularity);
        current_->Register(param.name->ToString(), {
            Value::kStack,
            clazz,
            GetParameterOffset(param_size, param_offset)
        });
        current_fun_->mutable_proto_params()->push_back(clazz->id());
    }
    if (ast->prototype()->vargs()) {
        const Class *clazz = metadata_space_->builtin_type(kType_array);
        param_offset += RoundUp(clazz->reference_size(), kStackSizeGranularity);
        current_->Register("argv", {
            Value::kStack,
            clazz,
            GetParameterOffset(param_size, param_offset)
        });
    }
    current_fun_->set_proto_vargs(ast->prototype()->vargs());
    if (auto rv = ast->prototype()->return_type()->Accept(this); rv.kind == Value::kError) {
        return nullptr;
    } else {
        current_fun_->set_proto_ret_type(rv.bundle.index);
    }

    for (auto stmt : ast->statements()) {
        if (auto rv = stmt->Accept(this); rv.kind == Value::kError) {
            return nullptr;
        }
    }

    EMIT(ast, Add<kReturn>());
    return BuildFunction(name, current_fun_);
}

Function *BytecodeGenerator::GenerateClassConstructor(const std::vector<FieldDesc> &fields_desc,
                                                      const Class *base, ClassDefinition *ast) {
    FunctionScope function_scope(arena_, current_file_->file_unit(), &current_, &current_fun_);
    EMIT(ast, Add<kCheckStack>());
    
    int param_size = kPointerSize; // self
    for (auto param : ast->parameters()) {
        size_t size = 0;
        if (param.field_declaration) {
            size = ast->field(param.as_field).declaration->type()->GetReferenceSize();
        } else {
            size = param.as_parameter->type()->GetReferenceSize();
        }
        param_size += RoundUp(size, kStackSizeGranularity);
    }
    param_size = RoundUp(param_size, kStackAligmentSize);

    int param_offset = kPointerSize; // self
    Value self{Value::kStack, ast->clazz(), GetParameterOffset(param_size, param_offset)};
    current_->Register("self", self);
    current_fun_->mutable_proto_params()->push_back(ast->clazz()->id());
    for (auto param : ast->parameters()) {
        VariableDeclaration *decl = param.field_declaration
                                  ? ast->field(param.as_field).declaration : param.as_parameter;
        auto rv = decl->type()->Accept(this);
        if (rv.kind == Value::kError) {
            return nullptr;
        }
        const Class *clazz = DCHECK_NOTNULL(metadata_space_->type(rv.bundle.type));
        param_offset += RoundUp(clazz->reference_size(), kStackSizeGranularity);
        Value value{Value::kStack, clazz, GetParameterOffset(param_size, param_offset)};
        current_->Register(decl->identifier()->ToString(), value);
        
        if (param.field_declaration) {
            DCHECK_LT(param.as_field, fields_desc.size());
            auto field = fields_desc[param.as_field];
            LdaStack(clazz, value.index, decl);
            StaProperty(field.type, self.index, field.offset, decl);
        }
        current_fun_->mutable_proto_params()->push_back(clazz->id());
    }
    current_fun_->set_proto_ret_type(ast->clazz()->id());
    
    if (!ProcessStructureInitializer(fields_desc, self.index, ast)) {
        return nullptr;
    }
    
    if (!ast->arguments().empty()) {
        ClassDefinition *base_ast = DCHECK_NOTNULL(ast->base());
        std::vector<const Class *> params;
        for (auto param : base_ast->parameters()) {
            auto rv = param.field_declaration
                    ? base_ast->field(param.as_field).declaration->type()->Accept(this)
                    : param.as_parameter->type()->Accept(this);
            if (rv.kind == Value::kError) {
                return nullptr;
            }
            params.push_back(metadata_space_->type(rv.bundle.index));
        }
        int arg_size = GenerateArguments(ast->arguments(), params, ast, self.index, false/*vargs*/);
        Closure *ctor = DCHECK_NOTNULL(DCHECK_NOTNULL(base->init())->fn());
        int kidx = current_fun_->constants()->FindOrInsertClosure(ctor);
        EMIT(ast, Add<kLdaConstPtr>(GetConstOffset(kidx)));
        current_fun_->EmitDirectlyCallFunction(ast, false/*native*/, 0/*slot*/, arg_size);
    }

    // $['yield', 'inline', 'uncheck']
    // native fun foo()
    FunctionDefinition *init = ast->FindMethodOrNull("init");
    if (IsMinorInitializer(ast, init)) {
        std::string name = current_file_->LocalFileFullName(ast->identifier()) + "::init";
        Value value = EnsureFindValue(name);
        DCHECK_EQ(Value::kGlobal, value.linkage);
        MoveToArgumentIfNeeded(ast->clazz(), self.index, self.linkage, kPointerSize, ast);
        LdaGlobal(metadata_space_->builtin_type(kType_closure), value.index, ast);
        current_fun_->EmitDirectlyCallFunction(ast, value.flags & kAttrNative, 0/*slot*/,
                                               kPointerSize);
    } else if (ast->arguments().empty()) {
        // return self
        LdaStack(ast->clazz(), self.index, ast);
    }
    EMIT(ast, Add<kReturn>());
    return BuildFunction("$init", &function_scope);
}

Function *BytecodeGenerator::GenerateObjectConstructor(const std::vector<FieldDesc> &fields_desc,
                                                       ObjectDefinition *ast) {
    FunctionScope function_scope(arena_, current_file_->file_unit(), &current_, &current_fun_);
    EMIT(ast, Add<kCheckStack>());

    Value self{Value::kStack, ast->clazz(), GetParameterOffset(kPointerSize, 0)};
    current_->Register("self", self);
    current_fun_->mutable_proto_params()->push_back(ast->clazz()->id());
    current_fun_->set_proto_ret_type(ast->clazz()->id());
    
    if (!ProcessStructureInitializer(fields_desc, self.index, ast)) {
        return nullptr;
    }

    // TODO: Minor Constructor
    LdaStack(ast->clazz(), self.index, ast);
    EMIT(ast, Add<kReturn>());
    return BuildFunction("$init", &function_scope);
}

bool BytecodeGenerator::ProcessStructureInitializer(const std::vector<FieldDesc> &fields_desc,
                                                    int self_index, StructureDefinition *ast) {

    for (size_t i = 0; i < ast->fields_size(); i++) {
        DCHECK_LT(i, fields_desc.size());

        auto field = ast->field(i);
        if (field.in_constructor) {
            continue;
        }
        
        auto rv = field.declaration->type()->Accept(this);
        if (rv.kind == Value::kError) {
            return false;
        }
        const Class *field_type = metadata_space_->type(rv.bundle.index);
        if (!field.declaration->initializer()) {
            EMIT(field.declaration, Add<kLdaZero>());
            StaProperty(field_type, self_index, fields_desc[i].offset, field.declaration);
            continue;
        }

        if (rv = field.declaration->initializer()->Accept(this); rv.kind == Value::kError) {
            return false;
        }
        
        const Class *clazz = metadata_space_->type(rv.bundle.type);
        if (NeedInbox(field_type, clazz)) {
            InboxIfNeeded(clazz, rv.bundle.index, static_cast<Value::Linkage>(rv.kind), field_type,
                          field.declaration);
        } else {
            LdaIfNeeded(rv, field.declaration);
        }
        StaProperty(field_type, self_index, fields_desc[i].offset, field.declaration);
    }
    return true;
}

ASTVisitor::Result BytecodeGenerator::GenerateRegularCalling(CallExpression *ast,
                                                             CallingReceiver *receiver) {
    Result rv;
    VISIT_CHECK(ast->callee());
    if (rv.kind == Value::kMetadata) {
        const Class *clazz = metadata_space_->type(rv.bundle.index);
        return GenerateNewObject(clazz, ast, receiver);
    }

    StackSpaceAllocator::Scope stack_scope(current_fun_->stack());
    DCHECK_EQ(kType_closure, rv.bundle.type);
    const Class *clazz = metadata_space_->builtin_type(kType_closure);

    bool saved_callee = false;
    int callee = 0;
    if (rv.kind == Value::kACC && (ast->prototype()->vargs() || ast->operands_size() > 0)) {
        saved_callee = true;
        callee = current_fun_->stack()->ReserveRef();
        StaStack(clazz, callee, ast->callee());
    }

    receiver->attributes = 0;
    bool native = false;
    if (rv.kind == Value::kGlobal) {
        native = (rv.bundle.flags & kAttrNative);
        receiver->attributes = rv.bundle.flags;
    }
    auto proto = ast->prototype();
    std::vector<const Class *> params;
    for (auto param : proto->parameters()) {
        auto param_rv = param.type->Accept(this);
        if (param_rv.kind == Value::kError) {
            return ResultWithError();
        }
        params.push_back(metadata_space_->type(param_rv.bundle.index));
    }
    int arg_size = GenerateArguments(ast->operands(), params, nullptr/*callee*/, 0/*self*/,
                                     proto->vargs());
    if (arg_size < 0) {
        return ResultWithError();
    }
    if (saved_callee) {
        LdaStack(clazz, callee, ast->callee());
    } else {
        LdaIfNeeded(rv, ast->callee());
    }
    receiver->ast = ast;
    receiver->kind = native ? CallingReceiver::kNative : CallingReceiver::kBytecode;
    receiver->arguments_size = arg_size;

    VISIT_CHECK(proto->return_type());
    return ResultWith(Value::kACC, rv.bundle.type, 0);
}

ASTVisitor::Result BytecodeGenerator::VisitVariableDeclaration(VariableDeclaration *ast)
/*override*/ {
    if (current_->is_file_scope()) { // It's global variable
        Value value = EnsureFindValue(current_file_->LocalFileFullName(ast->identifier()));
        if (HasGenerated(ast)) {
            return ResultWithVoid();
        }

        if (ast->initializer()) {
            ASTVisitor::Result rv = ast->initializer()->Accept(this);
            if (rv.kind == Value::kError) {
                return ResultWithError();
            }
            const Class *clazz = metadata_space_->type(rv.bundle.type);
            LdaIfNeeded(rv, ast->initializer());
            StaGlobal(clazz, value.index, ast);
        }
        symbol_trace_.insert(ast);
        return ResultWithVoid();
    }

    Result rv;
    VISIT_CHECK(DCHECK_NOTNULL(ast->type()));
    DCHECK_EQ(rv.kind, Value::kMetadata);
    const Class *clazz = metadata_space_->type(rv.bundle.index);
    
    Value local{Value::kStack, clazz};
    if (clazz->is_reference()) {
        local.index = current_fun_->stack()->ReserveRef();
    } else {
        local.index = current_fun_->stack()->Reserve(clazz->reference_size());
    }
    local.ast = ast;
    current_->Register(ast->identifier()->ToString(), local);

    StackSpaceScope scope(current_fun_->stack());
    VISIT_CHECK(ast->initializer());
    LdaIfNeeded(rv, ast->initializer());
    StaStack(clazz, local.index, ast);
    return ResultWithVoid();
}

ASTVisitor::Result BytecodeGenerator::VisitDotExpression(DotExpression *ast) /*override*/ {
    Result rv;
    if (!ast->primary()->IsIdentifier()) {
        VISIT_CHECK(ast->primary());
        const Class *clazz = metadata_space_->type(rv.bundle.type);
        return GenerateLoadProperty(clazz, rv.bundle.index, static_cast<Value::Linkage>(rv.kind),
                                    ast);
    }
    
    Identifier *id = DCHECK_NOTNULL(ast->primary()->AsIdentifier());
    Value value = current_file_->Find(id->name(), ast->rhs());
    if (value.linkage != Value::kError) { // Found
        if (!HasGenerated(value.ast) && !GenerateSymbolDependence(value)) {
            return ResultWithError();
        }
        return ResultWith(value);
    }
    
    auto found = current_->Resolve(id->name());
    value = std::get<1>(found);
    if (!HasGenerated(value.ast) && !GenerateSymbolDependence(value)) {
        return ResultWithError();
    }
    if (ShouldCaptureVar(std::get<0>(found), value)) {
        auto rv = CaptureVar(id->name()->ToString(), std::get<0>(found), value);
        DCHECK_NE(Value::kError, rv.kind);
        value.linkage = static_cast<Value::Linkage>(rv.kind);
        DCHECK_EQ(rv.bundle.type, value.type->id());
        value.index = rv.bundle.index;
    }
    return GenerateLoadProperty(value.type, value.index, value.linkage, ast);
}

ASTVisitor::Result BytecodeGenerator::VisitStringTemplateExpression(StringTemplateExpression *ast)
/*override*/ {
    StackSpaceAllocator::Scope stack_scope(current_fun_->stack());
    
    const Class *type = metadata_space_->builtin_type(kType_string);
    std::vector<Value> parts;
    for (auto part : ast->operands()) {
        Result rv;
        if (auto literal = part->AsStringLiteral()) {
            if (literal->value()->empty()) {
                continue;
            }
            VISIT_CHECK(literal);
        } else {
            VISIT_CHECK(part);
        }

        const Class *clazz = metadata_space_->type(rv.bundle.type);
        Value value{Value::kStack, clazz, current_fun_->stack()->ReserveRef()};
        if (clazz->id() == kType_string) {
            MoveToStackIfNeeded(clazz, rv.bundle.index, static_cast<Value::Linkage>(rv.kind),
                                value.index, part);
        } else {
            ToStringIfNeeded(clazz, rv.bundle.index, static_cast<Value::Linkage>(rv.kind), part);
            StaStack(type, value.index, part);
        }
        parts.push_back(value);
    }

    int argument_offset = 0;
    for (auto part : parts) {
        argument_offset += kPointerSize;
        MoveToArgumentIfNeeded(type, part.index, part.linkage, argument_offset, ast);
    }
    current_fun_->EmitContact(ast, argument_offset);
    return ResultWith(Value::kACC, kType_string, 0);
}

ASTVisitor::Result BytecodeGenerator::VisitUnaryExpression(UnaryExpression *ast) /*override*/ {
    switch (ast->op().kind) {
        case Operator::kNot: {
            OperandContext receiver;
            if (!GenerateUnaryOperands(&receiver, ast->operand())) {
                return ResultWithError();
            }
            EMIT(ast, Add<kNot>(GetStackOffset(receiver.lhs)));
            CleanupOperands(&receiver);
        } return ResultWith(Value::kACC, kType_bool, 0);
            
        case Operator::kBitwiseNot:
            TODO();
            break;
            
        case Operator::kMinus:
            TODO();
            break;
            
        case Operator::kIncrement:
        case Operator::kIncrementPost:
        case Operator::kDecrement:
        case Operator::kDecrementPost:
            TODO();
            break;
            
        case Operator::kRecv: {
            Result rv;
            VISIT_CHECK(ast->operand());
            const Class *type = metadata_space_->type(rv.bundle.type);
            MoveToArgumentIfNeeded(type, rv.bundle.index, static_cast<Value::Linkage>(rv.kind),
                                   kPointerSize, ast);
            VISIT_CHECK(ast->hint());
            type = metadata_space_->type(rv.bundle.index);
            Value value;
            if (type->is_reference()) {
                value = FindOrInsertExternalFunction("channel::recvPtr");
            } else {
                switch (type->reference_size()) {
                case 1:
                case 2:
                case 4:
                    if (type->IsFloating()) {
                        value = FindOrInsertExternalFunction("channel::recvF32");
                    } else {
                        value = FindOrInsertExternalFunction("channel::recv32");
                    }
                    break;
                case 8:
                    if (type->IsFloating()) {
                        value = FindOrInsertExternalFunction("channel::recvF64");
                    } else {
                        value = FindOrInsertExternalFunction("channel::recv64");
                    }
                    break;
                default:
                    NOREACHED();
                    break;
                }
            }
            DCHECK_EQ(Value::kGlobal, value.linkage);
            LdaGlobal(metadata_space_->builtin_type(kType_closure), value.index, ast);
            current_fun_->EmitDirectlyCallFunction(ast, true/*native*/, 0/*slot*/,
                                                   kPointerSize/*arg_size*/);
            current_fun_->EmitYield(ast, YIELD_PROPOSE);
            return ResultWith(Value::kACC, type->id(), 0);
        } break;
            
        default:
            NOREACHED();
            break;
    }
    return ResultWithError();
}

ASTVisitor::Result BytecodeGenerator::VisitBinaryExpression(BinaryExpression *ast) /*override*/ {
    switch (ast->op().kind) {
        case Operator::kAdd:
        case Operator::kSub:
        case Operator::kMul:
        case Operator::kDiv:
        case Operator::kMod:
        case Operator::kBitwiseOr:
        case Operator::kBitwiseAnd:
        case Operator::kBitwiseXor:
        case Operator::kBitwiseShl:
        case Operator::kBitwiseShr: {
            OperandContext receiver;
            if (!GenerateBinaryOperands(&receiver, ast->lhs(), ast->rhs())) {
                return ResultWithError();
            }
            if (receiver.rhs_pair) {
                GenerateOperation(ast->op(), receiver.lhs_type, receiver.lhs,
                                  /*lhs_linkage*/Value::kStack, ast->rhs()->AsPairExpression(), ast);
            } else if (receiver.lhs_type->IsArray()) {
                if (ast->op().kind == Operator::kSub) {
                    GenerateArrayMinus(receiver.lhs_type, receiver.lhs, /*lhs_linkage*/Value::kStack,
                                       receiver.rhs_type, receiver.rhs, /*rhs_linkage*/Value::kStack,
                                       ast);
                }
            } else {
                GenerateOperation(ast->op(), receiver.lhs_type, receiver.lhs, receiver.rhs, ast);
            }
            CleanupOperands(&receiver);
            return ResultWith(Value::kACC, receiver.lhs_type->id(), 0);
        } break;
            
        case Operator::kEqual:
        case Operator::kNotEqual:
        case Operator::kLess:
        case Operator::kLessEqual:
        case Operator::kGreater:
        case Operator::kGreaterEqual: {
            OperandContext receiver;
            if (!GenerateBinaryOperands(&receiver, ast->lhs(), ast->rhs())) {
                return ResultWithError();
            }
            GenerateComparation(receiver.lhs_type, ast->op(), receiver.lhs, receiver.rhs, ast);
            CleanupOperands(&receiver);
        } return ResultWith(Value::kACC, kType_bool, 0);
        
        case Operator::kOr:
        case Operator::kAnd: {
            Result rv;
            VISIT_CHECK(ast->lhs());
            DCHECK_EQ(kType_bool, rv.bundle.type);
            LdaIfNeeded(rv, ast->lhs());
            BytecodeLabel done;
            if (ast->op().kind == Operator::kOr) {
                EMIT(ast, JumpIfTrue(&done, 0/*slot*/));
            } else {
                EMIT(ast, JumpIfFalse(&done, 0/*slot*/));
            }

            VISIT_CHECK(ast->rhs());
            DCHECK_EQ(kType_bool, rv.bundle.type);
            LdaIfNeeded(rv, ast->lhs());

            current_fun_->builder()->Bind(&done);
        } return ResultWith(Value::kACC, kType_bool, 0);
            
        // val ok = ch <- value
        case Operator::kSend: {
            Result rv;
            VISIT_CHECK(ast->lhs());
            const Class *type = metadata_space_->type(rv.bundle.type);
            int argument_offset = RoundUp(type->reference_size(), kStackSizeGranularity);
            MoveToArgumentIfNeeded(type, rv.bundle.index, static_cast<Value::Linkage>(rv.kind),
                                   argument_offset, ast);
            VISIT_CHECK(ast->rhs());
            type = metadata_space_->type(rv.bundle.type);
            argument_offset += RoundUp(type->reference_size(), kStackSizeGranularity);
            MoveToArgumentIfNeeded(type, rv.bundle.index, static_cast<Value::Linkage>(rv.kind),
                                   argument_offset, ast);
            GenerateSend(type, 0/*lhs*/, 0/*rhs*/, ast);
        } return ResultWith(Value::kACC, kType_bool, 0);

        default:
            NOREACHED();
            break;
    }
    return ResultWithError();
}

ASTVisitor::Result BytecodeGenerator::VisitIfExpression(IfExpression *ast) /*override*/ {
    std::unique_ptr<BlockScope> block_scope;
    Result rv;
    if (ast->extra_statement()) {
        block_scope.reset(new BlockScope(Scope::kPlainBlockScope, &current_));
        VISIT_CHECK(ast->extra_statement());
    }

    VISIT_CHECK(ast->condition());
    DCHECK_EQ(kType_bool, rv.bundle.type);
    LdaIfNeeded(rv, ast->condition());
    BytecodeLabel br_false;
    EMIT(ast->condition(), JumpIfFalse(&br_false, 0/*slot*/));

    VISIT_CHECK(ast->branch_true());
    if (ast->branch_true()->IsExpression()) {
        LdaIfNeeded(rv, ast->branch_true());
    }
    BytecodeLabel trunk;
    //current_fun_->Incoming(ast)->Jump(&trunk, 0/*slot*/);
    EMIT(ast, Jump(&trunk, 0/*slot*/));
    
    current_fun_->builder()->Bind(&br_false);
    if (ast->branch_false()) {
        VISIT_CHECK(ast->branch_false());
        if (ast->branch_false()->IsExpression()) {
            LdaIfNeeded(rv, ast->branch_true());
        }
    }
    current_fun_->builder()->Bind(&trunk);
    return rv;
}

ASTVisitor::Result BytecodeGenerator::VisitStatementBlock(StatementBlock *ast) /*override*/ {
    BlockScope block_scope(Scope::kPlainBlockScope, &current_);
    Result rv;
    for (auto stmt : ast->statements()) {
        VISIT_CHECK(stmt);
    }
    
    return ResultWithVoid();
}

ASTVisitor::Result BytecodeGenerator::VisitArrayInitializer(ArrayInitializer *ast) /*override*/ {
    Result rv;
    VISIT_CHECK(ast->element_type());
    const Class *element_type = metadata_space_->type(rv.bundle.index);
    const Class *type = nullptr;
    if (element_type->is_reference()) {
        type = metadata_space_->type(kType_array);
    } else {
        switch (element_type->reference_size()) {
            case 1:
                type = metadata_space_->type(kType_array8);
                break;
            case 2:
                type = metadata_space_->type(kType_array16);
                break;
            case 4:
                type = metadata_space_->type(kType_array32);
                break;
            case 8:
                type = metadata_space_->type(kType_array64);
                break;
            default:
                NOREACHED();
                break;
        }
    }
    int kidx = current_fun_->constants()->FindOrInsertMetadata(element_type);
    if (ast->reserve()) {
        VISIT_CHECK(ast->reserve());
        const Class *reserve_type = metadata_space_->type(rv.bundle.type);
        bool reserve_tmp = (rv.kind != Value::kStack);
        int reserve = rv.bundle.index;
        if (reserve_tmp) {
            reserve = current_fun_->StackReserve(reserve_type);
            MoveToStackIfNeeded(reserve_type, rv.bundle.index, static_cast<Value::Linkage>(rv.kind),
                                reserve, ast);
        }
        current_fun_->EmitArray(ast, GetStackOffset(reserve), GetConstOffset(kidx));
        if (reserve_tmp) {
            current_fun_->StackFallback(reserve_type, reserve);
        }
    } else {
        StackSpaceScope stack_scope(current_fun_->stack());
        std::vector<Value> elements;
        for (auto element : ast->operands()) {
            VISIT_CHECK(element);
            Value value{Value::kStack};
            if (rv.kind != Value::kStack) {
                value.index = current_fun_->StackReserve(element_type);
                MoveToStackIfNeeded(element_type, rv.bundle.index,
                                    static_cast<Value::Linkage>(rv.kind), value.index, element);
            } else {
                value.index = rv.bundle.index;
            }
            value.linkage = Value::kStack;
            elements.push_back(value);
        }
        int argument_offset = 0;
        for (ssize_t i = elements.size() - 1; i >= 0 ;i--) {
            const Value &element = elements[i];
            argument_offset += RoundUp(element_type->reference_size(), kStackSizeGranularity);
            MoveToArgumentIfNeeded(element_type, element.index, element.linkage, argument_offset,
                                   ast);
        }
        current_fun_->EmitArrayWith(ast, argument_offset, GetConstOffset(kidx));
    }
    return ResultWith(Value::kACC, type->id(), 0);
}

ASTVisitor::Result BytecodeGenerator::VisitChannelInitializer(ChannelInitializer *ast) /*override*/ {
    Result rv;
    VISIT_CHECK(ast->value_type());
    
    const Class *u32_type = metadata_space_->builtin_type(kType_u32);
    int argument_offset = RoundUp(u32_type->reference_size(), kStackSizeGranularity);
    if (rv.bundle.index <= BytecodeNode::kMaxUSmi32) {
        EMIT(ast, Add<kLdaSmi32>(rv.bundle.index));
        MoveToArgumentIfNeeded(u32_type, 0, Value::kACC, argument_offset, ast);
    } else {
        int kidx = current_fun_->constants()->FindOrInsertU32(rv.bundle.index);
        MoveToArgumentIfNeeded(u32_type, kidx, Value::kConstant, argument_offset, ast);
    }
    
    const Class *int_type = metadata_space_->builtin_type(kType_int);
    VISIT_CHECK(ast->buffer_size());
    DCHECK_EQ(int_type->id(), rv.bundle.type);
    argument_offset += RoundUp(int_type->reference_size(), kStackSizeGranularity);
    MoveToArgumentIfNeeded(int_type, rv.bundle.index, static_cast<Value::Linkage>(rv.kind),
                           argument_offset, ast->buffer_size());

    Value value = FindOrInsertExternalFunction("lang.newChannel");
    LdaGlobal(metadata_space_->builtin_type(kType_closure), value.index, ast);
    current_fun_->EmitDirectlyCallFunction(ast, true/*native*/, 0/*slot*/,
                                           argument_offset);
    return ResultWith(Value::kACC, kType_channel, 0);
}

ASTVisitor::Result BytecodeGenerator::VisitIndexExpression(IndexExpression *ast) /*override*/ {
    Result rv;
    VISIT_CHECK(ast->hint());
    const Class *value_type = metadata_space_->type(rv.bundle.index);

    OperandContext recevier;
    if (!GenerateBinaryOperands(&recevier, ast->primary(), ast->index())) {
        return ResultWithError();
    }
    const Class *primary_type = recevier.lhs_type;
    if (primary_type->IsArray()) {
        LdaArrayAt(value_type, recevier.lhs, recevier.rhs, ast);
    } else {
        NOREACHED();
    }

    CleanupOperands(&recevier);
    return ResultWith(Value::kACC, value_type->id(), 0);;
}

ASTVisitor::Result BytecodeGenerator::VisitIdentifier(Identifier *ast) /*override*/ {
    auto [owns, value] = current_->Resolve(ast->name());
    DCHECK(owns != nullptr && value.linkage != Value::kError);

    if (owns->is_file_scope()) {
        if (!HasGenerated(value.ast) && !GenerateSymbolDependence(value)) {
            return ResultWithError();
        }
        DCHECK(HasGenerated(value.ast));
    }

    switch (owns->kind()) {
        case Scope::kFunctionScope:
        case Scope::kFileScope:
        case Scope::kLoopBlockScope:
        case Scope::kPlainBlockScope:
            if (ShouldCaptureVar(owns, value)) {
                return CaptureVar(ast->name()->ToString(), owns, value);
            } else {
                return ResultWith(value);
            }
        case Scope::kClassScope:
        default:
            NOREACHED();
            break;
    }
    return ResultWith(value);
}

ASTVisitor::Result BytecodeGenerator::VisitBoolLiteral(BoolLiteral *ast) /*override*/ {
    EMIT(ast, Add<kLdaSmi32>(ast->value()));
    return ResultWith(Value::kACC, kType_bool, 0);
}

ASTVisitor::Result BytecodeGenerator::VisitI8Literal(I8Literal *ast) /*override*/ {
    if (ast->value() < 0) {
        int kidx = current_fun_->constants()->FindOrInsertI32(ast->value());
        return ResultWith(Value::kConstant, kType_i8, kidx);
    } else if (ast->value() > 0) {
        EMIT(ast, Add<kLdaSmi32>(ast->value()));
        return ResultWith(Value::kACC, kType_i8, 0);
    } else {
        EMIT(ast, Add<kLdaZero>());
        return ResultWith(Value::kACC, kType_i8, 0);
    }
}

ASTVisitor::Result BytecodeGenerator::VisitU8Literal(U8Literal *ast) /*override*/ {
    EMIT(ast, Add<kLdaSmi32>(ast->value()));
    return ResultWith(Value::kACC, kType_u8, 0);
}

ASTVisitor::Result BytecodeGenerator::VisitI16Literal(I16Literal *ast) /*override*/ {
    int index = current_fun_->constants()->FindOrInsertI32(ast->value());
    return ResultWith(Value::kConstant, kType_i16, index);
}

ASTVisitor::Result BytecodeGenerator::VisitU16Literal(U16Literal *ast) /*override*/ {
    EMIT(ast, Add<kLdaSmi32>(ast->value()));
    return ResultWith(Value::kACC, kType_u16, 0);
}

ASTVisitor::Result BytecodeGenerator::VisitI32Literal(I32Literal *ast) /*override*/ {
    if (ast->value() >= 0 && ast->value() <= BytecodeNode::kMaxUSmi32) {
        EMIT(ast, Add<kLdaSmi32>(ast->value()));
        return ResultWith(Value::kACC, kType_i32, 0);
    } else {
        int index = current_fun_->constants()->FindOrInsertI32(ast->value());
        return ResultWith(Value::kConstant, kType_i32, index);
    }
}

ASTVisitor::Result BytecodeGenerator::VisitU32Literal(U32Literal *ast) /*override*/ {
    if (ast->value() <= BytecodeNode::kMaxUSmi32) {
        EMIT(ast, Add<kLdaSmi32>(ast->value()));
        return ResultWith(Value::kACC, kType_u32, 0);
    } else {
        int index = current_fun_->constants()->FindOrInsertU32(ast->value());
        return ResultWith(Value::kConstant, kType_u32, index);
    }
}

ASTVisitor::Result BytecodeGenerator::VisitIntLiteral(IntLiteral *ast) /*override*/ {
    if (ast->value() >= 0 && ast->value() <= BytecodeNode::kMaxUSmi32) {
        EMIT(ast, Add<kLdaSmi32>(ast->value()));
        return ResultWith(Value::kACC, kType_int, 0);
    } else {
        int index = current_fun_->constants()->FindOrInsertI32(ast->value());
        return ResultWith(Value::kConstant, kType_int, index);
    }
}

ASTVisitor::Result BytecodeGenerator::VisitUIntLiteral(UIntLiteral *ast) /*override*/ {
    if (ast->value() >= 0 && ast->value() <= BytecodeNode::kMaxUSmi32) {
        EMIT(ast, Add<kLdaSmi32>(ast->value()));
        return ResultWith(Value::kACC, kType_uint, 0);
    } else {
        int index = current_fun_->constants()->FindOrInsertU32(ast->value());
        return ResultWith(Value::kConstant, kType_uint, index);
    }
}

ASTVisitor::Result BytecodeGenerator::VisitI64Literal(I64Literal *ast) /*override*/ {
    if (ast->value() == 0) {
        EMIT(ast, Add<kLdaZero>());
        return ResultWith(Value::kACC, kType_i64, 0);
    } else {
        int index = current_fun_->constants()->FindOrInsertI64(ast->value());
        return ResultWith(Value::kConstant, kType_i64, index);
    }
}

ASTVisitor::Result BytecodeGenerator::VisitU64Literal(U64Literal *ast) /*override*/ {
    if (ast->value() == 0) {
        EMIT(ast, Add<kLdaZero>());
        return ResultWith(Value::kACC, kType_u64, 0);
    } else {
        int index = current_fun_->constants()->FindOrInsertU64(ast->value());
        return ResultWith(Value::kConstant, kType_u64, index);
    }
}

ASTVisitor::Result BytecodeGenerator::VisitF32Literal(F32Literal *ast) /*override*/ {
    int index = current_fun_->constants()->FindOrInsertF32(ast->value());
    return ResultWith(Value::kConstant, kType_f32, index);
}

ASTVisitor::Result BytecodeGenerator::VisitF64Literal(F64Literal *ast) /*override*/ {
    int index = current_fun_->constants()->FindOrInsertF64(ast->value());
    return ResultWith(Value::kConstant, kType_f64, index);
}

ASTVisitor::Result BytecodeGenerator::VisitNilLiteral(NilLiteral *ast) /*override*/ {
    int index = current_fun_->constants()->FindOrInsertU64(0);
    return ResultWith(Value::kConstant, kType_any, index);
}

ASTVisitor::Result BytecodeGenerator::VisitStringLiteral(StringLiteral *ast) /*override*/ {
    std::string_view key = ast->value()->ToSlice();
    int index = current_fun_->constants()->FindString(key);
    if (index >= 0) {
        return ResultWith(Value::kConstant, kType_string, index);
    }
    String *s = Machine::This()->NewUtf8String(key.data(), key.size(), Heap::kOld);
    index = current_fun_->constants()->FindOrInsertString(s);
    return ResultWith(Value::kConstant, kType_string, index);
}

ASTVisitor::Result BytecodeGenerator::VisitBreakableStatement(BreakableStatement *ast)
/*override*/ {
    switch (ast->control()) {
        case BreakableStatement::THROW:
        case BreakableStatement::RETURN: {
            Result rv;
            VISIT_CHECK(ast->value());
            LdaIfNeeded(rv, ast->value());
            if (ast->IsReturn()) {
                EMIT(ast, Add<kReturn>());
            } else {
                EMIT(ast, Add<kThrow>());
            }
        } break;
        case BreakableStatement::BREAK: {
            BlockScope *block_scope = current_->GetLocalBlockScope(Scope::kLoopBlockScope);
            EMIT(ast, Jump(DCHECK_NOTNULL(block_scope)->mutable_exit_label(), 0/*slot*/));
        } break;
        case BreakableStatement::CONTINUE: {
            BlockScope *block_scope = current_->GetLocalBlockScope(Scope::kLoopBlockScope);
            EMIT(ast, Jump(DCHECK_NOTNULL(block_scope)->mutable_retry_label(), 0/*slot*/));
        } break;
        case BreakableStatement::YIELD: {
            current_fun_->EmitYield(ast, YIELD_RANDOM);
        } break;
        default:
            NOREACHED();
            break;
    }
    return ResultWithVoid();
}

ASTVisitor::Result BytecodeGenerator::VisitTryCatchFinallyBlock(TryCatchFinallyBlock *ast)
/*override*/ {
    ASTNode *dummy = ast;
    intptr_t start_pc = current_fun_->builder()->pc();
    Result rv;
    BytecodeLabel finally;
    if (ast->try_statements_size() > 0) {
        BlockScope block_scope(Scope::kPlainBlockScope, &current_);
        for (auto stmt : ast->try_statements()) {
            VISIT_CHECK(stmt);
            dummy = stmt;
        }
    }
    intptr_t stop_pc = current_fun_->builder()->pc();
    EMIT(dummy, Jump(&finally, 0/*slot*/));
    
    for (auto block : ast->catch_blocks()) {
        BlockScope block_scope(Scope::kPlainBlockScope, &current_);
        intptr_t handler_pc = current_fun_->builder()->pc();

        VISIT_CHECK(block->expected_declaration()->type());
        const Class *clazz = metadata_space_->type(rv.bundle.type);
        DCHECK(clazz->IsSameOrBaseOf(class_exception_->clazz()));
        Value value {
            Value::kStack,
            clazz,
            current_fun_->stack()->ReserveRef(),
            block->expected_declaration()
        };
        StaStack(clazz, value.index, block->expected_declaration());
        block_scope.Register(block->expected_declaration()->identifier()->ToString(), value);
        
        dummy = block->expected_declaration();
        for (auto stmt : block->statements()) {
            VISIT_CHECK(stmt);
            dummy = stmt;
        }
        EMIT(dummy, Jump(&finally, 0/*slot*/));
        current_fun_->AddExceptionHandler(clazz, start_pc, stop_pc, handler_pc);
    }

    if (!finally.unlinked_nodes().empty()) {
        current_fun_->builder()->Bind(&finally);
    }
    if (ast->finally_statements_size() > 0) {
        BlockScope block_scope(Scope::kPlainBlockScope, &current_);
        for (auto stmt : ast->finally_statements()) {
            VISIT_CHECK(stmt);
        }
    }
    return ResultWithVoid();
}

ASTVisitor::Result BytecodeGenerator::VisitAssignmentStatement(AssignmentStatement *ast)
/*override*/ {
    switch(ast->lval()->kind()) {
        case ASTNode::kIdentifier: {
            const ASTString *name = ast->lval()->AsIdentifier()->name();
            auto [owns, lval] = current_->Resolve(name);
            return GenerateVariableAssignment(name, lval, owns, ast->assignment_op(), ast->rval(),
                                              ast);
        } break;

        case ASTNode::kDotExpression: {
            DotExpression *dot = ast->lval()->AsDotExpression();
            if (!dot->primary()->IsIdentifier()) {
                Result rv;
                VISIT_CHECK(dot->primary());
                Value self {
                    static_cast<Value::Linkage>(rv.kind),
                    metadata_space_->type(rv.bundle.type),
                    rv.bundle.index,
                };
                return GeneratePropertyAssignment(nullptr/*name*/, self, current_file_,
                                                  ast->assignment_op(), ast->rval(), dot);
            }

            Identifier *id = DCHECK_NOTNULL(dot->primary()->AsIdentifier());
            Value value = current_file_->Find(id->name(), dot->rhs());
            if (value.linkage != Value::kError) { // Found
                return GenerateVariableAssignment(id->name(), value, current_file_,
                                                  ast->assignment_op(), ast->rval(), ast);
            }
            
            auto [owns, self] = current_->Resolve(id->name());
            return GeneratePropertyAssignment(id->name(), self, owns, ast->assignment_op(),
                                              ast->rval(), dot);
        } break;

        case ASTNode::kIndexExpression: {
            IndexExpression *lval = ast->lval()->AsIndexExpression();
            OperandContext recevier;
            if (!GenerateBinaryOperands(&recevier, lval->primary(), lval->index())) {
                return ResultWithError();
            }
            Result rv = GenerateIndexAssignment(recevier.lhs_type, recevier.lhs, recevier.rhs,
                                                ast->assignment_op(), ast->rval(), lval);
            CleanupOperands(&recevier);
            return rv;
        } break;

        default:
            NOREACHED();
            break;
    }
    return ResultWithVoid();
}

ASTVisitor::Result BytecodeGenerator::VisitRunStatement(RunStatement *ast) /*override*/ {
    CallingReceiver receiver;
    auto rv = GenerateCalling(ast->calling(), &receiver);
    if (rv.kind == Value::kError) {
        return ResultWithError();
    }

    switch (receiver.kind) {
        case CallingReceiver::kCtor:
            error_feedback_->Printf(FindSourceLocation(ast->calling()),
                                    "Attempt run class constructor.");
            return ResultWithError();
        case CallingReceiver::kNative:
            error_feedback_->Printf(FindSourceLocation(ast->calling()),
                                    "Attempt run native function.");
            return ResultWithError();
        case CallingReceiver::kVtab:
        case CallingReceiver::kBytecode:
            EMIT(ast, Add<kRunCoroutine>(0/*flags*/, receiver.arguments_size));
            current_fun_->EmitYield(ast, YIELD_PROPOSE);
            break;
    }
    return ResultWithVoid();
}

ASTVisitor::Result BytecodeGenerator::VisitWhileLoop(WhileLoop *ast) /*override*/ {
    BlockScope block_scope(Scope::kLoopBlockScope, &current_);
    
    // Retry:
    current_fun_->builder()->Bind(block_scope.mutable_retry_label());
    Result rv;
    VISIT_CHECK(ast->condition());
    DCHECK_EQ(kType_bool, rv.bundle.type);
    LdaIfNeeded(rv, ast->condition());
    EMIT(ast->condition(), JumpIfFalse(block_scope.mutable_exit_label(), 0/*slot*/));

    for (auto stmt : ast->statements()) {
        VISIT_CHECK(stmt);
    }

    EMIT(ast, Jump(block_scope.mutable_retry_label(), 0/*slot*/)); // TODO: Profiling
    current_fun_->builder()->Bind(block_scope.mutable_exit_label());
    return ResultWithVoid();
}

ASTVisitor::Result BytecodeGenerator::VisitForLoop(ForLoop *ast) /*override*/ {
    BlockScope block_scope(Scope::kLoopBlockScope, &current_);
    switch (ast->control()) {
        case ForLoop::STEP:
            return GenerateForStep(ast);
        case ForLoop::ITERATE:
            TODO();
            break;
        case ForLoop::CHANNEL_ITERATE:
            TODO();
            break;
        default:
            NOREACHED();
            break;
    }
    return ResultWithVoid();
}

ASTVisitor::Result BytecodeGenerator::GenerateForStep(ForLoop *ast) {
    BlockScope *block_scope = down_cast<BlockScope>(current_);
    
    Result rv;
    VISIT_CHECK(ast->value()->type());
    const Class *type = metadata_space_->type(rv.bundle.index);
    Value value{Value::kStack, type, current_fun_->StackReserve(type)};
    current_->Register(ast->value()->identifier()->ToString(), value);

    VISIT_CHECK(ast->subject());
    Value iter{Value::kStack, type, current_fun_->StackReserve(type)};
    current_->Register("$__iter__", iter);
    MoveToStackIfNeeded(type, rv.bundle.index, static_cast<Value::Linkage>(rv.kind), iter.index,
                        ast->subject());

    VISIT_CHECK(ast->limit());
    int index = 0;
    if (rv.kind != Value::kStack) {
        index = current_fun_->StackReserve(type);
        MoveToStackIfNeeded(type, index, static_cast<Value::Linkage>(rv.kind), index, ast);
    } else {
        index = rv.bundle.index;
    }
    Value limit{Value::kStack, type, index};
    current_->Register("$__limit__", limit);

    // label: retry
    current_fun_->builder()->Bind(block_scope->mutable_retry_label());
    GenerateComparation(type, Operators::kGreaterEqual, iter.index, limit.index, ast);
    EMIT(ast, JumpIfTrue(block_scope->mutable_exit_label(), 0/*slot*/));
    
    // value = __iter__
    MoveToStackIfNeeded(type, iter.index, iter.linkage, value.index, ast);
    
    for (auto stmt : ast->statements()) {
        VISIT_CHECK(stmt);
    }

    DCHECK(type->IsNumber());
    if (type->IsFloating()) {
        switch (type->reference_size()) {
            case 4: {
                int kidx = current_fun_->constants()->FindOrInsertF32(1);
                GenerateOperation(Operators::kAdd, iter.type, iter.index, iter.linkage, iter.type,
                                  kidx, Value::kConstant, ast);
            } break;
            case 8: {
                int kidx = current_fun_->constants()->FindOrInsertF64(1);
                GenerateOperation(Operators::kAdd, iter.type, iter.index, iter.linkage, iter.type,
                                  kidx, Value::kConstant, ast);
            } break;
            default:
                NOREACHED();
                break;
        }
    } else {
        switch (type->reference_size()) {
            case 1: case 2: case 4:
                EMIT(ast, Add<kIncrement32>(GetStackOffset(iter.index), 1));
                break;
            case 8:
                EMIT(ast, Add<kIncrement64>(GetStackOffset(iter.index), 1));
                break;
            default:
                NOREACHED();
                break;
        }
    }
    EMIT(ast, Jump(block_scope->mutable_retry_label(), 0/*slot*/));
    // label: exit
    current_fun_->builder()->Bind(block_scope->mutable_exit_label());
    return ResultWithVoid();
}

ASTVisitor::Result BytecodeGenerator::GenerateCalling(CallExpression *ast,
                                                      CallingReceiver *receiver) {
    if (!ast->callee()->IsDotExpression()) {
        return GenerateRegularCalling(ast, receiver);
    }

    Result rv;
    DotExpression *dot = DCHECK_NOTNULL(ast->callee()->AsDotExpression());
    if (!dot->primary()->IsIdentifier()) {
        VISIT_CHECK(dot->primary());
        const Class *clazz = metadata_space_->type(rv.bundle.type);
        Value::Linkage linkage = static_cast<Value::Linkage>(rv.kind);
        return GenerateMethodCalling({linkage, clazz, rv.bundle.index}, ast, receiver);
    }

    Identifier *id = DCHECK_NOTNULL(dot->primary()->AsIdentifier());
    Value value = current_file_->Find(id->name(), dot->rhs());
    if (value.linkage != Value::kError) { // Found: pcakgeName.identifer
        if (!HasGenerated(value.ast) && !GenerateSymbolDependence(value)) {
            return ResultWithError();
        }
        return GenerateRegularCalling(ast, receiver);
    }

    auto found = current_->Resolve(id->name());
    value = std::get<1>(found);
    if (!HasGenerated(value.ast) && !GenerateSymbolDependence(value)) {
        return ResultWithError();
    }
    if (ShouldCaptureVar(std::get<0>(found), value)) {
        if (auto rv = CaptureVar(id->name()->ToString(), std::get<0>(found), value);
            rv.kind == Value::kError) {
            return ResultWithError();
        } else {
            value.linkage = static_cast<Value::Linkage>(rv.kind);
            value.index   = rv.bundle.index;
            value.flags   = rv.bundle.flags;
        }
    }
    return GenerateMethodCalling(value, ast, receiver);
}

bool BytecodeGenerator::ShouldCaptureVar(Scope *owns, Value value) {
    if (owns->is_file_scope()) {
        return false;
    }
    switch (value.linkage) {
        case Value::kMetadata:
        case Value::kACC:
        case Value::kGlobal:
        case Value::kConstant:
        case Value::kError:
            return false;
        default:
            break;
    }
    DCHECK(value.linkage == Value::kStack || value.linkage == Value::kCaptured);
    bool out_bound = false;
    for (Scope *scope = current_; scope; scope = down_cast<Scope>(scope->prev())) {
        if (scope == owns) {
            return out_bound;
        }
        if (scope == current_fun_) {
            out_bound = true;
        }
    }
    return false;
}

ASTVisitor::Result BytecodeGenerator::CaptureVar(const std::string &name, Scope *owns,
                                                 Value value) {
    std::deque<FunctionScope *> links;
    for (Scope *scope = current_fun_; scope; scope = down_cast<Scope>(scope->prev())) {
        if (scope == owns) {
            break;
        } else if (scope->is_function_scope()) {
            links.push_front(down_cast<FunctionScope>(scope));
        }
    }
    DCHECK(!links.empty());

    for (auto fun : links) {
        if (value.linkage == Value::kCaptured) {
            value.index = fun->AddCapturedVarDesc(name, value.type, Function::IN_CAPTURED,
                                                  value.index);
        } else {
            DCHECK_EQ(Value::kStack, value.linkage);
            value.index = fun->AddCapturedVarDesc(name, value.type, Function::IN_STACK,
                                                  value.index);
        }
        value.linkage = Value::kCaptured;
    }
    current_->Register(name, value);
    return ResultWith(value);
}

SourceLocation BytecodeGenerator::FindSourceLocation(const ASTNode *ast) {
    return DCHECK_NOTNULL(current_file_)->file_unit()->FindSourceLocation(ast);
}

ASTVisitor::Result BytecodeGenerator::GenerateNewObject(const Class *clazz, CallExpression *ast,
                                                        CallingReceiver *receiver) {
    int kidx = current_fun_->constants()->FindOrInsertMetadata(clazz);
    current_fun_->EmitNewObject(ast, GetConstOffset(kidx));
    if (!clazz->init()) {
        return ResultWith(Value::kACC, clazz->id(), 0);
    }
    StackSpaceAllocator::Scope stack_scope(current_fun_->stack());

    // first argument
    int self = current_fun_->stack()->ReserveRef();
    StaStack(clazz, self, ast);

    int argument_size = 0;
    if (argument_size = GenerateArguments(ast->operands(), {}, ast->callee(), self, false);
        argument_size < 0) {
        return ResultWithError();
    }

    kidx = current_fun_->constants()->FindOrInsertClosure(clazz->init()->fn());
    LdaConst(metadata_space_->builtin_type(kType_closure), kidx, ast);
    receiver->ast = ast;
    receiver->kind = CallingReceiver::kCtor;
    receiver->attributes = 0;
    receiver->arguments_size = argument_size;
    
    current_fun_->stack()->FallbackRef(self);
    return ResultWith(Value::kACC, clazz->id(), 0);
}

int BytecodeGenerator::GenerateArguments(const base::ArenaVector<Expression *> &operands,
                                         const std::vector<const Class *> &params,
                                         ASTNode *callee, int self, bool vargs) {
    std::vector<Value> args;
    for (size_t i = 0; i < operands.size(); i++) {
        Result rv = operands[i]->Accept(this);
        if (rv.kind == Value::kError) {
            return -1;
        }
        const Class *type = metadata_space_->type(rv.bundle.type);
        if (rv.kind == Value::kACC) {
            int idx = current_fun_->stack()->Reserve(type->reference_size());
            StaStack(type, idx, operands[i]);
            args.push_back({Value::kStack, type, idx});
        } else {
            args.push_back({static_cast<Value::Linkage>(rv.kind), type, rv.bundle.index});
        }
    }
    int argument_offset = 0;
    if (callee) {
        argument_offset += kPointerSize;
        EMIT(callee, Incomplete<kMovePtr>(-argument_offset, GetStackOffset(self)));
    }

    for (size_t i = 0; i < operands.size(); i++) {
        Expression *arg = operands[i];
        Value value = args[i];
        
        argument_offset += RoundUp(value.type->reference_size(), kStackSizeGranularity);
        if (value.type->is_reference()) {
            if (value.linkage == Value::kStack) {
                EMIT(arg, Incomplete<kMovePtr>(-argument_offset, GetStackOffset(value.index)));
            } else {
                LdaIfNeeded(value.type, value.index, value.linkage, arg);
                EMIT(arg, Incomplete<kStarPtr>(-argument_offset));
            }
            continue;
        }
        if (i < params.size() && NeedInbox(params[i], value.type)) {
            InboxIfNeeded(value.type, value.index, value.linkage, params[i], arg);
            continue;
        }
        MoveToArgumentIfNeeded(value.type, value.index, value.linkage, argument_offset, arg);
    }
    
    if (vargs) {
        // TODO:
        TODO();
    }
    return argument_offset;
}

ASTVisitor::Result
BytecodeGenerator::GeneratePropertyAssignment(const ASTString *name, Value self, Scope *owns,
                                              Operator op, Expression *rhs, DotExpression *ast) {
    Value rval{Value::kError};
    if (!rhs->IsPairExpression()) {
        Result rv;
        VISIT_CHECK(rhs);
        rval.linkage = static_cast<Value::Linkage>(rv.kind);
        rval.type = metadata_space_->type(rv.bundle.type);
        rval.index = rv.bundle.index;
    }
    
    if (!owns->is_file_scope()) {
        if (!HasGenerated(self.ast) && !GenerateSymbolDependence(self)) {
            return ResultWithError();
        }
    }
    
    if (ShouldCaptureVar(owns, self)) {
        auto rv = CaptureVar(name->ToString(), owns, self);
        DCHECK_NE(Value::kError, rv.kind);
        self.linkage = static_cast<Value::Linkage>(rv.kind);
        DCHECK_EQ(rv.bundle.type, self.type->id());
        self.index = rv.bundle.index;
    }
    
    switch (op.kind) {
        case Operator::kAdd:
        case Operator::kSub: {
            bool rval_tmp = (!rhs->IsPairExpression() && rval.linkage != Value::kStack);
            if (rval_tmp) {
                int index = current_fun_->StackReserve(rval.type);
                MoveToStackIfNeeded(rval.type, rval.index, rval.linkage, index, ast);
                rval.index = index;
                rval.linkage = Value::kStack;
            }
            auto rv = GenerateLoadProperty(self.type, self.index, self.linkage, ast);
            if (rv.kind == Value::kError) {
                return ResultWithError();
            }
            const Class *field_type = metadata_space_->type(rv.bundle.type);
            if (rhs->IsPairExpression()) {
                GenerateOperation(op, field_type, 0/*lhs_index*/, Value::kACC/*lhs_linkage*/,
                                  rhs->AsPairExpression(), ast);
            } else {
                GenerateOperation(op, field_type, 0/*lhs_index*/, Value::kACC/*lhs_linkage*/,
                                  rval.type, rval.index, rval.linkage, ast);
            }
            if (rval_tmp) {
                current_fun_->StackFallback(rval.type, rval.index);
            }
        } break;
        case Operator::NOT_OPERATOR: {
            const Field *field =
                DCHECK_NOTNULL(metadata_space_->FindClassFieldOrNull(self.type, ast->rhs()->data()));
            if (NeedInbox(field->type(), rval.type)) {
                InboxIfNeeded(rval.type, rval.index, rval.linkage, field->type(), ast);
            } else {
                LdaIfNeeded(rval.type, rval.index, rval.linkage, ast);
            }
        } break;
        default:
            NOREACHED();
            break;
    }
    return GenerateStoreProperty(self.type, self.index, self.linkage, ast);
}

ASTVisitor::Result
BytecodeGenerator::GenerateIndexAssignment(const Class *type, int primary, int index, Operator op,
                                           Expression *rhs, IndexExpression *ast) {
    Result rv;
    VISIT_CHECK(ast->hint());
    const Class *value_type = metadata_space_->type(rv.bundle.type);
    Value rval{Value::kError};
    if (!rhs->IsPairExpression()) {
        VISIT_CHECK(rhs);
        rval.linkage = static_cast<Value::Linkage>(rv.kind);
        rval.type = metadata_space_->type(rv.bundle.type);
        rval.index = rv.bundle.index;
    }

    switch (op.kind) {
        case Operator::kAdd:
        case Operator::kSub: {
            bool rval_tmp = (!rhs->IsPairExpression() && rval.linkage != Value::kStack);
            if (rval_tmp) {
                int index = current_fun_->StackReserve(rval.type);
                MoveToStackIfNeeded(rval.type, rval.index, rval.linkage, index, rhs);
                rval.index = index;
                rval.linkage = Value::kStack;
            }
            if (type->IsArray()) {
                LdaArrayAt(value_type, primary, index, ast);
            } else {
                // TODO:
                TODO();
            }
            if (rhs->IsPairExpression()) {
                GenerateOperation(op, type, 0/*lhs_index*/, Value::kACC/*lhs_linkage*/,
                                  rhs->AsPairExpression(), ast);
            } else {
                GenerateOperation(op, value_type, 0/*lhs_index*/, Value::kACC/*lhs_linkage*/,
                                  rval.type, rval.index, Value::kStack/*rhs_linkage*/, ast);
            }
            StaArrayAt(value_type, primary, index, ast);
    
            if (rval_tmp) {
                current_fun_->StackFallback(rval.type, rval.index);
            }
        } return ResultWith(Value::kACC, value_type->id(), 0);

        case Operator::NOT_OPERATOR: {
            if (NeedInbox(value_type, rval.type)) {
                InboxIfNeeded(rval.type, rval.index, rval.linkage, value_type, ast);
            } else {
                LdaIfNeeded(rval.type, rval.index, rval.linkage, ast);
            }
            StaArrayAt(value_type, primary, index, ast);
        } return ResultWithVoid();

        default:
            NOREACHED();
            break;
    }

    return ResultWithError();
}

ASTVisitor::Result
BytecodeGenerator::GenerateVariableAssignment(const ASTString *name, Value lval, Scope *owns,
                                              Operator op, Expression *rhs, ASTNode *ast) {
    Value rval{Value::kError};
    if (!rhs->IsPairExpression()) {
        Result rv;
        VISIT_CHECK(rhs);
        rval.linkage = static_cast<Value::Linkage>(rv.kind);
        rval.type = metadata_space_->type(rv.bundle.type);
        rval.index = rv.bundle.index;
    }

    if (!owns->is_file_scope()) {
        if (!HasGenerated(lval.ast) && !GenerateSymbolDependence(lval)) {
            return ResultWithError();
        }
    }
    if (ShouldCaptureVar(owns, lval)) {
        auto rv = CaptureVar(name->ToString(), owns, lval);
        lval.linkage = static_cast<Value::Linkage>(rv.kind);
        lval.type = metadata_space_->type(rv.bundle.type);
        lval.index = rv.bundle.index;
    }

    switch (op.kind) {
        case Operator::kAdd:
        case Operator::kSub:
            if (rhs->IsPairExpression()) {
                GenerateOperation(op, lval.type, lval.index, lval.linkage, rhs->AsPairExpression(),
                                  ast);
            } else {
                GenerateOperation(op, lval.type, lval.index, lval.linkage, rval.type, rval.index,
                                  rval.linkage, ast);
            }
            break;
        case Operator::NOT_OPERATOR:
            if (NeedInbox(lval.type, rval.type)) {
                InboxIfNeeded(rval.type, rval.index, rval.linkage, lval.type, ast);
            }
            break;
        default:
            NOREACHED();
            break;
    }
    StaIfNeeded(lval.type, lval.index, lval.linkage, ast);
    return ResultWith(lval);
}

ASTVisitor::Result
BytecodeGenerator::GenerateStoreProperty(const Class *clazz, int index, Value::Linkage linkage,
                                         DotExpression *ast) {
    DCHECK(clazz->is_reference());
    const Field *field = DCHECK_NOTNULL(metadata_space_->FindClassFieldOrNull(clazz,
                                                                              ast->rhs()->data()));
    bool has_reserve_self = true;
    int self = 0;
    if (linkage == Value::kStack) {
        self = index;
        has_reserve_self = false;
    } else {
        self = current_fun_->stack()->ReserveRef();
        MoveToStackIfNeeded(clazz, index, linkage, self, ast);
    }
    StaProperty(field->type(), self, field->offset(), ast);
    
    if (has_reserve_self) {
        current_fun_->stack()->FallbackRef(self);
    }
    return ResultWithVoid();
}

ASTVisitor::Result
BytecodeGenerator::GenerateLoadProperty(const Class *clazz, int index, Value::Linkage linkage,
                                        DotExpression *ast) {
    if (clazz->IsNumber()) {
        return GeneratePropertyCast(clazz, index, linkage, ast);
    }

    DCHECK(clazz->is_reference());
    const Field *field = DCHECK_NOTNULL(metadata_space_->FindClassFieldOrNull(clazz,
                                                                              ast->rhs()->data()));
    if (field) {
        bool has_reserve_self = true;
        int self = 0;
        if (linkage == Value::kStack) {
            self = index;
            has_reserve_self = false;
        } else {
            self = current_fun_->stack()->ReserveRef();
            MoveToStackIfNeeded(clazz, index, linkage, self, ast);
        }
        LdaProperty(field->type(), self, field->offset(), ast);

        if (has_reserve_self) {
            current_fun_->stack()->FallbackRef(self);
        }
        return ResultWith(Value::kACC, field->type()->id(), 0);
    }

    // load method
    const Method *method =
        DCHECK_NOTNULL(metadata_space_->FindClassMethodOrNull(clazz, ast->rhs()->data()));
    (void)method; // XXX

    // Method function is not generated yet
    std::string name = std::string(clazz->name()) + "::" + ast->rhs()->ToString();
    Value value = EnsureFindValue(name);
    DCHECK_EQ(Value::kGlobal, value.linkage);
    return ResultWith(value);
}

ASTVisitor::Result
BytecodeGenerator::GeneratePropertyCast(const Class *clazz, int index, Value::Linkage linkage,
                                        DotExpression *ast) {
    StackSpaceScope stack_scope(current_fun_->stack());
    int offset = index;
    if (linkage != Value::kStack) {
        int dest = current_fun_->StackReserve(clazz);
        MoveToStackIfNeeded(clazz, index, linkage, dest, ast);
        offset = dest;
    }
    offset = GetStackOffset(offset);

    switch (static_cast<BuiltinType>(clazz->id())) {
        case kType_u8:
        case kType_i8:
            if (!::strncmp("toU8", ast->rhs()->data(), ast->rhs()->size())) {
                return ResultWith(linkage, kType_i8, index);
            } else if (!::strncmp("toI8", ast->rhs()->data(), ast->rhs()->size())) {
                return ResultWith(linkage, kType_u8, index);
            } else if (!::strncmp("toU16", ast->rhs()->data(), ast->rhs()->size())) {
                EMIT(ast, Add<kZeroExtend8To32>(offset));
                return ResultWith(Value::kACC, kType_u16, 0);
            } else if (!::strncmp("toI16", ast->rhs()->data(), ast->rhs()->size())) {
                EMIT(ast, Add<kSignExtend8To32>(offset));
                return ResultWith(Value::kACC, kType_i16, 0);
            } else if (!::strncmp("toU32", ast->rhs()->data(), ast->rhs()->size())) {
                EMIT(ast, Add<kZeroExtend8To32>(offset));
                return ResultWith(Value::kACC, kType_u32, 0);
            } else if (!::strncmp("toI32", ast->rhs()->data(), ast->rhs()->size())) {
                EMIT(ast, Add<kSignExtend8To32>(offset));
                return ResultWith(Value::kACC, kType_i32, 0);
            } else if (!::strncmp("toUInt", ast->rhs()->data(), ast->rhs()->size())) {
                EMIT(ast, Add<kZeroExtend8To32>(offset));
                return ResultWith(Value::kACC, kType_uint, 0);
            } else if (!::strncmp("toInt", ast->rhs()->data(), ast->rhs()->size())) {
                EMIT(ast, Add<kSignExtend8To32>(offset));
                return ResultWith(Value::kACC, kType_int, offset);
            } else if (!::strncmp("toU64", ast->rhs()->data(), ast->rhs()->size())) {
                EMIT(ast, Add<kZeroExtend8To32>(offset));
                offset = GetStackOffset(current_fun_->stack()->Reserve(4));
                EMIT(ast, Add<kStar32>(offset));
                EMIT(ast, Add<kZeroExtend32To64>(offset));
                return ResultWith(Value::kACC, kType_u64, 0);
            } else if (!::strncmp("toI64", ast->rhs()->data(), ast->rhs()->size())) {
                EMIT(ast, Add<kSignExtend8To32>(offset));
                offset = GetStackOffset(current_fun_->stack()->Reserve(4));
                EMIT(ast, Add<kStar32>(offset));
                EMIT(ast, Add<kSignExtend32To64>(offset));
                return ResultWith(Value::kACC, kType_i64, 0);
            } else if (!::strncmp("toF32", ast->rhs()->data(), ast->rhs()->size())) {
                EMIT(ast, Add<kSignExtend8To32>(offset));
                offset = GetStackOffset(current_fun_->stack()->Reserve(4));
                EMIT(ast, Add<kStar32>(offset));
                EMIT(ast, Add<kI32ToF32>(offset));
                return ResultWith(Value::kACC, kType_f32, 0);
            } else if (!::strncmp("toF64", ast->rhs()->data(), ast->rhs()->size())) {
                EMIT(ast, Add<kSignExtend8To32>(offset));
                offset = GetStackOffset(current_fun_->stack()->Reserve(4));
                EMIT(ast, Add<kStar32>(offset));
                EMIT(ast, Add<kI32ToF64>(offset));
                return ResultWith(Value::kACC, kType_f64, 0);
            } else {
                NOREACHED();
            }
            break;
        case kType_u16:
        case kType_i16:
            if (!::strncmp("toU8", ast->rhs()->data(), ast->rhs()->size())) {
                EMIT(ast, Add<kTruncate32To8>(offset));
                return ResultWith(Value::kACC, kType_u8, 0);
            } else if (!::strncmp("toI8", ast->rhs()->data(), ast->rhs()->size())) {
                EMIT(ast, Add<kTruncate32To8>(offset));
                return ResultWith(Value::kACC, kType_i8, 0);
            } else if (!::strncmp("toU16", ast->rhs()->data(), ast->rhs()->size())) {
                return ResultWith(linkage, kType_u16, index);
            } else if (!::strncmp("toI16", ast->rhs()->data(), ast->rhs()->size())) {
                return ResultWith(linkage, kType_i16, index);
            } else if (!::strncmp("toU32", ast->rhs()->data(), ast->rhs()->size())) {
                EMIT(ast, Add<kZeroExtend16To32>(offset));
                return ResultWith(Value::kACC, kType_u32, 0);
            } else if (!::strncmp("toI32", ast->rhs()->data(), ast->rhs()->size())) {
                EMIT(ast, Add<kSignExtend16To32>(offset));
                return ResultWith(Value::kACC, kType_i32, 0);
            } else if (!::strncmp("toUInt", ast->rhs()->data(), ast->rhs()->size())) {
                EMIT(ast, Add<kZeroExtend16To32>(offset));
                return ResultWith(Value::kACC, kType_uint, 0);
            } else if (!::strncmp("toInt", ast->rhs()->data(), ast->rhs()->size())) {
                EMIT(ast, Add<kSignExtend16To32>(offset));
                return ResultWith(Value::kACC, kType_int, offset);
            } else if (!::strncmp("toU64", ast->rhs()->data(), ast->rhs()->size())) {
                EMIT(ast, Add<kZeroExtend16To32>(offset));
                offset = GetStackOffset(current_fun_->stack()->Reserve(4));
                EMIT(ast, Add<kStar32>(offset));
                EMIT(ast, Add<kZeroExtend32To64>(offset));
                return ResultWith(Value::kACC, kType_u64, 0);
            } else if (!::strncmp("toI64", ast->rhs()->data(), ast->rhs()->size())) {
                EMIT(ast, Add<kSignExtend16To32>(offset));
                offset = GetStackOffset(current_fun_->stack()->Reserve(4));
                EMIT(ast, Add<kStar32>(offset));
                EMIT(ast, Add<kSignExtend32To64>(offset));
                return ResultWith(Value::kACC, kType_i64, 0);
            } else if (!::strncmp("toF32", ast->rhs()->data(), ast->rhs()->size())) {
                EMIT(ast, Add<kSignExtend16To32>(offset));
                offset = GetStackOffset(current_fun_->stack()->Reserve(4));
                EMIT(ast, Add<kStar32>(offset));
                EMIT(ast, Add<kI32ToF32>(offset));
                return ResultWith(Value::kACC, kType_f32, 0);
            } else if (!::strncmp("toF64", ast->rhs()->data(), ast->rhs()->size())) {
                EMIT(ast, Add<kSignExtend16To32>(offset));
                offset = GetStackOffset(current_fun_->stack()->Reserve(4));
                EMIT(ast, Add<kStar32>(offset));
                EMIT(ast, Add<kI32ToF64>(offset));
                return ResultWith(Value::kACC, kType_f64, 0);
            } else {
                NOREACHED();
            }
            break;
        case kType_u32:
        case kType_i32:
        case kType_uint:
        case kType_int:
            if (!::strncmp("toU8", ast->rhs()->data(), ast->rhs()->size())) {
                EMIT(ast, Add<kTruncate32To8>(offset));
                return ResultWith(Value::kACC, kType_u8, 0);
            } else if (!::strncmp("toI8", ast->rhs()->data(), ast->rhs()->size())) {
                EMIT(ast, Add<kTruncate32To8>(offset));
                return ResultWith(Value::kACC, kType_i8, 0);
            } else if (!::strncmp("toU16", ast->rhs()->data(), ast->rhs()->size())) {
                EMIT(ast, Add<kZeroExtend16To32>(offset));
                return ResultWith(Value::kACC, kType_u16, 0);
            } else if (!::strncmp("toI16", ast->rhs()->data(), ast->rhs()->size())) {
                EMIT(ast, Add<kSignExtend16To32>(offset));
                return ResultWith(Value::kACC, kType_i16, 0);
            } else if (!::strncmp("toU32", ast->rhs()->data(), ast->rhs()->size())) {
                return ResultWith(linkage, kType_u32, index);
            } else if (!::strncmp("toI32", ast->rhs()->data(), ast->rhs()->size())) {
                return ResultWith(linkage, kType_i32, index);
            } else if (!::strncmp("toUInt", ast->rhs()->data(), ast->rhs()->size())) {
                return ResultWith(linkage, kType_uint, index);
            } else if (!::strncmp("toInt", ast->rhs()->data(), ast->rhs()->size())) {
                return ResultWith(linkage, kType_int, index);
            } else if (!::strncmp("toU64", ast->rhs()->data(), ast->rhs()->size())) {
                EMIT(ast, Add<kZeroExtend32To64>(offset));
                return ResultWith(Value::kACC, kType_u64, 0);
            } else if (!::strncmp("toI64", ast->rhs()->data(), ast->rhs()->size())) {
                EMIT(ast, Add<kSignExtend32To64>(offset));
                return ResultWith(Value::kACC, kType_i64, 0);
            } else if (!::strncmp("toF32", ast->rhs()->data(), ast->rhs()->size())) {
                if (clazz->IsUnsignedIntegral()) {
                    EMIT(ast, Add<kU32ToF32>(offset));
                } else {
                    EMIT(ast, Add<kI32ToF32>(offset));
                }
                return ResultWith(Value::kACC, kType_f32, 0);
            } else if (!::strncmp("toF64", ast->rhs()->data(), ast->rhs()->size())) {
                if (clazz->IsUnsignedIntegral()) {
                    EMIT(ast, Add<kU32ToF64>(offset));
                } else {
                    EMIT(ast, Add<kI32ToF64>(offset));
                }
                return ResultWith(Value::kACC, kType_f64, 0);
            } else {
                NOREACHED();
            }
            break;
        case kType_i64:
        case kType_u64:
            if (!::strncmp("toU8", ast->rhs()->data(), ast->rhs()->size())) {
                EMIT(ast, Add<kTruncate64To32>(offset));
                offset = GetStackOffset(current_fun_->stack()->Reserve(4));
                EMIT(ast, Add<kStar32>(offset));
                EMIT(ast, Add<kTruncate32To8>(offset));
                return ResultWith(Value::kACC, kType_u8, 0);
            } else if (!::strncmp("toI8", ast->rhs()->data(), ast->rhs()->size())) {
                EMIT(ast, Add<kTruncate64To32>(offset));
                offset = GetStackOffset(current_fun_->stack()->Reserve(4));
                EMIT(ast, Add<kStar32>(offset));
                EMIT(ast, Add<kTruncate32To8>(offset));
                return ResultWith(Value::kACC, kType_i8, 0);
            } else if (!::strncmp("toU16", ast->rhs()->data(), ast->rhs()->size())) {
                EMIT(ast, Add<kTruncate64To32>(offset));
                offset = GetStackOffset(current_fun_->stack()->Reserve(4));
                EMIT(ast, Add<kStar32>(offset));
                EMIT(ast, Add<kTruncate32To16>(offset));
                return ResultWith(Value::kACC, kType_u16, 0);
            } else if (!::strncmp("toI16", ast->rhs()->data(), ast->rhs()->size())) {
                EMIT(ast, Add<kTruncate64To32>(offset));
                offset = GetStackOffset(current_fun_->stack()->Reserve(4));
                EMIT(ast, Add<kStar32>(offset));
                EMIT(ast, Add<kTruncate32To16>(offset));
                return ResultWith(Value::kACC, kType_i16, 0);
            } else if (!::strncmp("toU32", ast->rhs()->data(), ast->rhs()->size())) {
                EMIT(ast, Add<kTruncate64To32>(offset));
                return ResultWith(Value::kACC, kType_u32, 0);
            } else if (!::strncmp("toI32", ast->rhs()->data(), ast->rhs()->size())) {
                EMIT(ast, Add<kTruncate64To32>(offset));
                return ResultWith(Value::kACC, kType_i32, 0);
            } else if (!::strncmp("toUInt", ast->rhs()->data(), ast->rhs()->size())) {
                EMIT(ast, Add<kTruncate64To32>(offset));
                return ResultWith(Value::kACC, kType_uint, 0);
            } else if (!::strncmp("toInt", ast->rhs()->data(), ast->rhs()->size())) {
                EMIT(ast, Add<kTruncate64To32>(offset));
                return ResultWith(Value::kACC, kType_int, 0);
            } else if (!::strncmp("toU64", ast->rhs()->data(), ast->rhs()->size())) {
                return ResultWith(linkage, kType_u64, index);
            } else if (!::strncmp("toI64", ast->rhs()->data(), ast->rhs()->size())) {
                return ResultWith(linkage, kType_i64, index);
            } else if (!::strncmp("toF32", ast->rhs()->data(), ast->rhs()->size())) {
                if (clazz->IsUnsignedIntegral()) {
                    EMIT(ast, Add<kU64ToF32>(offset));
                } else {
                    EMIT(ast, Add<kI64ToF32>(offset));
                }
                return ResultWith(Value::kACC, kType_f32, 0);
            } else if (!::strncmp("toF64", ast->rhs()->data(), ast->rhs()->size())) {
                if (clazz->IsUnsignedIntegral()) {
                    EMIT(ast, Add<kU64ToF64>(offset));
                } else {
                    EMIT(ast, Add<kI64ToF64>(offset));
                }
                return ResultWith(Value::kACC, kType_f64, 0);
            } else {
                NOREACHED();
            }
            break;
        case kType_f32:
            if (!::strncmp("toU8", ast->rhs()->data(), ast->rhs()->size())) {
                EMIT(ast, Add<kF32ToU32>(offset));
                offset = GetStackOffset(current_fun_->stack()->Reserve(4));
                EMIT(ast, Add<kStar32>(offset));
                EMIT(ast, Add<kTruncate32To8>(offset));
                return ResultWith(Value::kACC, kType_u8, 0);
            } else if (!::strncmp("toI8", ast->rhs()->data(), ast->rhs()->size())) {
                EMIT(ast, Add<kF32ToI32>(offset));
                offset = GetStackOffset(current_fun_->stack()->Reserve(4));
                EMIT(ast, Add<kStar32>(offset));
                EMIT(ast, Add<kTruncate32To8>(offset));
                return ResultWith(Value::kACC, kType_i8, 0);
            } else if (!::strncmp("toU16", ast->rhs()->data(), ast->rhs()->size())) {
                EMIT(ast, Add<kF32ToU32>(offset));
                offset = GetStackOffset(current_fun_->stack()->Reserve(4));
                EMIT(ast, Add<kStar32>(offset));
                EMIT(ast, Add<kTruncate32To16>(offset));
                return ResultWith(Value::kACC, kType_u16, 0);
            } else if (!::strncmp("toI16", ast->rhs()->data(), ast->rhs()->size())) {
                EMIT(ast, Add<kF32ToI32>(offset));
                offset = GetStackOffset(current_fun_->stack()->Reserve(4));
                EMIT(ast, Add<kStar32>(offset));
                EMIT(ast, Add<kTruncate32To16>(offset));
                return ResultWith(Value::kACC, kType_i16, 0);
            } else if (!::strncmp("toU32", ast->rhs()->data(), ast->rhs()->size())) {
                EMIT(ast, Add<kF32ToU32>(offset));
                return ResultWith(Value::kACC, kType_u32, 0);
            } else if (!::strncmp("toI32", ast->rhs()->data(), ast->rhs()->size())) {
                EMIT(ast, Add<kF32ToI32>(offset));
                return ResultWith(Value::kACC, kType_i32, 0);
            } else if (!::strncmp("toUInt", ast->rhs()->data(), ast->rhs()->size())) {
                EMIT(ast, Add<kF32ToU32>(offset));
                return ResultWith(Value::kACC, kType_uint, 0);
            } else if (!::strncmp("toInt", ast->rhs()->data(), ast->rhs()->size())) {
                EMIT(ast, Add<kF32ToI32>(offset));
                return ResultWith(Value::kACC, kType_int, 0);
            } else if (!::strncmp("toU64", ast->rhs()->data(), ast->rhs()->size())) {
                EMIT(ast, Add<kF32ToU64>(offset));
                return ResultWith(Value::kACC, kType_u64, 0);
            } else if (!::strncmp("toI64", ast->rhs()->data(), ast->rhs()->size())) {
                EMIT(ast, Add<kF32ToI64>(offset));
                return ResultWith(Value::kACC, kType_i64, 0);
            } else if (!::strncmp("toF32", ast->rhs()->data(), ast->rhs()->size())) {
                return ResultWith(linkage, kType_f32, index);
            } else if (!::strncmp("toF64", ast->rhs()->data(), ast->rhs()->size())) {
                EMIT(ast, Add<kF32ToF64>(offset));
                return ResultWith(Value::kACC, kType_f64, 0);
            } else {
                NOREACHED();
            }
            break;
        case kType_f64:
            if (!::strncmp("toU8", ast->rhs()->data(), ast->rhs()->size())) {
                EMIT(ast, Add<kF64ToU32>(offset));
                offset = GetStackOffset(current_fun_->stack()->Reserve(4));
                EMIT(ast, Add<kStar32>(offset));
                EMIT(ast, Add<kTruncate32To8>(offset));
                return ResultWith(Value::kACC, kType_u8, 0);
            } else if (!::strncmp("toI8", ast->rhs()->data(), ast->rhs()->size())) {
                EMIT(ast, Add<kF64ToI32>(offset));
                offset = GetStackOffset(current_fun_->stack()->Reserve(4));
                EMIT(ast, Add<kStar32>(offset));
                EMIT(ast, Add<kTruncate32To8>(offset));
                return ResultWith(Value::kACC, kType_i8, 0);
            } else if (!::strncmp("toU16", ast->rhs()->data(), ast->rhs()->size())) {
                EMIT(ast, Add<kF64ToU32>(offset));
                offset = GetStackOffset(current_fun_->stack()->Reserve(4));
                EMIT(ast, Add<kStar32>(offset));
                EMIT(ast, Add<kTruncate32To16>(offset));
                return ResultWith(Value::kACC, kType_u16, 0);
            } else if (!::strncmp("toI16", ast->rhs()->data(), ast->rhs()->size())) {
                EMIT(ast, Add<kF64ToI32>(offset));
                offset = GetStackOffset(current_fun_->stack()->Reserve(4));
                EMIT(ast, Add<kStar32>(offset));
                EMIT(ast, Add<kTruncate32To16>(offset));
                return ResultWith(Value::kACC, kType_i16, 0);
            } else if (!::strncmp("toU32", ast->rhs()->data(), ast->rhs()->size())) {
                EMIT(ast, Add<kF64ToU32>(offset));
                return ResultWith(Value::kACC, kType_u32, 0);
            } else if (!::strncmp("toI32", ast->rhs()->data(), ast->rhs()->size())) {
                EMIT(ast, Add<kF64ToI32>(offset));
                return ResultWith(Value::kACC, kType_i32, 0);
            } else if (!::strncmp("toUInt", ast->rhs()->data(), ast->rhs()->size())) {
                EMIT(ast, Add<kF64ToU32>(offset));
                return ResultWith(Value::kACC, kType_uint, 0);
            } else if (!::strncmp("toInt", ast->rhs()->data(), ast->rhs()->size())) {
                EMIT(ast, Add<kF64ToI32>(offset));
                return ResultWith(Value::kACC, kType_int, 0);
            } else if (!::strncmp("toU64", ast->rhs()->data(), ast->rhs()->size())) {
                EMIT(ast, Add<kF64ToU64>(offset));
                return ResultWith(Value::kACC, kType_u64, 0);
            } else if (!::strncmp("toI64", ast->rhs()->data(), ast->rhs()->size())) {
                EMIT(ast, Add<kF64ToI64>(offset));
                return ResultWith(Value::kACC, kType_i64, 0);
            } else if (!::strncmp("toF32", ast->rhs()->data(), ast->rhs()->size())) {
                EMIT(ast, Add<kF64ToF32>(offset));
                return ResultWith(Value::kACC, kType_f32, 0);
            } else if (!::strncmp("toF64", ast->rhs()->data(), ast->rhs()->size())) {
                return ResultWith(linkage, kType_f64, index);
            } else {
                NOREACHED();
            }
            break;
        default:
            NOREACHED();
            break;
    }
    return ResultWithError();
}

int BytecodeGenerator::LinkGlobalVariable(VariableDeclaration *var) {
    if (!var->initializer()) {
        return global_space_.Reserve(var->type()->GetReferenceSize());
    }
    
    symbol_trace_.insert(var);
    switch (var->initializer()->kind()) {
        case ASTNode::kBoolLiteral:
            return global_space_.AppendI32(var->initializer()->AsBoolLiteral()->value());
        case ASTNode::kI8Literal:
            return global_space_.AppendI32(var->initializer()->AsI8Literal()->value());
        case ASTNode::kU8Literal:
            return global_space_.AppendU32(var->initializer()->AsU8Literal()->value());
        case ASTNode::kI16Literal:
            return global_space_.AppendI32(var->initializer()->AsI16Literal()->value());
        case ASTNode::kU16Literal:
            return global_space_.AppendU32(var->initializer()->AsU16Literal()->value());
        case ASTNode::kI32Literal:
            return global_space_.AppendI32(var->initializer()->AsI32Literal()->value());
        case ASTNode::kU32Literal:
            return global_space_.AppendU32(var->initializer()->AsU32Literal()->value());
        case ASTNode::kIntLiteral:
            return global_space_.AppendI32(var->initializer()->AsIntLiteral()->value());
        case ASTNode::kUIntLiteral:
            return global_space_.AppendU32(var->initializer()->AsUIntLiteral()->value());
        case ASTNode::kI64Literal:
            return global_space_.AppendI64(var->initializer()->AsI64Literal()->value());
        case ASTNode::kU64Literal:
            return global_space_.AppendU64(var->initializer()->AsU64Literal()->value());
        case ASTNode::kF32Literal:
            return global_space_.AppendF32(var->initializer()->AsF32Literal()->value());
        case ASTNode::kF64Literal:
            return global_space_.AppendF64(var->initializer()->AsF64Literal()->value());
        case ASTNode::kNilLiteral:
            return global_space_.AppendAny(nullptr);
        case ASTNode::kStringLiteral: {
            auto literal = var->initializer()->AsStringLiteral();
            String *s = nullptr;
            if (literal->value()->empty()) {
                s = isolate_->factory()->empty_string();
            } else {
                s = Machine::This()->NewUtf8String(literal->value()->data(),
                                                   literal->value()->size(), Heap::kOld);
            }
            return global_space_.AppendAny(s);
        }
        default:
            symbol_trace_.erase(var);
            return global_space_.Reserve(var->type()->GetReferenceSize());
    }
}

void BytecodeGenerator::GenerateOperation(const Operator op,
                                          const Class *lhs_type,
                                          int lhs_index,
                                          Value::Linkage lhs_linkage,
                                          const Class *rhs_type,
                                          int rhs_index,
                                          Value::Linkage rhs_linkage,
                                          ASTNode *ast) {
    bool has_reserve_lhs = false;
    int lhs = lhs_index;
    if (lhs_linkage != Value::kStack) {
        has_reserve_lhs = true;
        lhs = current_fun_->StackReserve(lhs_type);
        MoveToStackIfNeeded(lhs_type, lhs_index, lhs_linkage, lhs, ast);
    }
    
    bool has_reserve_rhs = false;
    int rhs = rhs_index;
    if (rhs_linkage != Value::kStack) {
        has_reserve_rhs = true;
        rhs = current_fun_->StackReserve(rhs_type);
        MoveToStackIfNeeded(rhs_type, rhs_index, rhs_linkage, rhs, ast);
    }

    if (lhs_type->IsArray()) {
        if (op.kind == Operator::kSub) {
            GenerateArrayMinus(lhs_type, lhs, Value::kStack/*lhs_linkage*/, rhs_type, rhs,
                               Value::kStack/*lhs_linkage*/, ast);
        } else {
            NOREACHED();
        }
    } else {
        GenerateOperation(op, lhs_type, lhs, rhs, ast);
    }
    
    if (has_reserve_rhs) {
        current_fun_->StackFallback(rhs_type, rhs);
    }
    if (has_reserve_lhs) {
        current_fun_->StackFallback(lhs_type, lhs);
    }
}

bool BytecodeGenerator::GenerateUnaryOperands(OperandContext *receiver, Expression *ast) {
    Result rv;
    if (rv = ast->Accept(this); rv.kind == Value::kError) {
        return false;
    }
    receiver->lhs_type = metadata_space_->type(rv.bundle.type);
    receiver->lhs = rv.bundle.index;
    receiver->lhs_tmp = false;
    if (rv.kind != Value::kStack) {
        receiver->lhs = current_fun_->StackReserve(receiver->lhs_type);
        receiver->lhs_tmp = true;
        MoveToStackIfNeeded(receiver->lhs_type, rv.bundle.index,
                            static_cast<Value::Linkage>(rv.kind), receiver->lhs, ast);
    }
    receiver->rhs_tmp = false;
    receiver->rhs_pair = false;
    receiver->rhs = 0;
    receiver->rhs_type = nullptr;
    return true;
}

bool BytecodeGenerator::GenerateBinaryOperands(OperandContext *receiver, Expression *lhs,
                                               Expression *rhs) {
    Result rv;
    if (rv = lhs->Accept(this); rv.kind == Value::kError) {
        return false;
    }
    receiver->lhs_type = metadata_space_->type(rv.bundle.type);
    receiver->lhs = rv.bundle.index;
    receiver->lhs_tmp = false;
    if (rv.kind != Value::kStack) {
        receiver->lhs = current_fun_->StackReserve(receiver->lhs_type);
        receiver->lhs_tmp = true;
        MoveToStackIfNeeded(receiver->lhs_type, rv.bundle.index,
                            static_cast<Value::Linkage>(rv.kind), receiver->lhs, lhs);
    }
    if (rhs->IsPairExpression()) {
        receiver->rhs_pair = true;
        return true;
    }
    receiver->rhs_pair = false;
    if (rv = rhs->Accept(this); rv.kind == Value::kError) {
        return false;
    }
    receiver->rhs_type = metadata_space_->type(rv.bundle.type);
    receiver->rhs = rv.bundle.index;
    receiver->rhs_tmp = false;
    if (rv.kind != Value::kStack) {
        receiver->rhs_tmp = true;
        receiver->rhs = current_fun_->StackReserve(receiver->rhs_type);
        MoveToStackIfNeeded(receiver->rhs_type, rv.bundle.index,
                            static_cast<Value::Linkage>(rv.kind), receiver->rhs, rhs);
    }
    return true;
}

void BytecodeGenerator::CleanupOperands(OperandContext *receiver) {
    if (receiver->rhs_tmp) {
        current_fun_->StackFallback(receiver->rhs_type, receiver->rhs);
    }
    if (receiver->lhs_tmp) {
        current_fun_->StackFallback(receiver->lhs_type, receiver->lhs);
    }
}

void BytecodeGenerator::GenerateSend(const Class *clazz, int lhs, int rhs, ASTNode *ast) {
    int argument_offset = RoundUp(kPointerSize, kStackSizeGranularity) +
                          RoundUp(clazz->reference_size(), kStackSizeGranularity);
    Value value;
    if (clazz->is_reference()) {
        value = FindOrInsertExternalFunction("channel::sendPtr");
    } else {
        switch (clazz->reference_size()) {
            case 1:
            case 2:
            case 4:
                if (clazz->IsFloating()) {
                    value = FindOrInsertExternalFunction("channel::sendF32");
                } else {
                    value = FindOrInsertExternalFunction("channel::send32");
                }
                break;
            case 8:
                if (clazz->IsFloating()) {
                    value = FindOrInsertExternalFunction("channel::sendF64");
                } else {
                    value = FindOrInsertExternalFunction("channel::send64");
                }
                break;
            default:
                NOREACHED();
                break;
        }
    }
    LdaGlobal(metadata_space_->builtin_type(kType_closure), value.index, ast);
    current_fun_->EmitDirectlyCallFunction(ast, true/*native*/, 0/*slot*/,
                                           argument_offset);
    current_fun_->EmitYield(ast, YIELD_PROPOSE);
}

bool BytecodeGenerator::GenerateOperation(const Operator op, const Class *type, int lhs_index,
                                          Value::Linkage lhs_linkage,
                                          PairExpression *rhs, ASTNode *ast) {
    if (type->IsArray()) {
        OperandContext receiver;
        bool ok;
        if (rhs->addition_key()) {
            ok = GenerateUnaryOperands(&receiver, rhs->value());
        } else {
            ok = GenerateBinaryOperands(&receiver, rhs->key(), rhs->value());
        }
        if (!ok) {
            return false;
        }
        switch (op.kind) {
            case Operator::kAdd:
                if (rhs->addition_key()) {
                    GenerateArrayAppend(type, lhs_index, lhs_linkage, receiver.lhs_type,
                                        receiver.lhs, Value::kStack/*lhs_linkage*/, ast);
                } else {
                    GenerateArrayPlus(type, lhs_index, lhs_linkage, receiver.lhs_type,
                                      receiver.lhs, Value::kStack, receiver.rhs_type, receiver.rhs,
                                      Value::kStack/*lhs_linkage*/, ast);
                }
                break;
            default:
                NOREACHED();
                break;
        }
        CleanupOperands(&receiver);
        return true;
    } else {
        TODO();
    }
    return false;
}

void BytecodeGenerator::GenerateArrayAppend(const Class *clazz,
                                            int lhs_index,
                                            Value::Linkage lhs_linkage,
                                            const Class *value_type,
                                            int value_index,
                                            Value::Linkage value_linkage,
                                            ASTNode *ast) {
    int argument_offset = kPointerSize; // array
    MoveToArgumentIfNeeded(clazz, lhs_index, lhs_linkage, argument_offset, ast);
    argument_offset += RoundUp(value_type->reference_size(), kStackSizeGranularity);
    MoveToArgumentIfNeeded(value_type, value_index, value_linkage, argument_offset, ast);
    const char *external_name = nullptr;
    switch (clazz->id()) {
        case kType_array:
            external_name = "array::append";
            break;
        case kType_array8:
            external_name = "array8::append";
            break;
        case kType_array16:
            external_name = "array16::append";
            break;
        case kType_array32:
            external_name = "array32::append";
            break;
        case kType_array64:
            external_name = "array64::append";
            break;
        default:
            NOREACHED();
            break;
    }
    Value fun = FindOrInsertExternalFunction(external_name);
    DCHECK_EQ(Value::kGlobal, fun.linkage);
    LdaGlobal(metadata_space_->type(kType_closure), fun.index, ast);
    current_fun_->EmitDirectlyCallFunction(ast, true/*native*/, 0/*slot*/, argument_offset);
}

void BytecodeGenerator::GenerateArrayPlus(const Class *clazz,
                                          int lhs_index,
                                          Value::Linkage lhs_linkage,
                                          const Class *key_type,
                                          int key_index,
                                          Value::Linkage key_linkage,
                                          const Class *value_type,
                                          int value_index,
                                          Value::Linkage value_linkage,
                                          ASTNode *ast) {
    int argument_offset = kPointerSize; // array
    MoveToArgumentIfNeeded(clazz, lhs_index, lhs_linkage, argument_offset, ast);
    argument_offset += RoundUp(key_type->reference_size(), kStackSizeGranularity);
    MoveToArgumentIfNeeded(key_type, key_index, key_linkage, argument_offset, ast);
    argument_offset += RoundUp(value_type->reference_size(), kStackSizeGranularity);
    MoveToArgumentIfNeeded(value_type, value_index, value_linkage, argument_offset, ast);

    const char *external_name = nullptr;
    switch (clazz->id()) {
        case kType_array:
            external_name = "array::plus";
            break;
        case kType_array8:
            external_name = "array8::plus";
            break;
        case kType_array16:
            external_name = "array16::plus";
            break;
        case kType_array32:
            external_name = "array32::plus";
            break;
        case kType_array64:
            external_name = "array64::plus";
            break;
        default:
            NOREACHED();
            break;
    }
    Value fun = FindOrInsertExternalFunction(external_name);
    DCHECK_EQ(Value::kGlobal, fun.linkage);
    LdaGlobal(metadata_space_->type(kType_closure), fun.index, ast);
    current_fun_->EmitDirectlyCallFunction(ast, true/*native*/, 0/*slot*/, argument_offset);
}

void BytecodeGenerator::GenerateArrayMinus(const Class *clazz,
                                           int lhs_index,
                                           Value::Linkage lhs_linkage,
                                           const Class *key_type,
                                           int key_index,
                                           Value::Linkage key_linkage,
                                           ASTNode *ast) {
    int argument_offset = kPointerSize; // array
    MoveToArgumentIfNeeded(clazz, lhs_index, lhs_linkage, argument_offset, ast);
    argument_offset += RoundUp(key_type->reference_size(), kStackSizeGranularity);
    MoveToArgumentIfNeeded(key_type, key_index, key_linkage, argument_offset, ast);

    const char *external_name = nullptr;
    switch (clazz->id()) {
        case kType_array:
            external_name = "array::minus";
            break;
        case kType_array8:
            external_name = "array8::minus";
            break;
        case kType_array16:
            external_name = "array16::minus";
            break;
        case kType_array32:
            external_name = "array32::minus";
            break;
        case kType_array64:
            external_name = "array64::minus";
            break;
        default:
            NOREACHED();
            break;
    }
    Value fun = FindOrInsertExternalFunction(external_name);
    DCHECK_EQ(Value::kGlobal, fun.linkage);
    LdaGlobal(metadata_space_->type(kType_closure), fun.index, ast);
    current_fun_->EmitDirectlyCallFunction(ast, true/*native*/, 0/*slot*/, argument_offset);
}

void BytecodeGenerator::GenerateOperation(const Operator op, const Class *clazz, int lhs, int rhs,
                                          ASTNode *ast) {
    const int loff = GetStackOffset(lhs);
    const int roff = GetStackOffset(rhs);
    switch (static_cast<BuiltinType>(clazz->id())) {
        case kType_i8:
        case kType_u8:
        case kType_i16:
        case kType_u16:
        case kType_i32:
        case kType_u32:
        case kType_int:
        case kType_uint:
            switch (op.kind) {
                case Operator::kAdd:
                    EMIT(ast, Add<kAdd32>(loff, roff));
                    break;
                case Operator::kSub:
                    EMIT(ast, Add<kSub32>(loff, roff));
                    break;
                case Operator::kMul:
                    if (clazz->IsUnsignedIntegral()) {
                        EMIT(ast, Add<kMul32>(loff, roff));
                    } else {
                        EMIT(ast, Add<kIMul32>(loff, roff));
                    }
                    break;
                case Operator::kDiv:
                    if (clazz->IsUnsignedIntegral()) {
                        EMIT(ast, Add<kDiv32>(loff, roff));
                    } else {
                        EMIT(ast, Add<kIDiv32>(loff, roff));
                    }
                    break;
                case Operator::kMod:
                    EMIT(ast, Add<kMod32>(loff, roff));
                    break;
                case Operator::kBitwiseOr:
                    EMIT(ast, Add<kBitwiseOr32>(loff, roff));
                    break;
                case Operator::kBitwiseAnd:
                    EMIT(ast, Add<kBitwiseAnd32>(loff, roff));
                    break;
                case Operator::kBitwiseXor:
                    EMIT(ast, Add<kBitwiseXor32>(loff, roff));
                    break;
                case Operator::kBitwiseShl:
                    EMIT(ast, Add<kBitwiseShl32>(loff, roff));
                    break;
                case Operator::kBitwiseShr:
                    if (clazz->IsUnsignedIntegral()) {
                        EMIT(ast, Add<kBitwiseShr32>(loff, roff));
                    } else {
                        EMIT(ast, Add<kBitwiseLogicShr32>(loff, roff));
                    }
                    break;
                default:
                    NOREACHED();
                    break;
            }
            break;
        case kType_i64:
        case kType_u64:
            switch (op.kind) {
                case Operator::kAdd:
                    EMIT(ast, Add<kAdd64>(loff, roff));
                    break;
                case Operator::kSub:
                    EMIT(ast, Add<kSub64>(loff, roff));
                    break;
                case Operator::kMul:
                    if (clazz->IsUnsignedIntegral()) {
                        EMIT(ast, Add<kMul64>(loff, roff));
                    } else {
                        EMIT(ast, Add<kIMul64>(loff, roff));
                    }
                    break;
                case Operator::kDiv:
                    if (clazz->IsUnsignedIntegral()) {
                        EMIT(ast, Add<kDiv64>(loff, roff));
                    } else {
                        EMIT(ast, Add<kIDiv64>(loff, roff));
                    }
                    break;
                case Operator::kMod:
                    EMIT(ast, Add<kMod64>(loff, roff));
                    break;
                case Operator::kBitwiseOr:
                    EMIT(ast, Add<kBitwiseOr64>(loff, roff));
                    break;
                case Operator::kBitwiseAnd:
                    EMIT(ast, Add<kBitwiseAnd64>(loff, roff));
                    break;
                case Operator::kBitwiseXor:
                    EMIT(ast, Add<kBitwiseXor64>(loff, roff));
                    break;
                case Operator::kBitwiseShl:
                    EMIT(ast, Add<kBitwiseShl64>(loff, roff));
                    break;
                case Operator::kBitwiseShr:
                    if (clazz->IsUnsignedIntegral()) {
                        EMIT(ast, Add<kBitwiseShr64>(loff, roff));
                    } else {
                        EMIT(ast, Add<kBitwiseLogicShr64>(loff, roff));
                    }
                    break;
                default:
                    NOREACHED();
                    break;
            }
            break;
        case kType_f32:
            switch (op.kind) {
                case Operator::kAdd:
                    EMIT(ast, Add<kAddf32>(loff, roff));
                    break;
                case Operator::kSub:
                    EMIT(ast, Add<kSubf32>(loff, roff));
                    break;
                case Operator::kMul:
                    EMIT(ast, Add<kMulf32>(loff, roff));
                    break;
                case Operator::kDiv:
                    EMIT(ast, Add<kDivf32>(loff, roff));
                    break;
                default:
                    NOREACHED();
                    break;
            }
            break;
        case kType_f64:
            switch (op.kind) {
                case Operator::kAdd:
                    EMIT(ast, Add<kAddf64>(loff, roff));
                    break;
                case Operator::kSub:
                    EMIT(ast, Add<kSubf64>(loff, roff));
                    break;
                case Operator::kMul:
                    EMIT(ast, Add<kMulf64>(loff, roff));
                    break;
                case Operator::kDiv:
                    EMIT(ast, Add<kDivf64>(loff, roff));
                    break;
                default:
                    NOREACHED();
                    break;
            }
            break;
        default:
            NOREACHED();
            break;
    }
}

void BytecodeGenerator::GenerateComparation(const Class *clazz, Operator op, int lhs, int rhs,
                                            ASTNode *ast) {
    const int loff = GetStackOffset(lhs);
    const int roff = GetStackOffset(rhs);
    switch (static_cast<BuiltinType>(clazz->id())) {
        case kType_i8:
        case kType_u8:
        case kType_i16:
        case kType_u16:
        case kType_i32:
        case kType_u32:
        case kType_int:
        case kType_uint:
            switch (op.kind) {
                case Operator::kEqual:
                    EMIT(ast, Add<kTestEqual32>(loff, roff));
                    break;
                case Operator::kNotEqual:
                    EMIT(ast, Add<kTestNotEqual32>(loff, roff));
                    break;
                case Operator::kLess:
                    EMIT(ast, Add<kTestLessThan32>(loff, roff));
                    break;
                case Operator::kLessEqual:
                    EMIT(ast, Add<kTestLessThanOrEqual32>(loff, roff));
                    break;
                case Operator::kGreater:
                    EMIT(ast, Add<kTestGreaterThan32>(loff, roff));
                    break;
                case Operator::kGreaterEqual:
                    EMIT(ast, Add<kTestGreaterThanOrEqual32>(loff, roff));
                    break;
                default:
                    NOREACHED();
                    break;
            }
            break;
        case kType_i64:
        case kType_u64:
            switch (op.kind) {
                case Operator::kEqual:
                    EMIT(ast, Add<kTestEqual64>(loff, roff));
                    break;
                case Operator::kNotEqual:
                    EMIT(ast, Add<kTestNotEqual64>(loff, roff));
                    break;
                case Operator::kLess:
                    EMIT(ast, Add<kTestLessThan64>(loff, roff));
                    break;
                case Operator::kLessEqual:
                    EMIT(ast, Add<kTestLessThanOrEqual64>(loff, roff));
                    break;
                case Operator::kGreater:
                    EMIT(ast, Add<kTestGreaterThan64>(loff, roff));
                    break;
                case Operator::kGreaterEqual:
                    EMIT(ast, Add<kTestGreaterThanOrEqual64>(loff, roff));
                    break;
                default:
                    NOREACHED();
                    break;
            }
            break;
        case kType_f32:
            switch (op.kind) {
                case Operator::kEqual:
                    EMIT(ast, Add<kTestEqualf32>(loff, roff));
                    break;
                case Operator::kNotEqual:
                    EMIT(ast, Add<kTestNotEqualf32>(loff, roff));
                    break;
                case Operator::kLess:
                    EMIT(ast, Add<kTestLessThanf32>(loff, roff));
                    break;
                case Operator::kLessEqual:
                    EMIT(ast, Add<kTestLessThanOrEqualf32>(loff, roff));
                    break;
                case Operator::kGreater:
                    EMIT(ast, Add<kTestGreaterThanf32>(loff, roff));
                    break;
                case Operator::kGreaterEqual:
                    EMIT(ast, Add<kTestGreaterThanOrEqualf32>(loff, roff));
                    break;
                default:
                    NOREACHED();
                    break;
            }
            break;
        case kType_f64:
            switch (op.kind) {
                case Operator::kEqual:
                    EMIT(ast, Add<kTestEqualf64>(loff, roff));
                    break;
                case Operator::kNotEqual:
                    EMIT(ast, Add<kTestNotEqualf64>(loff, roff));
                    break;
                case Operator::kLess:
                    EMIT(ast, Add<kTestLessThanf64>(loff, roff));
                    break;
                case Operator::kLessEqual:
                    EMIT(ast, Add<kTestLessThanOrEqualf64>(loff, roff));
                    break;
                case Operator::kGreater:
                    EMIT(ast, Add<kTestGreaterThanf64>(loff, roff));
                    break;
                case Operator::kGreaterEqual:
                    EMIT(ast, Add<kTestGreaterThanOrEqualf64>(loff, roff));
                    break;
                default:
                    NOREACHED();
                    break;
            }
            break;
        case kType_string:
            switch (op.kind) {
                case Operator::kEqual:
                    EMIT(ast, Add<kTestStringEqual>(loff, roff));
                    break;
                case Operator::kNotEqual:
                    EMIT(ast, Add<kTestStringNotEqual>(loff, roff));
                    break;
                case Operator::kLess:
                    EMIT(ast, Add<kTestStringLessThan>(loff, roff));
                    break;
                case Operator::kLessEqual:
                    EMIT(ast, Add<kTestStringLessThanOrEqual>(loff, roff));
                    break;
                case Operator::kGreater:
                    EMIT(ast, Add<kTestStringGreaterThan>(loff, roff));
                    break;
                case Operator::kGreaterEqual:
                    EMIT(ast, Add<kTestStringLessThanOrEqual>(loff, roff));
                    break;
                default:
                    NOREACHED();
                    break;
            }
            break;
        default:
            switch (op.kind) {
                case Operator::kEqual:
                    EMIT(ast, Add<kTestPtrEqual>(loff, roff));
                    break;
                case Operator::kNotEqual:
                    EMIT(ast, Add<kTestPtrNotEqual>(loff, roff));
                    break;
                default:
                    NOREACHED();
                    break;
            }
            break;
    }
}

void BytecodeGenerator::ToStringIfNeeded(const Class *clazz, int index, Value::Linkage linkage,
                                         ASTNode *ast) {
    switch (static_cast<BuiltinType>(clazz->id())) {
#define DEFINE_TO_STRING(dest, name, ...) \
        case kType_##name: { \
            int args_size = RoundUp(clazz->reference_size(), kStackSizeGranularity); \
            MoveToArgumentIfNeeded(clazz, index, linkage, args_size, ast); \
            Value value = FindOrInsertExternalFunction("lang." #dest "::toString"); \
            LdaGlobal(value.type, value.index, ast); \
            current_fun_->EmitDirectlyCallFunction(ast, true, 0, args_size); \
        } break;
        DECLARE_BOX_NUMBER_TYPES(DEFINE_TO_STRING)
#undef DEFINE_TO_STRING
        case kType_string:
            LdaIfNeeded(clazz, index, linkage, ast);
            break;
        case kType_array:
        case kType_array8:
        case kType_array16:
        case kType_array32:
        case kType_array64: {
            MoveToArgumentIfNeeded(clazz, index, linkage, kPointerSize/*arg_offset*/, ast);
            Value value = EnsureFindValue("lang.Object::toString");
            DCHECK_EQ(Value::kGlobal, value.linkage);
            LdaGlobal(metadata_space_->builtin_type(kType_closure), value.index, ast);
            current_fun_->EmitDirectlyCallFunction(ast, value.flags & kAttrNative, 0/*slot*/,
                                                   kPointerSize/*arg_size*/);
        } break;
        default: {
            auto [owns, method] = metadata_space_->FindClassMethod(clazz, "toString");
            MoveToArgumentIfNeeded(clazz, index, linkage, kPointerSize/*arg_offset*/, ast);
            std::string name = std::string(owns->name()) + "::toString";
            Value value = EnsureFindValue(name);
            DCHECK_EQ(Value::kGlobal, value.linkage);
            LdaGlobal(metadata_space_->builtin_type(kType_closure), value.index, ast);
            current_fun_->EmitDirectlyCallFunction(ast, method->is_native(), 0/*slot*/,
                                                   kPointerSize/*arg_size*/);
        } break;
    }
}

void BytecodeGenerator::InboxIfNeeded(const Class *clazz, int index, Value::Linkage linkage,
                                      const Class *lval, ASTNode *ast) {
    switch (static_cast<BuiltinType>(clazz->id())) {
#define DEFINE_INBOX_PRIMITIVE(dest, name, ...) \
        case kType_##name: { \
            int args_size = RoundUp(clazz->reference_size(), kStackSizeGranularity); \
            MoveToArgumentIfNeeded(clazz, index, linkage, args_size, ast); \
            Value value = FindOrInsertExternalFunction("lang." #dest "::valueOf"); \
            LdaGlobal(value.type, value.index, ast); \
            current_fun_->EmitDirectlyCallFunction(ast, true, 0, args_size); \
        } break;
        DECLARE_BOX_NUMBER_TYPES(DEFINE_INBOX_PRIMITIVE)
#undef DEFINE_INBOX_PRIMITIVE
        default:
            break;
    }
}

BytecodeGenerator::Value BytecodeGenerator::FindOrInsertExternalFunction(const std::string &name) {
    auto iter = symbols_.find(name);
    if (iter != symbols_.end()) {
        DCHECK_EQ(Value::kGlobal, iter->second.linkage);
        return iter->second;
    }
    Closure *closure = DCHECK_NOTNULL(isolate_->FindExternalLinkageOrNull(name));
    int gidx = global_space_.AppendAny(closure);
    const Class *closure_type = metadata_space_->builtin_type(kType_closure);
    Value value{Value::kGlobal, closure_type, gidx};
    current_file_->Register(name, value);
    return value;
}

void BytecodeGenerator::MoveToStackIfNeeded(const Class *clazz, int index, Value::Linkage linkage,
                                            int dest, ASTNode *ast) {
    if (linkage == Value::kStack) {
        if (clazz->is_reference()) {
            EMIT(ast, Add<kMovePtr>(GetStackOffset(dest), GetStackOffset(index)));
            return;
        }
        switch (clazz->reference_size()) {
            case 1:
            case 2:
            case 4:
                EMIT(ast, Add<kMove32>(GetStackOffset(dest), GetStackOffset(index)));
                return;
            case 8:
                EMIT(ast, Add<kMove64>(GetStackOffset(dest), GetStackOffset(index)));
                return;
            default:
                NOREACHED();
                return;
        }
    }
    if (linkage != Value::kACC) {
        LdaIfNeeded(clazz, index, linkage, ast);
    }
    if (clazz->is_reference()) {
        EMIT(ast, Add<kStarPtr>(GetStackOffset(dest)));
        return;
    }
    switch (clazz->reference_size()) {
        case 1:
        case 2:
        case 4:
            if (clazz->id() == kType_f32) {
                EMIT(ast, Add<kStaf32>(GetStackOffset(dest)));
            } else {
                EMIT(ast, Add<kStar32>(GetStackOffset(dest)));
            }
            break;
        case 8:
            if (clazz->id() == kType_f64) {
                EMIT(ast, Add<kStaf64>(GetStackOffset(dest)));
            } else {
                EMIT(ast, Add<kStar64>(GetStackOffset(dest)));
            }
            break;
        default:
            NOREACHED();
            break;
    }
}

void BytecodeGenerator::MoveToArgumentIfNeeded(const Class *clazz, int index,
                                               Value::Linkage linkage, int dest, ASTNode *ast) {
    if (linkage == Value::kStack) {
        if (clazz->is_reference()) {
            EMIT(ast, Incomplete<kMovePtr>(-dest, GetStackOffset(index)));
            return;
        }
        switch (clazz->reference_size()) {
            case 1:
            case 2:
            case 4:
                EMIT(ast, Incomplete<kMove32>(-dest, GetStackOffset(index)));
                return;
            case 8:
                EMIT(ast, Incomplete<kMove64>(-dest, GetStackOffset(index)));
                return;
            default:
                NOREACHED();
                return;
        }
    }
    if (linkage != Value::kACC) {
        LdaIfNeeded(clazz, index, linkage, ast);
    }
    if (clazz->is_reference()) {
        EMIT(ast, Incomplete<kStarPtr>(-dest));
        return;
    }
    switch (clazz->reference_size()) {
        case 1:
        case 2:
        case 4:
            if (clazz->id() == kType_f32) {
                EMIT(ast, Incomplete<kStaf32>(-dest));
            } else {
                EMIT(ast, Incomplete<kStar32>(-dest));
            }
            break;
        case 8:
            if (clazz->id() == kType_f64) {
                EMIT(ast, Incomplete<kStaf64>(-dest));
            } else {
                EMIT(ast, Incomplete<kStar64>(-dest));
            }
            break;
        default:
            NOREACHED();
            break;
    }
}

void BytecodeGenerator::LdaIfNeeded(const Result &rv, ASTNode *ast) {
    DCHECK_NE(Value::kError, rv.kind);
    const Class *clazz = metadata_space_->type(rv.bundle.type);
    LdaIfNeeded(clazz, rv.bundle.index, static_cast<Value::Linkage>(rv.kind), ast);
}

void BytecodeGenerator::LdaIfNeeded(const Class *clazz, int index, Value::Linkage linkage,
                                    ASTNode *ast) {
    switch (linkage) {
        case Value::kConstant:
            LdaConst(clazz, index, ast);
            break;
        case Value::kStack:
            LdaStack(clazz, index, ast);
            break;
        case Value::kGlobal:
            LdaGlobal(clazz, index, ast);
            break;
        case Value::kCaptured:
            LdaCaptured(clazz, index, ast);
            break;
        case Value::kACC: // ignore
            break;
        case Value::kMetadata:
        case Value::kError:
        default:
            NOREACHED();
            break;
    }
}

void BytecodeGenerator::LdaStack(const Class *clazz, int index, ASTNode *ast) {
    int offset = GetStackOffset(index);
    if (clazz->is_reference()) {
        EMIT(ast, Add<kLdarPtr>(offset));
        return;
    }
    switch (clazz->reference_size()) {
        case 1:
        case 2:
        case 4:
            if (clazz->id() == kType_f32) {
                EMIT(ast, Add<kLdaf32>(offset));
            } else {
                EMIT(ast, Add<kLdar32>(offset));
            }
            break;
        case 8:
            if (clazz->id() == kType_f64) {
                EMIT(ast, Add<kLdaf64>(offset));
            } else {
                EMIT(ast, Add<kLdar64>(offset));
            }
            break;
        default:
            NOREACHED();
            break;
    }
}

void BytecodeGenerator::LdaConst(const Class *clazz, int index, ASTNode *ast) {
    int offset = GetConstOffset(index);
    if (clazz->is_reference()) {
        EMIT(ast, Add<kLdaConstPtr>(offset));
        return;
    }
    switch (clazz->reference_size()) {
        case 1:
        case 2:
        case 4:
            if (clazz->id() == kType_f32) {
                EMIT(ast, Add<kLdaConstf32>(offset));
            } else {
                EMIT(ast, Add<kLdaConst32>(offset));
            }
            break;
        case 8:
            if (clazz->id() == kType_f64) {
                EMIT(ast, Add<kLdaConstf64>(offset));
            } else {
                EMIT(ast, Add<kLdaConst64>(offset));
            }
            break;
        default:
            NOREACHED();
            break;
    }
}

void BytecodeGenerator::LdaGlobal(const Class *clazz, int index, ASTNode *ast) {
    int offset = GetGlobalOffset(index);
    if (clazz->is_reference()) {
        current_fun_->Incoming(ast)->Add<kLdaGlobalPtr>(offset);
        return;
    }
    switch (clazz->reference_size()) {
        case 1:
        case 2:
        case 4:
            if (clazz->id() == kType_f32) {
                EMIT(ast, Add<kLdaGlobalf32>(offset));
            } else {
                EMIT(ast, Add<kLdaGlobal32>(offset));
            }
            break;
        case 8:
            if (clazz->id() == kType_f64) {
                EMIT(ast, Add<kLdaGlobalf64>(offset));
            } else {
                EMIT(ast, Add<kLdaGlobal64>(offset));
            }
            break;
        default:
            NOREACHED();
            break;
    }
}

void BytecodeGenerator::LdaCaptured(const Class *clazz, int index, ASTNode *ast) {
    if (clazz->is_reference()) {
        EMIT(ast, Add<kLdaCapturedPtr>(index));
        return;
    }
    switch (clazz->reference_size()) {
        case 1:
        case 2:
        case 4:
            if (clazz->id() == kType_f32) {
                EMIT(ast, Add<kLdaCapturedf32>(index));
            } else {
                EMIT(ast, Add<kLdaCaptured32>(index));
            }
            break;
        case 8:
            if (clazz->id() == kType_f64) {
                EMIT(ast, Add<kLdaCapturedf64>(index));
            } else {
                EMIT(ast, Add<kLdaCaptured64>(index));
            }
            break;
        default:
            NOREACHED();
            break;
    }
}

void BytecodeGenerator::LdaProperty(const Class *clazz, int index, int offset, ASTNode *ast) {
    index = GetStackOffset(index);
    if (clazz->is_reference()) {
        EMIT(ast, Add<kLdaPropertyPtr>(index, offset));
        return;
    }
    switch (clazz->reference_size()) {
        case 1:
            EMIT(ast, Add<kLdaProperty8>(index, offset));
            break;
        case 2:
            EMIT(ast, Add<kLdaProperty16>(index, offset));
            break;
        case 4:
            if (clazz->id() == kType_f32) {
                EMIT(ast, Add<kLdaPropertyf32>(index, offset));
            } else {
                EMIT(ast, Add<kLdaProperty32>(index, offset));
            }
            break;
        case 8:
            if (clazz->id() == kType_f64) {
                EMIT(ast, Add<kLdaPropertyf64>(index, offset));
            } else {
                EMIT(ast, Add<kLdaProperty64>(index, offset));
            }
            break;
        default:
            NOREACHED();
            break;
    }
}

void BytecodeGenerator::LdaArrayAt(const Class *clazz, int primary, int index, ASTNode *ast) {
    primary = GetStackOffset(primary);
    index = GetStackOffset(index);
    
    if (clazz->is_reference()) {
        EMIT(ast, Add<kLdaArrayAtPtr>(primary, index));
        return;
    }
    
    switch (clazz->reference_size()) {
        case 1:
            EMIT(ast, Add<kLdaArrayAt8>(primary, index));
            break;
        case 2:
            EMIT(ast, Add<kLdaArrayAt16>(primary, index));
            break;
        case 4:
            if (clazz->IsFloating()) {
                EMIT(ast, Add<kLdaArrayAtf32>(primary, index));
            } else {
                EMIT(ast, Add<kLdaArrayAt32>(primary, index));
            }
            break;
        case 8:
            if (clazz->IsFloating()) {
                EMIT(ast, Add<kLdaArrayAtf64>(primary, index));
            } else {
                EMIT(ast, Add<kLdaArrayAt64>(primary, index));
            }
            break;
        default:
            NOREACHED();
            break;
    }
}

void BytecodeGenerator::StaIfNeeded(const Class *clazz, int index, Value::Linkage linkage,
                                    ASTNode *ast) {
    switch (static_cast<Value::Linkage>(linkage)) {
        case Value::kStack:
            StaStack(clazz, index, ast);
            break;
        case Value::kGlobal:
            StaGlobal(clazz, index, ast);
            break;
        case Value::kCaptured:
            StaCaptured(clazz, index, ast);
            break;
        case Value::kACC:
        case Value::kError:
        case Value::kConstant:
        case Value::kMetadata:
        default:
            NOREACHED();
            break;
    }
}

void BytecodeGenerator::StaStack(const Class *clazz, int index, ASTNode *ast) {
    int offset = GetStackOffset(index);
    if (clazz->is_reference()) {
        EMIT(ast, Add<kStarPtr>(offset));
        return;
    }
    switch (clazz->reference_size()) {
        case 1:
        case 2:
        case 4:
            if (clazz->id() == kType_f32) {
                EMIT(ast, Add<kStaf32>(offset));
            } else {
                EMIT(ast, Add<kStar32>(offset));
            }
            break;
        case 8:
            if (clazz->id() == kType_f64) {
                EMIT(ast, Add<kStaf64>(offset));
            } else {
                EMIT(ast, Add<kStar64>(offset));
            }
            break;
        default:
            NOREACHED();
            break;
    }
}

void BytecodeGenerator::StaGlobal(const Class *clazz, int index, ASTNode *ast) {
    int offset = GetGlobalOffset(index);
    if (clazz->is_reference()) {
        EMIT(ast, Add<kStaGlobalPtr>(offset));
        return;
    }
    switch (clazz->reference_size()) {
        case 1:
        case 2:
        case 4:
            if (clazz->id() == kType_f32) {
                EMIT(ast, Add<kStaGlobalf32>(offset));
            } else {
                EMIT(ast, Add<kStaGlobal32>(offset));
            }
            break;
        case 8:
            if (clazz->id() == kType_f64) {
                EMIT(ast, Add<kStaGlobalf64>(offset));
            } else {
                EMIT(ast, Add<kStaGlobal64>(offset));
            }
            break;
        default:
            NOREACHED();
            break;
    }
}

void BytecodeGenerator::StaCaptured(const Class *clazz, int index, ASTNode *ast) {
    if (clazz->is_reference()) {
        EMIT(ast, Add<kStaCapturedPtr>(index));
        return;
    }
    switch (clazz->reference_size()) {
        case 1:
        case 2:
        case 4:
            if (clazz->id() == kType_f32) {
                EMIT(ast, Add<kStaCapturedf32>(index));
            } else {
                EMIT(ast, Add<kStaCaptured32>(index));
            }
            break;
        case 8:
            if (clazz->id() == kType_f64) {
                EMIT(ast, Add<kStaCapturedf64>(index));
            } else {
                EMIT(ast, Add<kStaCaptured64>(index));
            }
            break;
        default:
            NOREACHED();
            break;
    }
}

void BytecodeGenerator::StaProperty(const Class *clazz, int index, int offset, ASTNode *ast) {
    index = GetStackOffset(index);
    if (clazz->is_reference()) {
        EMIT(ast, Add<kStaPropertyPtr>(index, offset));
        return;
    }
    switch (clazz->reference_size()) {
        case 1:
            EMIT(ast, Add<kStaProperty8>(index, offset));
            break;
        case 2:
            EMIT(ast, Add<kStaProperty16>(index, offset));
            break;
        case 4:
            if (clazz->id() == kType_f32) {
                EMIT(ast, Add<kStaPropertyf32>(index, offset));
            } else {
                EMIT(ast, Add<kStaProperty32>(index, offset));
            }
            break;
        case 8:
            if (clazz->id() == kType_f64) {
                EMIT(ast, Add<kStaPropertyf64>(index, offset));
            } else {
                EMIT(ast, Add<kStaProperty64>(index, offset));
            }
            break;
        default:
            NOREACHED();
            break;
    }
}

void BytecodeGenerator::StaArrayAt(const Class *clazz, int primary, int index, ASTNode *ast) {
    primary = GetStackOffset(primary);
    index = GetStackOffset(index);
    
    if (clazz->is_reference()) {
        EMIT(ast, Add<kStaArrayAtPtr>(primary, index));
        return;
    }

    switch (clazz->reference_size()) {
        case 1:
            EMIT(ast, Add<kStaArrayAt8>(primary, index));
            break;
        case 2:
            EMIT(ast, Add<kStaArrayAt16>(primary, index));
            break;
        case 4:
            if (clazz->IsFloating()) {
                EMIT(ast, Add<kStaArrayAtf32>(primary, index));
            } else {
                EMIT(ast, Add<kStaArrayAt32>(primary, index));
            }
            break;
        case 8:
            if (clazz->IsFloating()) {
                EMIT(ast, Add<kStaArrayAtf64>(primary, index));
            } else {
                EMIT(ast, Add<kStaArrayAt64>(primary, index));
            }
            break;
        default:
            NOREACHED();
            break;
    }
}

bool BytecodeGenerator::IsMinorInitializer(const StructureDefinition *owns,
                                           const FunctionDefinition *fun) const {
    if (!fun) {
        return false;
    }
    if (fun->parameters_size() != 0 || fun->prototype()->vargs()) {
        return false;
    }
    auto tid = fun->prototype()->return_type()->id();
    if (tid != Token::kClass && tid != Token::kObject && tid != Token::kRef) {
        return false;
    }
    return fun->prototype()->return_type()->structure() == owns;
}

} // namespace lang

} // namespace mai
