#include <msgpack11.hpp>

#include <iostream>
#include <algorithm>

#include <gtest/gtest.h>

TEST(MSGPACK_ARRAY, pack_unpack_fixarray)
{
    msgpack11::MsgPack::array v1{ 0xbead, 0xbeef, 0x0101 };
    msgpack11::MsgPack packed{v1};

    std::string dumped{packed.dump()};
    EXPECT_EQ(static_cast<uint8_t>(dumped[0]), 0x93u);

    std::string err;
    msgpack11::MsgPack parsed{ msgpack11::MsgPack::parse(dumped, err) };
    EXPECT_TRUE(parsed.is_array());

    msgpack11::MsgPack::array v2 = parsed.array_items();
    msgpack11::MsgPack packed2{v2};

    EXPECT_TRUE(v1 == v2);
}

TEST(MSGPACK_ARRAY, pack_unpack_array16_l)
{
    msgpack11::MsgPack::array v1(0x0010, 0xbead);
    msgpack11::MsgPack packed{v1};

    std::string dumped{packed.dump()};
    EXPECT_EQ(static_cast<uint8_t>(dumped[0]), 0xdcu);
    EXPECT_EQ(static_cast<uint8_t>(dumped[1]), 0x00u);
    EXPECT_EQ(static_cast<uint8_t>(dumped[2]), 0x10u);

    std::string err;
    msgpack11::MsgPack parsed{ msgpack11::MsgPack::parse(dumped, err) };
    EXPECT_TRUE(parsed.is_array());

    msgpack11::MsgPack::array v2 = parsed.array_items();
    EXPECT_TRUE(v1 == v2);
}

TEST(MSGPACK_ARRAY, pack_unpack_array16_h)
{
    msgpack11::MsgPack::array v1(0xffff, 0xdead);
    msgpack11::MsgPack packed{v1};

    std::string dumped{packed.dump()};
    EXPECT_EQ(static_cast<uint8_t>(dumped[0]), 0xdcu);
    EXPECT_EQ(static_cast<uint8_t>(dumped[1]), 0xffu);
    EXPECT_EQ(static_cast<uint8_t>(dumped[2]), 0xffu);

    std::string err;
    msgpack11::MsgPack parsed{ msgpack11::MsgPack::parse(dumped, err) };
    EXPECT_TRUE(parsed.is_array());

    msgpack11::MsgPack::array v2 = parsed.array_items();
    EXPECT_TRUE(v1 == v2);
}

TEST(MSGPACK_ARRAY, pack_unpack_array32_l)
{
    msgpack11::MsgPack::array v1(0x10000, 0xdead);
    msgpack11::MsgPack packed{v1};

    std::string dumped{packed.dump()};
    EXPECT_EQ(static_cast<uint8_t>(dumped[0]), 0xddu);
    EXPECT_EQ(static_cast<uint8_t>(dumped[1]), 0x00u);
    EXPECT_EQ(static_cast<uint8_t>(dumped[2]), 0x01u);
    EXPECT_EQ(static_cast<uint8_t>(dumped[3]), 0x00u);
    EXPECT_EQ(static_cast<uint8_t>(dumped[4]), 0x00u);

    std::string err;
    msgpack11::MsgPack parsed{ msgpack11::MsgPack::parse(dumped, err) };
    EXPECT_TRUE(parsed.is_array());

    msgpack11::MsgPack::array v2 = parsed.array_items();
    EXPECT_TRUE(v1 == v2);
}

TEST(MSGPACK_ARRAY, pack_unpack_object_array)
{
    msgpack11::MsgPack::array v1(0x0f, msgpack11::MsgPack::object{{"a", 100}, {"b", 200}});
    msgpack11::MsgPack packed{v1};

    std::string dumped{packed.dump()};
    EXPECT_EQ(static_cast<uint8_t>(dumped[0]), 0x9fu);
    EXPECT_EQ(static_cast<uint8_t>(dumped[1]), 0x82u);
    EXPECT_EQ(static_cast<uint8_t>(dumped[2]), 0xa1u);
    EXPECT_EQ(static_cast<uint8_t>(dumped[3]), 'a');
    EXPECT_EQ(static_cast<uint8_t>(dumped[4]), 0x64u);
    EXPECT_EQ(static_cast<uint8_t>(dumped[5]), 0xa1u);
    EXPECT_EQ(static_cast<uint8_t>(dumped[6]), 'b');
    EXPECT_EQ(static_cast<uint8_t>(dumped[7]), 0xccu);
    EXPECT_EQ(static_cast<uint8_t>(dumped[8]), 0xc8u);

    std::string err;
    msgpack11::MsgPack parsed{ msgpack11::MsgPack::parse(dumped, err) };
    EXPECT_TRUE(parsed.is_array());

    msgpack11::MsgPack::array v2 = parsed.array_items();
    EXPECT_TRUE(v1 == v2);
}
