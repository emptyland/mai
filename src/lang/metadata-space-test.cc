#include "lang/metadata-space.h"
#include "mai/allocator.h"
#include "mai/env.h"
#include "gtest/gtest.h"

namespace mai {

namespace lang {

class MetadataSpaceTest : public ::testing::Test {
public:
    // Dummy
    MetadataSpaceTest()
        : space_(new MetadataSpace(lla_)) {
    }
    
//    void SetUp() override {
//        auto err = space_->Initialize();
//        ASSERT_TRUE(err.ok());
//    }

    Env *env_ = Env::Default();
    Allocator *lla_ = env_->GetLowLevelAllocator();
    std::unique_ptr<MetadataSpace> space_;
};


TEST_F(MetadataSpaceTest, Sanity) {
    auto result = space_->Allocate(1024, false);
    ASSERT_TRUE(result.ok());
    
    ASSERT_EQ(1024, space_->used_size());
    ASSERT_EQ(kPageSize, space_->GetRSS());
}

TEST_F(MetadataSpaceTest, MoreAllocation) {
    while (space_->used_size() < 10 * kPageSize) {
        auto result = space_->Allocate(1024, false);
        ASSERT_TRUE(result.ok());
        ASSERT_NE(nullptr, result.address());
    }
    EXPECT_EQ(10 * kPageSize, space_->used_size());
    EXPECT_EQ(11 * kPageSize, space_->GetRSS());
}

TEST_F(MetadataSpaceTest, LargeAllocation) {
    for (int i = 0; i < 10; i++) {
        auto result = space_->Allocate(kPageSize, false);
        ASSERT_TRUE(result.ok());
        ASSERT_NE(nullptr, result.address());
    }
    
    EXPECT_EQ(10 * kPageSize, space_->used_size());
    EXPECT_EQ(10526720, space_->GetRSS());
}

TEST_F(MetadataSpaceTest, StringAllocation) {
    auto str = space_->NewString("Hello, World!");
    ASSERT_NE(nullptr, str);
    ASSERT_STREQ("Hello, World!", str);
    
    str = space_->NewString("AAA");
    ASSERT_NE(nullptr, str);
    ASSERT_EQ(0, reinterpret_cast<uintptr_t>(str) % kAligmentSize);
}

TEST_F(MetadataSpaceTest, ClassBuilder) {
    auto err = space_->Initialize();
    ASSERT_TRUE(err.ok());
    
    const Class *type = ClassBuilder("foo.foo")
        .tags(Type::kReferenceTag)
        .field("a")
            .flags(Field::kPublic|Field::kRdWr)
            .offset(0)
            .tag(1)
            .type(space_->builtin_type(kType_int))
        .End()
        .field("b")
            .flags(Field::kPublic|Field::kRdWr)
            .offset(4)
            .tag(2)
            .type(space_->builtin_type(kType_int))
        .End()
    .Build(space_.get());
    ASSERT_NE(type, nullptr);
    
    ASSERT_EQ(kUserTypeIdBase, type->id());
    ASSERT_FALSE(type->tags() & Type::kBuiltinTag);
    ASSERT_FALSE(type->tags() & Type::kPrimitiveTag);

    ASSERT_EQ(2, type->n_fields());
    
    const Field *field = type->field(0);
    ASSERT_STREQ("a", field->name());
    ASSERT_EQ(space_->builtin_type(kType_int), field->type());
    ASSERT_EQ(1, field->tag());
    ASSERT_EQ(0, field->offset());
    ASSERT_TRUE(field->is_public());
    ASSERT_TRUE(field->is_readwrite());
    
    field = type->field(1);
    ASSERT_STREQ("b", field->name());
    ASSERT_EQ(space_->builtin_type(kType_int), field->type());
    ASSERT_EQ(2, field->tag());
    ASSERT_EQ(4, field->offset());
    ASSERT_TRUE(field->is_public());
    ASSERT_TRUE(field->is_readwrite());
}

} // namespace lang

} // namespace mai
