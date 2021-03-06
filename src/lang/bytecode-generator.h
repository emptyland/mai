#pragma once
#ifndef MAI_LANG_BYTECODE_GENERATOR_H_
#define MAI_LANG_BYTECODE_GENERATOR_H_

#include "lang/stable-space-builder.h"
#include "lang/ast.h"
#include "lang/syntax.h"
#include <map>
#include <set>
#include <unordered_set>
#include <unordered_map>

namespace mai {

namespace lang {

class Isolate;
class BytecodeArrayBuilder;

class BytecodeGenerator final : public PartialASTVisitor {
public:
    struct Value {
        enum Linkage {
            kError = 0, // Error flag
            kMetadata,
            kGlobal,
            kStack,
            kConstant,
            kCaptured,
            kACC,
        }; // enum Linkage
        Linkage linkage = kError;
        const Class *type = nullptr;
        int index = 0;
        Symbolize *ast = nullptr;
        uint32_t flags = 0;
        
        static Value Of(const Result &rv, BytecodeGenerator *g);
    }; // struct BytecodeGenerator::Value

    BytecodeGenerator(Isolate *isolate, SyntaxFeedback *feedback,
                      ClassDefinition *class_exception,
                      ClassDefinition *class_object,
                      std::map<std::string, std::vector<FileUnit *>> &&path_units,
                      std::map<std::string, std::vector<FileUnit *>> &&pkg_units,
                      base::Arena *arena);
    
    BytecodeGenerator(Isolate *isolate, SyntaxFeedback *feedback, base::Arena *arena);
    
    ~BytecodeGenerator();
    
    bool Prepare();
    bool Generate();
    
    DEF_PTR_GETTER(Function, generated_init0_fun);
    
    Value FindValue(const std::string &name) const {
        if (auto iter = symbols_.find(name); iter != symbols_.end()) {
            return iter->second;
        } else {
            return {Value::kError};
        }
    }
    
    template<class T>
    inline T *FindGlobalOrNull(const std::string &name) {
        Value value = FindValue(name);
        if (value.linkage == Value::kError) {
            return nullptr;
        }
        DCHECK_EQ(Value::kGlobal, value.linkage);
        return reinterpret_cast<T *>(global_space_.address(value.index));
    }
private:
    class Scope;
    class FileScope;
    class ClassScope;
    class FunctionScope;
    class BlockScope;
    
    struct FieldDesc {
        const ASTString *name;
        const Class *type;
        uint32_t offset;
    };
    
    struct CallingReceiver {
        enum Kind {
            kVtab,
            kCtor,
            kNative,
            kBytecode,
        };
        Kind kind;
        uint32_t attributes;
        ASTNode *ast;
        int arguments_size;
    }; // struct CallingReceiver
    
    struct OperandContext {
        int lhs = 0;
        bool lhs_tmp = false;
        int rhs = 0;
        bool rhs_tmp = false;
        bool rhs_pair = false;
        const Class *lhs_type = nullptr;
        const Class *rhs_type = nullptr;
    }; // struct OperandContext
    
    bool PrepareUnit(const std::string &pkg_name, FileUnit *unit);
    bool GenerateUnit(const std::string &pkg_name, FileUnit *unit);

    Function *BuildFunction(const std::string &name, FunctionScope *scope);
    
    Result VisitTypeSign(TypeSign *) override;
    Result VisitClassDefinition(ClassDefinition *) override;
    Result VisitObjectDefinition(ObjectDefinition *) override;
    Result VisitFunctionDefinition(FunctionDefinition *) override;
    Result VisitLambdaLiteral(LambdaLiteral *) override;
    Result VisitVariableDeclaration(VariableDeclaration *) override;
    Result VisitCallExpression(CallExpression *) override;
    Result VisitDotExpression(DotExpression *) override;
    Result VisitStringTemplateExpression(StringTemplateExpression *) override;
    Result VisitUnaryExpression(UnaryExpression *) override;
    Result VisitBinaryExpression(BinaryExpression *) override;
    Result VisitWhenExpression(WhenExpression *) override;
    Result VisitIfExpression(IfExpression *) override;
    Result VisitStatementBlock(StatementBlock *) override;
    Result VisitArrayInitializer(ArrayInitializer *) override;
    Result VisitMapInitializer(MapInitializer *) override;
    Result VisitChannelInitializer(ChannelInitializer *) override;
    Result VisitIndexExpression(IndexExpression *) override;
    Result VisitTypeCastExpression(TypeCastExpression *) override;
    Result VisitTypeTestExpression(TypeTestExpression *) override;
    Result VisitIdentifier(Identifier *) override;
    Result VisitBoolLiteral(BoolLiteral *) override;
    Result VisitI8Literal(I8Literal *) override;
    Result VisitU8Literal(U8Literal *) override;
    Result VisitI16Literal(I16Literal *) override;
    Result VisitU16Literal(U16Literal *) override;
    Result VisitI32Literal(I32Literal *) override;
    Result VisitU32Literal(U32Literal *) override;
    Result VisitIntLiteral(IntLiteral *) override;
    Result VisitUIntLiteral(UIntLiteral *) override;
    Result VisitI64Literal(I64Literal *) override;
    Result VisitU64Literal(U64Literal *) override;
    Result VisitF32Literal(F32Literal *) override;
    Result VisitF64Literal(F64Literal *) override;
    Result VisitNilLiteral(NilLiteral *) override;
    Result VisitStringLiteral(StringLiteral *) override;
    Result VisitBreakableStatement(BreakableStatement *) override;
    Result VisitTryCatchFinallyBlock(TryCatchFinallyBlock *) override;
    Result VisitAssignmentStatement(AssignmentStatement *) override;
    Result VisitRunStatement(RunStatement *) override;
    Result VisitWhileLoop(WhileLoop *) override;
    Result VisitForLoop(ForLoop *) override;
    
    Result GenerateForStep(ForLoop *ast);
    Result GenerateForEach(ForLoop *ast);
    Result GenerateCalling(CallExpression *ast, CallingReceiver *receiver);
    uint32_t ProcessStructure(StructureDefinition *ast, uint32_t offset, ClassBuilder *builder,
                              std::vector<FieldDesc> *fields_desc);
    
    Result GenerateTestIs(const Value &operand, TypeSign *dest, ASTNode *ast);
    Result GenerateTestAs(const Value &operand, const Class *dest_type, TypeSign *dest, ASTNode *ast);
    
    void GenerateTypeTest(const Value &operand, const char *external, const Class *dest_type,
                          const MetadataObject *extra, ASTNode *ast);

    bool ShouldCaptureVar(Scope *owns, Value value);
    Result CaptureVar(const std::string &name, Scope *owns, Value value);
    SourceLocation FindSourceLocation(const ASTNode *ast);
    
    Function *GenerateLambdaLiteral(const std::string &name, bool is_method, LambdaLiteral *ast);
    Function *GenerateClassConstructor(const std::vector<FieldDesc> &fields_desc, const Class *base,
                                       ClassDefinition *ast);
    Function *GenerateObjectConstructor(const std::vector<FieldDesc> &fields_desc,
                                        ObjectDefinition *ast);
    bool ProcessStructureInitializer(const std::vector<FieldDesc> &fields_desc, int self_index,
                                     StructureDefinition *ast);
    bool ProcessMethodsPost(Function *ctor, StructureDefinition *ast);
    Result GenerateRegularCalling(CallExpression *ast, CallingReceiver *receiver);
    Result GenerateMethodCalling(Value primary, CallExpression *ast, CallingReceiver *receiver);
    Result GenerateNewObject(const Class *clazz, CallExpression *ast, CallingReceiver *receiver);
    int GenerateArguments(const base::ArenaVector<Expression *> &args,
                          const std::vector<const Class *> &params, ASTNode *self_ast, int self_idx,
                          bool vargs, ASTNode *ast);
    Result GeneratePropertyAssignment(const ASTString *name, Value self, Scope *owns, Operator op,
                                      Expression *rhs, DotExpression *ast);
    Result GenerateIndexAssignment(const Class *type, int primary, const Class *index_type,
                                   int index, Operator op, Expression *rhs, IndexExpression *ast);
    Result GenerateVariableAssignment(const ASTString *name, Value lval, Scope *owns, Operator op,
                                      Expression *rhs, ASTNode *ast);
    Result GenerateIncrement(const Class *lval_type, int lval_index, Value::Linkage lval_linkage,
                             Operator op);
    Result GenerateStoreProperty(const Class *clazz, int index, Value::Linkage linkage,
                                 DotExpression *ast);
    Result GenerateLoadProperty(const Value &value, DotExpression *ast){
        return GenerateLoadProperty(value.type, value.index, value.linkage, ast);
    }
    Result GenerateLoadProperty(const Class *clazz, int index, Value::Linkage linkage,
                                DotExpression *ast);
    Result GeneratePropertyCast(const Class *clazz, int index, Value::Linkage linkage,
                                DotExpression *ast);

    void GenerateComparation(const Class *clazz, Operator op, int lhs, int rhs, ASTNode *ast);
    void GenerateOperation(const Operator op, const Class *lhs_type, int lhs_index,
                           Value::Linkage lhs_linkage, const Class *rhs_type, int rhs_index,
                           Value::Linkage rhs_linkage, ASTNode *ast);
    void GenerateOperation(const Operator op, const Class *clazz, int lhs, int rhs, ASTNode *ast);
    bool GenerateOperation(const Operator op, const Class *lhs_type, int lhs_index,
                           Value::Linkage lhs_linkage, PairExpression *rhs, ASTNode *ast);
    void GenerateArrayAppend(const Class *clazz, int lhs_index, Value::Linkage lhs_linkage,
                             const Class *value, int value_index, Value::Linkage value_linkage,
                             ASTNode *ast);
    void GenerateArrayPlus(const Class *clazz, int lhs_index, Value::Linkage lhs_linkage,
                           const Class *key, int key_index, Value::Linkage key_linkage,
                           const Class *value, int value_index, Value::Linkage value_linkage,
                           ASTNode *ast);
    void GenerateArrayMinus(const Class *clazz, int lhs_index, Value::Linkage lhs_linkage,
                            const Class *key, int key_index, Value::Linkage key_linkage,
                            ASTNode *ast);
    void GenerateMapPlus(const Class *clazz, int lhs_index, Value::Linkage lhs_linkage,
                         const Class *key, int key_index, Value::Linkage key_linkage,
                         const Class *value, int value_index, Value::Linkage value_linkage,
                         ASTNode *ast);
    void GenerateMapMinus(const Class *clazz, int lhs_index, Value::Linkage lhs_linkage,
                          const Class *key, int key_index, Value::Linkage key_linkage,
                          ASTNode *ast);
    void GenerateMapNext(const Value &map, const Value &iter, ASTNode *ast);
    void GenerateMapKey(const Value &map, const Value &iter, const Class *key, ASTNode *ast);
    void GenerateMapValue(const Value &map, const Value &iter, const Class *value, ASTNode *ast);
    void GenerateSend(const Class *clazz, int lhs, int rhs, ASTNode *ast);

    bool GenerateUnaryOperands(OperandContext *receiver, Expression *ast);
    bool GenerateBinaryOperands(OperandContext *receiver, Expression *lhs, Expression *rhs);
    void AssociateLHSOperand(OperandContext *receiver, const Value &value, ASTNode *ast) {
        AssociateLHSOperand(receiver, value.type, value.index, value.linkage, ast);
    }
    void AssociateLHSOperand(OperandContext *receiver, const Class *clazz, int index,
                             Value::Linkage linkage, ASTNode *ast);
    void AssociateRHSOperand(OperandContext *receiver, const Value &value, ASTNode *ast) {
        AssociateRHSOperand(receiver, value.type, value.index, value.linkage, ast);
    }
    void AssociateRHSOperand(OperandContext *receiver, const Class *clazz, int index,
                             Value::Linkage linkage, ASTNode *ast);
    void CleanupOperands(OperandContext *receiver);

    void ToStringIfNeeded(const Value &value, ASTNode *ast) {
        ToStringIfNeeded(value.type, value.index, value.linkage, ast);
    }
    void ToStringIfNeeded(const Class *clazz, int index, Value::Linkage linkage, ASTNode *ast);
    const Class *InboxIfNeeded(const Value &value, const Class *lval, ASTNode *ast) {
        return InboxIfNeeded(value.type, value.index, value.linkage, lval, ast);
    }
    const Class *InboxIfNeeded(const Class *clazz, int index, Value::Linkage linkage,
                               const Class *lval, ASTNode *ast);
    void MoveToStackIfNeeded(const Value &value, int dest, ASTNode *ast){
        MoveToStackIfNeeded(value.type, value.index, value.linkage, dest, ast);
    }
    void MoveToStackIfNeeded(const Class *clazz, int index, Value::Linkage linkage, int dest,
                             ASTNode *ast);
    void MoveToArgumentIfNeeded(const Value &value, int dest, ASTNode *ast) {
        MoveToArgumentIfNeeded(value.type, value.index, value.linkage, dest, ast);
    }
    void MoveToArgumentIfNeeded(const Class *clazz, int index, Value::Linkage linkage, int dest,
                                ASTNode *ast);
    
    void LdaIfNeeded(const Value &value, ASTNode *ast) {
        LdaIfNeeded(value.type, value.index, value.linkage, ast);
    }
    void LdaIfNeeded(const Result &rv, ASTNode *ast);
    void LdaIfNeeded(const Class *clazz, int index, Value::Linkage linkage, ASTNode *ast);
    void LdaStack(const Class *clazz, int index, ASTNode *ast);
    void LdaConst(const Class *clazz, int index, ASTNode *ast);
    void LdaGlobal(const Class *clazz, int index, ASTNode *ast);
    void LdaCaptured(const Class *clazz, int index, ASTNode *ast);
    void LdaProperty(const Class *clazz, int index, int offset, ASTNode *ast);
    void LdaArrayAt(const Class *clazz, int primary, int index, ASTNode *ast);
    
    void StaIfNeeded(const Value &value, ASTNode *ast) {
        StaIfNeeded(value.type, value.index, value.linkage, ast);
    }
    void StaIfNeeded(const Class *clazz, int index, Value::Linkage linkage, ASTNode *ast);
    void StaStack(const Class *clazz, int index, ASTNode *ast);
    void StaConst(const Class *clazz, int index, ASTNode *ast);
    void StaGlobal(const Class *clazz, int index, ASTNode *ast);
    void StaCaptured(const Class *clazz, int index, ASTNode *ast);
    void StaProperty(const Class *clazz, int index, int offset, ASTNode *ast);
    void StaArrayAt(const Class *clazz, int primary, int index, ASTNode *ast);
    
    void LdaMapGet(const Class *clazz, int primary, const Class *key_type, int key,
                   const Class *value_type, ASTNode *ast);
    void StaMapSet(const Class *clazz, int primary, const Class *key_type, int key,
                   const Class *value_type, ASTNode *ast);
    
    PrototypeDesc *GenerateFunctionPrototype(FunctionPrototype *ast);
    
    bool GenerateSymbolDependence(const Value &value);
    bool GenerateSymbolDependence(Symbolize *ast);
    
    int LinkGlobalVariable(VariableDeclaration *var);
    Value FindOrInsertExternalFunction(const std::string &name);
    
    Value EnsureFindValue(const std::string &name) const {
        Value value = FindValue(name);
        DCHECK_NE(value.linkage, Value::kError);
        return value;
    }

    bool HasGenerated(ASTNode *ast) const { return symbol_trace_.find(ast) != symbol_trace_.end(); }
    
    bool IsMinorInitializer(const StructureDefinition *owns, const FunctionDefinition *fun) const;

    Isolate *isolate_;
    MetadataSpace *metadata_space_;
    SyntaxFeedback *error_feedback_;
    ClassDefinition *class_exception_;
    ClassDefinition *class_object_;
    std::map<std::string, std::vector<FileUnit *>> path_units_;
    std::map<std::string, std::vector<FileUnit *>> pkg_units_;
    std::unordered_map<std::string, Value> symbols_;
    std::unordered_set<void *> symbol_trace_;
    base::Arena *arena_;
    GlobalSpaceBuilder global_space_;
    FunctionScope *current_init0_fun_ = nullptr;
    FunctionScope *current_fun_ = nullptr;
    ClassScope    *current_class_ = nullptr;
    FileScope     *current_file_ = nullptr;
    Scope         *current_ = nullptr;
    Function *generated_init0_fun_ = nullptr;
}; // class BytecodeGenerator

} // namespace lang

} // namespace mai

#endif // MAI_LANG_BYTECODE_GENERATOR_H_
