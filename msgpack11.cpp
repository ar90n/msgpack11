#include "msgpack11.hpp"
#include <array>
#include <cassert>
#include <cmath>
#include <cstdlib>
#include <cstdio>
#include <limits>
#include <array>
#include <tuple>
#include <algorithm>
#include <functional>
#include <stdexcept>

namespace msgpack11 {

static const int max_depth = 200;

using std::string;
using std::vector;
using std::map;
using std::make_shared;
using std::initializer_list;
using std::move;

/* Helper for representing null - just a do-nothing struct, plus comparison
 * operators so the helpers in MsgPackValue work. We can't use nullptr_t because
 * it may not be orderable.
 */
struct NullStruct {
    bool operator==(NullStruct) const { return true; }
    bool operator<(NullStruct) const { return false; }
};

/* * * * * * * * * * * * * * * * * * * *
 * MasPackValue
 */

class MsgPackValue {
public:
    virtual bool equals(const MsgPackValue * other) const = 0;
    virtual bool less(const MsgPackValue * other) const = 0;
    virtual void dump(std::string &out) const = 0;
    virtual MsgPack::Type type() const = 0;
    virtual double number_value() const;
    virtual float float32_value() const;
    virtual double float64_value() const;
    virtual int32_t int_value() const;
    virtual int8_t int8_value() const;
    virtual int16_t int16_value() const;
    virtual int32_t int32_value() const;
    virtual int64_t int64_value() const;
    virtual uint8_t uint8_value() const;
    virtual uint16_t uint16_value() const;
    virtual uint32_t uint32_value() const;
    virtual uint64_t uint64_value() const;
    virtual bool bool_value() const;
    virtual const std::string &string_value() const;
    virtual const MsgPack::array &array_items() const;
    virtual const MsgPack::binary &binary_items() const;
    virtual const MsgPack &operator[](size_t i) const;
    virtual const MsgPack::object &object_items() const;
    virtual const MsgPack &operator[](const std::string &key) const;
    virtual const MsgPack::extension &extension_items() const;
    virtual ~MsgPackValue() {}
};

/* * * * * * * * * * * * * * * * * * * *
 * Serialization
 */

namespace {
static const union {
    uint16_t dummy;
    uint8_t bytes[2];
} endian_check_data { 0x0001 };
static const bool is_big_endian = endian_check_data.bytes[0] == 0x00;

template< typename T >
void dump_data(T value, std::string &out)
{
    union {
        T packed;
        std::array<uint8_t, sizeof(T)> bytes;
    } converter;
    converter.packed = value;

    int const n = sizeof(T);
    int const offsets[] = {(n-1), 0};
    int const directions[] = {-1, 1};
    uint8_t const off = offsets[static_cast<int>(is_big_endian)];
    int const dir = directions[static_cast<int>(is_big_endian)];
    for(int i = 0; i < n; ++i)
    {
        out.push_back(converter.bytes[off + dir * i]);
    }
}

inline void dump(NullStruct, std::string &out) {
    out.push_back(0xc0);
}

inline void dump(float value, std::string &out) {
    out.push_back(0xca);
    dump_data(value, out);
}

inline void dump(double value, std::string &out) {
    out.push_back(0xcb);
    dump_data(value, out);
}

inline void dump(uint8_t value, std::string &out) {
    if(128 <= value)
    {
        out.push_back(0xcc);
    }
    out.push_back(value);
}

inline void dump(uint16_t value, std::string &out) {
    if( value < (1 << 8) )
    {
        dump(static_cast<uint8_t>(value), out );
    }
    else
    {
        out.push_back(0xcd);
        dump_data(value, out);
    }
}

inline void dump(uint32_t value, std::string &out) {
    if( value < (1 << 16) )
    {
        dump(static_cast<uint16_t>(value), out );
    }
    else
    {
        out.push_back(0xce);
        dump_data(value, out);
    }
}

inline void dump(uint64_t value, std::string &out) {
    if( value < (1ULL << 32) )
    {
        dump(static_cast<uint32_t>(value), out );
    }
    else
    {
        out.push_back(0xcf);
        dump_data(value, out);
    }
}


inline void dump(int8_t value, std::string &out) {
    if( value < -32 )
    {
        out.push_back(0xd0);
    }
    out.push_back(value);
}

inline void dump(int16_t value, std::string &out) {
    if( value < -(1 << 7) )
    {
        out.push_back(0xd1);
        dump_data(value, out);
    }
    else if( value <= 0 )
    {
        dump(static_cast<int8_t>(value), out );
    }
    else
    {
        dump(static_cast<uint16_t>(value), out );
    }
}

inline void dump(int32_t value, std::string &out) {
    if( value < -(1 << 15) )
    {
        out.push_back(0xd2);
        dump_data(value, out);
    }
    else if( value <= 0 )
    {
        dump(static_cast<int16_t>(value), out );
    }
    else
    {
        dump(static_cast<uint32_t>(value), out );
    }
}

inline void dump(int64_t value, std::string &out) {
    if( value < -(1LL << 31) )
    {
        out.push_back(0xd3);
        dump_data(value, out);
    }
    else if( value <= 0 )
    {
        dump(static_cast<int32_t>(value), out );
    }
    else
    {
        dump(static_cast<uint64_t>(value), out );
    }
}

inline void dump(bool value, std::string &out) {
    const uint8_t msgpack_value = (value) ? 0xc3 : 0xc2;
    out.push_back(msgpack_value);
}

inline void dump(const std::string& value, std::string &out) {
    size_t const len = value.size();
    if(len <= 0x1f)
    {
        uint8_t const first_byte = 0xa0 | static_cast<uint8_t>(len);
        out.push_back(first_byte);
    }
    else if(len <= 0xff)
    {
        out.push_back(0xd9);
        out.push_back(static_cast<uint8_t>(len));
    }
    else if(len <= 0xffff)
    {
        out.push_back(0xda);
        dump_data(static_cast<uint16_t>(len), out);
    }
    else if(len <= 0xffffffff)
    {
        out.push_back(0xdb);
        dump_data(static_cast<uint32_t>(len), out);
    }
    else
    {
        throw std::runtime_error("exceeded maximum data length");
    }

    std::for_each(std::begin(value), std::end(value), [&out](char v){
        dump_data(v, out);
    });
}

inline void dump(const MsgPack::array& value, std::string &out) {
    size_t const len = value.size();
    if(len <= 15)
    {
        uint8_t const first_byte = 0x90 | static_cast<uint8_t>(len);
        out.push_back(first_byte);
    }
    else if(len <= 0xffff)
    {
        out.push_back(0xdc);
        dump_data(static_cast<uint16_t>(len), out);
    }
    else if(len <= 0xffffffff)
    {
        out.push_back(0xdd);
        dump_data(static_cast<uint32_t>(len), out);
    }
    else
    {
        throw std::runtime_error("exceeded maximum data length");
    }

    std::for_each(std::begin(value), std::end(value), [&out](MsgPack::array::value_type const& v){
        v.dump(out);
    });
}

inline void dump(const MsgPack::object& value, std::string &out) {
    size_t const len = value.size();
    if(len <= 15)
    {
        uint8_t const first_byte = 0x80 | static_cast<uint8_t>(len);
        out.push_back(first_byte);
    }
    else if(len <= 0xffff)
    {
        out.push_back(0xde);
        dump_data(static_cast<uint16_t>(len), out);
    }
    else if(len <= 0xffffffff)
    {
        out.push_back(0xdf);
        dump_data(static_cast<uint32_t>(len), out);
    }
    else
    {
        throw std::runtime_error("too long value.");
    }

    std::for_each(std::begin(value), std::end(value), [&out](MsgPack::object::value_type const& v){
        v.first.dump(out);
        v.second.dump(out);
    });
}

inline void dump(const MsgPack::binary& value, std::string &out) {
    size_t const len = value.size();
    if(len <= 0xff)
    {
        out.push_back(0xc4);
        dump_data(static_cast<uint8_t>(len), out);
    }
    else if(len <= 0xffff)
    {
        out.push_back(0xc5);
        dump_data(static_cast<uint16_t>(len), out);
    }
    else if(len <= 0xffffffff)
    {
        out.push_back(0xc6);
        dump_data(static_cast<uint32_t>(len), out);
    }
    else
    {
        throw std::runtime_error("exceeded maximum data length");
    }

    std::for_each(std::begin(value), std::end(value), [&out](MsgPack::binary::value_type const& v){
        out.push_back(v);
    });
}

inline void dump(const MsgPack::extension& value, std::string &out) {
    const uint8_t type = std::get<0>( value );
    const MsgPack::binary& data = std::get<1>( value );
    const size_t len = data.size();

    if(len == 0x01) {
        out.push_back(0xd4);
    }
    else if(len == 0x02) {
        out.push_back(0xd5);
    }
    else if(len == 0x04) {
        out.push_back(0xd6);
    }
    else if(len == 0x08) {
        out.push_back(0xd7);
    }
    else if(len == 0x10) {
        out.push_back(0xd8);
    }
    else if(len <= 0xff) {
        out.push_back(0xc7);
        out.push_back(static_cast<uint8_t>(len));
    }
    else if(len <= 0xffff) {
        out.push_back(0xc8);
        dump_data(static_cast<uint16_t>(len), out);
    }
    else if(len <= 0xffffffff) {
        out.push_back(0xc9);
        dump_data(static_cast<uint32_t>(len), out);
    }
    else {
        throw std::runtime_error("exceeded maximum data length");
    }

    out.push_back(type);
    std::copy(std::begin(data), std::end(data), std::back_inserter(out));
}
}

void MsgPack::dump(std::string &out) const {
    m_ptr->dump(out);
}

/* * * * * * * * * * * * * * * * * * * *
 * Value wrappers
 */

template <MsgPack::Type tag, typename T>
class Value : public MsgPackValue {
protected:

    // Constructors
    explicit Value(const T &value) : m_value(value) {}
    explicit Value(T &&value)      : m_value(move(value)) {}

    // Get type tag
    MsgPack::Type type() const override {
        return tag;
    }

    // Comparisons
    bool equals(const MsgPackValue * other) const override {
        bool const is_same_type = tag == other->type();
        return is_same_type && (m_value == static_cast<const Value<tag, T> *>(other)->m_value);
    }
    bool less(const MsgPackValue * other) const override {
        bool const is_same_type = tag == other->type();
        bool const is_less_type = tag < other->type();
        return is_less_type || (is_same_type && (m_value < static_cast<const Value<tag, T> *>(other)->m_value));
    }

    const T m_value;
    void dump(std::string &out) const override { msgpack11::dump(m_value, out); }
};

bool equal_uint64_int64( uint64_t uint64_value, int64_t int64_value )
{
    bool const is_positive = 0 <= int64_value;
    bool const is_leq_int64_max = uint64_value <= std::numeric_limits<int64_t>::max();
    return is_positive && is_leq_int64_max && ( uint64_value == static_cast<uint64_t>(int64_value));
}

bool less_uint64_int64( uint64_t uint64_value, int64_t int64_value )
{
    bool const is_positive = 0 <= int64_value;
    bool const is_leq_int64_max = uint64_value <= std::numeric_limits<int64_t>::max();
    return is_positive && is_leq_int64_max && ( uint64_value < static_cast<uint64_t>(int64_value));
}

bool less_int64_uint64( int64_t int64_value, uint64_t uint64_value )
{
    bool const is_negative = int64_value < 0;
    bool const is_gt_int64_max = std::numeric_limits<int64_t>::max() < uint64_value;
    return is_negative || is_gt_int64_max || ( static_cast<uint64_t>(int64_value) < uint64_value );
}

template <MsgPack::Type tag, typename T>
class NumberValue : public Value<tag, T>  {
protected:

    // Constructors
    explicit NumberValue(const T &value) : Value<tag, T>(value) {}
    explicit NumberValue(T &&value)      : Value<tag, T>(move(value)) {}

    bool equals(const MsgPackValue * other) const override {
        switch( other->type() )
        {
            case MsgPack::FLOAT32 : // fall through
            case MsgPack::FLOAT64 : // fall through
            case MsgPack::UINT8   : // fall through
            case MsgPack::UINT16  : // fall through
            case MsgPack::UINT32  : // fall through
            case MsgPack::UINT64  : // fall through
            case MsgPack::INT8    : // fall through
            case MsgPack::INT16   : // fall through
            case MsgPack::INT32   : // fall through
            case MsgPack::INT64   : // fall through
            {
                return float64_value() == other->float64_value();
            } break;
            default               :
            {
                return Value<tag,T>::equals( other );
            } break;
        }
    }

    bool less(const MsgPackValue * other) const override {
        switch( other->type() )
        {
            case MsgPack::FLOAT32 : // fall through
            case MsgPack::FLOAT64 : // fall through
            case MsgPack::UINT8   : // fall through
            case MsgPack::UINT16  : // fall through
            case MsgPack::UINT32  : // fall through
            case MsgPack::UINT64  : // fall through
            case MsgPack::INT8    : // fall through
            case MsgPack::INT16   : // fall through
            case MsgPack::INT32   : // fall through
            case MsgPack::INT64   : // fall through
            {
                return float64_value() < other->float64_value();
            } break;
            default               :
            {
                return Value<tag,T>::less( other );
            } break;
        }
    }

    double  number_value()  const override { return static_cast<double>( Value<tag,T>::m_value ); }
    float float32_value()   const override { return static_cast<float>( Value<tag,T>::m_value ); }
    double float64_value()  const override { return static_cast<double>( Value<tag,T>::m_value ); }
    int32_t int_value()     const override { return static_cast<int32_t>( Value<tag,T>::m_value ); }
    int8_t int8_value()     const override { return static_cast<int8_t>( Value<tag,T>::m_value ); }
    int16_t int16_value()   const override { return static_cast<int16_t>( Value<tag,T>::m_value ); }
    int32_t int32_value()   const override { return static_cast<int32_t>( Value<tag,T>::m_value ); }
    int64_t int64_value()   const override { return static_cast<int64_t>( Value<tag,T>::m_value ); }
    uint8_t uint8_value()   const override { return static_cast<uint8_t>( Value<tag,T>::m_value ); }
    uint16_t uint16_value() const override { return static_cast<uint16_t>( Value<tag,T>::m_value ); }
    uint32_t uint32_value() const override { return static_cast<uint32_t>( Value<tag,T>::m_value ); }
    uint64_t uint64_value() const override { return static_cast<uint64_t>( Value<tag,T>::m_value ); }
};

class MsgPackFloat final : public NumberValue<MsgPack::FLOAT32, float> {
public:
    explicit MsgPackFloat(float value) : NumberValue(value) {}
};

class MsgPackDouble final : public NumberValue<MsgPack::FLOAT64, double> {
public:
    explicit MsgPackDouble(double value) : NumberValue(value) {}
};

class MsgPackInt8 final : public NumberValue<MsgPack::INT8, int8_t> {
public:
    explicit MsgPackInt8(int8_t value) : NumberValue(value) {}
};

class MsgPackInt16 final : public NumberValue<MsgPack::INT16, int16_t> {
public:
    explicit MsgPackInt16(int16_t value) : NumberValue(value) {}
};

class MsgPackInt32 final : public NumberValue<MsgPack::INT32, int32_t> {
public:
    explicit MsgPackInt32(int32_t value) : NumberValue(value) {}
};

class MsgPackInt64 final : public NumberValue<MsgPack::INT64, int64_t> {
    bool equals(const MsgPackValue * other) const override
    {
        switch( other->type() )
        {
            case MsgPack::INT64 :
            {
                return int64_value() == other->int64_value();
            } break;
            case MsgPack::UINT64 :
            {
                return equal_uint64_int64( other->uint64_value(), int64_value() );
            } break;
            default :
            {
                return NumberValue<MsgPack::INT64, int64_t>::equals( other );
            }
        }
    }
    bool less(const MsgPackValue * other)   const override
    {
        switch( other->type() )
        {
            case MsgPack::INT64 :
            {
                return int64_value() < other->int64_value();
            } break;
            case MsgPack::UINT64 :
            {
                return less_int64_uint64( int64_value(), other->uint64_value() );
            } break;
            default :
            {
                return NumberValue<MsgPack::INT64, int64_t>::less( other );
            }
        }
    }
public:
    explicit MsgPackInt64(int64_t value) : NumberValue(value) {}
};

class MsgPackUint8 final : public NumberValue<MsgPack::UINT8, uint8_t> {
public:
    explicit MsgPackUint8(uint8_t value) : NumberValue(value) {}
};

class MsgPackUint16 final : public NumberValue<MsgPack::UINT16, uint16_t> {
public:
    explicit MsgPackUint16(uint16_t value) : NumberValue(value) {}
};

class MsgPackUint32 final : public NumberValue<MsgPack::UINT32, uint32_t> {
public:
    explicit MsgPackUint32(uint32_t value) : NumberValue(value) {}
};

class MsgPackUint64 final : public NumberValue<MsgPack::UINT64, uint64_t> {
    bool equals(const MsgPackValue * other) const override
    {
        switch( other->type() )
        {
            case MsgPack::INT64 :
            {
                return equal_uint64_int64( uint64_value(), other->int64_value() );
            } break;
            case MsgPack::UINT64 :
            {
                return uint64_value() == other->uint64_value();
            } break;
            default :
            {
                return NumberValue<MsgPack::UINT64, uint64_t>::equals( other );
            }
        }
    }
    bool less(const MsgPackValue * other)   const override
    {
        switch( other->type() )
        {
            case MsgPack::INT64 :
            {
                return less_uint64_int64( uint64_value(), other->uint64_value() );
            } break;
            case MsgPack::UINT64 :
            {
                return uint64_value() < other->uint64_value();
            } break;
            default :
            {
                return NumberValue<MsgPack::UINT64, uint64_t>::less( other );
            }
        }
    }
public:
    explicit MsgPackUint64(uint64_t value) : NumberValue(value) {}
};

class MsgPackBoolean final : public Value<MsgPack::BOOL, bool> {
    bool bool_value() const override { return m_value; }
public:
    explicit MsgPackBoolean(bool value) : Value(value) {}
};

class MsgPackString final : public Value<MsgPack::STRING, string> {
    const string &string_value() const override { return m_value; }
public:
    explicit MsgPackString(const string &value) : Value(value) {}
    explicit MsgPackString(string &&value)      : Value(move(value)) {}
};

class MsgPackArray final : public Value<MsgPack::ARRAY, MsgPack::array> {
    const MsgPack::array &array_items() const override { return m_value; }
    const MsgPack & operator[](size_t i) const override;
public:
    explicit MsgPackArray(const MsgPack::array &value) : Value(value) {}
    explicit MsgPackArray(MsgPack::array &&value)      : Value(move(value)) {}
};

class MsgPackBinary final : public Value<MsgPack::BINARY, MsgPack::binary> {
    const MsgPack::binary &binary_items() const override { return m_value; }
public:
    explicit MsgPackBinary(const MsgPack::binary &value) : Value(value) {}
    explicit MsgPackBinary(MsgPack::binary &&value)      : Value(move(value)) {}
};

class MsgPackObject final : public Value<MsgPack::OBJECT, MsgPack::object> {
    const MsgPack::object &object_items() const override { return m_value; }
    const MsgPack & operator[](const string &key) const override;
public:
    explicit MsgPackObject(const MsgPack::object &value) : Value(value) {}
    explicit MsgPackObject(MsgPack::object &&value)      : Value(move(value)) {}
};

class MsgPackExtension final : public Value<MsgPack::EXTENSION, MsgPack::extension> {
    const MsgPack::extension &extension_items() const override { return m_value; }
public:
    explicit MsgPackExtension(const MsgPack::extension &value) : Value(value) {}
    explicit MsgPackExtension(MsgPack::extension &&value)      : Value(move(value)) {}
};

class MsgPackNull final : public Value<MsgPack::NUL, NullStruct> {
public:
    MsgPackNull() : Value({}) {}
};

/* * * * * * * * * * * * * * * * * * * *
 * Static globals - static-init-safe
 */
struct Statics {
    const std::shared_ptr<MsgPackValue> null = make_shared<MsgPackNull>();
    const std::shared_ptr<MsgPackValue> t = make_shared<MsgPackBoolean>(true);
    const std::shared_ptr<MsgPackValue> f = make_shared<MsgPackBoolean>(false);
    const string empty_string;
    const vector<MsgPack> empty_vector;
    const map<MsgPack, MsgPack> empty_map;
    const MsgPack::binary empty_binary;
    const MsgPack::extension empty_extension;
    Statics() {}
};

static const Statics & statics() {
    static const Statics s {};
    return s;
}

static const MsgPack & static_null() {
    // This has to be separate, not in Statics, because MsgPack() accesses statics().null.
    static const MsgPack msgpack_null;
    return msgpack_null;
}

/* * * * * * * * * * * * * * * * * * * *
 * Constructors
 */

MsgPack::MsgPack() noexcept                        : m_ptr(statics().null) {}
MsgPack::MsgPack(std::nullptr_t) noexcept          : m_ptr(statics().null) {}
MsgPack::MsgPack(float value)                      : m_ptr(make_shared<MsgPackFloat>(value)) {}
MsgPack::MsgPack(double value)                     : m_ptr(make_shared<MsgPackDouble>(value)) {}
MsgPack::MsgPack(int8_t value)                     : m_ptr(make_shared<MsgPackInt8>(value)) {}
MsgPack::MsgPack(int16_t value)                    : m_ptr(make_shared<MsgPackInt16>(value)) {}
MsgPack::MsgPack(int32_t value)                    : m_ptr(make_shared<MsgPackInt32>(value)) {}
MsgPack::MsgPack(int64_t value)                    : m_ptr(make_shared<MsgPackInt64>(value)) {}
MsgPack::MsgPack(uint8_t value)                    : m_ptr(make_shared<MsgPackUint8>(value)) {}
MsgPack::MsgPack(uint16_t value)                   : m_ptr(make_shared<MsgPackUint16>(value)) {}
MsgPack::MsgPack(uint32_t value)                   : m_ptr(make_shared<MsgPackUint32>(value)) {}
MsgPack::MsgPack(uint64_t value)                   : m_ptr(make_shared<MsgPackUint64>(value)) {}
MsgPack::MsgPack(bool value)                       : m_ptr(value ? statics().t : statics().f) {}
MsgPack::MsgPack(const string &value)              : m_ptr(make_shared<MsgPackString>(value)) {}
MsgPack::MsgPack(string &&value)                   : m_ptr(make_shared<MsgPackString>(move(value))) {}
MsgPack::MsgPack(const char * value)               : m_ptr(make_shared<MsgPackString>(value)) {}
MsgPack::MsgPack(const MsgPack::array &values)     : m_ptr(make_shared<MsgPackArray>(values)) {}
MsgPack::MsgPack(MsgPack::array &&values)          : m_ptr(make_shared<MsgPackArray>(move(values))) {}
MsgPack::MsgPack(const MsgPack::object &values)    : m_ptr(make_shared<MsgPackObject>(values)) {}
MsgPack::MsgPack(MsgPack::object &&values)         : m_ptr(make_shared<MsgPackObject>(move(values))) {}
MsgPack::MsgPack(const MsgPack::binary &values)    : m_ptr(make_shared<MsgPackBinary>(values)) {}
MsgPack::MsgPack(MsgPack::binary &&values)         : m_ptr(make_shared<MsgPackBinary>(move(values))) {}
MsgPack::MsgPack(const MsgPack::extension &values) : m_ptr(make_shared<MsgPackExtension>(values)) {}
MsgPack::MsgPack(MsgPack::extension &&values)      : m_ptr(make_shared<MsgPackExtension>(move(values))) {}

/* * * * * * * * * * * * * * * * * * * *
 * Accessors
 */

MsgPack::Type MsgPack::type()                           const { return m_ptr->type(); }
double MsgPack::number_value()                          const { return m_ptr->float64_value(); }
float MsgPack::float32_value()                          const { return m_ptr->float32_value(); }
double MsgPack::float64_value()                         const { return m_ptr->float64_value(); }
int32_t MsgPack::int_value()                            const { return m_ptr->int32_value(); }
int8_t MsgPack::int8_value()                            const { return m_ptr->int8_value(); }
int16_t MsgPack::int16_value()                          const { return m_ptr->int16_value(); }
int32_t MsgPack::int32_value()                          const { return m_ptr->int32_value(); }
int64_t MsgPack::int64_value()                          const { return m_ptr->int64_value(); }
uint8_t MsgPack::uint8_value()                          const { return m_ptr->uint8_value(); }
uint16_t MsgPack::uint16_value()                        const { return m_ptr->uint16_value(); }
uint32_t MsgPack::uint32_value()                        const { return m_ptr->uint32_value(); }
uint64_t MsgPack::uint64_value()                        const { return m_ptr->uint64_value(); }
bool MsgPack::bool_value()                              const { return m_ptr->bool_value(); }
const string & MsgPack::string_value()                  const { return m_ptr->string_value(); }
const vector<MsgPack>& MsgPack::array_items()           const { return m_ptr->array_items(); }
const MsgPack::binary& MsgPack::binary_items()          const { return m_ptr->binary_items(); }
const MsgPack::extension& MsgPack::extension_items()    const { return m_ptr->extension_items(); }
const map<MsgPack, MsgPack> & MsgPack::object_items()   const { return m_ptr->object_items(); }
const MsgPack & MsgPack::operator[] (size_t i)          const { return (*m_ptr)[i]; }
const MsgPack & MsgPack::operator[] (const string &key) const { return (*m_ptr)[key]; }

double                        MsgPackValue::number_value()              const { return 0.0; }
float                         MsgPackValue::float32_value()             const { return 0.0f; }
double                        MsgPackValue::float64_value()             const { return 0.0; }
int32_t                       MsgPackValue::int_value()                 const { return 0; }
int8_t                        MsgPackValue::int8_value()                const { return 0; }
int16_t                       MsgPackValue::int16_value()               const { return 0; }
int32_t                       MsgPackValue::int32_value()               const { return 0; }
int64_t                       MsgPackValue::int64_value()               const { return 0; }
uint8_t                       MsgPackValue::uint8_value()               const { return 0; }
uint16_t                      MsgPackValue::uint16_value()              const { return 0; }
uint32_t                      MsgPackValue::uint32_value()              const { return 0; }
uint64_t                      MsgPackValue::uint64_value()              const { return 0; }
bool                          MsgPackValue::bool_value()                const { return false; }
const string &                MsgPackValue::string_value()              const { return statics().empty_string; }
const vector<MsgPack> &       MsgPackValue::array_items()               const { return statics().empty_vector; }
const map<MsgPack, MsgPack> & MsgPackValue::object_items()              const { return statics().empty_map; }
const MsgPack::binary & MsgPackValue::binary_items()                    const { return statics().empty_binary; }
const MsgPack::extension & MsgPackValue::extension_items()              const { return statics().empty_extension; }
const MsgPack &               MsgPackValue::operator[] (size_t)         const { return static_null(); }
const MsgPack &               MsgPackValue::operator[] (const string &) const { return static_null(); }

const MsgPack & MsgPackObject::operator[] (const string &key) const {
    auto iter = m_value.find(key);
    return (iter == m_value.end()) ? static_null() : iter->second;
}
const MsgPack & MsgPackArray::operator[] (size_t i) const {
    if (i >= m_value.size()) return static_null();
    else return m_value[i];
}

/* * * * * * * * * * * * * * * * * * * *
 * Comparison
 */

bool MsgPack::operator== (const MsgPack &other) const {
    return m_ptr->equals(other.m_ptr.get());
}

bool MsgPack::operator< (const MsgPack &other) const {
    return m_ptr->less(other.m_ptr.get());
}

namespace {
/* MsgPackParser
 *
 * Object that tracks all state of an in-progress parse.
 */
class MsgPackParser final {
public:
    MsgPackParser( const std::string& buffer, size_t i, string& err, bool failed )
        : m_ptr_beg( buffer.c_str() ),
          m_ptr_end( m_ptr_beg + buffer.size() ),
          m_ptr_cur( m_ptr_beg + i ),
          m_err( err ),
          m_failed( failed )
    {
    }

    /* fail(msg, err_ret = MsgPack())
     *
     * Mark this parse as m_failed.
     */
    MsgPack fail(string &&msg) {
        return fail(move(msg), MsgPack());
    }

    template <typename T>
    T fail(string &&msg, const T err_ret) {
        if (!m_failed)
        {
            m_err = std::move(msg);
        }

        m_failed = true;
        return err_ret;
    }

    size_t pos() const
    {
        return static_cast<size_t>(std::distance(m_ptr_beg, m_ptr_cur));
    }

    bool is_failed() const
    {
        return m_failed;
    }

    MsgPack parse_invalid(uint8_t, int) {
        m_err = "invalid first byte.";
        m_failed = true;
        return MsgPack();
    }

    MsgPack parse_nil(uint8_t, int) {
        return MsgPack();
    }

    MsgPack parse_bool(uint8_t first_byte, int) {
        return MsgPack(first_byte == 0xc3);
    }

    template< typename T >
    MsgPack parse_arith(uint8_t, int) {
        T tmp;
        read_bytes(reinterpret_cast<uint8_t*>(&tmp), sizeof(T));
        return MsgPack(tmp);
    }

    inline  std::string parse_string_impl(uint32_t bytes) {
        m_ptr_cur += bytes;
        return std::string(m_ptr_cur - bytes, m_ptr_cur);
    }

    template< typename T >
    MsgPack parse_string(uint8_t, int) {
        T bytes;
        read_bytes(reinterpret_cast<uint8_t*>(&bytes), sizeof(T));
        return MsgPack(parse_string_impl(static_cast<uint32_t>(bytes)));
    }

    MsgPack::array parse_array_impl(uint32_t bytes, int depth) {
        MsgPack::array res;
        res.reserve(bytes);

        for(uint32_t i = 0; i < bytes; ++i) {
            res.push_back(parse_msgpack(depth));
        }
        return res;
    }

    template< typename T >
    MsgPack parse_array(uint8_t, int depth) {
        T bytes;
        read_bytes(reinterpret_cast<uint8_t*>(&bytes), sizeof(T));
        return MsgPack(parse_array_impl(static_cast<uint32_t>(bytes), depth));
    }

    MsgPack::object parse_object_impl(uint32_t bytes, int depth) {
        MsgPack::object res;

        for(uint32_t i = 0; i < bytes; ++i) {
            MsgPack key = parse_msgpack(depth);
            MsgPack value = parse_msgpack(depth);
            res.insert(std::make_pair(std::move(key), std::move(value)));
        }
        return res;
    }

    template< typename T >
    MsgPack parse_object(uint8_t, int depth) {
        T bytes;
        read_bytes(reinterpret_cast<uint8_t*>(&bytes), sizeof(T));
        return MsgPack(parse_object_impl(static_cast<uint32_t>(bytes), depth));
    }

    MsgPack::binary parse_binary_impl(uint32_t bytes) {
        m_ptr_cur += bytes;
        return MsgPack::binary(m_ptr_cur - bytes, m_ptr_cur);
    }

    template< typename T >
    MsgPack parse_binary(uint8_t, int) {
        T bytes;
        read_bytes(reinterpret_cast<uint8_t*>(&bytes), sizeof(T));
        return MsgPack(parse_binary_impl(static_cast<uint32_t>(bytes)));
    }

    template< typename T >
    MsgPack parse_extension(uint8_t, int) {
        T bytes;
        read_bytes(reinterpret_cast<uint8_t*>(&bytes), sizeof(T));
        uint8_t type;
        read_bytes(&type, 1);
        const MsgPack::binary data =  parse_binary_impl(static_cast<uint32_t>(bytes));
        return MsgPack(std::make_tuple(type, std::move(data)));
    }

    MsgPack parse_pos_fixint(uint8_t first_byte, int) {
        return MsgPack( first_byte );
    }

    MsgPack parse_fixobject(uint8_t first_byte, int depth) {
        uint32_t const bytes = first_byte & 0x0f;
        return MsgPack(parse_object_impl(bytes, depth));
    }

    MsgPack parse_fixarray(uint8_t first_byte, int depth) {
        uint32_t const bytes = first_byte & 0x0f;
        return MsgPack(parse_array_impl(bytes, depth));
    }

    MsgPack parse_fixstring(uint8_t first_byte, int) {
        uint32_t const bytes = first_byte & 0x1f;
        return MsgPack(parse_string_impl(bytes));
    }

    MsgPack parse_neg_fixint(uint8_t first_byte, int) {
        return MsgPack(*reinterpret_cast<int8_t*>(&first_byte));
    }

    MsgPack parse_fixext(uint8_t first_byte, int) {
        uint8_t type;
        read_bytes(&type, 1);
        uint32_t const BYTES = 1 << (first_byte - 0xd4u);
        const MsgPack::binary data = parse_binary_impl(BYTES);
        return MsgPack(std::make_tuple(type, std::move(data)));
    }

    /* parse_msgpack()
     *
     * Parse a JSON object.
     */
    MsgPack parse_msgpack(int depth) {
        static const std::array< MsgPack(MsgPackParser::*)(uint8_t, int), 256 > parsers = [](){
            using parser_template_element_type = std::tuple<uint8_t, MsgPack(MsgPackParser::*)(uint8_t,int)>;
            std::array< parser_template_element_type, 36 > const parser_template{{
                parser_template_element_type{ 0x7fu, &MsgPackParser::parse_pos_fixint},
                parser_template_element_type{ 0x8fu, &MsgPackParser::parse_fixobject},
                parser_template_element_type{ 0x9fu, &MsgPackParser::parse_fixarray},
                parser_template_element_type{ 0xbfu, &MsgPackParser::parse_fixstring},
                parser_template_element_type{ 0xc0u, &MsgPackParser::parse_nil},
                parser_template_element_type{ 0xc1u, &MsgPackParser::parse_invalid},
                parser_template_element_type{ 0xc3u, &MsgPackParser::parse_bool},
                parser_template_element_type{ 0xc4u, &MsgPackParser::parse_binary<uint8_t>},
                parser_template_element_type{ 0xc5u, &MsgPackParser::parse_binary<uint16_t>},
                parser_template_element_type{ 0xc6u, &MsgPackParser::parse_binary<uint32_t>},
                parser_template_element_type{ 0xc7u, &MsgPackParser::parse_extension<uint8_t>},
                parser_template_element_type{ 0xc8u, &MsgPackParser::parse_extension<uint16_t>},
                parser_template_element_type{ 0xc9u, &MsgPackParser::parse_extension<uint32_t>},
                parser_template_element_type{ 0xcau, &MsgPackParser::parse_arith<float>},
                parser_template_element_type{ 0xcbu, &MsgPackParser::parse_arith<double>},
                parser_template_element_type{ 0xccu, &MsgPackParser::parse_arith<uint8_t>},
                parser_template_element_type{ 0xcdu, &MsgPackParser::parse_arith<uint16_t>},
                parser_template_element_type{ 0xceu, &MsgPackParser::parse_arith<uint32_t>},
                parser_template_element_type{ 0xcfu, &MsgPackParser::parse_arith<uint64_t>},
                parser_template_element_type{ 0xd0u, &MsgPackParser::parse_arith<int8_t>},
                parser_template_element_type{ 0xd1u, &MsgPackParser::parse_arith<int16_t>},
                parser_template_element_type{ 0xd2u, &MsgPackParser::parse_arith<int32_t>},
                parser_template_element_type{ 0xd3u, &MsgPackParser::parse_arith<int64_t>},
                parser_template_element_type{ 0xd8u, &MsgPackParser::parse_fixext},
                parser_template_element_type{ 0xd9u, &MsgPackParser::parse_string<uint8_t>},
                parser_template_element_type{ 0xdau, &MsgPackParser::parse_string<uint16_t>},
                parser_template_element_type{ 0xdbu, &MsgPackParser::parse_string<uint32_t>},
                parser_template_element_type{ 0xdcu, &MsgPackParser::parse_array<uint16_t>},
                parser_template_element_type{ 0xddu, &MsgPackParser::parse_array<uint32_t>},
                parser_template_element_type{ 0xdeu, &MsgPackParser::parse_object<uint16_t>},
                parser_template_element_type{ 0xdfu, &MsgPackParser::parse_object<uint32_t>},
                parser_template_element_type{ 0xffu, &MsgPackParser::parse_neg_fixint}
            }};

            std::array< MsgPack(MsgPackParser::*)(uint8_t, int), 256 > parsers;
            int i = 0;
            std::for_each(std::begin(parser_template),
                         std::end(parser_template),
                         [&parsers,&i]( parser_template_element_type const& element ) {
                            int upto = std::get<0>( element );
                            auto parser = std::get<1>( element );
                            while(i <= upto)
                            {
                                parsers[i] = parser;
                                ++i;
                            }
                         });
            return parsers;
        }();

        if (max_depth < depth) {
            return fail("exceeded maximum nesting depth.");
        }

        if(m_ptr_end <= m_ptr_cur)
        {
            return fail("end of buffer.");
        }

        uint8_t const first_byte = *m_ptr_cur;
        ++m_ptr_cur;

        return (this->*parsers[first_byte])(first_byte, depth + 1);
    }

private:
    /* State
     */
    const char* const m_ptr_beg;
    const char* const m_ptr_end;
    const char* m_ptr_cur;
    string &m_err;
    bool m_failed;

    void read_bytes(uint8_t* bytes, int n)
    {
        int const offsets[] = {(n-1), 0};
        int const directions[] = {-1, 1};

        uint8_t* dst_ptr = bytes + offsets[static_cast<int>(is_big_endian)];
        int const dir = directions[static_cast<int>(is_big_endian)];
        for(int i = 0; i < n; ++i)
        {
            *dst_ptr = *m_ptr_cur;
            dst_ptr += dir;
            ++m_ptr_cur;
        }
    }
};

}//namespace {
MsgPack MsgPack::parse(const std::string &in, string &err) {
    MsgPackParser parser { in, 0, err, false };
    return parser.parse_msgpack(0);
}

// Documented in msgpack.hpp
vector<MsgPack> MsgPack::parse_multi(const string &in,
                                     std::string::size_type &parser_stop_pos,
                                     string &err) {
    MsgPackParser parser { in, 0, err, false };
    parser_stop_pos = 0;
    vector<MsgPack> msgpack_vec;
    while (parser.pos() != in.size() && !parser.is_failed()) {
        msgpack_vec.push_back(parser.parse_msgpack(0));
        if (!parser.is_failed())
            parser_stop_pos = parser.pos();
    }
    return msgpack_vec;
}

/* * * * * * * * * * * * * * * * * * * *
 * Shape-checking
 */

bool MsgPack::has_shape(const shape & types, string & err) const {
    if (!is_object()) {
        err = "expected MessagePack object";
        return false;
    }

    for (auto & item : types) {
        if ((*this)[item.first].type() != item.second) {
            err = "bad type for " + item.first;
            return false;
        }
    }

    return true;
}

} // namespace msgpack11
