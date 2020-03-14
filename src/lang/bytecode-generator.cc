#include "lang/bytecode-generator.h"
#include "lang/token.h"
#include "lang/bytecode-array-builder.h"
#include "lang/machine.h"
#include "lang/isolate-inl.h"
#include "lang/metadata-space.h"
#include "lang/metadata.h"
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
        return file_unit_->package_name()->ToString() + "." + name->ToString();
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
        DCHECK_NE(this, *current_class_);
        *current_class_ = this;
    }

    ~ClassScope() {
        DCHECK_EQ(this, *current_class_);
        *current_class_ = !prev_ ? nullptr : prev_->GetClassScope();
        Exit();
    }
    
    DEF_PTR_GETTER(StructureDefinition, clazz);
    
    DISALLOW_IMPLICIT_CONSTRUCTORS(ClassScope);
private:
    StructureDefinition *clazz_;
    ClassScope **current_class_;
}; // class BytecodeGenerator::ClassScope


class BytecodeGenerator::BlockScope : public Scope {
public:
    BlockScope(Kind kind, Scope **current)
        : Scope(kind, current) {
        Enter();
    }
    ~BlockScope() override {
        Exit();
    }
    
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
    BytecodeLabel out_label_;
}; // class BytecodeGenerator::BlockScope

class BytecodeGenerator::FunctionScope : public BlockScope {
public:
    FunctionScope(base::Arena *arena, FileUnit *file_unit, Scope **current,
                  FunctionScope **current_fun)
        : BlockScope(kFunctionScope, current)
        , file_unit_(file_unit)
        , builder_(arena)
        , current_fun_(current_fun) {
        DCHECK_NE(this, *current_fun_);
        *current_fun_ = this;
    }
    ~FunctionScope() override {
        DCHECK_EQ(this, *current_fun_);
        *current_fun_ = !prev_ ? nullptr : prev_->GetFunctionScope();
    }

    DEF_PTR_PROP_RW(FileUnit, file_unit);
    DEF_VAL_GETTER(std::vector<int>, source_lines);
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
    
    BytecodeArrayBuilder *Incoming(ASTNode *ast) {
        SourceLocation loc = file_unit_->FindSourceLocation(ast);
        source_lines_.push_back(loc.begin_line);
        return &builder_;
    }
    
    BytecodeArrayBuilder *IncomingWithLine(int line) {
        source_lines_.push_back(line);
        return &builder_;
    }
    
    DISALLOW_IMPLICIT_CONSTRUCTORS(FunctionScope);
private:    
    FileUnit *file_unit_;
    std::vector<int> source_lines_;
    BytecodeArrayBuilder builder_;
    ConstantPoolBuilder constants_;
    StackSpaceAllocator stack_;
    FunctionScope **current_fun_;
}; // class BytecodeGenerator::FunctionScope

using BValue = BytecodeGenerator::Value;

namespace {

inline ASTVisitor::Result ResultWith(BValue::Linkage linkage, int type, int index) {
    ASTVisitor::Result rv;
    rv.kind = linkage;
    rv.bundle.index = index;
    rv.bundle.type  = type;
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
                                     ClassDefinition *class_any,
                                     std::map<std::string, std::vector<FileUnit *>> &&path_units,
                                     std::map<std::string, std::vector<FileUnit *>> &&pkg_units,
                                     base::Arena *arena)
    : isolate_(isolate)
    , metadata_space_(isolate->metadata_space())
    , error_feedback_(DCHECK_NOTNULL(feedback))
    , class_exception_(class_exception)
    , class_any_(class_any)
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
    , class_any_(nullptr)
    , arena_(arena) {
}

BytecodeGenerator::~BytecodeGenerator() {}

bool BytecodeGenerator::Prepare() {
    symbol_trace_.insert(class_any_); // ignore class any
    
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
    //base::StandaloneArena arena(isolate_->env()->GetLowLevelAllocator());
    FunctionScope function_scope(arena_, nullptr, &current_, &current_fun_);

    function_scope.IncomingWithLine(0)->Add<kCheckStack>();
    
    for (auto pair : pkg_units_) {
        const std::string &pkg_name = pair.first;
        
        for (auto unit : pair.second) {
            error_feedback_->set_file_name(unit->file_name()->ToString());
            error_feedback_->set_package_name(unit->package_name()->ToString());
            function_scope.set_file_unit(unit);

            if (!GenerateUnit(pkg_name, unit)) {
                return false;
            }
        }
    }

    Value fun_main_main = FindValue("main.main");
    if (fun_main_main.linkage == Value::kError) {
        error_feedback_->DidFeedback("Unservole 'main.main' entery function");
        return false;
    }
    DCHECK_EQ(Value::kGlobal, fun_main_main.linkage);
    Closure *main = *global_space_.offset<Closure *>(fun_main_main.index);
    function_scope.IncomingWithLine(1)->Add<kLdaGlobalPtr>(GetGlobalOffset(fun_main_main.index));
    if (main->is_mai_function()) {
        function_scope.IncomingWithLine(1)->Add<kCallBytecodeFunction>(0);
    } else {
        function_scope.IncomingWithLine(1)->Add<kCallNativeFunction>(0);
    }
    function_scope.IncomingWithLine(1)->Add<kReturn>();
    generated_init0_fun_ = BuildFunction("init0", &function_scope);
    isolate_->SetGlobalSpace(global_space_.TakeSpans(), global_space_.TakeBitmap(),
                             global_space_.capacity(), global_space_.length());
    
    for (auto &pair : path_units_) {
        for (auto unit : pair.second) {
            delete unit->scope();
            unit->set_scope(nullptr);
        }
    }
    return true;
}

bool BytecodeGenerator::PrepareUnit(const std::string &pkg_name, FileUnit *unit) {
    FileScope::Holder file_holder(unit->scope());

    for (auto ast : unit->definitions()) {
        std::string name = pkg_name + "." + ast->identifier()->ToString();
        
        if (auto clazz_ast = ast->AsClassDefinition(); clazz_ast && ast != class_any_/*filter class Any*/) {
            Class *clazz = metadata_space_->PrepareClass(name, Type::kReferenceTag, kPointerSize);
            clazz_ast->set_clazz(clazz);
            current_->Register(name, {Value::kMetadata, clazz, static_cast<int>(clazz->id()), ast});
        } else if (auto object_ast = ast->AsObjectDefinition()) {
            Class *clazz = metadata_space_->PrepareClass(name, Type::kReferenceTag, kPointerSize);
            object_ast->set_clazz(clazz);
            // object foo ==> main.foo$class
            current_->Register(name + "$class", {Value::kMetadata, clazz, static_cast<int>(clazz->id()), ast});
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
                //printf("%s GS+%d\n", name.c_str(), index);
                current_->Register(name, {Value::kGlobal, clazz, index, ast});
                symbol_trace_.insert(ast);
            } else {
                int index = global_space_.ReserveRef();
                //printf("%s GS+%d\n", name.c_str(), index);
                current_->Register(name, {Value::kGlobal, clazz, index, ast});
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
                current_->Register(name, {Value::kGlobal, clazz, index, ast});
                symbol_trace_.insert(ast);
            } else {
                int index = global_space_.ReserveRef();
                current_->Register(name, {Value::kGlobal, clazz, index, ast});
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
    DCHECK(Value::kGlobal == value.linkage || Value::kMetadata == value.linkage);
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
    
    FileScope *saved_file = current_file_;
    //DCHECK_EQ(saved_file, current_);
    current_file_ = nullptr;
    current_ = nullptr;
    {
        FileScope::Holder file_holder(ast->file_unit()->scope());
        DCHECK_NOTNULL(current_fun_)->set_file_unit(ast->file_unit());
        if (auto rv = ast->Accept(this); rv.kind == Value::kError) {
            return false;
        }
    }
    current_file_ = saved_file;
    current_ = saved_file;
    DCHECK_NOTNULL(current_fun_)->set_file_unit(saved_file->file_unit());
    return true;
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

//    base::StdFilePrinter printer(stdout);
//    scope->builder()->Print(&printer);
    
    return FunctionBuilder(name)
        // TODO:
        .stack_size(stack_size)
        .stack_bitmap(stack_bitmap)
        .prototype({}/*parameters*/, false/*vargs*/, kType_void)
        .source_line_info(source_info)
        .bytecode(bytecodes)
        .const_pool(const_pool->spans(), const_pool->length())
        .const_pool_bitmap(const_pool->bitmap(), (const_pool->length() + 31) / 32)
        .Build(metadata_space_);
}

ASTVisitor::Result BytecodeGenerator::VisitTypeSign(TypeSign *ast) /*override*/ {
    switch (static_cast<Token::Kind>(ast->id())) {
        case Token::kRef:
        case Token::kObject: {
            int type_id = DCHECK_NOTNULL(ast->structure()->clazz())->id();
            return ResultWith(Value::kMetadata, type_id, type_id);
        }
        case Token::kClass:
            NOREACHED();
            break;
        default:
            break;
    }
    return ResultWith(Value::kMetadata, ast->ToBuiltinType(), ast->ToBuiltinType());
}

ASTVisitor::Result BytecodeGenerator::VisitClassDefinition(ClassDefinition *ast) /*override*/ {
    std::string name(DCHECK_NOTNULL(current_file_)->file_unit()->package_name()->ToString());
    name.append(".").append(ast->identifier()->ToString());
    symbol_trace_.insert(ast);
    
    if (ast->base() && !HasGenerated(ast->base())) {
        if (!GenerateSymbolDependence(ast->base())) {
            return ResultWithError();
        }
    }

    ClassScope class_scope(ast, &current_, &current_class_);
    ClassBuilder builder(name);

    uint32_t offset = 0;
    if (ast->base() == class_any_) {
        builder.base(metadata_space_->builtin_type(kType_any));
        offset = metadata_space_->builtin_type(kType_any)->instrance_size();
    } else if (ast == class_exception_) {
        builder.base(metadata_space_->builtin_type(kType_Throwable));
        offset = metadata_space_->builtin_type(kType_Throwable)->instrance_size();
    } else {
        builder.base(ast->base()->clazz());
        offset = ast->base()->clazz()->instrance_size();
    }
    
    std::vector<FieldDesc> fields_desc;
    int tag = 1;
    for (auto field : ast->fields()) {
        auto &field_builder = builder.field(field.declaration->identifier()->ToString());
        Result rv = field.declaration->type()->Accept(this);
        if (rv.kind == Value::kError) {
            return ResultWithError();
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
            offset += field_class->reference_size();
        } else {
            field_offset = RoundUp(offset, 2);
            offset = field_offset + field_class->reference_size();
        }
        field_builder.type(field_class).offset(field_offset).tag(tag++).flags(flags).End();
        fields_desc.push_back({field.declaration->identifier(), field_class, field_offset});
    }
    
    for (auto method : ast->methods()) {
        auto &method_builder = builder.method(method->identifier()->ToString());
        auto full_name = current_file_->LocalFileFullName(ast->identifier()) + "::" +
            method->identifier()->ToString();
        Value value = EnsureFindValue(full_name);
        DCHECK_EQ(Value::kGlobal, value.linkage);
        if (method->is_native()) {
            method_builder.fn(*global_space_.offset<Closure *>(value.index));
            continue;
        }
        auto rv = method->Accept(this);
        if (rv.kind == Value::kError) {
            return ResultWithError();
        }
        Closure *closure = Machine::This()->NewClosure(rv.fun, 0, Heap::kOld);
        method_builder.fn(closure);
        method_builder.End();
        *global_space_.offset<Closure *>(value.index) = closure;
    }

    Function *ctor = GenerateClassConstructor(fields_desc, ast);
    if (!ctor) {
        return ResultWithError();
    }
    builder
        .tags(Type::kReferenceTag)
        .reference_size(kPointerSize)
        .instrance_size(offset)
        .init(Machine::This()->NewClosure(ctor, 0, Heap::kMetadata))
        .BuildWithPreparedClass(metadata_space_, ast->clazz());

    symbol_trace_.insert(ast);
    return ResultWithVoid();
}

ASTVisitor::Result BytecodeGenerator::VisitObjectDefinition(ObjectDefinition *ast) /*override*/ {
    // TODO:
    TODO();
    return ResultWithError();
}

ASTVisitor::Result BytecodeGenerator::VisitFunctionDefinition(FunctionDefinition *ast) /*override*/ {
    if (ast->is_native()) {
        if (ast->file_unit()) {
            symbol_trace_.insert(ast);
        }
        return ResultWithVoid();
    }

    const bool is_method = current_->is_class_scope();
    auto rv = ast->body()->Accept(this);
    if (rv.kind == Value::kError) {
        return ResultWithError();
    }
    if (is_method) {
        std::string name = ast->file_unit()->package_name()->ToString() + "." +
            current_class_->clazz()->identifier()->ToString() + "::" +
            ast->identifier()->ToString();
        metadata_space_->RenameFunction(rv.fun, name);
        return rv;
    }
    if (ast->file_unit()) { // Global var
        Closure *closure = Machine::This()->NewClosure(rv.fun, 0/*captured_var_size*/, 0/*flags*/);
        DCHECK_EQ(ast->file_unit(), current_file_->file_unit());
        const std::string name = current_file_->LocalFileFullName(ast->identifier());
        metadata_space_->RenameFunction(rv.fun, name);
        Value value = EnsureFindValue(name);
        DCHECK_EQ(Value::kGlobal, value.linkage);
        *global_space_.offset<Closure *>(value.index) = closure;
        symbol_trace_.insert(ast);
        return ResultWithVoid();
    }
    
    // Local var
    metadata_space_->RenameFunction(rv.fun, ast->identifier()->ToString());
    int kidx = current_fun_->constants()->FindOrInsertMetadata(rv.fun);
    current_fun_->Incoming(ast)->Add<kClose>(kidx);
    int location = current_fun_->stack()->ReserveRef();
    const Class *clazz = metadata_space_->builtin_type(kType_closure);
    current_->Register(ast->identifier()->ToString(), {Value::kStack, clazz, location});
    return ResultWithVoid();
}

ASTVisitor::Result BytecodeGenerator::VisitLambdaLiteral(LambdaLiteral *ast) /*override*/ {
    const bool is_method = current_->is_class_scope();
    
    //base::StandaloneArena arena(isolate_->env()->GetLowLevelAllocator());
    FunctionScope function_scope(arena_, current_file_->file_unit(), &current_, &current_fun_);
    current_fun_->Incoming(ast)->Add<kCheckStack>();
    
    int param_size = 0;
    for (auto param : ast->prototype()->parameters()) {
        param_size += RoundUp(param.type->GetReferenceSize(), StackSpaceAllocator::kSizeGranularity);
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
    }
    for (auto param : ast->prototype()->parameters()) {
        auto rv = param.type->Accept(this);
        if (rv.kind == Value::kError) {
            return ResultWithError();
        }
        const Class *clazz = DCHECK_NOTNULL(metadata_space_->type(rv.bundle.type));
        param_offset += RoundUp(clazz->reference_size(), StackSpaceAllocator::kSizeGranularity);
        current_->Register(param.name->ToString(), {
            Value::kStack,
            clazz,
            GetParameterOffset(param_size, param_offset)
        });
    }
    if (ast->prototype()->vargs()) {
        const Class *clazz = metadata_space_->builtin_type(kType_array);
        param_offset += RoundUp(clazz->reference_size(), StackSpaceAllocator::kSizeGranularity);
        current_->Register("argv", {
            Value::kStack,
            clazz,
            GetParameterOffset(param_size, param_offset)
        });
    }

    for (auto stmt : ast->statements()) {
        if (auto rv = stmt->Accept(this); rv.kind == Value::kError) {
            return ResultWithError();
        }
    }

    current_fun_->Incoming(ast)->Add<kReturn>();
    Function *fun = BuildFunction("$lambda$", &function_scope);
    Result result;
    result.kind = Value::kMetadata;
    result.fun  = fun;
    return result;
}

ASTVisitor::Result BytecodeGenerator::VisitCallExpression(CallExpression *ast) /*override*/ {
    if (!ast->callee()->IsDotExpression()) {
        return GenerateRegularCalling(ast);
    }

    DotExpression *dot = DCHECK_NOTNULL(ast->callee()->AsDotExpression());
    if (!dot->primary()->IsIdentifier()) {
        auto rv = dot->primary()->Accept(this);
        if (rv.kind == Value::kError) {
            return ResultWithError();
        }
        const Class *clazz = metadata_space_->type(rv.bundle.type);
        Value::Linkage linkage = static_cast<Value::Linkage>(rv.kind);
        return GenerateMethodCalling({linkage, clazz, rv.bundle.index}, ast);
    }

    Identifier *id = DCHECK_NOTNULL(dot->primary()->AsIdentifier());
    Value value = current_file_->Find(id->name(), dot->rhs());
    if (value.linkage != Value::kError) { // Found: pcakgeName.identifer
        if (!HasGenerated(value.ast) && !GenerateSymbolDependence(value)) {
            return ResultWithError();
        }
        return GenerateRegularCalling(ast);
    }

    auto found = current_->Resolve(id->name());
    value = std::get<1>(found);
    if (!HasGenerated(value.ast) && !GenerateSymbolDependence(value)) {
        return ResultWithError();
    }
    // TODO: Captured var

    return GenerateMethodCalling(value, ast);
}

ASTVisitor::Result BytecodeGenerator::GenerateMethodCalling(Value primary, CallExpression *ast) {
    DotExpression *dot = DCHECK_NOTNULL(ast->callee()->AsDotExpression());
    const Class *clazz = primary.type;
    DCHECK(clazz->is_reference());
    StackSpaceAllocator::Scope stack_scope(current_fun_->stack());
    
    int self = current_fun_->stack()->ReserveRef();
    MoveToStackIfNeeded(clazz, primary.index, primary.linkage, self, dot);
    
    auto proto = ast->prototype();
    std::vector<const Class *> params;
    for (auto param : proto->parameters()) {
        auto rv = param.type->Accept(this);
        if (rv.kind == Value::kError) {
            return ResultWithError();
        }
        params.push_back(metadata_space_->type(rv.bundle.index));
    }
    int arg_size = GenerateArguments(ast->operands(), params, dot->primary(), self, proto->vargs());
    if (arg_size < 0) {
        return ResultWithError();
    }

    if (clazz->id() == kType_any) { // interface
        // TODO:
        TODO();
    }

    const Method *method =
        DCHECK_NOTNULL(metadata_space_->FindClassMethodOrNull(clazz, dot->rhs()->data()));
    int kidx = current_fun_->constants()->FindOrInsertClosure(method->fn());
    LdaConst(metadata_space_->builtin_type(kType_closure), kidx, dot);
    if (method->fn()->is_cxx_function()) {
        current_fun_->Incoming(ast)->Add<kCallNativeFunction>(arg_size);
    } else {
        current_fun_->Incoming(ast)->Add<kCallBytecodeFunction>(arg_size);
    }
    //current_fun_->stack()->FallbackRef(self);
    return proto->return_type()->Accept(this);
}

Function *BytecodeGenerator::GenerateClassConstructor(const std::vector<FieldDesc> &fields_desc,
                                                      ClassDefinition *ast) {
    FunctionScope function_scope(arena_, current_file_->file_unit(), &current_, &current_fun_);
    current_fun_->Incoming(ast)->Add<kCheckStack>();
    
    int param_size = kPointerSize; // self
    for (auto param : ast->parameters()) {
        size_t size = 0;
        if (param.field_declaration) {
            size = ast->field(param.as_field).declaration->type()->GetReferenceSize();
        } else {
            size = param.as_parameter->type()->GetReferenceSize();
        }
        param_size += RoundUp(size, StackSpaceAllocator::kSizeGranularity);
    }
    param_size = RoundUp(param_size, kStackAligmentSize);

    int param_offset = kPointerSize; // self
    Value self{Value::kStack, ast->clazz(), GetParameterOffset(param_size, param_offset)};
    current_->Register("self", self);
    for (auto param : ast->parameters()) {
        VariableDeclaration *decl = param.field_declaration
                                  ? ast->field(param.as_field).declaration : param.as_parameter;
        auto rv = decl->type()->Accept(this);
        if (rv.kind == Value::kError) {
            return nullptr;
        }
        const Class *clazz = DCHECK_NOTNULL(metadata_space_->type(rv.bundle.type));
        param_offset += RoundUp(clazz->reference_size(), StackSpaceAllocator::kSizeGranularity);
        Value value{Value::kStack, clazz, GetParameterOffset(param_size, param_offset)};
        current_->Register(decl->identifier()->ToString(), value);
        
        if (param.field_declaration) {
            DCHECK_LT(param.as_field, fields_desc.size());
            auto field = fields_desc[param.as_field];
            LdaStack(clazz, value.index, decl);
            StaProperty(field.type, self.index, field.offset, decl);
        }
    }
    
    for (size_t i = 0; i < ast->fields_size(); i++) {
        DCHECK_LT(i, fields_desc.size());

        auto field = ast->field(i);
        if (field.in_constructor) {
            continue;
        }
        
        auto rv = field.declaration->type()->Accept(this);
        if (rv.kind == Value::kError) {
            return nullptr;
        }
        const Class *field_type = metadata_space_->type(rv.bundle.index);
        if (!field.declaration->initializer()) {
            current_fun_->Incoming(field.declaration)->Add<kLdaZero>();
            StaProperty(field_type, self.index, fields_desc[i].offset, field.declaration);
            continue;
        }

        if (rv = field.declaration->initializer()->Accept(this); rv.kind == Value::kError) {
            return nullptr;
        }
        
        const Class *clazz = metadata_space_->type(rv.bundle.type);
        if (NeedInbox(field_type, clazz)) {
            InboxIfNeeded(clazz, rv.bundle.index, static_cast<Value::Linkage>(rv.kind), field_type,
                          field.declaration);
        } else {
            LdaIfNeeded(clazz, rv.bundle.index, static_cast<Value::Linkage>(rv.kind),
                        field.declaration);
        }
        StaProperty(field_type, self.index, fields_desc[i].offset, field.declaration);
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
        const Class *base = DCHECK_NOTNULL(ast->clazz()->base());
        int kidx = current_fun_->constants()->FindOrInsertClosure(DCHECK_NOTNULL(base->init())->fn());
        current_fun_->Incoming(ast)->Add<kLdaConstPtr>(kidx);
        current_fun_->Incoming(ast)->Add<kCallBytecodeFunction>(arg_size);
    }
    
    // return self
    LdaStack(ast->clazz(), self.index, ast);
    current_fun_->Incoming(ast)->Add<kReturn>();
    return BuildFunction(ast->identifier()->ToString(), &function_scope);
}

ASTVisitor::Result BytecodeGenerator::GenerateRegularCalling(CallExpression *ast) {
    auto rv = ast->callee()->Accept(this);
    if (rv.kind == Value::kError) {
        return ResultWithError();
    }
    if (rv.kind == Value::kMetadata) {
        const Class *clazz = metadata_space_->type(rv.bundle.index);
        return GenerateNewObject(clazz, ast);
    }

    StackSpaceAllocator::Scope stack_scope(current_fun_->stack());
    DCHECK_EQ(kType_closure, rv.bundle.type);
    const Class *clazz = metadata_space_->builtin_type(kType_closure);
    bool native = false;
    if (rv.kind == Value::kGlobal) {
        Closure *closure = *global_space_.offset<Closure *>(rv.bundle.index);
        native = closure->is_cxx_function();
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
    LdaIfNeeded(clazz, rv.bundle.index, static_cast<Value::Linkage>(rv.kind), ast->callee());
    if (native) {
        current_fun_->Incoming(ast)->Add<kCallNativeFunction>(arg_size);
    } else {
        current_fun_->Incoming(ast)->Add<kCallBytecodeFunction>(arg_size);
    }

    return proto->return_type()->Accept(this);
}

ASTVisitor::Result BytecodeGenerator::VisitVariableDeclaration(VariableDeclaration *ast) /*override*/ {
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
            LdaIfNeeded(clazz, rv.bundle.index, static_cast<Value::Linkage>(rv.kind), ast->initializer());
            StaGlobal(clazz, value.index, ast);
        }
        symbol_trace_.insert(ast);
        return ResultWithVoid();
    }

    Result rv = DCHECK_NOTNULL(ast->type())->Accept(this);
    if (rv.kind == Value::kError) {
        return ResultWithError();
    }
    DCHECK_EQ(rv.kind, Value::kMetadata);
    const Class *clazz = metadata_space_->type(rv.bundle.index);
    
    Value local{Value::kStack, clazz};
    if (clazz->is_reference()) {
        local.index = current_fun_->stack()->ReserveRef();
    } else {
        local.index = current_fun_->stack()->Reserve(clazz->reference_size());
    }
    current_->Register(ast->identifier()->ToString(), local);

    StackSpaceScope scope(current_fun_->stack());
    if (rv = ast->initializer()->Accept(this); rv.kind == Value::kError) {
        return ResultWithError();
    }
    LdaIfNeeded(clazz, rv.bundle.index, static_cast<Value::Linkage>(rv.kind), ast->initializer());
    StaStack(clazz, local.index, ast);
    return ResultWithVoid();
}

ASTVisitor::Result BytecodeGenerator::VisitDotExpression(DotExpression *ast) /*override*/ {
    if (!ast->primary()->IsIdentifier()) {
        auto rv = ast->primary()->Accept(this);
        if (rv.kind == Value::kError) {
            return ResultWithError();
        }
        const Class *clazz = metadata_space_->type(rv.bundle.type);
        return GenerateDotExpression(clazz, rv.bundle.index, static_cast<Value::Linkage>(rv.kind),
                                     ast);
    }
    
    Identifier *id = DCHECK_NOTNULL(ast->primary()->AsIdentifier());
    Value value = current_file_->Find(id->name(), ast->rhs());
    if (value.linkage != Value::kError) { // Found
        if (!HasGenerated(value.ast) && !GenerateSymbolDependence(value)) {
            return ResultWithError();
        }
        return ResultWith(value.linkage, value.type->id(), value.index);
    }
    
    auto found = current_->Resolve(id->name());
    value = std::get<1>(found);
    if (!HasGenerated(value.ast) && !GenerateSymbolDependence(value)) {
        return ResultWithError();
    }
    // TODO: Captured var

    return GenerateDotExpression(value.type, value.index, value.linkage, ast);
}

ASTVisitor::Result BytecodeGenerator::VisitStringTemplateExpression(StringTemplateExpression *ast)
/*override*/ {
    StackSpaceAllocator::Scope stack_scope(current_fun_->stack());
    std::vector<Value> parts;
    for (auto part : ast->operands()) {
        Result rv;
        if (auto literal = part->AsStringLiteral()) {
            if (literal->value()->empty()) {
                continue;
            }
            if (rv = literal->Accept(this); rv.kind == Value::kError) {
                return ResultWithError();
            }
        } else {
            if (rv = part->Accept(this); rv.kind == Value::kError) {
                return ResultWithError();
            }
        }
        const Class *clazz = metadata_space_->type(rv.bundle.type);
        if (clazz->id() == kType_string && rv.kind != Value::kStack) {
            Value value{static_cast<Value::Linkage>(rv.kind), clazz, rv.bundle.index};
            parts.push_back(value);
            continue;
        }
        ToStringIfNeeded(clazz, rv.bundle.index, static_cast<Value::Linkage>(rv.kind), part);
        int index = current_fun_->stack()->ReserveRef();
        const Class *type = metadata_space_->builtin_type(kType_string);
        StaStack(type, index, part);
        Value value{Value::kStack, type, index};
        parts.push_back(value);
    }
    
    int argument_offset = 0;
    for(auto part : parts) {
        argument_offset += kPointerSize; // Must be string
        MoveToArgumentIfNeeded(part.type, part.index, part.linkage, argument_offset, ast);
    }
    
    current_fun_->Incoming(ast)->Add<kContact>(argument_offset);
    return ResultWith(Value::kACC, kType_string, 0);
}

ASTVisitor::Result BytecodeGenerator::VisitUnaryExpression(UnaryExpression *ast) /*override*/ {
    TODO();
    return ResultWithError();
}

ASTVisitor::Result BytecodeGenerator::VisitBinaryExpression(BinaryExpression *ast) /*override*/ {
    switch (ast->op().kind) {
        case Operator::kEqual:
        case Operator::kNotEqual:
        case Operator::kLess:
        case Operator::kLessEqual:
        case Operator::kGreater:
        case Operator::kGreaterEqual: {
            auto rv = ast->lhs()->Accept(this);
            if (rv.kind == Value::kError) {
                return ResultWithError();
            }
            const Class *clazz = metadata_space_->type(rv.bundle.type);
            int lhsidx = rv.bundle.index;
            bool lhstmp = false;
            if (rv.kind != Value::kStack) {
                lhsidx = current_fun_->StackReserve(clazz);
                lhstmp = true;
                MoveToStackIfNeeded(clazz, rv.bundle.index, static_cast<Value::Linkage>(rv.kind),
                                    lhsidx, ast->lhs());
            }
            if (rv = ast->rhs()->Accept(this); rv.kind == Value::kError) {
                return ResultWithError();
            }
            int rhsidx = rv.bundle.index;
            bool rhstmp = false;
            if (rv.kind != Value::kStack) {
                rhstmp = true;
                rhsidx = current_fun_->StackReserve(clazz);
                MoveToStackIfNeeded(clazz, rv.bundle.index, static_cast<Value::Linkage>(rv.kind),
                                    rhsidx, ast->lhs());
            }
            GenerateComparation(clazz, ast->op(), lhsidx, rhsidx, ast);
            if (rhstmp) {
                current_fun_->StackFallback(clazz, rhsidx);
            }
            if (lhstmp) {
                current_fun_->StackFallback(clazz, lhsidx);
            }
        } return ResultWith(Value::kACC, kType_bool, 0);
            
        case Operator::kAnd: {
            auto rv = ast->lhs()->Accept(this);
            if (rv.kind == Value::kError) {
                return ResultWithError();
            }
            const Class *clazz = metadata_space_->type(rv.bundle.type);
            DCHECK_EQ(kType_bool, clazz->id());
            LdaIfNeeded(clazz, rv.bundle.index, static_cast<Value::Linkage>(rv.kind), ast->lhs());
            BytecodeLabel done;
            current_fun_->Incoming(ast)->GotoIfFalse(&done);
            
            if (rv = ast->rhs()->Accept(this); rv.kind == Value::kError) {
                return ResultWithError();
            }
            clazz = metadata_space_->type(rv.bundle.type);
            DCHECK_EQ(kType_bool, clazz->id());
            LdaIfNeeded(clazz, rv.bundle.index, static_cast<Value::Linkage>(rv.kind), ast->lhs());

            current_fun_->builder()->Bind(&done);
        } break;
            
        default:
            NOREACHED();
            break;
    }
    TODO();
    return ResultWithError();
}

ASTVisitor::Result BytecodeGenerator::VisitIdentifier(Identifier *ast) /*override*/ {
    auto [scope, value] = current_->Resolve(ast->name());
    DCHECK(scope != nullptr && value.linkage != Value::kError);

    if (scope->is_file_scope()) {
        if (!HasGenerated(value.ast) && !GenerateSymbolDependence(value)) {
            return ResultWithError();
        }
        DCHECK(HasGenerated(value.ast));
    }

    switch (scope->kind()) {
        case Scope::kFunctionScope:
            if (scope == current_fun_) {
                return ResultWith(value.linkage, value.type->id(), value.index);
            }
            TODO(); // TODO: Captured var
            break;
        case Scope::kFileScope:
        case Scope::kLoopBlockScope:
        case Scope::kPlainBlockScope:
            return ResultWith(value.linkage, value.type->id(), value.index);
        case Scope::kClassScope:
        default:
            NOREACHED();
            break;
    }
    return ResultWith(value.linkage, value.type->id(), value.index);
}

ASTVisitor::Result BytecodeGenerator::VisitBoolLiteral(BoolLiteral *ast) /*override*/ {
    current_fun_->Incoming(ast)->Add<kLdaSmi32>(ast->value());
    //int index = current_fun_->constants()->FindOrInsertI32(ast->value());
    return ResultWith(Value::kACC, kType_bool, 0);
}

ASTVisitor::Result BytecodeGenerator::VisitI8Literal(I8Literal *ast) /*override*/ {
    current_fun_->Incoming(ast)->Add<kLdaSmi32>(ast->value());
    //int index = current_fun_->constants()->FindOrInsertI32(ast->value());
    return ResultWith(Value::kACC, kType_i8, 0);
}

ASTVisitor::Result BytecodeGenerator::VisitU8Literal(U8Literal *ast) /*override*/ {
    current_fun_->Incoming(ast)->Add<kLdaSmi32>(ast->value());
    //int index = current_fun_->constants()->FindOrInsertU32(ast->value());
    return ResultWith(Value::kACC, kType_u8, 0);
}

ASTVisitor::Result BytecodeGenerator::VisitI16Literal(I16Literal *ast) /*override*/ {
    int index = current_fun_->constants()->FindOrInsertI32(ast->value());
    return ResultWith(Value::kConstant, kType_i16, index);
}

ASTVisitor::Result BytecodeGenerator::VisitU16Literal(U16Literal *ast) /*override*/ {
    current_fun_->Incoming(ast)->Add<kLdaSmi32>(ast->value());
    //int index = current_fun_->constants()->FindOrInsertU32(ast->value());
    return ResultWith(Value::kACC, kType_u16, 0);
}

ASTVisitor::Result BytecodeGenerator::VisitI32Literal(I32Literal *ast) /*override*/ {
    if (ast->value() >= 0 && ast->value() <= BytecodeNode::kMaxUSmi32) {
        current_fun_->Incoming(ast)->Add<kLdaSmi32>(ast->value());
        return ResultWith(Value::kACC, kType_i32, 0);
    } else {
        int index = current_fun_->constants()->FindOrInsertI32(ast->value());
        return ResultWith(Value::kConstant, kType_i32, index);
    }
}

ASTVisitor::Result BytecodeGenerator::VisitU32Literal(U32Literal *ast) /*override*/ {
    if (ast->value() <= BytecodeNode::kMaxUSmi32) {
        current_fun_->Incoming(ast)->Add<kLdaSmi32>(ast->value());
        return ResultWith(Value::kACC, kType_u32, 0);
    } else {
        int index = current_fun_->constants()->FindOrInsertU32(ast->value());
        return ResultWith(Value::kConstant, kType_u32, index);
    }
}

ASTVisitor::Result BytecodeGenerator::VisitIntLiteral(IntLiteral *ast) /*override*/ {
    if (ast->value() >= 0 && ast->value() <= BytecodeNode::kMaxUSmi32) {
        current_fun_->Incoming(ast)->Add<kLdaSmi32>(ast->value());
        return ResultWith(Value::kACC, kType_int, 0);
    } else {
        int index = current_fun_->constants()->FindOrInsertI32(ast->value());
        return ResultWith(Value::kConstant, kType_int, index);
    }
}

ASTVisitor::Result BytecodeGenerator::VisitUIntLiteral(UIntLiteral *ast) /*override*/ {
    if (ast->value() >= 0 && ast->value() <= BytecodeNode::kMaxUSmi32) {
        current_fun_->Incoming(ast)->Add<kLdaSmi32>(ast->value());
        return ResultWith(Value::kACC, kType_uint, 0);
    } else {
        int index = current_fun_->constants()->FindOrInsertU32(ast->value());
        return ResultWith(Value::kConstant, kType_uint, index);
    }
}

ASTVisitor::Result BytecodeGenerator::VisitI64Literal(I64Literal *ast) /*override*/ {
    if (ast->value() == 0) {
        current_fun_->Incoming(ast)->Add<kLdaZero>();
        return ResultWith(Value::kACC, kType_i64, 0);
    } else {
        int index = current_fun_->constants()->FindOrInsertI64(ast->value());
        return ResultWith(Value::kConstant, kType_i64, index);
    }
}

ASTVisitor::Result BytecodeGenerator::VisitU64Literal(U64Literal *ast) /*override*/ {
    if (ast->value() == 0) {
        current_fun_->Incoming(ast)->Add<kLdaZero>();
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

SourceLocation BytecodeGenerator::FindSourceLocation(const ASTNode *ast) {
    return DCHECK_NOTNULL(current_file_)->file_unit()->FindSourceLocation(ast);
}

ASTVisitor::Result BytecodeGenerator::GenerateNewObject(const Class *clazz, CallExpression *ast) {
    int kidx = current_fun_->constants()->FindOrInsertMetadata(clazz);
    current_fun_->Incoming(ast)->Add<kNewObject>(0, kidx);
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
    current_fun_->Incoming(ast)->Add<kCallBytecodeFunction>(argument_size);
    
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
        current_fun_->Incoming(callee)->Incomplete<kMovePtr>(-(argument_offset), GetStackOffset(self));
    }

    for (size_t i = 0; i < operands.size(); i++) {
        Expression *arg = operands[i];
        Value value = args[i];
        
        argument_offset += RoundUp(value.type->reference_size(),
                                   StackSpaceAllocator::kSizeGranularity);
        if (value.type->is_reference()) {
            if (value.linkage == Value::kStack) {
                current_fun_->Incoming(arg)->Incomplete<kMovePtr>(-(argument_offset),
                                                                  GetStackOffset(value.index));
            } else {
                LdaIfNeeded(value.type, value.index, value.linkage, arg);
                current_fun_->Incoming(arg)->Incomplete<kStarPtr>(-(argument_offset));
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

ASTVisitor::Result BytecodeGenerator::GenerateDotExpression(const Class *clazz, int index,
                                                            Value::Linkage linkage,
                                                            DotExpression *ast) {
    const Field *field = metadata_space_->FindClassFieldOrNull(clazz, ast->rhs()->data());
    if (field) {
        if (linkage != Value::kACC) {
            LdaIfNeeded(clazz, index, linkage, ast);
        }
        DCHECK(clazz->is_reference());
        int self = current_fun_->stack()->ReserveRef();
        StaStack(clazz, self, ast);
        
        if (field->type()->is_reference()) {
            current_fun_->Incoming(ast)->Add<kLdaPropertyPtr>(self, field->offset());
            return ResultWith(Value::kACC, field->type()->id(), 0);
        }
        
        switch (field->type()->reference_size()) {
            case 1:
                current_fun_->Incoming(ast)->Add<kLdaProperty8>(self, field->offset());
                break;
            case 2:
                current_fun_->Incoming(ast)->Add<kLdaProperty16>(self, field->offset());
                break;
            case 4:
                if (field->type()->id() == kType_f32) {
                    current_fun_->Incoming(ast)->Add<kLdaPropertyf32>(self, field->offset());
                } else {
                    current_fun_->Incoming(ast)->Add<kLdaProperty32>(self, field->offset());
                }
                break;
            case 8:
                if (field->type()->id() == kType_f64) {
                    current_fun_->Incoming(ast)->Add<kLdaPropertyf64>(self, field->offset());
                } else {
                    current_fun_->Incoming(ast)->Add<kLdaProperty64>(self, field->offset());
                }
                break;
            default:
                NOREACHED();
                break;
        }
        return ResultWith(Value::kACC, field->type()->id(), 0);
    }
    
    // load method
    const Method *method = DCHECK_NOTNULL(metadata_space_->FindClassMethodOrNull(clazz,
                                                                                 ast->rhs()->data()));
    int kidx = current_fun_->constants()->FindOrInsertClosure(method->fn());
    return ResultWith(Value::kConstant, kType_closure, kidx);
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
                    current_fun_->Incoming(ast)->Add<kTestEqual32>(loff, roff);
                    break;
                case Operator::kNotEqual:
                    current_fun_->Incoming(ast)->Add<kTestNotEqual32>(loff, roff);
                    break;
                case Operator::kLess:
                    current_fun_->Incoming(ast)->Add<kTestLessThan32>(loff, roff);
                    break;
                case Operator::kLessEqual:
                    current_fun_->Incoming(ast)->Add<kTestLessThanOrEqual32>(loff, roff);
                    break;
                case Operator::kGreater:
                    current_fun_->Incoming(ast)->Add<kTestGreaterThan32>(loff, roff);
                    break;
                case Operator::kGreaterEqual:
                    current_fun_->Incoming(ast)->Add<kTestGreaterThanOrEqual32>(loff, roff);
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
                    current_fun_->Incoming(ast)->Add<kTestEqual64>(loff, roff);
                    break;
                case Operator::kNotEqual:
                    current_fun_->Incoming(ast)->Add<kTestNotEqual64>(loff, roff);
                    break;
                case Operator::kLess:
                    current_fun_->Incoming(ast)->Add<kTestLessThan64>(loff, roff);
                    break;
                case Operator::kLessEqual:
                    current_fun_->Incoming(ast)->Add<kTestLessThanOrEqual64>(loff, roff);
                    break;
                case Operator::kGreater:
                    current_fun_->Incoming(ast)->Add<kTestGreaterThan64>(loff, roff);
                    break;
                case Operator::kGreaterEqual:
                    current_fun_->Incoming(ast)->Add<kTestGreaterThanOrEqual64>(loff, roff);
                    break;
                default:
                    NOREACHED();
                    break;
            }
            break;
        case kType_f32:
            switch (op.kind) {
                case Operator::kEqual:
                    current_fun_->Incoming(ast)->Add<kTestEqualf32>(loff, roff);
                    break;
                case Operator::kNotEqual:
                    current_fun_->Incoming(ast)->Add<kTestNotEqualf32>(loff, roff);
                    break;
                case Operator::kLess:
                    current_fun_->Incoming(ast)->Add<kTestLessThanf32>(loff, roff);
                    break;
                case Operator::kLessEqual:
                    current_fun_->Incoming(ast)->Add<kTestLessThanOrEqualf32>(loff, roff);
                    break;
                case Operator::kGreater:
                    current_fun_->Incoming(ast)->Add<kTestGreaterThanf32>(loff, roff);
                    break;
                case Operator::kGreaterEqual:
                    current_fun_->Incoming(ast)->Add<kTestGreaterThanOrEqualf32>(loff, roff);
                    break;
                default:
                    NOREACHED();
                    break;
            }
            break;
        case kType_f64:
            switch (op.kind) {
                case Operator::kEqual:
                    current_fun_->Incoming(ast)->Add<kTestEqualf64>(loff, roff);
                    break;
                case Operator::kNotEqual:
                    current_fun_->Incoming(ast)->Add<kTestNotEqualf64>(loff, roff);
                    break;
                case Operator::kLess:
                    current_fun_->Incoming(ast)->Add<kTestLessThanf64>(loff, roff);
                    break;
                case Operator::kLessEqual:
                    current_fun_->Incoming(ast)->Add<kTestLessThanOrEqualf64>(loff, roff);
                    break;
                case Operator::kGreater:
                    current_fun_->Incoming(ast)->Add<kTestGreaterThanf64>(loff, roff);
                    break;
                case Operator::kGreaterEqual:
                    current_fun_->Incoming(ast)->Add<kTestGreaterThanOrEqualf64>(loff, roff);
                    break;
                default:
                    NOREACHED();
                    break;
            }
            break;
        case kType_string:
            switch (op.kind) {
                case Operator::kEqual:
                    current_fun_->Incoming(ast)->Add<kTestStringEqual>(loff, roff);
                    break;
                case Operator::kNotEqual:
                    current_fun_->Incoming(ast)->Add<kTestStringNotEqual>(loff, roff);
                    break;
                case Operator::kLess:
                    current_fun_->Incoming(ast)->Add<kTestStringLessThan>(loff, roff);
                    break;
                case Operator::kLessEqual:
                    current_fun_->Incoming(ast)->Add<kTestStringLessThanOrEqual>(loff, roff);
                    break;
                case Operator::kGreater:
                    current_fun_->Incoming(ast)->Add<kTestStringGreaterThan>(loff, roff);
                    break;
                case Operator::kGreaterEqual:
                    current_fun_->Incoming(ast)->Add<kTestStringLessThanOrEqual>(loff, roff);
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

void BytecodeGenerator::ToStringIfNeeded(const Class *clazz, int index, Value::Linkage linkage,
                                         ASTNode *ast) {
    switch (static_cast<BuiltinType>(clazz->id())) {
#define DEFINE_TO_STRING(dest, name, ...) \
        case kType_##name: { \
            int args_size = RoundUp(clazz->reference_size(), StackSpaceAllocator::kSizeGranularity); \
            MoveToArgumentIfNeeded(clazz, index, linkage, args_size, ast); \
            Value value = FindOrInsertExternalFunction("lang." #dest "::toString"); \
            LdaGlobal(value.type, value.index, ast); \
            current_fun_->Incoming(ast)->Add<kCallNativeFunction>(args_size); \
        } break;
        DECLARE_BOX_NUMBER_TYPES(DEFINE_TO_STRING)
#undef DEFINE_TO_STRING
        case kType_string:
            LdaIfNeeded(clazz, index, linkage, ast);
            break;
        default: {
            auto method = metadata_space_->FindClassMethodOrNull(clazz, "toString");
            if (!method) {
                auto any = metadata_space_->builtin_type(kType_any);
                method = DCHECK_NOTNULL(metadata_space_->FindClassMethodOrNull(any, "toString"));
            }
            int argument_offset = kPointerSize;
            MoveToArgumentIfNeeded(clazz, index, linkage, argument_offset, ast);
            int kidx = current_fun_->constants()->FindOrInsertClosure(method->fn());
            LdaConst(method->fn()->clazz(), kidx, ast);
            if (method->fn()->is_cxx_function()) {
                current_fun_->Incoming(ast)->Add<kCallNativeFunction>(argument_offset);
            } else {
                current_fun_->Incoming(ast)->Add<kCallBytecodeFunction>(argument_offset);
            }
        } break;
    }
}

void BytecodeGenerator::InboxIfNeeded(const Class *clazz, int index, Value::Linkage linkage,
                                      const Class *lval, ASTNode *ast) {
    switch (static_cast<BuiltinType>(clazz->id())) {
#define DEFINE_INBOX_PRIMITIVE(dest, name, ...) \
        case kType_##name: { \
            int args_size = RoundUp(clazz->reference_size(), StackSpaceAllocator::kSizeGranularity); \
            MoveToArgumentIfNeeded(clazz, index, linkage, args_size, ast); \
            Value value = FindOrInsertExternalFunction("lang." #dest "::valueOf"); \
            LdaGlobal(value.type, value.index, ast); \
            current_fun_->Incoming(ast)->Add<kCallNativeFunction>(args_size); \
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
        DCHECK_GE(index, 0);
        if (clazz->is_reference()) {
            current_fun_->Incoming(ast)->Add<kMovePtr>(GetStackOffset(dest), GetStackOffset(index));
            return;
        }
        switch (clazz->reference_size()) {
            case 1:
            case 2:
            case 4:
                current_fun_->Incoming(ast)->Add<kMove32>(GetStackOffset(dest), GetStackOffset(index));
                return;
            case 8:
                current_fun_->Incoming(ast)->Add<kMove64>(GetStackOffset(dest), GetStackOffset(index));
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
        current_fun_->Incoming(ast)->Add<kStarPtr>(GetStackOffset(dest));
        return;
    }
    switch (clazz->reference_size()) {
        case 1:
        case 2:
        case 4:
            if (clazz->id() == kType_f32) {
                current_fun_->Incoming(ast)->Add<kStaf32>(GetStackOffset(dest));
            } else {
                current_fun_->Incoming(ast)->Add<kStar32>(GetStackOffset(dest));
            }
            break;
        case 8:
            if (clazz->id() == kType_f64) {
                current_fun_->Incoming(ast)->Add<kStaf64>(GetStackOffset(dest));
            } else {
                current_fun_->Incoming(ast)->Add<kStar64>(GetStackOffset(dest));
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
        DCHECK_GE(index, 0);
        if (clazz->is_reference()) {
            current_fun_->Incoming(ast)->Incomplete<kMovePtr>(-(dest), GetStackOffset(index));
            return;
        }
        switch (clazz->reference_size()) {
            case 1:
            case 2:
            case 4:
                current_fun_->Incoming(ast)->Incomplete<kMove32>(-(dest), GetStackOffset(index));
                return;
            case 8:
                current_fun_->Incoming(ast)->Incomplete<kMove64>(-(dest), GetStackOffset(index));
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
        current_fun_->Incoming(ast)->Incomplete<kStarPtr>(-(dest));
        return;
    }
    switch (clazz->reference_size()) {
        case 1:
        case 2:
        case 4:
            if (clazz->id() == kType_f32) {
                current_fun_->Incoming(ast)->Incomplete<kStaf32>(-(dest));
            } else {
                current_fun_->Incoming(ast)->Incomplete<kStar32>(-(dest));
            }
            break;
        case 8:
            if (clazz->id() == kType_f64) {
                current_fun_->Incoming(ast)->Incomplete<kStaf64>(-(dest));
            } else {
                current_fun_->Incoming(ast)->Incomplete<kStar64>(-(dest));
            }
            break;
        default:
            NOREACHED();
            break;
    }
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
        current_fun_->Incoming(ast)->Add<kLdarPtr>(offset);
        return;
    }
    switch (clazz->reference_size()) {
        case 1:
        case 2:
        case 4:
            if (clazz->id() == kType_f32) {
                current_fun_->Incoming(ast)->Add<kLdaf32>(offset);
            } else {
                current_fun_->Incoming(ast)->Add<kLdar32>(offset);
            }
            break;
        case 8:
            if (clazz->id() == kType_f64) {
                current_fun_->Incoming(ast)->Add<kLdaf64>(offset);
            } else {
                current_fun_->Incoming(ast)->Add<kLdar64>(offset);
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
        current_fun_->Incoming(ast)->Add<kLdaConstPtr>(offset);
        return;
    }
    switch (clazz->reference_size()) {
        case 1:
        case 2:
        case 4:
            if (clazz->id() == kType_f32) {
                current_fun_->Incoming(ast)->Add<kLdaConstf32>(offset);
            } else {
                current_fun_->Incoming(ast)->Add<kLdaConst32>(offset);
            }
            break;
        case 8:
            if (clazz->id() == kType_f64) {
                current_fun_->Incoming(ast)->Add<kLdaConstf64>(offset);
            } else {
                current_fun_->Incoming(ast)->Add<kLdaConst64>(offset);
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
                current_fun_->Incoming(ast)->Add<kLdaGlobalf32>(offset);
            } else {
                current_fun_->Incoming(ast)->Add<kLdaGlobal32>(offset);
            }
            break;
        case 8:
            if (clazz->id() == kType_f64) {
                current_fun_->Incoming(ast)->Add<kLdaGlobalf64>(offset);
            } else {
                current_fun_->Incoming(ast)->Add<kLdaGlobal64>(offset);
            }
            break;
        default:
            NOREACHED();
            break;
    }
}

void BytecodeGenerator::LdaCaptured(const Class *clazz, int index, ASTNode *ast) {
    if (clazz->is_reference()) {
        current_fun_->Incoming(ast)->Add<kLdaCapturedPtr>(index);
        return;
    }
    switch (clazz->reference_size()) {
        case 1:
        case 2:
        case 4:
            if (clazz->id() == kType_f32) {
                current_fun_->Incoming(ast)->Add<kLdaCapturedf32>(index);
            } else {
                current_fun_->Incoming(ast)->Add<kLdaCaptured32>(index);
            }
            break;
        case 8:
            if (clazz->id() == kType_f64) {
                current_fun_->Incoming(ast)->Add<kLdaCapturedf64>(index);
            } else {
                current_fun_->Incoming(ast)->Add<kLdaCaptured64>(index);
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
        current_fun_->Incoming(ast)->Add<kStarPtr>(offset);
        return;
    }
    switch (clazz->reference_size()) {
        case 1:
        case 2:
        case 4:
            if (clazz->id() == kType_f32) {
                current_fun_->Incoming(ast)->Add<kStaf32>(offset);
            } else {
                current_fun_->Incoming(ast)->Add<kStar32>(offset);
            }
            break;
        case 8:
            if (clazz->id() == kType_f64) {
                current_fun_->Incoming(ast)->Add<kStaf64>(offset);
            } else {
                current_fun_->Incoming(ast)->Add<kStar64>(offset);
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
        current_fun_->Incoming(ast)->Add<kStaGlobalPtr>(offset);
        return;
    }
    switch (clazz->reference_size()) {
        case 1:
        case 2:
        case 4:
            if (clazz->id() == kType_f32) {
                current_fun_->Incoming(ast)->Add<kStaGlobalf32>(offset);
            } else {
                current_fun_->Incoming(ast)->Add<kStaGlobal32>(offset);
            }
            break;
        case 8:
            if (clazz->id() == kType_f64) {
                current_fun_->Incoming(ast)->Add<kStaGlobalf64>(offset);
            } else {
                current_fun_->Incoming(ast)->Add<kStaGlobal64>(offset);
            }
            break;
        default:
            NOREACHED();
            break;
    }
}

void BytecodeGenerator::StaCaptured(const Class *clazz, int index, ASTNode *ast) {
    if (clazz->is_reference()) {
        current_fun_->Incoming(ast)->Add<kStaCapturedPtr>(index);
        return;
    }
    switch (clazz->reference_size()) {
        case 1:
        case 2:
        case 4:
            if (clazz->id() == kType_f32) {
                current_fun_->Incoming(ast)->Add<kStaCapturedf32>(index);
            } else {
                current_fun_->Incoming(ast)->Add<kStaCaptured32>(index);
            }
            break;
        case 8:
            if (clazz->id() == kType_f64) {
                current_fun_->Incoming(ast)->Add<kStaCapturedf64>(index);
            } else {
                current_fun_->Incoming(ast)->Add<kStaCaptured64>(index);
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
        current_fun_->Incoming(ast)->Add<kStaPropertyPtr>(index, offset);
        return;
    }
    switch (clazz->reference_size()) {
        case 1:
            current_fun_->Incoming(ast)->Add<kStaProperty8>(index, offset);
            break;
        case 2:
            current_fun_->Incoming(ast)->Add<kStaProperty16>(index, offset);
            break;
        case 4:
            if (clazz->id() == kType_f32) {
                current_fun_->Incoming(ast)->Add<kStaPropertyf32>(index, offset);
            } else {
                current_fun_->Incoming(ast)->Add<kStaProperty32>(index, offset);
            }
            break;
        case 8:
            if (clazz->id() == kType_f64) {
                current_fun_->Incoming(ast)->Add<kStaPropertyf64>(index, offset);
            } else {
                current_fun_->Incoming(ast)->Add<kStaProperty64>(index, offset);
            }
            break;
        default:
            NOREACHED();
            break;
    }
}

} // namespace lang

} // namespace mai
