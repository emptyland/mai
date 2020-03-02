#pragma once
#ifndef MAI_LANG_BYTECODE_GENERATOR_H_
#define MAI_LANG_BYTECODE_GENERATOR_H_

#include "lang/stable-space-builder.h"
#include "lang/ast.h"
#include "lang/syntax.h"
#include <map>
#include <set>
#include <unordered_set>

namespace mai {

namespace lang {

class Isolate;

class BytecodeGenerator final : public PartialASTVisitor {
public:
    BytecodeGenerator(Isolate *isolate, SyntaxFeedback *feedback,
                      ClassDefinition *class_exception,
                      ClassDefinition *class_any,
                      std::map<std::string, std::vector<FileUnit *>> &&path_units,
                      std::map<std::string, std::vector<FileUnit *>> &&pkg_units,
                      std::map<std::string, Symbolize *> &&symbols);
    
    BytecodeGenerator(Isolate *isolate, SyntaxFeedback *feedback);

    bool Generate();
private:
    bool PrepareUnit(const std::string &pkg_name, FileUnit *unit);
    int LinkGlobalVariable(VariableDeclaration *var);
    
    Result VisitClassDefinition(ClassDefinition *) override;
    Result VisitObjectDefinition(ObjectDefinition *) override;
    Result VisitFunctionDefinition(FunctionDefinition *) override;
    
    Isolate *isolate_;
    SyntaxFeedback *error_feedback_;
    ClassDefinition *class_exception_;
    ClassDefinition *class_any_;
    std::map<std::string, std::vector<FileUnit *>> path_units_;
    std::map<std::string, std::vector<FileUnit *>> pkg_units_;
    std::map<std::string, Symbolize *> symbols_;
    std::unordered_set<void *> symbol_trace_;
    GlobalSpaceBuilder global_space_;
}; // class BytecodeGenerator

} // namespace lang

} // namespace mai

#endif // MAI_LANG_BYTECODE_GENERATOR_H_
