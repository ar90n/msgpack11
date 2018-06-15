#include <msgpack11.hpp>

#include <iostream>

#include <cmath>
#include <string>
#include <vector>
#include <map>
#include <deque>
#include <set>
#include <list>
#include <limits>
#include <algorithm>

#include <gtest/gtest.h>

#if defined(_MSC_VER) || defined(__MINGW32__)
#define msgpack_rand() ((double)rand() / RAND_MAX)
#else  // _MSC_VER || __MINGW32__
#define msgpack_rand() drand48()
#endif // _MSC_VER || __MINGW32__

using namespace std;

const unsigned int kLoop = 10000;
const unsigned int kElements = 100;
const double kEPS = 1e-10;

#define GEN_TEST(test_type,get_value_func)                                              \
    do {                                                                                \
        vector<test_type> v;                                                            \
        v.push_back(0);                                                                 \
        v.push_back(1);                                                                 \
        v.push_back(2);                                                                 \
        v.push_back(numeric_limits<test_type>::min());                                  \
        v.push_back(numeric_limits<test_type>::max());                                  \
        for (unsigned int i = 0; i < kLoop; i++)                                        \
            v.push_back(rand());                                                        \
        for (unsigned int i = 0; i < v.size() ; i++) {                                  \
            test_type val1 = v[i];                                                      \
            msgpack11::MsgPack packed{val1};                                            \
            std::string err;                                                            \
            msgpack11::MsgPack parsed = msgpack11::MsgPack::parse(packed.dump(), err ); \
            EXPECT_EQ(val1, parsed.get_value_func());                                   \
        }                                                                               \
    } while(0)

TEST(MSGPACK, simple_buffer_uint8)
{
    GEN_TEST(uint8_t, uint8_value);
}

TEST(MSGPACK, simple_buffer_uint16)
{
    GEN_TEST(uint16_t, uint16_value);
}

TEST(MSGPACK, simple_buffer_uint32)
{
    GEN_TEST(uint32_t, uint32_value);
}

TEST(MSGPACK, simple_buffer_uint64)
{
    GEN_TEST(uint64_t, uint64_value);
}

TEST(MSGPACK, simple_buffer_int8)
{
    GEN_TEST(int8_t, int8_value);
}

TEST(MSGPACK, simple_buffer_int16)
{
    GEN_TEST(int16_t, int16_value);
}

TEST(MSGPACK, simple_buffer_int32)
{
    GEN_TEST(int32_t, int32_value);
}

TEST(MSGPACK, simple_buffer_int64)
{
    GEN_TEST(int64_t, int64_value);
}

#if !defined(_MSC_VER) || _MSC_VER >=1800

TEST(MSGPACK, simple_buffer_float)
{
    vector<float> v;
    v.push_back(0.0);
    v.push_back(-0.0);
    v.push_back(1.0);
    v.push_back(-1.0);
    v.push_back(numeric_limits<float>::min());
    v.push_back(numeric_limits<float>::max());
    v.push_back(nanf("tag"));
    if (numeric_limits<float>::has_infinity) {
        v.push_back(numeric_limits<float>::infinity());
        v.push_back(-numeric_limits<float>::infinity());
    }
    if (numeric_limits<float>::has_quiet_NaN) {
        v.push_back(numeric_limits<float>::quiet_NaN());
    }
    if (numeric_limits<float>::has_signaling_NaN) {
        v.push_back(numeric_limits<float>::signaling_NaN());
    }

    for (unsigned int i = 0; i < kLoop; i++) {
        v.push_back(static_cast<float>(msgpack_rand()));
        v.push_back(static_cast<float>(-msgpack_rand()));
    }
    for (unsigned int i = 0; i < v.size() ; i++) {
        float val1 = v[i];
        msgpack11::MsgPack packed{val1};
        std::string err;
        msgpack11::MsgPack parsed{ msgpack11::MsgPack::parse(packed.dump(), err ) };
        float val2 = parsed.float32_value();

        if (std::isnan(val1))
            EXPECT_TRUE(std::isnan(val2));
        else if (std::isinf(val1))
            EXPECT_TRUE(std::isinf(val2));
        else
            EXPECT_TRUE(fabs(val2 - val1) <= kEPS);
    }
}

#endif // !defined(_MSC_VER) || _MSC_VER >=1800

namespace {
template<typename F, typename I>
struct TypePair {
    typedef F float_type;
    typedef I integer_type;
};

template<typename F> struct ValueTypeTraits {};
template<> struct ValueTypeTraits<float> {
    using func_ptr_type = float (msgpack11::MsgPack::*)() const;
    static func_ptr_type constexpr value = &msgpack11::MsgPack::float32_value;
};
template<> struct ValueTypeTraits<double> {
    using func_ptr_type = double (msgpack11::MsgPack::*)() const;
    static func_ptr_type constexpr value = &msgpack11::MsgPack::float64_value;
};
} // namespace

template <typename T>
class IntegerToFloatingPointTest : public testing::Test {
};
TYPED_TEST_CASE_P(IntegerToFloatingPointTest);

TYPED_TEST_P(IntegerToFloatingPointTest, simple_buffer)
{
    typedef typename TypeParam::float_type float_type;
    typedef typename TypeParam::integer_type integer_type;
    vector<integer_type> v;
    v.push_back(0);
    v.push_back(1);
    if (numeric_limits<integer_type>::is_signed) v.push_back(-1);
    else v.push_back(2);
    for (unsigned int i = 0; i < kLoop; i++) {
        v.push_back(rand() % 0x7FFFFF);
    }
    for (unsigned int i = 0; i < v.size() ; i++) {
        integer_type val1 = v[i];
        msgpack11::MsgPack packed(val1);
        std::string err;
        msgpack11::MsgPack parsed{ msgpack11::MsgPack::parse(packed.dump(), err ) };
        float_type val2 = (parsed.*ValueTypeTraits<float_type>::value)();
        EXPECT_TRUE(fabs(val2 - val1) <= kEPS);
    }
}

REGISTER_TYPED_TEST_CASE_P(IntegerToFloatingPointTest,
                           simple_buffer);

typedef testing::Types<TypePair<float, int64_t>,
                       TypePair<float, uint64_t>,
                       TypePair<double, int64_t>,
                       TypePair<double, uint64_t> > IntegerToFloatingPointTestTypes;
INSTANTIATE_TYPED_TEST_CASE_P(IntegerToFloatingPointTestInstance,
                              IntegerToFloatingPointTest,
                              IntegerToFloatingPointTestTypes);

#if !defined(_MSC_VER) || _MSC_VER >=1800

TEST(MSGPACK, simple_buffer_double)
{
    vector<double> v;
    v.push_back(0.0);
    v.push_back(-0.0);
    v.push_back(1.0);
    v.push_back(-1.0);
    v.push_back(numeric_limits<double>::min());
    v.push_back(numeric_limits<double>::max());
    v.push_back(nanf("tag"));
    if (numeric_limits<double>::has_infinity) {
        v.push_back(numeric_limits<double>::infinity());
        v.push_back(-numeric_limits<double>::infinity());
    }
    if (numeric_limits<double>::has_quiet_NaN) {
        v.push_back(numeric_limits<double>::quiet_NaN());
    }
    if (numeric_limits<double>::has_signaling_NaN) {
        v.push_back(numeric_limits<double>::signaling_NaN());
    }
    for (unsigned int i = 0; i < kLoop; i++) {
        v.push_back(msgpack_rand());
        v.push_back(-msgpack_rand());
    }

    for (unsigned int i = 0; i < kLoop; i++) {
        v.push_back(msgpack_rand());
        v.push_back(-msgpack_rand());
    }
    for (unsigned int i = 0; i < v.size() ; i++) {
        double val1 = v[i];
        msgpack11::MsgPack packed{val1};
        std::string err;
        msgpack11::MsgPack parsed{ msgpack11::MsgPack::parse(packed.dump(), err ) };
        double val2 = parsed.float64_value();

        if (std::isnan(val1))
            EXPECT_TRUE(std::isnan(val2));
        else if (std::isinf(val1))
            EXPECT_TRUE(std::isinf(val2));
        else
            EXPECT_TRUE(fabs(val2 - val1) <= kEPS);
    }
}

#endif // !defined(_MSC_VER) || _MSC_VER >=1800

TEST(MSGPACK, simple_buffer_nil)
{
    msgpack11::MsgPack packed{nullptr};
    std::string err;
    msgpack11::MsgPack parsed{ msgpack11::MsgPack::parse(packed.dump(), err ) };
    EXPECT_EQ(parsed.type(), msgpack11::MsgPack::Type::NUL);
}

TEST(MSGPACK, simple_buffer_true)
{
    bool val1 = true;
    msgpack11::MsgPack packed{val1};
    std::string err;
    msgpack11::MsgPack parsed{ msgpack11::MsgPack::parse(packed.dump(), err ) };
    bool val2 = parsed.bool_value();
    EXPECT_EQ(val1, val2);
}

TEST(MSGPACK, simple_buffer_false)
{
    bool val1 = false;
    msgpack11::MsgPack packed{val1};
    std::string err;
    msgpack11::MsgPack parsed{ msgpack11::MsgPack::parse(packed.dump(), err ) };
    bool val2 = parsed.bool_value();
    EXPECT_EQ(val1, val2);
}

TEST(MSGPACK, simple_buffer_fixext1)
{
    msgpack11::MsgPack::binary data{2};
    uint8_t type = 1;
    msgpack11::MsgPack packed{ std::make_tuple(type, data) };
    std::string err;
    msgpack11::MsgPack parsed{ msgpack11::MsgPack::parse(packed.dump(), err ) };

    EXPECT_EQ(data.size(), std::get<1>(parsed.extension_items()).size());
    EXPECT_EQ(type, std::get<0>(parsed.extension_items()));
    EXPECT_EQ(data[0],  std::get<1>(parsed.extension_items())[0]);
}

TEST(MSGPACK, simple_buffer_fixext2)
{
    const msgpack11::MsgPack::binary data{ 2, 3 };
    const uint8_t type = 0;
    msgpack11::MsgPack packed{ std::make_tuple(type, data) };

    std::string err;
    msgpack11::MsgPack parsed{ msgpack11::MsgPack::parse(packed.dump(), err ) };
    const uint8_t parsed_type = std::get<0>( parsed.extension_items() );
    const msgpack11::MsgPack::binary& parsed_data = std::get<1>( parsed.extension_items() );

    EXPECT_EQ(data.size(), parsed_data.size());
    EXPECT_EQ(type, parsed_type);
    EXPECT_TRUE(std::equal(data.begin(), data.end(), parsed_data.begin()));
}

TEST(MSGPACK, simple_buffer_fixext4)
{
    const msgpack11::MsgPack::binary data{ 2, 3, 4, 5 };
    const uint8_t type = 1;
    msgpack11::MsgPack packed{ std::make_tuple(type, data) };

    std::string err;
    msgpack11::MsgPack parsed{ msgpack11::MsgPack::parse(packed.dump(), err ) };
    const uint8_t parsed_type = std::get<0>( parsed.extension_items() );
    const msgpack11::MsgPack::binary& parsed_data = std::get<1>( parsed.extension_items() );

    EXPECT_EQ(data.size(), parsed_data.size());
    EXPECT_EQ(type, parsed_type);
    EXPECT_TRUE(std::equal(data.begin(), data.end(), parsed_data.begin()));
}

TEST(MSGPACK, simple_buffer_fixext8)
{
    const msgpack11::MsgPack::binary data{ 2, 3, 4, 5, 6, 7, 8, 9 };
    const uint8_t type = 1;
    msgpack11::MsgPack packed{ std::make_tuple(type, data) };

    std::string err;
    msgpack11::MsgPack parsed{ msgpack11::MsgPack::parse(packed.dump(), err ) };
    const uint8_t parsed_type = std::get<0>( parsed.extension_items() );
    const msgpack11::MsgPack::binary& parsed_data = std::get<1>( parsed.extension_items() );

    EXPECT_EQ(data.size(), parsed_data.size());
    EXPECT_EQ(type, parsed_type);
    EXPECT_TRUE(std::equal(data.begin(), data.end(), parsed_data.begin()));
}

TEST(MSGPACK, simple_buffer_fixext16)
{
    const msgpack11::MsgPack::binary data{ 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17 };
    const uint8_t type = 1;
    msgpack11::MsgPack packed{ std::make_tuple(type, data) };

    std::string err;
    msgpack11::MsgPack parsed{ msgpack11::MsgPack::parse(packed.dump(), err ) };
    const uint8_t parsed_type = std::get<0>( parsed.extension_items() );
    const msgpack11::MsgPack::binary& parsed_data = std::get<1>( parsed.extension_items() );

    EXPECT_EQ(data.size(), parsed_data.size());
    EXPECT_EQ(type, parsed_type);
    EXPECT_TRUE(std::equal(data.begin(), data.end(), parsed_data.begin()));
}

TEST(MSGPACK, simple_buffer_fixext_1byte_0)
{
    const msgpack11::MsgPack::binary data{};
    const uint8_t type = 77;
    msgpack11::MsgPack packed{ std::make_tuple(type, data) };

    std::string err;
    msgpack11::MsgPack parsed{ msgpack11::MsgPack::parse(packed.dump(), err ) };
    const uint8_t parsed_type = std::get<0>( parsed.extension_items() );
    const msgpack11::MsgPack::binary& parsed_data = std::get<1>( parsed.extension_items() );

    EXPECT_EQ(data.size(), parsed_data.size());
    EXPECT_EQ(type, parsed_type);
    EXPECT_TRUE(std::equal(data.begin(), data.end(), parsed_data.begin()));
}

TEST(MSGPACK, simple_buffer_fixext_1byte_255)
{
    msgpack11::MsgPack::binary data(255, 0);
    for (std::size_t i = 0; i != data.size(); ++i) data[i] = static_cast<uint8_t>(i);
    const uint8_t type = 77;
    msgpack11::MsgPack packed{ std::make_tuple(type, data) };

    std::string err;
    msgpack11::MsgPack parsed{ msgpack11::MsgPack::parse(packed.dump(), err ) };
    const uint8_t parsed_type = std::get<0>( parsed.extension_items() );
    const msgpack11::MsgPack::binary& parsed_data = std::get<1>( parsed.extension_items() );

    EXPECT_EQ(data.size(), parsed_data.size());
    EXPECT_EQ(type, parsed_type);
    EXPECT_TRUE(std::equal(data.begin(), data.end(), parsed_data.begin()));
}

TEST(MSGPACK, simple_buffer_fixext_2byte_256)
{
    msgpack11::MsgPack::binary data(256, 0);
    for (std::size_t i = 0; i != data.size(); ++i) data[i] = static_cast<uint8_t>(i);
    const uint8_t type = 77;
    msgpack11::MsgPack packed{ std::make_tuple(type, data) };

    std::string err;
    msgpack11::MsgPack parsed{ msgpack11::MsgPack::parse(packed.dump(), err ) };
    const uint8_t parsed_type = std::get<0>( parsed.extension_items() );
    const msgpack11::MsgPack::binary& parsed_data = std::get<1>( parsed.extension_items() );

    EXPECT_EQ(data.size(), parsed_data.size());
    EXPECT_EQ(type, parsed_type);
    EXPECT_TRUE(std::equal(data.begin(), data.end(), parsed_data.begin()));
}

TEST(MSGPACK, simple_buffer_fixext_2byte_65535)
{
    msgpack11::MsgPack::binary data(65535, 0);
    for (std::size_t i = 0; i != data.size(); ++i) data[i] = static_cast<uint8_t>(i);
    const uint8_t type = 77;
    msgpack11::MsgPack packed{ std::make_tuple(type, data) };

    std::string err;
    msgpack11::MsgPack parsed{ msgpack11::MsgPack::parse(packed.dump(), err ) };
    const uint8_t parsed_type = std::get<0>( parsed.extension_items() );
    const msgpack11::MsgPack::binary& parsed_data = std::get<1>( parsed.extension_items() );

    EXPECT_EQ(data.size(), parsed_data.size());
    EXPECT_EQ(type, parsed_type);
    EXPECT_TRUE(std::equal(data.begin(), data.end(), parsed_data.begin()));
}

TEST(MSGPACK, simple_buffer_fixext_4byte_65536)
{
    msgpack11::MsgPack::binary data(65536, 0);
    for (std::size_t i = 0; i != data.size(); ++i) data[i] = static_cast<uint8_t>(i);
    const uint8_t type = 77;
    msgpack11::MsgPack packed{ std::make_tuple(type, data) };

    std::string err;
    msgpack11::MsgPack parsed{ msgpack11::MsgPack::parse(packed.dump(), err ) };
    const uint8_t parsed_type = std::get<0>( parsed.extension_items() );
    const msgpack11::MsgPack::binary& parsed_data = std::get<1>( parsed.extension_items() );

    EXPECT_EQ(data.size(), parsed_data.size());
    EXPECT_EQ(type, parsed_type);
    EXPECT_TRUE(std::equal(data.begin(), data.end(), parsed_data.begin()));
}

TEST(MSGPACK_STL, simple_buffer_string)
{
    for (unsigned int k = 0; k < kLoop; k++) {
        string val1;
        for (unsigned int i = 0; i < kElements; i++)
            val1 += 'a' + rand() % 26;
        msgpack11::MsgPack packed{val1};

        std::string err;
        msgpack11::MsgPack parsed{ msgpack11::MsgPack::parse(packed.dump(), err ) };

        EXPECT_EQ(parsed.type(), msgpack11::MsgPack::Type::STRING);
        string val2 = parsed.string_value();
        EXPECT_EQ(val1.size(), val2.size());
        EXPECT_EQ(val1, val2);
    }
}

TEST(MSGPACK_STL, simple_buffer_cstring)
{
    for (unsigned int k = 0; k < kLoop; k++) {
        string val1;
        for (unsigned int i = 0; i < kElements; i++)
            val1 += 'a' + rand() % 26;
        msgpack11::MsgPack packed{val1.c_str()};

        std::string err;
        msgpack11::MsgPack parsed{ msgpack11::MsgPack::parse(packed.dump(), err ) };

        EXPECT_EQ(parsed.type(), msgpack11::MsgPack::Type::STRING);
        string val2 = parsed.string_value();
        EXPECT_EQ(val1.size(), val2.size());
        EXPECT_EQ(val1, val2);
    }

}

TEST(MSGPACK_STL, simple_buffer_non_const_cstring)
{
    for (unsigned int k = 0; k < kLoop; k++) {
        string val1;
        for (unsigned int i = 0; i < kElements; i++)
            val1 += 'a' + rand() % 26;
        char* s = new char[val1.size() + 1];
        std::copy(val1.begin(), val1.end(), s );
        s[val1.size()] = '\0';
        msgpack11::MsgPack packed{s};
        delete [] s;

        std::string err;
        msgpack11::MsgPack parsed{ msgpack11::MsgPack::parse(packed.dump(), err ) };

        EXPECT_EQ(parsed.type(), msgpack11::MsgPack::Type::STRING);
        string val2 = parsed.string_value();
        EXPECT_EQ(val1.size(), val2.size());
        EXPECT_EQ(val1, val2);
    }
}

template< typename T, typename std::enable_if< std::is_integral< T >::value, std::nullptr_t >::type = nullptr >
T inc( T value )
{
    return value + static_cast<T>(1);
}

template< typename T, typename std::enable_if< std::is_integral< T >::value, std::nullptr_t >::type = nullptr >
T dec( T value )
{
    return value - static_cast<T>(1);
}

template< typename T, typename std::enable_if< std::is_floating_point< T >::value, std::nullptr_t >::type = nullptr >
T inc( T value )
{
    return std::nextafter( value, std::numeric_limits< T >::max() );
}

template< typename T, typename std::enable_if< std::is_floating_point< T >::value, std::nullptr_t >::type = nullptr >
T dec( T value )
{
    return std::nextafter( value, std::numeric_limits< T >::lowest() );
}

template< typename T >
struct OperatorTest1
{
    static void run()
    {
        msgpack11::MsgPack min_value( std::numeric_limits< T >::lowest() );
        msgpack11::MsgPack max_value( std::numeric_limits< T >::max() );
        msgpack11::MsgPack min_value_inc( inc( std::numeric_limits< T >::lowest() ) );
        msgpack11::MsgPack max_value_dec( dec( std::numeric_limits< T >::max() ) );
        EXPECT_TRUE(min_value == min_value);
        EXPECT_FALSE(min_value == min_value_inc);
        EXPECT_TRUE(min_value < min_value_inc);
        EXPECT_TRUE(max_value == max_value);
        EXPECT_FALSE(max_value == max_value_dec);
        EXPECT_FALSE(max_value < max_value_dec);
        EXPECT_TRUE(min_value < max_value);
    }
};

template< typename T, typename U >
struct OperatorTest2
{
    static void run()
    {
        msgpack11::MsgPack min_value( std::numeric_limits< T >::lowest() );
        msgpack11::MsgPack max_value( std::numeric_limits< U >::max() );
        EXPECT_FALSE(min_value == max_value);
        EXPECT_TRUE(min_value < max_value);
    }
};

TEST(MSGPACK_OPERATOR, simple_operator_uint8)
{
    OperatorTest1< uint8_t >::run();
    OperatorTest2< uint8_t, uint8_t >::run();
    OperatorTest2< uint8_t, uint16_t >::run();
    OperatorTest2< uint8_t, uint32_t >::run();
    OperatorTest2< uint8_t, uint64_t >::run();
    OperatorTest2< uint8_t, int8_t >::run();
    OperatorTest2< uint8_t, int16_t >::run();
    OperatorTest2< uint8_t, int32_t >::run();
    OperatorTest2< uint8_t, int64_t >::run();
    OperatorTest2< uint8_t, float >::run();
    OperatorTest2< uint8_t, double >::run();
}

TEST(MSGPACK_OPERATOR, simple_operator_uint16)
{
    OperatorTest1< uint16_t >::run();
    OperatorTest2< uint16_t, uint16_t >::run();
    OperatorTest2< uint16_t, uint32_t >::run();
    OperatorTest2< uint16_t, uint64_t >::run();
    OperatorTest2< uint16_t, int8_t >::run();
    OperatorTest2< uint16_t, int16_t >::run();
    OperatorTest2< uint16_t, int32_t >::run();
    OperatorTest2< uint16_t, int64_t >::run();
    OperatorTest2< uint16_t, float >::run();
    OperatorTest2< uint16_t, double >::run();
}

TEST(MSGPACK_OPERATOR, simple_operator_uint32)
{
    OperatorTest1< uint32_t >::run();
    OperatorTest2< uint32_t, uint8_t >::run();
    OperatorTest2< uint32_t, uint16_t >::run();
    OperatorTest2< uint32_t, uint32_t >::run();
    OperatorTest2< uint32_t, uint64_t >::run();
    OperatorTest2< uint32_t, int8_t >::run();
    OperatorTest2< uint32_t, int16_t >::run();
    OperatorTest2< uint32_t, int32_t >::run();
    OperatorTest2< uint32_t, int64_t >::run();
    OperatorTest2< uint32_t, float >::run();
    OperatorTest2< uint32_t, double >::run();
}

TEST(MSGPACK_OPERATOR, simple_operator_uint64)
{
    OperatorTest1< uint64_t >::run();
    OperatorTest2< uint64_t, uint8_t >::run();
    OperatorTest2< uint64_t, uint16_t >::run();
    OperatorTest2< uint64_t, uint32_t >::run();
    OperatorTest2< uint64_t, uint64_t >::run();
    OperatorTest2< uint64_t, int8_t >::run();
    OperatorTest2< uint64_t, int16_t >::run();
    OperatorTest2< uint64_t, int32_t >::run();
    OperatorTest2< uint64_t, int64_t >::run();
    OperatorTest2< uint64_t, float >::run();
    OperatorTest2< uint64_t, double >::run();
}

TEST(MSGPACK_OPERATOR, simple_operator_int8)
{
    OperatorTest1< int8_t >::run();
    OperatorTest2< int8_t, uint8_t >::run();
    OperatorTest2< int8_t, uint16_t >::run();
    OperatorTest2< int8_t, uint32_t >::run();
    OperatorTest2< int8_t, uint64_t >::run();
    OperatorTest2< int8_t, int8_t >::run();
    OperatorTest2< int8_t, int16_t >::run();
    OperatorTest2< int8_t, int32_t >::run();
    OperatorTest2< int8_t, int64_t >::run();
    OperatorTest2< int8_t, float >::run();
    OperatorTest2< int8_t, double >::run();
}

TEST(MSGPACK_OPERATOR, simple_operator_int16)
{
    OperatorTest1< int16_t >::run();
    OperatorTest2< int16_t, uint8_t >::run();
    OperatorTest2< int16_t, uint16_t >::run();
    OperatorTest2< int16_t, uint32_t >::run();
    OperatorTest2< int16_t, uint64_t >::run();
    OperatorTest2< int16_t, int8_t >::run();
    OperatorTest2< int16_t, int16_t >::run();
    OperatorTest2< int16_t, int32_t >::run();
    OperatorTest2< int16_t, int64_t >::run();
    OperatorTest2< int16_t, float >::run();
    OperatorTest2< int16_t, double >::run();
}

TEST(MSGPACK_OPERATOR, simple_operator_int32)
{
    OperatorTest1< int32_t >::run();
    OperatorTest2< int32_t, uint8_t >::run();
    OperatorTest2< int32_t, uint16_t >::run();
    OperatorTest2< int32_t, uint32_t >::run();
    OperatorTest2< int32_t, uint64_t >::run();
    OperatorTest2< int32_t, int8_t >::run();
    OperatorTest2< int32_t, int16_t >::run();
    OperatorTest2< int32_t, int32_t >::run();
    OperatorTest2< int32_t, int64_t >::run();
    OperatorTest2< int32_t, float >::run();
    OperatorTest2< int32_t, double >::run();
}

TEST(MSGPACK_OPERATOR, simple_operator_int64)
{
    OperatorTest1< int64_t >::run();
    OperatorTest2< int64_t, uint8_t >::run();
    OperatorTest2< int64_t, uint16_t >::run();
    OperatorTest2< int64_t, uint32_t >::run();
    OperatorTest2< int64_t, uint64_t >::run();
    OperatorTest2< int64_t, int8_t >::run();
    OperatorTest2< int64_t, int16_t >::run();
    OperatorTest2< int64_t, int32_t >::run();
    OperatorTest2< int64_t, int64_t >::run();
    OperatorTest2< int64_t, float >::run();
    OperatorTest2< int64_t, double >::run();

    msgpack11::MsgPack int64_max_value( std::numeric_limits<int64_t>::max() );
    msgpack11::MsgPack int64_min_value( std::numeric_limits<int64_t>::lowest() );
    msgpack11::MsgPack int64_max_value_ull( static_cast<uint64_t>(std::numeric_limits<int64_t>::max()) );
    msgpack11::MsgPack int64_max_value_p1_ull( static_cast<uint64_t>( static_cast<uint64_t>(std::numeric_limits<int64_t>::max()) + 1ULL ) );
    EXPECT_TRUE(int64_max_value == int64_max_value_ull);
    EXPECT_TRUE(int64_max_value < int64_max_value_p1_ull);
    EXPECT_TRUE(int64_min_value < int64_max_value_ull);
    EXPECT_TRUE(int64_min_value < int64_max_value_p1_ull);
}

TEST(MSGPACK_OPERATOR, simple_operator_float32)
{
    OperatorTest1< float >::run();
    OperatorTest2< float, uint8_t >::run();
    OperatorTest2< float, uint16_t >::run();
    OperatorTest2< float, uint32_t >::run();
    OperatorTest2< float, uint64_t >::run();
    OperatorTest2< float, int8_t >::run();
    OperatorTest2< float, int16_t >::run();
    OperatorTest2< float, int32_t >::run();
    OperatorTest2< float, int64_t >::run();
    OperatorTest2< float, float >::run();
    OperatorTest2< float, double >::run();
}

TEST(MSGPACK_OPERATOR, simple_operator_float64)
{
    OperatorTest1< double >::run();
    OperatorTest2< double, uint8_t >::run();
    OperatorTest2< double, uint16_t >::run();
    OperatorTest2< double, uint32_t >::run();
    OperatorTest2< double, uint64_t >::run();
    OperatorTest2< double, int8_t >::run();
    OperatorTest2< double, int16_t >::run();
    OperatorTest2< double, int32_t >::run();
    OperatorTest2< double, int64_t >::run();
    OperatorTest2< double, float >::run();
    OperatorTest2< double, double >::run();
}

TEST(MSGPACK_OPERATOR, simple_operator_others)
{
    msgpack11::MsgPack nul_value;
    msgpack11::MsgPack number_value(1.0f);
    msgpack11::MsgPack bool_value(true);
    msgpack11::MsgPack string_value("string");
    msgpack11::MsgPack binary_value(msgpack11::MsgPack::binary{0});
    msgpack11::MsgPack array_value(msgpack11::MsgPack::array{ nul_value });
    msgpack11::MsgPack object_value(msgpack11::MsgPack::object{{0,1}});
    msgpack11::MsgPack extension_value(msgpack11::MsgPack::extension{0,msgpack11::MsgPack::binary{0}});

    EXPECT_TRUE(nul_value == nul_value);
    EXPECT_TRUE(number_value == number_value);
    EXPECT_TRUE(bool_value == bool_value);
    EXPECT_TRUE(string_value == string_value);
    EXPECT_TRUE(binary_value == binary_value);
    EXPECT_TRUE(array_value == array_value);
    EXPECT_TRUE(object_value == object_value);
    EXPECT_TRUE(extension_value == extension_value);

    EXPECT_TRUE(nul_value < number_value);
    EXPECT_TRUE(number_value < bool_value);
    EXPECT_TRUE(bool_value < string_value);
    EXPECT_TRUE(string_value < binary_value);
    EXPECT_TRUE(binary_value < array_value);
    EXPECT_TRUE(array_value < object_value);
    EXPECT_TRUE(object_value < extension_value);
    EXPECT_FALSE(extension_value < nul_value);
}

TEST(MSGPACK_PREDICATES, predicates_null)
{
    msgpack11::MsgPack null_value;
    EXPECT_TRUE(null_value.is_null());
    EXPECT_FALSE(null_value.is_bool());
    EXPECT_FALSE(null_value.is_number());
    EXPECT_FALSE(null_value.is_float32());
    EXPECT_FALSE(null_value.is_float64());
    EXPECT_FALSE(null_value.is_int());
    EXPECT_FALSE(null_value.is_int8());
    EXPECT_FALSE(null_value.is_int16());
    EXPECT_FALSE(null_value.is_int32());
    EXPECT_FALSE(null_value.is_int64());
    EXPECT_FALSE(null_value.is_uint8());
    EXPECT_FALSE(null_value.is_uint16());
    EXPECT_FALSE(null_value.is_uint32());
    EXPECT_FALSE(null_value.is_uint64());
    EXPECT_FALSE(null_value.is_string());
    EXPECT_FALSE(null_value.is_array());
    EXPECT_FALSE(null_value.is_binary());
    EXPECT_FALSE(null_value.is_object());
    EXPECT_FALSE(null_value.is_extension());
}

TEST(MSGPACK_PREDICATES, predicates_float)
{
    msgpack11::MsgPack float_value(0.0f);
    EXPECT_FALSE(float_value.is_null());
    EXPECT_FALSE(float_value.is_bool());
    EXPECT_TRUE(float_value.is_number());
    EXPECT_TRUE(float_value.is_float32());
    EXPECT_FALSE(float_value.is_float64());
    EXPECT_FALSE(float_value.is_int());
    EXPECT_FALSE(float_value.is_int8());
    EXPECT_FALSE(float_value.is_int16());
    EXPECT_FALSE(float_value.is_int32());
    EXPECT_FALSE(float_value.is_int64());
    EXPECT_FALSE(float_value.is_uint8());
    EXPECT_FALSE(float_value.is_uint16());
    EXPECT_FALSE(float_value.is_uint32());
    EXPECT_FALSE(float_value.is_uint64());
    EXPECT_FALSE(float_value.is_string());
    EXPECT_FALSE(float_value.is_array());
    EXPECT_FALSE(float_value.is_binary());
    EXPECT_FALSE(float_value.is_object());
    EXPECT_FALSE(float_value.is_extension());
}

TEST(MSGPACK_PREDICATES, predicates_double)
{
    msgpack11::MsgPack double_value(0.0);
    EXPECT_FALSE(double_value.is_null());
    EXPECT_FALSE(double_value.is_bool());
    EXPECT_TRUE(double_value.is_number());
    EXPECT_FALSE(double_value.is_float32());
    EXPECT_TRUE(double_value.is_float64());
    EXPECT_FALSE(double_value.is_int());
    EXPECT_FALSE(double_value.is_int8());
    EXPECT_FALSE(double_value.is_int16());
    EXPECT_FALSE(double_value.is_int32());
    EXPECT_FALSE(double_value.is_int64());
    EXPECT_FALSE(double_value.is_uint8());
    EXPECT_FALSE(double_value.is_uint16());
    EXPECT_FALSE(double_value.is_uint32());
    EXPECT_FALSE(double_value.is_uint64());
    EXPECT_FALSE(double_value.is_string());
    EXPECT_FALSE(double_value.is_array());
    EXPECT_FALSE(double_value.is_binary());
    EXPECT_FALSE(double_value.is_object());
    EXPECT_FALSE(double_value.is_extension());
}

TEST(MSGPACK_PREDICATES, predicates_int8)
{
    msgpack11::MsgPack double_value(static_cast<int8_t>(0x00));
    EXPECT_FALSE(double_value.is_null());
    EXPECT_FALSE(double_value.is_bool());
    EXPECT_TRUE(double_value.is_number());
    EXPECT_FALSE(double_value.is_float32());
    EXPECT_FALSE(double_value.is_float64());
    EXPECT_TRUE(double_value.is_int());
    EXPECT_TRUE(double_value.is_int8());
    EXPECT_FALSE(double_value.is_int16());
    EXPECT_FALSE(double_value.is_int32());
    EXPECT_FALSE(double_value.is_int64());
    EXPECT_FALSE(double_value.is_uint8());
    EXPECT_FALSE(double_value.is_uint16());
    EXPECT_FALSE(double_value.is_uint32());
    EXPECT_FALSE(double_value.is_uint64());
    EXPECT_FALSE(double_value.is_string());
    EXPECT_FALSE(double_value.is_array());
    EXPECT_FALSE(double_value.is_binary());
    EXPECT_FALSE(double_value.is_object());
    EXPECT_FALSE(double_value.is_extension());
}

TEST(MSGPACK_PREDICATES, predicates_int16)
{
    msgpack11::MsgPack double_value(static_cast<int16_t>(0x00));
    EXPECT_FALSE(double_value.is_null());
    EXPECT_FALSE(double_value.is_bool());
    EXPECT_TRUE(double_value.is_number());
    EXPECT_FALSE(double_value.is_float32());
    EXPECT_FALSE(double_value.is_float64());
    EXPECT_TRUE(double_value.is_int());
    EXPECT_FALSE(double_value.is_int8());
    EXPECT_TRUE(double_value.is_int16());
    EXPECT_FALSE(double_value.is_int32());
    EXPECT_FALSE(double_value.is_int64());
    EXPECT_FALSE(double_value.is_uint8());
    EXPECT_FALSE(double_value.is_uint16());
    EXPECT_FALSE(double_value.is_uint32());
    EXPECT_FALSE(double_value.is_uint64());
    EXPECT_FALSE(double_value.is_string());
    EXPECT_FALSE(double_value.is_array());
    EXPECT_FALSE(double_value.is_binary());
    EXPECT_FALSE(double_value.is_object());
    EXPECT_FALSE(double_value.is_extension());
}

TEST(MSGPACK_PREDICATES, predicates_int32)
{
    msgpack11::MsgPack double_value(static_cast<int32_t>(0x00));
    EXPECT_FALSE(double_value.is_null());
    EXPECT_FALSE(double_value.is_bool());
    EXPECT_TRUE(double_value.is_number());
    EXPECT_FALSE(double_value.is_float32());
    EXPECT_FALSE(double_value.is_float64());
    EXPECT_TRUE(double_value.is_int());
    EXPECT_FALSE(double_value.is_int8());
    EXPECT_FALSE(double_value.is_int16());
    EXPECT_TRUE(double_value.is_int32());
    EXPECT_FALSE(double_value.is_int64());
    EXPECT_FALSE(double_value.is_uint8());
    EXPECT_FALSE(double_value.is_uint16());
    EXPECT_FALSE(double_value.is_uint32());
    EXPECT_FALSE(double_value.is_uint64());
    EXPECT_FALSE(double_value.is_string());
    EXPECT_FALSE(double_value.is_array());
    EXPECT_FALSE(double_value.is_binary());
    EXPECT_FALSE(double_value.is_object());
    EXPECT_FALSE(double_value.is_extension());
}

TEST(MSGPACK_PREDICATES, predicates_int64)
{
    msgpack11::MsgPack double_value(static_cast<int64_t>(0x00));
    EXPECT_FALSE(double_value.is_null());
    EXPECT_FALSE(double_value.is_bool());
    EXPECT_TRUE(double_value.is_number());
    EXPECT_FALSE(double_value.is_float32());
    EXPECT_FALSE(double_value.is_float64());
    EXPECT_TRUE(double_value.is_int());
    EXPECT_FALSE(double_value.is_int8());
    EXPECT_FALSE(double_value.is_int16());
    EXPECT_FALSE(double_value.is_int32());
    EXPECT_TRUE(double_value.is_int64());
    EXPECT_FALSE(double_value.is_uint8());
    EXPECT_FALSE(double_value.is_uint16());
    EXPECT_FALSE(double_value.is_uint32());
    EXPECT_FALSE(double_value.is_uint64());
    EXPECT_FALSE(double_value.is_string());
    EXPECT_FALSE(double_value.is_array());
    EXPECT_FALSE(double_value.is_binary());
    EXPECT_FALSE(double_value.is_object());
    EXPECT_FALSE(double_value.is_extension());
}

TEST(MSGPACK_PREDICATES, predicates_uint8)
{
    msgpack11::MsgPack double_value(static_cast<uint8_t>(0x00));
    EXPECT_FALSE(double_value.is_null());
    EXPECT_FALSE(double_value.is_bool());
    EXPECT_TRUE(double_value.is_number());
    EXPECT_FALSE(double_value.is_float32());
    EXPECT_FALSE(double_value.is_float64());
    EXPECT_TRUE(double_value.is_int());
    EXPECT_FALSE(double_value.is_int8());
    EXPECT_FALSE(double_value.is_int16());
    EXPECT_FALSE(double_value.is_int32());
    EXPECT_FALSE(double_value.is_int64());
    EXPECT_TRUE(double_value.is_uint8());
    EXPECT_FALSE(double_value.is_uint16());
    EXPECT_FALSE(double_value.is_uint32());
    EXPECT_FALSE(double_value.is_uint64());
    EXPECT_FALSE(double_value.is_string());
    EXPECT_FALSE(double_value.is_array());
    EXPECT_FALSE(double_value.is_binary());
    EXPECT_FALSE(double_value.is_object());
    EXPECT_FALSE(double_value.is_extension());
}

TEST(MSGPACK_PREDICATES, predicates_uint16)
{
    msgpack11::MsgPack double_value(static_cast<uint16_t>(0x00));
    EXPECT_FALSE(double_value.is_null());
    EXPECT_FALSE(double_value.is_bool());
    EXPECT_TRUE(double_value.is_number());
    EXPECT_FALSE(double_value.is_float32());
    EXPECT_FALSE(double_value.is_float64());
    EXPECT_TRUE(double_value.is_int());
    EXPECT_FALSE(double_value.is_int8());
    EXPECT_FALSE(double_value.is_int16());
    EXPECT_FALSE(double_value.is_int32());
    EXPECT_FALSE(double_value.is_int64());
    EXPECT_FALSE(double_value.is_uint8());
    EXPECT_TRUE(double_value.is_uint16());
    EXPECT_FALSE(double_value.is_uint32());
    EXPECT_FALSE(double_value.is_uint64());
    EXPECT_FALSE(double_value.is_string());
    EXPECT_FALSE(double_value.is_array());
    EXPECT_FALSE(double_value.is_binary());
    EXPECT_FALSE(double_value.is_object());
    EXPECT_FALSE(double_value.is_extension());
}

TEST(MSGPACK_PREDICATES, predicates_uint32)
{
    msgpack11::MsgPack double_value(static_cast<uint32_t>(0x00));
    EXPECT_FALSE(double_value.is_null());
    EXPECT_FALSE(double_value.is_bool());
    EXPECT_TRUE(double_value.is_number());
    EXPECT_FALSE(double_value.is_float32());
    EXPECT_FALSE(double_value.is_float64());
    EXPECT_TRUE(double_value.is_int());
    EXPECT_FALSE(double_value.is_int8());
    EXPECT_FALSE(double_value.is_int16());
    EXPECT_FALSE(double_value.is_int32());
    EXPECT_FALSE(double_value.is_int64());
    EXPECT_FALSE(double_value.is_uint8());
    EXPECT_FALSE(double_value.is_uint16());
    EXPECT_TRUE(double_value.is_uint32());
    EXPECT_FALSE(double_value.is_uint64());
    EXPECT_FALSE(double_value.is_string());
    EXPECT_FALSE(double_value.is_array());
    EXPECT_FALSE(double_value.is_binary());
    EXPECT_FALSE(double_value.is_object());
    EXPECT_FALSE(double_value.is_extension());
}

TEST(MSGPACK_PREDICATES, predicates_uint64)
{
    msgpack11::MsgPack double_value(static_cast<uint64_t>(0x00));
    EXPECT_FALSE(double_value.is_null());
    EXPECT_FALSE(double_value.is_bool());
    EXPECT_TRUE(double_value.is_number());
    EXPECT_FALSE(double_value.is_float32());
    EXPECT_FALSE(double_value.is_float64());
    EXPECT_TRUE(double_value.is_int());
    EXPECT_FALSE(double_value.is_int8());
    EXPECT_FALSE(double_value.is_int16());
    EXPECT_FALSE(double_value.is_int32());
    EXPECT_FALSE(double_value.is_int64());
    EXPECT_FALSE(double_value.is_uint8());
    EXPECT_FALSE(double_value.is_uint16());
    EXPECT_FALSE(double_value.is_uint32());
    EXPECT_TRUE(double_value.is_uint64());
    EXPECT_FALSE(double_value.is_string());
    EXPECT_FALSE(double_value.is_array());
    EXPECT_FALSE(double_value.is_binary());
    EXPECT_FALSE(double_value.is_object());
    EXPECT_FALSE(double_value.is_extension());
}

TEST(MSGPACK_PREDICATES, predicates_bool)
{
    msgpack11::MsgPack bool_value(true);
    EXPECT_FALSE(bool_value.is_null());
    EXPECT_TRUE(bool_value.is_bool());
    EXPECT_FALSE(bool_value.is_number());
    EXPECT_FALSE(bool_value.is_float32());
    EXPECT_FALSE(bool_value.is_float64());
    EXPECT_FALSE(bool_value.is_int());
    EXPECT_FALSE(bool_value.is_int8());
    EXPECT_FALSE(bool_value.is_int16());
    EXPECT_FALSE(bool_value.is_int32());
    EXPECT_FALSE(bool_value.is_int64());
    EXPECT_FALSE(bool_value.is_uint8());
    EXPECT_FALSE(bool_value.is_uint16());
    EXPECT_FALSE(bool_value.is_uint32());
    EXPECT_FALSE(bool_value.is_uint64());
    EXPECT_FALSE(bool_value.is_string());
    EXPECT_FALSE(bool_value.is_array());
    EXPECT_FALSE(bool_value.is_binary());
    EXPECT_FALSE(bool_value.is_object());
    EXPECT_FALSE(bool_value.is_extension());
}

TEST(MSGPACK_PREDICATES, predicates_string)
{
    msgpack11::MsgPack string_value(std::string{});
    EXPECT_FALSE(string_value.is_null());
    EXPECT_FALSE(string_value.is_bool());
    EXPECT_FALSE(string_value.is_number());
    EXPECT_FALSE(string_value.is_float32());
    EXPECT_FALSE(string_value.is_float64());
    EXPECT_FALSE(string_value.is_int());
    EXPECT_FALSE(string_value.is_int8());
    EXPECT_FALSE(string_value.is_int16());
    EXPECT_FALSE(string_value.is_int32());
    EXPECT_FALSE(string_value.is_int64());
    EXPECT_FALSE(string_value.is_uint8());
    EXPECT_FALSE(string_value.is_uint16());
    EXPECT_FALSE(string_value.is_uint32());
    EXPECT_FALSE(string_value.is_uint64());
    EXPECT_TRUE(string_value.is_string());
    EXPECT_FALSE(string_value.is_array());
    EXPECT_FALSE(string_value.is_binary());
    EXPECT_FALSE(string_value.is_object());
    EXPECT_FALSE(string_value.is_extension());
}

TEST(MSGPACK_PREDICATES, predicates_array)
{
    msgpack11::MsgPack array_value(msgpack11::MsgPack::array{});
    EXPECT_FALSE(array_value.is_null());
    EXPECT_FALSE(array_value.is_bool());
    EXPECT_FALSE(array_value.is_number());
    EXPECT_FALSE(array_value.is_float32());
    EXPECT_FALSE(array_value.is_float64());
    EXPECT_FALSE(array_value.is_int());
    EXPECT_FALSE(array_value.is_int8());
    EXPECT_FALSE(array_value.is_int16());
    EXPECT_FALSE(array_value.is_int32());
    EXPECT_FALSE(array_value.is_int64());
    EXPECT_FALSE(array_value.is_uint8());
    EXPECT_FALSE(array_value.is_uint16());
    EXPECT_FALSE(array_value.is_uint32());
    EXPECT_FALSE(array_value.is_uint64());
    EXPECT_FALSE(array_value.is_string());
    EXPECT_TRUE(array_value.is_array());
    EXPECT_FALSE(array_value.is_binary());
    EXPECT_FALSE(array_value.is_object());
    EXPECT_FALSE(array_value.is_extension());
}

TEST(MSGPACK_PREDICATES, predicates_binary)
{
    msgpack11::MsgPack binary_value(msgpack11::MsgPack::binary{});
    EXPECT_FALSE(binary_value.is_null());
    EXPECT_FALSE(binary_value.is_bool());
    EXPECT_FALSE(binary_value.is_number());
    EXPECT_FALSE(binary_value.is_float32());
    EXPECT_FALSE(binary_value.is_float64());
    EXPECT_FALSE(binary_value.is_int());
    EXPECT_FALSE(binary_value.is_int8());
    EXPECT_FALSE(binary_value.is_int16());
    EXPECT_FALSE(binary_value.is_int32());
    EXPECT_FALSE(binary_value.is_int64());
    EXPECT_FALSE(binary_value.is_uint8());
    EXPECT_FALSE(binary_value.is_uint16());
    EXPECT_FALSE(binary_value.is_uint32());
    EXPECT_FALSE(binary_value.is_uint64());
    EXPECT_FALSE(binary_value.is_string());
    EXPECT_FALSE(binary_value.is_array());
    EXPECT_TRUE(binary_value.is_binary());
    EXPECT_FALSE(binary_value.is_object());
    EXPECT_FALSE(binary_value.is_extension());
}

TEST(MSGPACK_PREDICATES, predicates_object)
{
    msgpack11::MsgPack object_value(msgpack11::MsgPack::object{});
    EXPECT_FALSE(object_value.is_null());
    EXPECT_FALSE(object_value.is_bool());
    EXPECT_FALSE(object_value.is_number());
    EXPECT_FALSE(object_value.is_float32());
    EXPECT_FALSE(object_value.is_float64());
    EXPECT_FALSE(object_value.is_int());
    EXPECT_FALSE(object_value.is_int8());
    EXPECT_FALSE(object_value.is_int16());
    EXPECT_FALSE(object_value.is_int32());
    EXPECT_FALSE(object_value.is_int64());
    EXPECT_FALSE(object_value.is_uint8());
    EXPECT_FALSE(object_value.is_uint16());
    EXPECT_FALSE(object_value.is_uint32());
    EXPECT_FALSE(object_value.is_uint64());
    EXPECT_FALSE(object_value.is_string());
    EXPECT_FALSE(object_value.is_array());
    EXPECT_FALSE(object_value.is_binary());
    EXPECT_TRUE(object_value.is_object());
    EXPECT_FALSE(object_value.is_extension());
}

TEST(MSGPACK_PREDICATES, predicates_extension)
{
    msgpack11::MsgPack extension_value(msgpack11::MsgPack::extension{});
    EXPECT_FALSE(extension_value.is_null());
    EXPECT_FALSE(extension_value.is_bool());
    EXPECT_FALSE(extension_value.is_number());
    EXPECT_FALSE(extension_value.is_float32());
    EXPECT_FALSE(extension_value.is_float64());
    EXPECT_FALSE(extension_value.is_int());
    EXPECT_FALSE(extension_value.is_int8());
    EXPECT_FALSE(extension_value.is_int16());
    EXPECT_FALSE(extension_value.is_int32());
    EXPECT_FALSE(extension_value.is_int64());
    EXPECT_FALSE(extension_value.is_uint8());
    EXPECT_FALSE(extension_value.is_uint16());
    EXPECT_FALSE(extension_value.is_uint32());
    EXPECT_FALSE(extension_value.is_uint64());
    EXPECT_FALSE(extension_value.is_string());
    EXPECT_FALSE(extension_value.is_array());
    EXPECT_FALSE(extension_value.is_binary());
    EXPECT_FALSE(extension_value.is_object());
    EXPECT_TRUE(extension_value.is_extension());
}
