#include <msgpack11.hpp>

#include <iostream>
#include <string>

#include <gtest/gtest.h>

TEST(MSGPACK_OBJECT, unpack_incomplete_data)
{
    msgpack11::MsgPack::object object {
        { "key1", "value1" },
        { "key2", false },
        { "key3", msgpack11::MsgPack::array { 1, 2, 3 } },
        { "key4", msgpack11::MsgPack::object {
                      {"key4-a" , 1024},
                      {"key4-b" , msgpack11::MsgPack::binary { 0, 2, 4, 6 }}
                  }},
        { "key5", msgpack11::MsgPack::array {
                      msgpack11::MsgPack::object {
                          {"key5-1-a", 100},
                          {"key5-1-b", 200}
                      },
                      msgpack11::MsgPack::object {
                          {"key5-2-a", 300},
                          {"key5-2-b", 400}
                      }
                  }}
    };

    msgpack11::MsgPack packed{object};
    
    std::string dumped{packed.dump()};
    std::string corrupted = dumped.substr(0, dumped.size()/2);
    std::string err;
    msgpack11::MsgPack parsed{ msgpack11::MsgPack::parse(corrupted, err) };
    
    EXPECT_TRUE(err.size());
    
    // EXPECT_EQ(static_cast<uint8_t>(dumped[0]), 0x83u);
    // EXPECT_EQ(static_cast<uint8_t>(dumped[1]), 0xccu);
    // EXPECT_EQ(static_cast<uint8_t>(dumped[2]), 0xffu);
    // EXPECT_EQ(static_cast<uint8_t>(dumped[3]), 0xa4u);
    // EXPECT_EQ(static_cast<uint8_t>(dumped[4]), 'a');
    // EXPECT_EQ(static_cast<uint8_t>(dumped[5]), 'b');
    // EXPECT_EQ(static_cast<uint8_t>(dumped[6]), 'c');
    // EXPECT_EQ(static_cast<uint8_t>(dumped[7]), 'd');
    // EXPECT_EQ(static_cast<uint8_t>(dumped[8]), 0xa1u);
    // EXPECT_EQ(static_cast<uint8_t>(dumped[9]), 'a');
    // EXPECT_EQ(static_cast<uint8_t>(dumped[10]), 0x64u);
    // EXPECT_EQ(static_cast<uint8_t>(dumped[11]), 0xa1u);
    // EXPECT_EQ(static_cast<uint8_t>(dumped[12]), 'b');
    // EXPECT_EQ(static_cast<uint8_t>(dumped[13]), 0xccu);
    // EXPECT_EQ(static_cast<uint8_t>(dumped[14]), 0xc8u);

    // std::string err;
    // msgpack11::MsgPack parsed{ msgpack11::MsgPack::parse(dumped, err) };
    // EXPECT_TRUE(parsed.is_object());

    // msgpack11::MsgPack::object v2{ parsed.object_items() };
    // EXPECT_TRUE(v1 == v2);
}
