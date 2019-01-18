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
        
        rs = schema_->NewDatabase(kPrimaryDatabaseName, "maidb.column");
        ASSERT_TRUE(rs.ok()) << rs.ToString();
    }

    ~FormSchemaTest() override {
        int i = 0;
        while (tmp_dirs[i]) {
            env_->DeleteFile(tmp_dirs[i++], true);
        }
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
    patch.set_engine_name("maidb:column");
    
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
    
} // namespace sql
    
} // namespace mai
