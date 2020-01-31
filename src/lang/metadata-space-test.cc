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

TEST_F(MetadataSpaceTest, FunctionBuilder) {
    auto err = space_->Initialize();
    ASSERT_TRUE(err.ok());
    
    Function::Builder builder("foo");
    auto func = builder.prototype("():void")
           .kind(Function::BYTECODE)
           .stack_size(RoundUp(32 + 40, kStackAligmentSize))
           .stack_bitmap({0})
           .bytecode(space_->NewBytecodeArray({0, 0, 0, 0}))
           .source_line_info(space_->NewSourceLineInfo("foo.mai", {1,2,3,4}))
           .AddCapturedVar("a", Function::IN_STACK, 0)
           .AddCapturedVar("b", Function::IN_STACK, 4)
    .Build(space_.get());

    ASSERT_NE(nullptr, func);
    ASSERT_EQ(Function::BYTECODE, func->kind());
    ASSERT_EQ(RoundUp(32 + 40, kStackAligmentSize), func->stack_size());
    ASSERT_EQ(0, func->stack_bitmap()[0]);
    ASSERT_EQ(2, func->captured_var_size());
    
    ASSERT_EQ(4, func->bytecode()->size());
    ASSERT_EQ(4, func->source_line_info()->length());
    ASSERT_EQ(SourceLineInfo::SIMPLE, func->source_line_info()->kind());
    
    const SourceLineInfo *source_lines = func->source_line_info();
    ASSERT_STREQ("foo.mai", source_lines->file_name());
    ASSERT_EQ(1, source_lines->source_line(0));
    ASSERT_EQ(2, source_lines->source_line(1));
    ASSERT_EQ(3, source_lines->source_line(2));
    ASSERT_EQ(4, source_lines->source_line(3));
    
    auto desc = func->captured_var(0);
    ASSERT_EQ(0, desc->index);
    ASSERT_STREQ("a", desc->name);
    ASSERT_EQ(Function::IN_STACK, desc->kind);
    
    desc = func->captured_var(1);
    ASSERT_EQ(4, desc->index);
    ASSERT_STREQ("b", desc->name);
    ASSERT_EQ(Function::IN_STACK, desc->kind);
}

} // namespace lang

} // namespace mai
