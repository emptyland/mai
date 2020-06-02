#include "lang/channel.h"
#include "lang/value-inl.h"
#include "lang/machine.h"
#include "mai/handle.h"
#include "test/isolate-initializer.h"
#include "gtest/gtest.h"

namespace mai {

namespace lang {

class ChannelTest : public test::IsolateInitializer {
public:
    // Dummy
};


TEST_F(ChannelTest, NewChannel) {
    HandleScope handle_scope(HandleScope::INITIALIZER);
    Local<Channel> chan(Machine::This()->NewChannel(kType_i8, 0, 0));
    ASSERT_TRUE(chan->is_not_buffered());
    ASSERT_FALSE(chan->is_buffered());
    ASSERT_EQ(0, chan->capacity());
    ASSERT_FALSE(chan->has_close());
    ASSERT_EQ(STATE->builtin_type(kType_i8), chan->data_type());
    chan->Close();
    ASSERT_TRUE(chan->has_close());
}

TEST_F(ChannelTest, AddBufferTail) {
    HandleScope handle_scope(HandleScope::INITIALIZER);
    Local<Channel> chan(Machine::This()->NewChannel(kType_i8, 3, 0));
    ASSERT_EQ(3, chan->capacity());

    int8_t data = 1;
    chan->AddBufferTail(&data, sizeof(data));
    data = 2;
    chan->AddBufferTail(&data, sizeof(data));
    data = 3;
    chan->AddBufferTail(&data, sizeof(data));
    ASSERT_EQ(3, chan->length());
    ASSERT_TRUE(chan->is_buffer_full());
}

TEST_F(ChannelTest, TakeBufferHead) {
    HandleScope handle_scope(HandleScope::INITIALIZER);
    Local<Channel> chan(Machine::This()->NewChannel(kType_int, 3, 0));
    ASSERT_EQ(3, chan->capacity());
    
    int data = 1;
    chan->AddBufferTail(&data, sizeof(data));
    data = 2;
    chan->AddBufferTail(&data, sizeof(data));
    data = 3;
    chan->AddBufferTail(&data, sizeof(data));
    ASSERT_EQ(3, chan->length());
    ASSERT_TRUE(chan->is_buffer_full());

    chan->TakeBufferHead(&data, sizeof(data));
    ASSERT_EQ(1, data);
    chan->TakeBufferHead(&data, sizeof(data));
    ASSERT_EQ(2, data);
    chan->TakeBufferHead(&data, sizeof(data));
    ASSERT_EQ(3, data);
    ASSERT_EQ(0, chan->length());
    ASSERT_FALSE(chan->is_buffer_full());
}

TEST_F(ChannelTest, RingBuffer) {
    HandleScope handle_scope(HandleScope::INITIALIZER);
    Local<Channel> chan(Machine::This()->NewChannel(kType_i8, 3, 0));
    ASSERT_EQ(3, chan->capacity());
    
    int data = 1;
    chan->AddBufferTail(&data, chan->data_type()->reference_size());
    data = 2;
    chan->AddBufferTail(&data, chan->data_type()->reference_size());
    
    chan->TakeBufferHead(&data, chan->data_type()->reference_size());
    ASSERT_EQ(1, data);
    
    data = 3;
    chan->AddBufferTail(&data, chan->data_type()->reference_size());
    
    chan->TakeBufferHead(&data, chan->data_type()->reference_size());
    ASSERT_EQ(2, data);
    
    chan->TakeBufferHead(&data, chan->data_type()->reference_size());
    ASSERT_EQ(3, data);
    
    data = 4;
    chan->AddBufferTail(&data, chan->data_type()->reference_size());
    
    chan->TakeBufferHead(&data, chan->data_type()->reference_size());
    ASSERT_EQ(4, data);
}

} // namespace lang

} // namespace mai
