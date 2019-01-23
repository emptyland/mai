#include "sql/form-schema.h"
#include "mai-sql/mai-sql.h"
#include "mai/env.h"
#include "gtest/gtest.h"

namespace mai {
    
namespace sql {
    
class FormSchemaTest : public ::testing::Test {
public:
    FormSchemaTest() {
        auto abs_meta_dir = env_->GetAbsolutePath(tmp_dirs[0]);
        auto abs_data_dir = env_->GetAbsolutePath(tmp_dirs[1]);
        schema_.reset(new FormSchemaSet(abs_meta_dir, abs_data_dir, env_));
    }
    
    void SetUp() override {
        Error rs;
        int i = 0;
        while (tmp_dirs[i]) {
            rs = env_->MakeDirectory(tmp_dirs[i++], true);
            ASSERT_TRUE(rs.ok()) << rs.ToString();
        }
        
        rs = schema_->NewDatabase(kPrimaryDatabaseName, kPrimaryDatabaseName,
                                  SQL_COLUMN_STORE);
        ASSERT_TRUE(rs.ok()) << rs.ToString();
    }

    ~FormSchemaTest() override {
        int i = 0;
        while (tmp_dirs[i]) {
            env_->DeleteFile(tmp_dirs[i++], true);
        }
    }
    
    void BuildBaseTable() {
        FormSchemaPatch patch(kPrimaryDatabaseName, "t1");
        patch.set_engine_name("MaiDB");
        
        auto spec = new FormSpecPatch(FormSpecPatch::kCreatTable);
        FormColumn col;
        col.name = "a";
        col.type = SQL_BIGINT;
        col.key = SQL_PRIMARY_KEY;
        col.auto_increment = true;
        col.not_null = false;
        col.comment.clear();
        spec->AddColumn(col);
        
        col.name = "b";
        col.type = SQL_VARCHAR;
        col.fixed_size = 255;
        col.key = SQL_KEY;
        col.auto_increment = false;
        col.not_null = false;
        col.comment.clear();
        spec->AddColumn(col);
        
        col.name = "c";
        col.type = SQL_CHAR;
        col.fixed_size = 16;
        col.key = SQL_KEY;
        col.auto_increment = false;
        col.not_null = true;
        col.comment = "This is c";
        spec->AddColumn(col);
        
        patch.AddSpec(spec);
        
        Error rs = schema_->LogAndApply(&patch);
        ASSERT_TRUE(rs.ok()) << rs.ToString();
    }
    
    Env *env_ = Env::Default();
    std::unique_ptr<FormSchemaSet> schema_;
    
    static const char *tmp_dirs[];
};
    
const char *FormSchemaTest::tmp_dirs[] = {
    "tests/00-form-schema-meta",
    "tests/01-form-schema-data",
    nullptr,
};
    
TEST_F(FormSchemaTest, Sanity) {
    FormSchemaPatch patch(kPrimaryDatabaseName, "t1");
    patch.set_engine_name("ColumnMaiDB");
    
    auto spec = new FormSpecPatch(FormSpecPatch::kCreatTable);
    FormColumn col;
    col.name = "a";
    col.type = SQL_BIGINT;
    col.key  = SQL_PRIMARY_KEY;
    spec->AddColumn(col);
    
    col.name = "b";
    col.type = SQL_VARCHAR;
    col.fixed_size = 255;
    col.key  = SQL_KEY;
    spec->AddColumn(col);

    patch.AddSpec(spec);
    
    Error rs = schema_->LogAndApply(&patch);
    ASSERT_TRUE(rs.ok()) << rs.ToString();
}
    
TEST_F(FormSchemaTest, CreateTable) {
    FormSchemaPatch patch(kPrimaryDatabaseName, "t1");
    patch.set_engine_name("ColumnMaiDB");
    
    auto spec = new FormSpecPatch(FormSpecPatch::kCreatTable);
    FormColumn col;
    col.name = "a";
    col.type = SQL_BIGINT;
    col.key  = SQL_PRIMARY_KEY;
    spec->AddColumn(col);
    
    col.name = "b";
    col.type = SQL_VARCHAR;
    col.fixed_size = 255;
    col.key  = SQL_KEY;
    col.comment = "this is b column";
    spec->AddColumn(col);
    
    patch.AddSpec(spec);
    
    Error rs = schema_->LogAndApply(&patch);
    ASSERT_TRUE(rs.ok()) << rs.ToString();
    
    FormSchemaSet schema(schema_->abs_meta_dir(), schema_->abs_data_dir(),
                         env_);
    auto fn = schema_->meta_file_number();
    schema_.reset();
    
    rs = schema.Recovery(fn);
    ASSERT_TRUE(rs.ok()) << rs.ToString();
    
    base::intrusive_ptr<Form> form;
    rs = schema.AcquireFormSchema("primary", "t1", &form);
    ASSERT_TRUE(rs.ok()) << rs.ToString();
    
    EXPECT_EQ("a", form->column(0)->name);
    EXPECT_EQ(SQL_BIGINT, form->column(0)->type);
    EXPECT_EQ(SQL_PRIMARY_KEY, form->column(0)->key);
}
    
} // namespace sql
    
} // namespace mai
