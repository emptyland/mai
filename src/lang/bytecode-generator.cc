#include "lang/bytecode-generator.h"
#include "lang/bytecode-array-builder.h"
#include "lang/machine.h"
#include "lang/isolate-inl.h"
#include "lang/metadata-space.h"
#include "lang/metadata.h"
#include "base/arenas.h"

namespace mai {

namespace lang {

class BytecodeGenerator::AbstractScope {
public:
    enum Kind {
        kFileScope,
        kClassScope,
        kFunctionScope,
        kLoopBlockScope,
        kPlainBlockScope,
    };
    
    bool is_file_scope() const { return kind_ == kFileScope; }
    bool is_class_scope() const { return kind_ == kClassScope; }
    bool is_function_scope() const { return kind_ == kFunctionScope; }
    
    virtual Value Find(const ASTString *name) { return {Value::kError}; }
    virtual void Register(const std::string &name, Value value) {}

    std::tuple<AbstractScope *, Value> Resolve(const ASTString *name) {
        for (auto i = this; i != nullptr; i = i->prev_) {
            if (Value value = i->Find(name); value.linkage != Value::kError) {
                return {i, value};
            }
        }
        return {nullptr, {Value::kError}};
    }
    
    AbstractScope *GetScope(Kind kind) {
        for (AbstractScope *scope = this; scope != nullptr; scope = scope->prev_) {
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
    
    DISALLOW_IMPLICIT_CONSTRUCTORS(AbstractScope);
protected:
    AbstractScope(Kind kind, AbstractScope **current)
        : kind_(kind)
        , prev_(*current)
        , current_(current) {
        DCHECK_NE(*current_, this);
        *current_ = this;
    }
    
    ~AbstractScope() {
        DCHECK_EQ(*current_, this);
        *current_ = prev_;
    }

    Kind kind_;
    AbstractScope *prev_;
    AbstractScope **current_;
}; // class BytecodeGenerator::AbstractScope

class BytecodeGenerator::FileScope : public AbstractScope {
public:
    FileScope(const std::map<std::string, std::vector<FileUnit *>> &path_units,
              std::map<std::string, Value> *symbols,
              FileUnit *file_unit, AbstractScope **current, FileScope **current_file);
    
    ~FileScope() {
        DCHECK_EQ(this, *current_file_);
        *current_file_ = nullptr;
    }
    
    DEF_PTR_GETTER(FileUnit, file_unit);
    
    Value Find(const ASTString *prefix, const ASTString *name);
    Value Find(const ASTString *name) override {
        std::string_view exists;
        return FindExcludePackageName(name, "", &exists);
    }
    Value FindExcludePackageName(const ASTString *name, std::string_view exclude,
                                 std::string_view *exists);

    DISALLOW_IMPLICIT_CONSTRUCTORS(FileScope);
private:
    FileUnit *const file_unit_;
    std::map<std::string, Value> *symbols_;
    std::set<std::string> all_name_space_;
    std::map<std::string, std::string> alias_name_space_;
    FileScope **current_file_;
}; // class BytecodeGenerator::FileScope

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

BytecodeGenerator::FileScope::FileScope(const std::map<std::string, std::vector<FileUnit *>> &path_units,
                                        std::map<std::string, Value> *symbols, FileUnit *file_unit,
                                        AbstractScope **current, FileScope **current_file)
    : AbstractScope(kFileScope, current)
    , file_unit_(file_unit)
    , symbols_(symbols) {
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
    DCHECK(*current_file == nullptr);
    *current_file = this;
}

class BytecodeGenerator::ClassScope : public AbstractScope {
public:
    ClassScope(StructureDefinition *clazz, AbstractScope **current, ClassScope **current_class)
        : AbstractScope(kClassScope, current)
        , clazz_(clazz)
        , current_class_(current_class) {
        DCHECK_NE(this, *current_class_);
    }

    ~ClassScope() {
        DCHECK_EQ(this, *current_class_);
        *current_class_ = !prev_ ? nullptr : prev_->GetClassScope();
    }
    
    DEF_PTR_GETTER(StructureDefinition, clazz);
    
    DISALLOW_IMPLICIT_CONSTRUCTORS(ClassScope);
private:
    StructureDefinition *clazz_;
    ClassScope **current_class_;
}; // class BytecodeGenerator::ClassScope


class BytecodeGenerator::BlockScope : public AbstractScope {
public:
    BlockScope(Kind kind, AbstractScope **current): AbstractScope(kind, current) {}
    
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
    FunctionScope(base::Arena *arena, FileUnit *file_unit, AbstractScope **current,
                  FunctionScope **current_fun)
        : BlockScope(kFunctionScope, current)
        , file_unit_(file_unit)
        , builder_(arena)
        , current_fun_(current_fun) {
        DCHECK_NE(this, *current_fun_);
    }
    
    ~FunctionScope() {
        DCHECK_EQ(this, *current_fun_);
        *current_fun_ = !prev_ ? nullptr : prev_->GetFunctionScope();
    }

    DEF_PTR_PROP_RW(FileUnit, file_unit);
    DEF_VAL_GETTER(std::vector<int>, source_lines);
    BytecodeArrayBuilder *builder() { return &builder_; }
    ConstantPoolBuilder *constants() { return &constants_; }
    StackSpaceAllocator *stack() { return &stack_; }
    
    BytecodeArrayBuilder *Incoming(ASTNode *ast) {
        SourceLocation loc = file_unit_->FindSourceLocation(ast);
        source_lines_.push_back(loc.begin_line);
        return &builder_;
    }
    
    BytecodeArrayBuilder *IncomingWithLine(int line) {
        source_lines_.push_back(line);
        return &builder_;
    }

    ConstantPoolBuilder *constant_pool() { return &constants_; }
    
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

} // namespace

BytecodeGenerator::BytecodeGenerator(Isolate *isolate,
                                     SyntaxFeedback *feedback,
                                     ClassDefinition *class_exception,
                                     ClassDefinition *class_any,
                                     std::map<std::string, std::vector<FileUnit *>> &&path_units,
                                     std::map<std::string, std::vector<FileUnit *>> &&pkg_units)
    : isolate_(isolate)
    , metadata_space_(isolate->metadata_space())
    , error_feedback_(DCHECK_NOTNULL(feedback))
    , class_exception_(class_exception)
    , class_any_(class_any)
    , path_units_(std::move(path_units))
    , pkg_units_(std::move(pkg_units)) {
}

BytecodeGenerator::BytecodeGenerator(Isolate *isolate, SyntaxFeedback *feedback)
    : isolate_(isolate)
    , error_feedback_(DCHECK_NOTNULL(feedback))
    , class_exception_(nullptr)
    , class_any_(nullptr) {
}

bool BytecodeGenerator::Prepare() {
    symbol_trace_.insert(class_any_); // ignore class any
    
    for (auto pair : pkg_units_) {
        const std::string &pkg_name = pair.first;
        
        for (auto unit : pair.second) {
            error_feedback_->set_file_name(unit->file_name()->ToString());
            error_feedback_->set_package_name(unit->package_name()->ToString());
            if (!PrepareUnit(pkg_name, unit)) {
                return false;
            }
        }
    }
    
    for (auto pair : pkg_units_) {
        const std::string &pkg_name = pair.first;
        
        for (auto unit : pair.second) {
            FileScope file_scope(path_units_, &symbols_, unit, &current_, &current_file_);

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
                    const Class *clazz = metadata_space_->type(rv.bundle.index);
                    current_->Register(name, {Value::kGlobal, clazz, index});
                }
            }
        }
    }
    return true;
}

bool BytecodeGenerator::Generate() {

    isolate_->SetGlobalSpace(global_space_.TakeSpans(), global_space_.TakeBitmap(),
                             global_space_.capacity(), global_space_.length());
    return true;
}

bool BytecodeGenerator::PrepareUnit(const std::string &pkg_name, FileUnit *unit) {
    FileScope file_scope(path_units_, &symbols_, unit, &current_, &current_file_);

    for (auto ast : unit->definitions()) {
        std::string name = pkg_name + "." + ast->identifier()->ToString();
        
        if (auto clazz_ast = ast->AsClassDefinition(); ast != class_any_/*filter class Any*/) {
            Class *clazz = metadata_space_->PrepareClass();
            clazz_ast->set_clazz(clazz);
            current_->Register(name, {Value::kMetadata, clazz, static_cast<int>(clazz->id())});
        } else if (auto object_ast = ast->AsObjectDefinition()) {
            Class *clazz = metadata_space_->PrepareClass();
            object_ast->set_clazz(clazz);
            // object foo ==> main.foo$class
            current_->Register(name + "$class", {Value::kMetadata, clazz, static_cast<int>(clazz->id())});
            int index = global_space_.ReserveRef();
            current_->Register(name, {Value::kGlobal, clazz, index});
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
                current_->Register(name, {Value::kGlobal, clazz, index});
            } else {
                int index = global_space_.ReserveRef();
                current_->Register(name, {Value::kGlobal, clazz, index});
            }
        }
    }
    
    for (auto impl : unit->implements()) {
        for (auto ast : impl->methods()) {
            std::string name = pkg_name + "." + impl->owner()->identifier()->ToString() + "::" +
                ast->identifier()->ToString();
            const Class *clazz = metadata_space_->builtin_type(kType_closure);
            int index = global_space_.ReserveRef();
            current_->Register(name, {Value::kGlobal, clazz, index});
        }
    }
    return true;
}

Function *BytecodeGenerator::BuildFunction(const std::string &name, FunctionScope *scope) {
    const char *file_name = scope->file_unit() ? scope->file_unit()->file_name()->data() : "init0";
    SourceLineInfo *source_info = metadata_space_->NewSourceLineInfo(file_name,
                                                                     scope->source_lines());
    BytecodeArray *bytecodes = metadata_space_->NewBytecodeArray(scope->builder()->Build());
    ConstantPoolBuilder *const_pool = scope->constant_pool();
    StackSpaceAllocator *stack = scope->stack();

    return FunctionBuilder(name)
        // TODO:
        .stack_size(stack->GetMaxStackSize())
        .stack_bitmap(stack->bitmap())
        .prototype({}/*parameters*/, false/*vargs*/, kType_void)
        .source_line_info(source_info)
        .bytecode(bytecodes)
        .const_pool(const_pool->spans(), const_pool->length())
        .const_pool_bitmap(const_pool->bitmap(), (const_pool->length() + 31) / 32)
        .Build(metadata_space_);
}

ASTVisitor::Result BytecodeGenerator::VisitTypeSign(TypeSign *ast) /*override*/ {
    // TODO:
    TODO();
    return ResultWith(Value::kMetadata, ast->ToBuiltinType(), 0/*TODO*/);
}

ASTVisitor::Result BytecodeGenerator::VisitClassDefinition(ClassDefinition *ast) /*override*/ {
    std::string name(DCHECK_NOTNULL(current_file_)->file_unit()->package_name()->ToString());
    name.append(".").append(ast->identifier()->ToString());
    symbol_trace_.insert(ast);

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
    int tag = 1;
    for (auto field : ast->fields()) {
        auto field_builder = builder.field(field.declaration->identifier()->ToString());
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
        const Class *field_class = metadata_space_->type(rv.bundle.index);
        if ((offset + field_class->reference_size()) % 2 == 0) {
            field_builder.offset(offset);
            offset += field_class->reference_size();
        } else {
            field_builder.offset(RoundUp(offset, 2));
            offset = RoundUp(offset, 2) + field_class->reference_size();
        }
        field_builder
            .tag(tag++)
            .flags(flags)
            .End();
    }
    
    for (auto method : ast->methods()) {
        auto method_builder = builder.method(method->identifier()->ToString());
        // TODO:
    }
    
    base::StandaloneArena arena(isolate_->env()->GetLowLevelAllocator());
    FunctionScope function_scope(&arena, current_file_->file_unit(), &current_, &current_fun_);
    // TODO:

    builder
        .tags(Type::kReferenceTag)
        .reference_size(kPointerSize)
        .instrance_size(offset)
        .BuildWithPreparedClass(metadata_space_, ast->clazz());

    symbol_trace_.insert(ast);
    return ResultWithVoid();
}

ASTVisitor::Result BytecodeGenerator::VisitObjectDefinition(ObjectDefinition *ast) /*override*/ {
    // TODO:
    return ResultWithError();
}

ASTVisitor::Result BytecodeGenerator::VisitFunctionDefinition(FunctionDefinition *ast) /*override*/ {
    //if (cur)
    return ResultWithError();
}

ASTVisitor::Result BytecodeGenerator::VisitVariableDeclaration(VariableDeclaration *ast) /*override*/ {
    if (current_->is_file_scope()) { // It's global variable
        int index = LinkGlobalVariable(ast);
        if (!HasGenerated(ast)) {
            if (ast->initializer()) {
                ASTVisitor::Result rv = ast->initializer()->Accept(this);
                if (rv.kind == Value::kError) {
                    return ResultWithError();
                }
                const Class *clazz = metadata_space_->type(rv.bundle.type);
                LdaIfNeeded(clazz, rv.bundle.index, rv.kind, current_fun_->Incoming(ast->initializer()));
                StaGlobal(clazz, index, current_fun_->Incoming(ast));
            }
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
    
    Value local{Value::kStack, clazz, 0};
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
    LdaIfNeeded(clazz, rv.bundle.index, rv.kind, current_fun_->Incoming(ast->initializer()));
    StaStack(clazz, local.index, current_fun_->Incoming(ast));
    return ResultWithVoid();
}

ASTVisitor::Result BytecodeGenerator::VisitBoolLiteral(BoolLiteral *ast) /*override*/ {
    int index = current_fun_->constant_pool()->FindOrInsertI32(ast->value());
    return ResultWith(Value::kConstant, kType_bool, index);
}

ASTVisitor::Result BytecodeGenerator::VisitI8Literal(I8Literal *ast) /*override*/ {
    int index = current_fun_->constant_pool()->FindOrInsertI32(ast->value());
    return ResultWith(Value::kConstant, kType_i8, index);
}

ASTVisitor::Result BytecodeGenerator::VisitU8Literal(U8Literal *ast) /*override*/ {
    int index = current_fun_->constant_pool()->FindOrInsertU32(ast->value());
    return ResultWith(Value::kConstant, kType_u8, index);
}

ASTVisitor::Result BytecodeGenerator::VisitI16Literal(I16Literal *ast) /*override*/ {
    int index = current_fun_->constant_pool()->FindOrInsertI32(ast->value());
    return ResultWith(Value::kConstant, kType_i16, index);
}

ASTVisitor::Result BytecodeGenerator::VisitU16Literal(U16Literal *ast) /*override*/ {
    int index = current_fun_->constant_pool()->FindOrInsertU32(ast->value());
    return ResultWith(Value::kConstant, kType_u16, index);
}

ASTVisitor::Result BytecodeGenerator::VisitI32Literal(I32Literal *ast) /*override*/ {
    int index = current_fun_->constant_pool()->FindOrInsertI32(ast->value());
    return ResultWith(Value::kConstant, kType_i32, index);
}

ASTVisitor::Result BytecodeGenerator::VisitU32Literal(U32Literal *ast) /*override*/ {
    int index = current_fun_->constant_pool()->FindOrInsertU32(ast->value());
    return ResultWith(Value::kConstant, kType_u32, index);
}

ASTVisitor::Result BytecodeGenerator::VisitIntLiteral(IntLiteral *ast) /*override*/ {
    int index = current_fun_->constant_pool()->FindOrInsertI32(ast->value());
    return ResultWith(Value::kConstant, kType_int, index);
}

ASTVisitor::Result BytecodeGenerator::VisitUIntLiteral(UIntLiteral *ast) /*override*/ {
    int index = current_fun_->constant_pool()->FindOrInsertU32(ast->value());
    return ResultWith(Value::kConstant, kType_uint, index);
}

ASTVisitor::Result BytecodeGenerator::VisitI64Literal(I64Literal *ast) /*override*/ {
    int index = current_fun_->constant_pool()->FindOrInsertI64(ast->value());
    return ResultWith(Value::kConstant, kType_i64, index);
}

ASTVisitor::Result BytecodeGenerator::VisitU64Literal(U64Literal *ast) /*override*/ {
    int index = current_fun_->constant_pool()->FindOrInsertU64(ast->value());
    return ResultWith(Value::kConstant, kType_u64, index);
}

ASTVisitor::Result BytecodeGenerator::VisitF32Literal(F32Literal *ast) /*override*/ {
    int index = current_fun_->constant_pool()->FindOrInsertF32(ast->value());
    return ResultWith(Value::kConstant, kType_f32, index);
}

ASTVisitor::Result BytecodeGenerator::VisitF64Literal(F64Literal *ast) /*override*/ {
    int index = current_fun_->constant_pool()->FindOrInsertF64(ast->value());
    return ResultWith(Value::kConstant, kType_f64, index);
}

ASTVisitor::Result BytecodeGenerator::VisitNilLiteral(NilLiteral *ast) /*override*/ {
    int index = current_fun_->constant_pool()->FindOrInsertU64(0);
    return ResultWith(Value::kConstant, kType_any, index);
}

ASTVisitor::Result BytecodeGenerator::VisitStringLiteral(StringLiteral *ast) /*override*/ {
    std::string_view key = ast->value()->ToSlice();
    int index = current_fun_->constant_pool()->FindString(key);
    if (index >= 0) {
        return ResultWith(Value::kConstant, kType_string, index);
    }
    String *s = Machine::This()->NewUtf8String(key.data(), key.size(), Heap::kOld);
    index = current_fun_->constant_pool()->FindOrInsertString(s);
    return ResultWith(Value::kConstant, kType_string, index);
}

SourceLocation BytecodeGenerator::FindSourceLocation(const ASTNode *ast) {
    return DCHECK_NOTNULL(current_file_)->file_unit()->FindSourceLocation(ast);
}

//ASTVisitor::Result BytecodeGenerator::GenerateSymbolDependence(Symbolize *sym) {
//    Result rv;
//    if (sym->file_unit() && current_->GetFileScope()->file_unit() != sym->file_unit()) {
//        auto saved = current_;
//        auto saved_file = current_file_;
//        auto saved_class = current_class_;
//        auto saved_fun = current_fun_;
//        current_ = nullptr;
//        current_file_ = nullptr;
//        current_fun_ = nullptr;
//        current_class_ = nullptr;
//        if (current_->is_file_scope()) {
//            current_fun_ = saved_fun;
//            current_ = saved_fun;
//        }
//        do {
//            FileScope file_scope(path_units_, &symbols_, sym->file_unit(), &current_,
//                                 &current_file_);
//            if (rv = sym->Accept(this); rv.kind == Value::kError) {
//                return ResultWithError();
//            }
//        } while (0);
//        DCHECK(current_ == nullptr);
//        current_ = saved;
//        current_file_ = saved_file;
//        current_class_ = saved_class;
//        current_fun_ = saved_fun;
//    } else {
//        if (rv = sym->Accept(this); rv.kind == Value::kError) {
//            return ResultWithError();
//        }
//    }
//    return rv;
//}

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

void BytecodeGenerator::LdaIfNeeded(const Class *clazz, int index, int linkage,
                                    BytecodeArrayBuilder *emitter) {
    switch (static_cast<Value::Linkage>(linkage)) {
        case Value::kConstant:
            LdaConst(clazz, index, emitter);
            break;
        case Value::kStack:
            LdaStack(clazz, index, emitter);
            break;
        case Value::kGlobal:
            LdaGlobal(clazz, index, emitter);
            break;
        case Value::kCaptured:
            LdaCaptured(clazz, index, emitter);
            break;
        case Value::kACC: // ignore
        case Value::kMetadata:
        case Value::kError:
        default:
            NOREACHED();
            break;
    }
}

void BytecodeGenerator::LdaStack(const Class *clazz, int index, BytecodeArrayBuilder *emitter) {
    DCHECK_EQ(0, index % kStackOffsetGranularity);
    int offset = index / kStackOffsetGranularity;
    if (clazz->is_reference()) {
        emitter->Add<kLdarPtr>(offset);
        return;
    }
    switch (clazz->reference_size()) {
        case 1:
        case 2:
        case 4:
            if (clazz->id() == kType_f32) {
                emitter->Add<kLdaf32>(offset);
            } else {
                emitter->Add<kLdar32>(offset);
            }
            break;
        case 8:
            if (clazz->id() == kType_f64) {
                emitter->Add<kLdaf64>(offset);
            } else {
                emitter->Add<kLdar64>(offset);
            }
            break;
        default:
            NOREACHED();
            break;
    }
}

void BytecodeGenerator::LdaConst(const Class *clazz, int index, BytecodeArrayBuilder *emitter) {
    DCHECK_EQ(0, index % kConstPoolOffsetGranularity);
    int offset = index / kConstPoolOffsetGranularity;
    if (clazz->is_reference()) {
        emitter->Add<kLdaConstPtr>(offset);
        return;
    }
    switch (clazz->reference_size()) {
        case 1:
        case 2:
        case 4:
            if (clazz->id() == kType_f32) {
                emitter->Add<kLdaConstf32>(offset);
            } else {
                emitter->Add<kLdaConst32>(offset);
            }
            break;
        case 8:
            if (clazz->id() == kType_f64) {
                emitter->Add<kLdaConstf64>(offset);
            } else {
                emitter->Add<kLdaConst64>(offset);
            }
            break;
        default:
            NOREACHED();
            break;
    }
}

void BytecodeGenerator::LdaGlobal(const Class *clazz, int index, BytecodeArrayBuilder *emitter) {
    DCHECK_EQ(0, index % kGlobalSpaceOffsetGranularity);
    int offset = index / kGlobalSpaceOffsetGranularity;
    if (clazz->is_reference()) {
        emitter->Add<kLdaGlobalPtr>(offset);
        return;
    }
    switch (clazz->reference_size()) {
        case 1:
        case 2:
        case 4:
            if (clazz->id() == kType_f32) {
                emitter->Add<kLdaGlobalf32>(offset);
            } else {
                emitter->Add<kLdaGlobal32>(offset);
            }
            break;
        case 8:
            if (clazz->id() == kType_f64) {
                emitter->Add<kLdaGlobalf64>(offset);
            } else {
                emitter->Add<kLdaGlobal64>(offset);
            }
            break;
        default:
            NOREACHED();
            break;
    }
}

void BytecodeGenerator::LdaCaptured(const Class *clazz, int index, BytecodeArrayBuilder *emitter) {
    if (clazz->is_reference()) {
        emitter->Add<kLdaCapturedPtr>(index);
        return;
    }
    switch (clazz->reference_size()) {
        case 1:
        case 2:
        case 4:
            if (clazz->id() == kType_f32) {
                emitter->Add<kLdaCapturedf32>(index);
            } else {
                emitter->Add<kLdaCaptured32>(index);
            }
            break;
        case 8:
            if (clazz->id() == kType_f64) {
                emitter->Add<kLdaCapturedf64>(index);
            } else {
                emitter->Add<kLdaCaptured64>(index);
            }
            break;
        default:
            NOREACHED();
            break;
    }
}

void BytecodeGenerator::StaIfNeeded(const Class *clazz, int index, int linkage,
                                    BytecodeArrayBuilder *emitter) {
    switch (static_cast<Value::Linkage>(linkage)) {
        case Value::kStack:
            StaStack(clazz, index, emitter);
            break;
        case Value::kGlobal:
            StaGlobal(clazz, index, emitter);
            break;
        case Value::kCaptured:
            StaCaptured(clazz, index, emitter);
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

void BytecodeGenerator::StaStack(const Class *clazz, int index, BytecodeArrayBuilder *emitter) {
    DCHECK_EQ(0, index % kStackOffsetGranularity);
    int offset = index / kStackOffsetGranularity;
    if (clazz->is_reference()) {
        emitter->Add<kStarPtr>(offset);
        return;
    }
    switch (clazz->reference_size()) {
        case 1:
        case 2:
        case 4:
            if (clazz->id() == kType_f32) {
                emitter->Add<kStaf32>(offset);
            } else {
                emitter->Add<kStar32>(offset);
            }
            break;
        case 8:
            if (clazz->id() == kType_f64) {
                emitter->Add<kStaf64>(offset);
            } else {
                emitter->Add<kStar64>(offset);
            }
            break;
        default:
            NOREACHED();
            break;
    }
}

void BytecodeGenerator::StaGlobal(const Class *clazz, int index, BytecodeArrayBuilder *emitter) {
    DCHECK_EQ(0, index % kGlobalSpaceOffsetGranularity);
    int offset = index / kGlobalSpaceOffsetGranularity;
    if (clazz->is_reference()) {
        emitter->Add<kStaGlobalPtr>(offset);
        return;
    }
    switch (clazz->reference_size()) {
        case 1:
        case 2:
        case 4:
            if (clazz->id() == kType_f32) {
                emitter->Add<kStaGlobalf32>(offset);
            } else {
                emitter->Add<kStaGlobal32>(offset);
            }
            break;
        case 8:
            if (clazz->id() == kType_f64) {
                emitter->Add<kStaGlobalf64>(offset);
            } else {
                emitter->Add<kStaGlobal64>(offset);
            }
            break;
        default:
            NOREACHED();
            break;
    }
}

void BytecodeGenerator::StaCaptured(const Class *clazz, int index, BytecodeArrayBuilder *emitter) {
    if (clazz->is_reference()) {
        emitter->Add<kStaCapturedPtr>(index);
        return;
    }
    switch (clazz->reference_size()) {
        case 1:
        case 2:
        case 4:
            if (clazz->id() == kType_f32) {
                emitter->Add<kStaCapturedf32>(index);
            } else {
                emitter->Add<kStaCaptured32>(index);
            }
            break;
        case 8:
            if (clazz->id() == kType_f64) {
                emitter->Add<kStaCapturedf64>(index);
            } else {
                emitter->Add<kStaCaptured64>(index);
            }
            break;
        default:
            NOREACHED();
            break;
    }
}

} // namespace lang

} // namespace mai
