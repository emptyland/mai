#include "lang/bytecode-generator.h"
#include "lang/compiler.h"
#include "lang/type-checker.h"
#include "lang/lexer.h"
#include "lang/ast.h"
#include "lang/syntax-mock-test.h"
#include "test/isolate-initializer.h"
#include "base/arenas.h"
#include "base/slice.h"
#include "base/arena-utils.h"
#include "base/arenas.h"
#include "gtest/gtest.h"

namespace mai {

namespace lang {

class BytecodeGeneratorTest : public test::IsolateInitializer {
public:
    BytecodeGeneratorTest()
        : arena_(Env::Default()->GetLowLevelAllocator())
        , env_(Env::Default())
        , resolver_(env_, &arena_, &feedback_, {})
        , checker_(&arena_, &feedback_) {}
    
    void SetUp() override {
        IsolateInitializer::SetUp();
    }
    
    void TearDown() override {
        IsolateInitializer::TearDown();
        delete generator_;
        generator_ = nullptr;
    }
    
    Error Parse(const std::string &dir) {
        std::vector<std::string> files;
        if (auto rs = Compiler::FindSourceFiles("src/lang/pkg", env_, false, &files); !rs) {
            return rs;
        }
        std::vector<FileUnit *> base_units;
        if (auto rs = resolver_.ParseAll(files, &base_units); !rs) {
            return rs;
        }
        if (auto rs = checker_.AddBootFileUnits("src/lang/pkg", base_units, &resolver_); !rs){
            return rs;
        }

        files.clear();
        if (auto rs = Compiler::FindSourceFiles(dir, env_, false, &files); !rs) {
            return rs;
        }
        resolver_.mutable_search_path()->insert(dir);
        
        std::vector<FileUnit *> units;
        if (auto rs = resolver_.ParseAll(files, &units); !rs) {
            return rs;
        }
        if (auto rs = checker_.AddBootFileUnits(dir, units, &resolver_); !rs) {
            return rs;
        }
        if (!checker_.Prepare() || !checker_.Check()) {
            return MAI_CORRUPTION("Check fail!");
        }
        
        generator_ = new BytecodeGenerator(isolate_, &feedback_,
                                           checker_.class_exception(),
                                           checker_.class_any(),
                                           std::move(*checker_.mutable_path_units()),
                                           std::move(*checker_.mutable_pkg_units()));
        return Error::OK();
    }
    
    MockFeedback feedback_;
    base::StandaloneArena arena_;
    Env *env_;
    SourceFileResolve resolver_;
    TypeChecker checker_;
    BytecodeGenerator *generator_ = nullptr;
}; // class BytecodeGeneratorTest

using Value = BytecodeGenerator::Value;

TEST_F(BytecodeGeneratorTest, Sanity) {
    HandleScope handle_scope(HandleScope::INITIALIZER);
    
    auto err = Parse("tests/lang/011-generator-sanity");
    ASSERT_TRUE(err.ok()) << err.ToString();
    ASSERT_TRUE(generator_->Prepare());
    
    auto value = generator_->FindValue("main.a");
    ASSERT_EQ(Value::kGlobal, value.linkage);
    ASSERT_EQ(isolate_->builtin_type(kType_string), value.type) << value.type->name();

    value = generator_->FindValue("main.b");
    ASSERT_EQ(Value::kGlobal, value.linkage);
    ASSERT_EQ(isolate_->metadata_space()->FindClassOrNull("foo.Foo"), value.type) << value.type->name();
    
    value = generator_->FindValue("runtime.VERSION");
    ASSERT_EQ(Value::kGlobal, value.linkage);
    ASSERT_EQ(isolate_->builtin_type(kType_string), value.type) << value.type->name();
    
    ASSERT_TRUE(generator_->Generate());
}

} // namespace lang

} // namespace mai