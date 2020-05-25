#include "lang/marking-sweep.h"
#include "lang/heap.h"
#include "test/isolate-initializer.h"
#include "base/slice.h"
#include "gtest/gtest.h"

namespace mai {

namespace lang {

class MarkingSweepTest : public test::IsolateInitializer {
public:
    MarkingSweepTest() {}
}; // class MarkingSweepTest

TEST_F(MarkingSweepTest, Sanity) {
    HandleScope handle_scope(HandleScope::INITIALIZER);
    Local<String> local(String::NewUtf8("Hello"));
    String *p1 = *local;
    String *p2 = isolate_->factory()->empty_string();
    
    MarkingSweep marking_sweep(isolate_, isolate_->heap());
    base::NullPrinter printer;
    marking_sweep.Run(&printer);
    ASSERT_EQ(p1, *local);
    ASSERT_EQ(p2, isolate_->factory()->empty_string());
}

TEST_F(MarkingSweepTest, NestedMarking) {
    HandleScope handle_scope(HandleScope::INITIALIZER);
    Handle<String> elems[] = {
        String::NewUtf8("Hello"),
        String::NewUtf8("World"),
        String::NewUtf8("Demo"),
        String::NewUtf8("Doom"),
        String::NewUtf8("Owns"),
    };
    Local<Array<String *>> local(Array<String *>::New(elems, 5));
    
    MarkingSweep marking_sweep(isolate_, isolate_->heap());
    base::StdFilePrinter printer(stdout);
    marking_sweep.Run(&printer);
    
    ASSERT_STREQ("Hello", local->At(0)->data());
    ASSERT_STREQ("World", local->At(1)->data());
    ASSERT_STREQ("Demo", local->At(2)->data());
    ASSERT_STREQ("Doom", local->At(3)->data());
    ASSERT_STREQ("Owns", local->At(4)->data());
}

} // namespace lang

} // namespace mai
