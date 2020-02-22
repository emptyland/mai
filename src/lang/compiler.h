#pragma once
#ifndef MAI_LANG_COMPILER_H_
#define MAI_LANG_COMPILER_H_

#include "lang/syntax.h"
#include "base/base.h"
#include "mai/error.h"
#include <set>

namespace mai {
class Env;
namespace lang {

class FileUnit;

class SourceFileResolve {
public:
    SourceFileResolve(Env *env, base::Arena *arena, SyntaxFeedback *feedback,
                      std::set<std::string> search_path)
        : env_(env)
        , arena_(arena)
        , feedback_(feedback)
        , search_path_(search_path) {}

    Error Resolve(const std::string &dot_path, std::vector<FileUnit *> *file_units);
    Error ParseAll(const std::vector<std::string> &source_files, std::vector<FileUnit *> *file_units);
private:
    Env *env_;
    base::Arena *arena_;
    SyntaxFeedback *feedback_;
    std::set<std::string> search_path_;
}; // class SourceFileResolve

struct Compiler {
    
    static constexpr const char kFileExtName[] = ".mai";
    static constexpr const char kTestFilePostfix[] = "-test.mai";

    static Error FindSourceFiles(const std::string &dir, Env *env, bool unittest,
                                 std::vector<std::string> *files);
    
    static Error CompileInterpretion(const std::vector<std::string> &source_files,
                                     SyntaxFeedback *feedback,
                                     base::Arena *arena,
                                     Env *env);
    
}; // struct Compiler

} // namespace lang

} // namespace mai

#endif // MAI_LANG_COMPILER_H_
