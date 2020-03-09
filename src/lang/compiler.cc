#include "lang/compiler.h"
#include "lang/type-checker.h"
#include "lang/bytecode-generator.h"
#include "lang/isolate-inl.h"
#include "lang/parser.h"
#include "lang/ast.h"
#include "lang/lexer.h"

namespace mai {

namespace lang {

Error SourceFileResolve::Resolve(const std::string &dot_path, std::vector<FileUnit *> *file_units) {
    std::string dir(dot_path);
    for (size_t i = 0; i < dir.length(); i++) {
        if (dir[i] == '.') {
            dir[i] = '/';
        }
    }
    
    if (auto rs = env_->FileExists(dir); rs.fail()) {
        bool ok = false;
        for (auto search : search_path_) {
            if (rs = env_->FileExists(search + "/" + dir); rs.ok()) {
                dir = search + "/" + dir;
                ok = true;
                break;
            }
        }
        if (!ok) {
            return rs;
        }
    }

    std::vector<std::string> files;
    if (auto rs = Compiler::FindSourceFiles(dir, env_, false, &files); rs.fail()) {
        return rs;
    }
    return ParseAll(files, file_units);
}

Error SourceFileResolve::ParseAll(const std::vector<std::string> &source_files,
                                  std::vector<FileUnit *> *file_units) {
    int compile_fail = 0;
    Parser parser(arena_, feedback_);
    for (auto file_name : source_files) {
        std::unique_ptr<SequentialFile> input;
        if (auto rs = env_->NewSequentialFile(file_name, &input); rs.fail()) {
            return rs;
        }
        parser.SwitchInputFile(file_name, input.get());
        bool ok = true;
        FileUnit *unit = parser.Parse(&ok);
        if (ok) {
            file_units->push_back(unit);
        } else {
            compile_fail++;
        }
    }
    if (compile_fail > 0) {
        return MAI_CORRUPTION("Compile fail!");
    }
    return Error::OK();
}


/*static*/ Error Compiler::FindSourceFiles(const std::string &dir, Env *env, bool unittest,
                                           std::vector<std::string> *files) {
    if (auto rs = env->FileExists(dir); rs.fail()) {
        return rs;
    }
    std::vector<std::string> children;
    if (auto rs = env->GetChildren(dir, &children); rs.fail()) {
        return rs;
    }

    for (auto child : children) {
        bool isdir = false;
        child = dir + "/" + child;
        if (auto rs = env->IsDirectory(child, &isdir); rs.fail()) {
            return rs;
        }
        if (isdir) {
            if (auto rs = FindSourceFiles(child, env, unittest, files); rs.fail()) {
                return rs;
            }
        } else {
            if (child.length() - child.rfind(kFileExtName) == sizeof(kFileExtName) - 1) {
                files->push_back(child);
            }
        }
    }
    return Error::OK();
}

/*static*/ Error Compiler::CompileInterpretion(Isolate *isolate, const std::string &dir,
                                               SyntaxFeedback *feedback, base::Arena *arena) {
    std::vector<std::string> files;
    if (auto rs = Compiler::FindSourceFiles(isolate->base_pkg_dir(), isolate->env(), false, &files);
        !rs) {
        return rs;
    }
    SourceFileResolve resolver(isolate->env(), arena, feedback, {}/*TODO*/);
    std::vector<FileUnit *> base_units;
    if (auto rs = resolver.ParseAll(files, &base_units); !rs) {
        return rs;
    }
    
    TypeChecker checker(arena, feedback);
    if (auto rs = checker.AddBootFileUnits(isolate->base_pkg_dir(), base_units, &resolver); !rs) {
        return rs;
    }

    files.clear();
    if (auto rs = Compiler::FindSourceFiles(dir, isolate->env(), false, &files); !rs) {
        return rs;
    }
    resolver.mutable_search_path()->insert(dir);
    
    std::vector<FileUnit *> units;
    if (auto rs = resolver.ParseAll(files, &units); !rs) {
        return rs;
    }
    return checker.AddBootFileUnits(dir, units, &resolver);
    // TODO
}


} // namespace lang

} // namespace mai