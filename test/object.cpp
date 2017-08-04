#include <msgpack11.hpp>

#include <iostream>
#include <string>

#include <gtest/gtest.h>

TEST(MSGPACK_OBJECT, pack_unpack_object)
{
    msgpack11::MsgPack::object v1{
        {static_cast<uint8_t>(0xffu), std::string{"abcd"} },
        {"a", static_cast<int32_t>(100)},
        {"b", static_cast<int16_t>(200)}
    };

    msgpack11::MsgPack packed{v1};

    std::string dumped{packed.dump()};
    EXPECT_EQ(static_cast<uint8_t>(dumped[0]), 0x83u);
    EXPECT_EQ(static_cast<uint8_t>(dumped[1]), 0xccu);
    EXPECT_EQ(static_cast<uint8_t>(dumped[2]), 0xffu);
    EXPECT_EQ(static_cast<uint8_t>(dumped[3]), 0xa4u);
    EXPECT_EQ(static_cast<uint8_t>(dumped[4]), 'a');
    EXPECT_EQ(static_cast<uint8_t>(dumped[5]), 'b');
    EXPECT_EQ(static_cast<uint8_t>(dumped[6]), 'c');
    EXPECT_EQ(static_cast<uint8_t>(dumped[7]), 'd');
    EXPECT_EQ(static_cast<uint8_t>(dumped[8]), 0xa1u);
    EXPECT_EQ(static_cast<uint8_t>(dumped[9]), 'a');
    EXPECT_EQ(static_cast<uint8_t>(dumped[10]), 0x64u);
    EXPECT_EQ(static_cast<uint8_t>(dumped[11]), 0xa1u);
    EXPECT_EQ(static_cast<uint8_t>(dumped[12]), 'b');
    EXPECT_EQ(static_cast<uint8_t>(dumped[13]), 0xccu);
    EXPECT_EQ(static_cast<uint8_t>(dumped[14]), 0xc8u);

    std::string err;
    msgpack11::MsgPack parsed{ msgpack11::MsgPack::parse(dumped, err) };
    EXPECT_TRUE(parsed.is_object());

    msgpack11::MsgPack::object v2{ parsed.object_items() };
    EXPECT_TRUE(v1 == v2);
}
