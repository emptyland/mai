#include "sql/storage-engine.h"
#include "sql/storage-engine-factory.h"
#include "sql/form-schema.h"
#include "mai-sql/mai-sql.h"
#include "mai/env.h"
#include "gtest/gtest.h"

namespace mai {
    
namespace sql {
    
class StorageEngineTest : public ::testing::Test {
public:
    StorageEngineTest() {
        auto abs_meta_dir = env_->GetAbsolutePath(tmp_dirs[0]);
        auto abs_data_dir = env_->GetAbsolutePath(tmp_dirs[1]);
        schema_.reset(new FormSchemaSet(abs_meta_dir, abs_data_dir, env_));
        factory_.reset(new StorageEngineFactory(abs_meta_dir, env_));
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
    
    ~StorageEngineTest() override {
        int i = 0;
        while (tmp_dirs[i]) {
            env_->DeleteFile(tmp_dirs[i++], true);
        }
    }
    
    void BuildBaseTable(const std::string &table_name) {
        FormSchemaPatch patch(kPrimaryDatabaseName, table_name);
        patch.set_engine_name("MaiDB");
        
        auto spec = new FormSpecPatch(FormSpecPatch::kCreatTable);
        Column col;
        col.name = "a";
        col.type = SQL_BIGINT;
        col.key = SQL_PRIMARY_KEY;
        col.auto_increment = true;
        col.not_null = false;
        col.comment.clear();
        spec->AddColumn(col);
        
        col.name = "b";
        col.type = SQL_VARCHAR;
        col.m_size = 255;
        col.key = SQL_KEY;
        col.auto_increment = false;
        col.not_null = false;
        col.comment.clear();
        spec->AddColumn(col);
        
        col.name = "c";
        col.type = SQL_CHAR;
        col.m_size = 16;
        col.key = SQL_KEY;
        col.auto_increment = false;
        col.not_null = true;
        col.comment = "This is c";
        spec->AddColumn(col);
        
        patch.AddSpec(spec);
        
        Error rs = schema_->LogAndApply(&patch);
        ASSERT_TRUE(rs.ok()) << rs.ToString();
    }
    
    void BuildBaseEngine(const std::string &table_name,
                         base::intrusive_ptr<Form> *frm,
                         std::unique_ptr<StorageEngine> *holder) {
        BuildBaseTable("t1");

        Error rs = schema_->AcquireFormSchema(kPrimaryDatabaseName, "t1", frm);
        ASSERT_TRUE(rs.ok()) << rs.ToString();
        
        StorageEngine *engine;
        rs = factory_->NewEngine(schema_->abs_data_dir(), kPrimaryDatabaseName,
                                 "MaiDB", SQL_COLUMN_STORE, &engine);
        ASSERT_TRUE(rs.ok()) << rs.ToString();
        holder->reset(engine);
        
        rs = engine->Prepare(true);
        ASSERT_TRUE(rs.ok()) << rs.ToString();
    }
    
    Env *env_ = Env::Default();
    std::unique_ptr<FormSchemaSet> schema_;
    std::unique_ptr<StorageEngineFactory> factory_;

    static const char *tmp_dirs[];
};

const char *StorageEngineTest::tmp_dirs[] = {
    "tests/00-storage-engine-meta",
    "tests/01-storage-engine-data",
    nullptr,
};


TEST_F(StorageEngineTest, Sanity) {
    BuildBaseTable("t1");
    
    base::intrusive_ptr<Form> frm;
    Error rs = schema_->AcquireFormSchema(kPrimaryDatabaseName, "t1", &frm);
    ASSERT_TRUE(rs.ok()) << rs.ToString();
    
    StorageEngine *engine;
    std::unique_ptr<StorageEngine> holder;
    rs = factory_->NewEngine(schema_->abs_data_dir(), kPrimaryDatabaseName,
                             "MaiDB", SQL_COLUMN_STORE, &engine);
    ASSERT_TRUE(rs.ok()) << rs.ToString();
    holder.reset(engine);
    
    rs = engine->Prepare(true);
    ASSERT_TRUE(rs.ok()) << rs.ToString();
}
    
TEST_F(StorageEngineTest, NewTable) {
    base::intrusive_ptr<Form> frm;
    std::unique_ptr<StorageEngine> holder;
    BuildBaseEngine("t1", &frm, &holder);
    
    Error rs = holder->NewTable(kPrimaryDatabaseName, frm.get());
    ASSERT_TRUE(rs.ok()) << rs.ToString();
    
    ASSERT_EQ(1, holder->item_owns_.size());
    auto it = holder->item_owns_.find("t1");
    ASSERT_TRUE(it != holder->item_owns_.end());
    ASSERT_EQ(0, it->second.auto_increment);
    
    ASSERT_EQ(6, it->second.items.size());
    ASSERT_EQ(6, holder->items_.size());
    
    it->second.auto_increment = 12;
    rs = holder->SyncMetadata("t1");
    ASSERT_TRUE(rs.ok()) << rs.ToString();
}
    
TEST_F(StorageEngineTest, Recovery) {
    base::intrusive_ptr<Form> frm;
    std::unique_ptr<StorageEngine> holder;
    BuildBaseEngine("t1", &frm, &holder);
    
    Error rs = holder->NewTable(kPrimaryDatabaseName, frm.get());
    ASSERT_TRUE(rs.ok()) << rs.ToString();
    
    BuildBaseTable("t2");
    rs = schema_->AcquireFormSchema(kPrimaryDatabaseName, "t2", &frm);
    ASSERT_TRUE(rs.ok()) << rs.ToString();
    
    rs = holder->NewTable(kPrimaryDatabaseName, frm.get());
    ASSERT_TRUE(rs.ok()) << rs.ToString();
    
    holder.reset();
    
    StorageEngine *engine;
    rs = factory_->NewEngine(schema_->abs_data_dir(), kPrimaryDatabaseName,
                             "MaiDB", SQL_COLUMN_STORE, &engine);
    ASSERT_TRUE(rs.ok()) << rs.ToString();
    holder.reset(engine);
    
    rs = holder->Prepare(false);
    ASSERT_TRUE(rs.ok()) << rs.ToString();
    
    ASSERT_EQ(2, holder->item_owns_.size());
    auto it = holder->item_owns_.find("t1");
    ASSERT_TRUE(it != holder->item_owns_.end());
    ASSERT_EQ(0, it->second.auto_increment);
    it = holder->item_owns_.find("t2");
    ASSERT_TRUE(it != holder->item_owns_.end());
    ASSERT_EQ(0, it->second.auto_increment);
}

} // namespace sql

} // namespace mai
