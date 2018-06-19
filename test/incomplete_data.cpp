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
    
    for (size_t i = 1; i < dumped.size() - 1; ++i) {
        std::string corrupted = dumped.substr(0, i);
        std::string err;
        msgpack11::MsgPack parsed{ msgpack11::MsgPack::parse(corrupted, err) };
        EXPECT_GT(err.size(), 0);
    }
}
