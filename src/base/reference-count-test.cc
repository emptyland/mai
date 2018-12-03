#include "base/reference-count.h"
#include "gtest/gtest.h"
#include <thread>
#include <vector>

namespace mai {
    
namespace base {

namespace {
    
class RefTestStub : public ReferenceCounted<RefTestStub> {
public:
    RefTestStub(bool *has_deleted) : has_deleted_(has_deleted) {}
    ~RefTestStub() { *has_deleted_ = true; }

    bool *has_deleted_;
}; // class RefTestStub

class RefTestStub2 : public ReferenceCountable {
public:
    RefTestStub2(bool *has_deleted) : has_deleted_(has_deleted) {}
    virtual ~RefTestStub2() { *has_deleted_ = true; }
    
    bool *has_deleted_;
}; // class RefTestStub2

} // namespace
    
TEST(ReferenceCountTest, Sanity) {
    auto h1 = intrusive_ptr<RefTestStub>::Null();
    ASSERT_TRUE(h1.is_null());
    ASSERT_TRUE(!h1);
}
    
TEST(ReferenceCountTest, StaticRefCount) {
    bool has_deleted = false;
    
    ASSERT_FALSE(has_deleted);
    {
        intrusive_ptr<RefTestStub> stub(new RefTestStub{&has_deleted});
        ASSERT_EQ(1, stub->ref_count());
        
        intrusive_ptr<RefTestStub> h1(stub);
        ASSERT_EQ(2, stub->ref_count());
        ASSERT_EQ(2, h1->ref_count());
        ASSERT_TRUE(h1 == stub);
        
        bool has_deleted_stub = false;
        auto h2 = intrusive_ptr<RefTestStub>::Make(new RefTestStub{&has_deleted_stub});
        ASSERT_FALSE(h2 == h1);
        
        intrusive_ptr<RefTestStub> h3 = std::move(h2);
        ASSERT_EQ(1, h3->ref_count());
    }
    ASSERT_TRUE(has_deleted);
}
    
TEST(ReferenceCountTest, DynamicRefCount) {
    bool has_deleted = false;
    
    ASSERT_FALSE(has_deleted);
    {
        auto h1 = MakeRef(new RefTestStub2{&has_deleted});
        ASSERT_EQ(1, h1->ref_count());
        {
            auto h2 = h1;
            ASSERT_EQ(2, h2->ref_count());
            ASSERT_EQ(2, h1->ref_count());
        }
        ASSERT_EQ(1, h1->ref_count());
    }
    ASSERT_TRUE(has_deleted);
}

TEST(ReferenceCountTest, AsyncRefCount) {
    bool has_deleted = false;
    auto obj = new RefTestStub{&has_deleted};
    obj->AddRef();
    
    std::vector<std::thread> add, rel;
    for (int i = 0; i < 4; ++i) {
        add.emplace_back([](ReferenceCounted<RefTestStub> *ref){
            for (int i = 0; i < 1000; ++i) {
                ref->AddRef();
            }
        }, obj);
    }
    for (int i = 0; i < 4; ++i) {
        rel.emplace_back([](ReferenceCounted<RefTestStub> *ref){
            for (int i = 0; i < 1000; ++i) {
                ref->ReleaseRef();
            }
        }, obj);
    }
    
    for (int i = 0; i < 4; ++i) {
        add[i].join();
        rel[i].join();
    }
    
    ASSERT_EQ(1, obj->ref_count());
    obj->ReleaseRef();
}

} // namespace base
    
} // namespace mai
