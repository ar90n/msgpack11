#include <msgpack11.hpp>
#include <sstream>

#include <gtest/gtest.h>

TEST(MSGPACK_MULTI, pack_unpack_multi)
{
    msgpack11::MsgPack::object v1{
        {static_cast<uint8_t>(0xffu), std::string{"abcd"} },
        {"a", static_cast<int32_t>(100)},
        {"b", static_cast<int16_t>(200)}
    };
    msgpack11::MsgPack::object v2{
        {static_cast<uint8_t>(0x44u), v1},
        {"a", static_cast<int8_t>(1)},
        {"b", static_cast<int8_t>(2)}
    };
    msgpack11::MsgPack::array v3{ 1, 2, 3 };

    msgpack11::MsgPack packed_v1{v1};
    msgpack11::MsgPack packed_v2{v2};
    msgpack11::MsgPack packed_v3{v3};

    std::stringstream ss;
    ss << packed_v1.dump() << packed_v2.dump() << packed_v3.dump();

    std::string err;
    std::vector< msgpack11::MsgPack > multi_parsed = msgpack11::MsgPack::parse_multi(ss.str(), err);

    EXPECT_EQ(multi_parsed.size(), 3);

    EXPECT_TRUE(multi_parsed[0].is_object());
    msgpack11::MsgPack::object parsed_v1{ multi_parsed[0].object_items() };
    EXPECT_TRUE(v1 == parsed_v1);

    EXPECT_TRUE(multi_parsed[1].is_object());
    msgpack11::MsgPack::object parsed_v2{ multi_parsed[1].object_items() };
    EXPECT_TRUE(v2["a"].int8_value() == parsed_v2["a"].int8_value());
    EXPECT_TRUE(v2["b"].int8_value() == parsed_v2["b"].int8_value());
    EXPECT_TRUE(v2[0x44u].object_items() == parsed_v2[0x44u].object_items());

    EXPECT_TRUE(multi_parsed[2].is_array());
    msgpack11::MsgPack::array parsed_v3( multi_parsed[2].array_items() );
    EXPECT_EQ(v3.size(), parsed_v3.size());
    EXPECT_TRUE(std::equal(v3.begin(), v3.end(), parsed_v3.begin()));
}
