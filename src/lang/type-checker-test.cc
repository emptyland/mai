#include "lang/compiler.h"
#include "lang/type-checker.h"
#include "lang/lexer.h"
#include "lang/ast.h"
#include "lang/syntax-mock-test.h"
#include "base/arena-utils.h"
#include "base/arenas.h"
#include "gtest/gtest.h"

namespace mai {

namespace lang {

class TypeCheckerTest : public ::testing::Test {
public:
    TypeCheckerTest()
        : arena_(Env::Default()->GetLowLevelAllocator())
        , env_(Env::Default())
        , resolver_(env_, &arena_, &feedback_, {})
        , checker_(&arena_, &feedback_) {}
    
    void SetUp() override {
        InitializeSyntaxLibrary();
    }
    
    void TearDown() override {
        FreeSyntaxLibrary();
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
        return checker_.AddBootFileUnits(dir, units, &resolver_);
    }
    
    MockFeedback feedback_;
    base::StandaloneArena arena_;
    Env *env_;
    SourceFileResolve resolver_;
    TypeChecker checker_;
};

TEST_F(TypeCheckerTest, Sanity) {
    static constexpr char kPath[] = "tests/lang/000-type-checker-sanity";
    
    auto rs = Parse(kPath);
    ASSERT_TRUE(rs.ok()) << rs.ToString();
    
    auto path_units = checker_.FindPathUnits("foo");
    ASSERT_EQ(1, path_units.size());
    
    path_units = checker_.FindPathUnits("mai.lang");
    ASSERT_LE(1, path_units.size());
    
    path_units = checker_.FindPathUnits("mai.runtime");
    ASSERT_LE(1, path_units.size());
    
    path_units = checker_.FindPackageUnits("foo");
    ASSERT_EQ(1, path_units.size());
    
    path_units = checker_.FindPackageUnits("main");
    ASSERT_EQ(1, path_units.size());
}

TEST_F(TypeCheckerTest, Prepare) {
    auto rs = Parse("tests/lang/000-type-checker-sanity");
    ASSERT_TRUE(rs.ok()) << rs.ToString();
    ASSERT_TRUE(checker_.Prepare());
    
    auto sym = checker_.FindSymbolOrNull("lang.println");
    ASSERT_NE(nullptr, sym);
    ASSERT_TRUE(sym->IsFunctionDefinition());
    ASSERT_STREQ("println", sym->AsFunctionDefinition()->identifier()->data());
    ASSERT_TRUE(sym->AsFunctionDefinition()->is_native());
    
    sym = checker_.FindSymbolOrNull("foo.i");
    ASSERT_NE(nullptr, sym);
    ASSERT_TRUE(sym->IsVariableDeclaration());
    
    sym = checker_.FindSymbolOrNull("main.i");
    ASSERT_NE(nullptr, sym);
    ASSERT_TRUE(sym->IsVariableDeclaration());
    
    sym = checker_.FindSymbolOrNull("main.main");
    ASSERT_NE(nullptr, sym);
    ASSERT_TRUE(sym->IsFunctionDefinition());
    ASSERT_FALSE(sym->AsFunctionDefinition()->is_native());
}

TEST_F(TypeCheckerTest, SimpleGlobalVariableTypeReduce) {
    auto rs = Parse("tests/lang/001-type-checker-variable");
    ASSERT_TRUE(rs.ok()) << rs.ToString();
    ASSERT_TRUE(checker_.Prepare());
    ASSERT_TRUE(checker_.Check());
    
    auto sym = checker_.FindSymbolOrNull("main.i");
    ASSERT_NE(nullptr, sym);
    ASSERT_TRUE(sym->IsVariableDeclaration());
    ASSERT_STREQ("i", sym->identifier()->data());
    ASSERT_EQ(Token::kInt, sym->AsVariableDeclaration()->type()->id());
    
    sym = checker_.FindSymbolOrNull("main.j");
    ASSERT_NE(nullptr, sym);
    ASSERT_TRUE(sym->IsVariableDeclaration());
    ASSERT_STREQ("j", sym->identifier()->data());
    ASSERT_EQ(Token::kF32, sym->AsVariableDeclaration()->type()->id());
}

TEST_F(TypeCheckerTest, TypeNameResolve) {
    auto rs = Parse("tests/lang/002-type-checker-type-resolve");
    ASSERT_TRUE(rs.ok()) << rs.ToString();
    ASSERT_TRUE(checker_.Prepare());
    ASSERT_TRUE(checker_.Check());
    
    auto sym = checker_.FindSymbolOrNull("main.o1");
    ASSERT_NE(nullptr, sym);
    ASSERT_TRUE(sym->IsVariableDeclaration());
    
    auto var = sym->AsVariableDeclaration();
    ASSERT_NE(nullptr, var);
    ASSERT_EQ(Token::kClass, var->type()->id());
    
    auto clazz = var->type()->clazz();
    ASSERT_NE(nullptr, clazz);
    ASSERT_STREQ("Foo", clazz->identifier()->data());
}

TEST_F(TypeCheckerTest, ClassDefinitionBaseClass) {
    auto rs = Parse("tests/lang/003-type-checker-class-definition");
    ASSERT_TRUE(rs.ok()) << rs.ToString();
    ASSERT_TRUE(checker_.Prepare());
    ASSERT_TRUE(checker_.Check());
    
    auto sym = checker_.FindSymbolOrNull("main.Bar");
    ASSERT_NE(nullptr, sym);
    ASSERT_TRUE(sym->IsClassDefinition());
    
    auto clazz = sym->AsClassDefinition();
    ASSERT_STREQ("Bar", clazz->identifier()->data());
    ASSERT_EQ(Token::kInt, clazz->FindFieldOrNull("max")->declaration->type()->id());
    
    clazz = clazz->base();
    ASSERT_NE(nullptr, clazz);
    ASSERT_STREQ("Foo", clazz->identifier()->data());
}

TEST_F(TypeCheckerTest, ClassDefinitionFieldPerm) {
    auto rs = Parse("tests/lang/004-type-checker-class-perm");
    ASSERT_TRUE(rs.ok()) << rs.ToString();
    ASSERT_TRUE(checker_.Prepare());
    ASSERT_TRUE(checker_.Check());
}

}

}
