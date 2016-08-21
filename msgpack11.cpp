#include "msgpack11.hpp"
#include <cassert>
#include <cmath>
#include <cstdlib>
#include <cstdio>
#include <limits>
#include <array>
#include <tuple>
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
struct EndianConverter {
    union {
        T packed;
        std::array<uint8_t, sizeof(T)> bytes;
    } value;
};

template< typename T >
void dump_data(T value, std::vector<uint8_t> &out)
{
    EndianConverter<T> converter;
    converter.value.packed = value;

    auto f = [&](uint8_t byte) {
        out.push_back(byte);
    };

    if(is_big_endian)
    {
        std::for_each(converter.value.bytes.begin(), converter.value.bytes.end(), f);
    }
    else
    {
        std::for_each(converter.value.bytes.rbegin(), converter.value.bytes.rend(), f);
    }
}

static void dump(std::nullptr_t, std::vector<uint8_t> &out) {
    out.push_back(0xc0);
}

static void dump(float value, std::vector<uint8_t> &out) {
    out.push_back(0xca);
    dump_data(value, out);
}

static void dump(double value, std::vector<uint8_t> &out) {
    out.push_back(0xcb);
    dump_data(value, out);
}

static void dump(int8_t value, std::vector<uint8_t> &out) {
    if( value < -32 )
    {
        out.push_back(0xd0);
    }
    out.push_back(value);
}

static void dump(int16_t value, std::vector<uint8_t> &out) {
    out.push_back(0xd1);
    dump_data(value, out);
}

static void dump(int32_t value, std::vector<uint8_t> &out) {
    out.push_back(0xd2);
    dump_data(value, out);
}

static void dump(int64_t value, std::vector<uint8_t> &out) {
    out.push_back(0xd3);
    dump_data(value, out);
}

static void dump(uint8_t value, std::vector<uint8_t> &out) {
    if(128 <= value)
    {
        out.push_back(0xcc);
    }
    out.push_back(value);
}

static void dump(uint16_t value, std::vector<uint8_t> &out) {
    out.push_back(0xcd);
    dump_data(value, out);
}

static void dump(uint32_t value, std::vector<uint8_t> &out) {
    out.push_back(0xce);
    dump_data(value, out);
}

static void dump(uint64_t value, std::vector<uint8_t> &out) {
    out.push_back(0xcf);
    dump_data(value, out);
}

static void dump(bool value, std::vector<uint8_t> &out) {
    const uint8_t msgpack_value = (value) ? 0xc3 : 0xc2;
    out.push_back(msgpack_value);
}

static void dump(const std::string& value, std::vector<uint8_t> &out) {
    size_t const len = value.size();
    if(len <= 0x1f)
    {
        uint8_t const first_byte = 0xa0 | static_cast<uint8_t>(len);
        out.push_back(first_byte);
    }
    else if(len <= 0xff)
    {
        uint8_t const length = static_cast<uint8_t>(len);
        out.push_back(0xd9);
        out.push_back(length);
    }
    else if(len <= 0xffff)
    {
        uint16_t const length = static_cast<uint16_t>(len);
        out.push_back(0xda);
        dump_data(length, out);
    }
    else if(len <= 0xffffffff)
    {
        uint32_t const length = static_cast<uint32_t>(len);
        out.push_back(0xdb);
        dump_data(length, out);
    }
    else
    {
        throw std::runtime_error("exceeded maximum data length");
    }

    std::for_each(std::begin(value), std::end(value), [&out](char v){
        dump_data(v, out);
    });
}

static void dump(const MsgPack::array& value, std::vector<uint8_t> &out) {
    size_t const len = value.size();
    if(len <= 15)
    {
        uint8_t const first_byte = 0x90 | static_cast<uint8_t>(len);
        out.push_back(first_byte);
    }
    else if(len <= 0xffff)
    {
        uint16_t const length = static_cast<uint16_t>(len);
        out.push_back(0xdc);
        dump_data(length, out);
    }
    else if(len <= 0xffffffff)
    {
        uint32_t const length = static_cast<uint32_t>(len);
        out.push_back(0xdd);
        dump_data(length, out);
    }
    else
    {
        throw std::runtime_error("exceeded maximum data length");
    }

    std::for_each(std::begin(value), std::end(value), [&out](MsgPack::array::value_type const& v){
        v.dump(out);
    });
}

static void dump(const MsgPack::object& value, std::vector<uint8_t> &out) {
    size_t const len = value.size();
    if(len <= 15)
    {
        uint8_t const first_byte = 0x80 | static_cast<uint8_t>(len);
        out.push_back(first_byte);
    }
    else if(len <= 0xffff)
    {
        uint16_t const length = static_cast<uint16_t>(len);
        out.push_back(0xde);
        dump_data(length, out);
    }
    else if(len <= 0xffffffff)
    {
        uint32_t const length = static_cast<uint32_t>(len);
        out.push_back(0xdf);
        dump_data(length, out);
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

static void dump(const MsgPack::binary& value, std::vector<uint8_t> &out) {
    size_t const len = value.size();
    if(len <= 0xff)
    {
        uint8_t const length = static_cast<uint8_t>(len);
        out.push_back(0xc4);
        dump_data(length, out);
    }
    else if(len <= 0xffff)
    {
        uint16_t const length = static_cast<uint16_t>(len);
        out.push_back(0xc5);
        dump_data(length, out);
    }
    else if(len <= 0xffffffff)
    {
        uint32_t const length = static_cast<uint32_t>(len);
        out.push_back(0xc6);
        dump_data(length, out);
    }
    else
    {
        throw std::runtime_error("exceeded maximum data length");
    }

    std::for_each(std::begin(value), std::end(value), [&out](MsgPack::binary::value_type const& v){
        out.push_back(v);
    });
}

static void dump(const MsgPack::extension& value, std::vector<uint8_t> &out) {
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
        uint8_t const length = static_cast<uint8_t>(len);
        out.push_back(0xc7);
        out.push_back(length);
    }
    else if(len <= 0xffff) {
        uint16_t const length = static_cast<uint16_t>(len);
        out.push_back(0xc8);
        dump_data(length, out);
    }
    else if(len <= 0xffffffff) {
        uint32_t const length = static_cast<uint32_t>(len);
        out.push_back(0xc9);
        dump_data(length, out);
    }
    else {
        throw std::runtime_error("exceeded maximum data length");
    }

    out.push_back(type);
    std::for_each(std::begin(data), std::end(data), [&out](uint8_t const& v){
        out.push_back(v);
    });
}
}

void MsgPack::dump(std::vector<uint8_t> &out) const {
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
        return m_value == static_cast<const Value<tag, T> *>(other)->m_value;
    }
    bool less(const MsgPackValue * other) const override {
        return m_value < static_cast<const Value<tag, T> *>(other)->m_value;
    }

    const T m_value;
    void dump(std::vector<uint8_t> &out) const override { msgpack11::dump(m_value, out); }
};

template <MsgPack::Type tag, typename T>
class NumberValue : public Value<tag, T>  {
protected:

    // Constructors
    explicit NumberValue(const T &value) : Value<tag, T>(value) {}
    explicit NumberValue(T &&value)      : Value<tag, T>(move(value)) {}

    int8_t int8_value()     const override { return static_cast<int8_t>( Value<tag,T>::m_value ); }
    int16_t int16_value()   const override { return static_cast<int16_t>( Value<tag,T>::m_value ); }
    int32_t int32_value()   const override { return static_cast<int32_t>( Value<tag,T>::m_value ); }
    int64_t int64_value()   const override { return static_cast<int64_t>( Value<tag,T>::m_value ); }
    uint8_t uint8_value()   const override { return static_cast<uint8_t>( Value<tag,T>::m_value ); }
    uint16_t uint16_value() const override { return static_cast<uint16_t>( Value<tag,T>::m_value ); }
    uint32_t uint32_value() const override { return static_cast<uint32_t>( Value<tag,T>::m_value ); }
    uint64_t uint64_value() const override { return static_cast<uint64_t>( Value<tag,T>::m_value ); }
    float float32_value()   const override { return static_cast<float>( Value<tag,T>::m_value ); }
    double float64_value()  const override { return static_cast<double>( Value<tag,T>::m_value ); }
};

class MsgPackFloat final : public NumberValue<MsgPack::FLOAT32, float> {
    bool equals(const MsgPackValue * other) const override { return m_value == other->float64_value(); }
    bool less(const MsgPackValue * other)   const override { return m_value <  other->float64_value(); }
public:
    explicit MsgPackFloat(float value) : NumberValue(value) {}
};

class MsgPackDouble final : public NumberValue<MsgPack::FLOAT64, double> {
    bool equals(const MsgPackValue * other) const override { return m_value == other->float64_value(); }
    bool less(const MsgPackValue * other)   const override { return m_value <  other->float64_value(); }
public:
    explicit MsgPackDouble(double value) : NumberValue(value) {}
};

class MsgPackInt8 final : public NumberValue<MsgPack::INT8, int8_t> {
    bool equals(const MsgPackValue * other) const override { return m_value == other->float64_value(); }
    bool less(const MsgPackValue * other)   const override { return m_value <  other->float64_value(); }
public:
    explicit MsgPackInt8(int8_t value) : NumberValue(value) {}
};

class MsgPackInt16 final : public NumberValue<MsgPack::INT16, int16_t> {
    bool equals(const MsgPackValue * other) const override { return m_value == other->float64_value(); }
    bool less(const MsgPackValue * other)   const override { return m_value <  other->float64_value(); }
public:
    explicit MsgPackInt16(int16_t value) : NumberValue(value) {}
};

class MsgPackInt32 final : public NumberValue<MsgPack::INT32, int32_t> {
    bool equals(const MsgPackValue * other) const override { return m_value == other->float64_value(); }
    bool less(const MsgPackValue * other)   const override { return m_value <  other->float64_value(); }
public:
    explicit MsgPackInt32(int32_t value) : NumberValue(value) {}
};

class MsgPackInt64 final : public NumberValue<MsgPack::INT64, int64_t> {
    bool equals(const MsgPackValue * other) const override { return m_value == other->float64_value(); }
    bool less(const MsgPackValue * other)   const override { return m_value <  other->float64_value(); }
public:
    explicit MsgPackInt64(int64_t value) : NumberValue(value) {}
};

class MsgPackUint8 final : public NumberValue<MsgPack::UINT8, uint8_t> {
    bool equals(const MsgPackValue * other) const override { return m_value == other->float64_value(); }
    bool less(const MsgPackValue * other)   const override { return m_value <  other->float64_value(); }
public:
    explicit MsgPackUint8(uint8_t value) : NumberValue(value) {}
};

class MsgPackUint16 final : public NumberValue<MsgPack::UINT16, uint16_t> {
    bool equals(const MsgPackValue * other) const override { return m_value == other->float64_value(); }
    bool less(const MsgPackValue * other)   const override { return m_value <  other->float64_value(); }
public:
    explicit MsgPackUint16(uint16_t value) : NumberValue(value) {}
};

class MsgPackUint32 final : public NumberValue<MsgPack::UINT32, uint32_t> {
    bool equals(const MsgPackValue * other) const override { return m_value == other->float64_value(); }
    bool less(const MsgPackValue * other)   const override { return m_value <  other->float64_value(); }
public:
    explicit MsgPackUint32(uint32_t value) : NumberValue(value) {}
};

class MsgPackUint64 final : public NumberValue<MsgPack::UINT64, uint64_t> {
    bool equals(const MsgPackValue * other) const override { return m_value == other->float64_value(); }
    bool less(const MsgPackValue * other)   const override { return m_value <  other->float64_value(); }
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

class MsgPackNull final : public Value<MsgPack::NUL, std::nullptr_t> {
public:
    MsgPackNull() : Value(nullptr) {}
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
float MsgPack::float32_value()                          const { return m_ptr->float32_value(); }
double MsgPack::float64_value()                         const { return m_ptr->float64_value(); }
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
const vector<MsgPack> & MsgPack::array_items()          const { return m_ptr->array_items(); }
const vector<uint8_t> & MsgPack::binary_items()         const { return m_ptr->binary_items(); }
const MsgPack::extension& MsgPack::extension_items()    const { return m_ptr->extension_items(); }
const map<MsgPack, MsgPack> & MsgPack::object_items()   const { return m_ptr->object_items(); }
const MsgPack & MsgPack::operator[] (size_t i)          const { return (*m_ptr)[i]; }
const MsgPack & MsgPack::operator[] (const string &key) const { return (*m_ptr)[key]; }

float                         MsgPackValue::float32_value()             const { return 0.0f; }
double                        MsgPackValue::float64_value()             const { return 0.0; }
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
    if (m_ptr->type() != other.m_ptr->type())
        return false;

    return m_ptr->equals(other.m_ptr.get());
}

bool MsgPack::operator< (const MsgPack &other) const {
    if (m_ptr->type() != other.m_ptr->type())
        return m_ptr->type() < other.m_ptr->type();

    return m_ptr->less(other.m_ptr.get());
}

namespace {
/* MsgPackParser
 *
 * Object that tracks all state of an in-progress parse.
 */
struct MsgPackParser final {

    /* State
     */
    const std::vector< uint8_t > &buffer;
    size_t i;
    string &err;
    bool failed;

    /* fail(msg, err_ret = MsgPack())
     *
     * Mark this parse as failed.
     */
    MsgPack fail(string &&msg) {
        return fail(move(msg), MsgPack());
    }

    template <typename T>
    T fail(string &&msg, const T err_ret) {
        if (!failed)
            err = std::move(msg);
        failed = true;
        return err_ret;
    }

    uint8_t get_first_byte()
    {
        if(buffer.size() <= i)
        {
            err = "end of buffer.";
            failed = true;
            return 0x00;
        }

        const uint8_t first_byte = buffer[i];
        ++i;
        return first_byte;
    }

    std::nullptr_t parse_invalid(uint8_t) {
        err = "invalid first byte.";
        failed = true;
        return nullptr;
    }

    std::nullptr_t parse_nil(uint8_t) {
        return nullptr;
    }

    bool parse_bool(uint8_t first_byte) {
        return (first_byte == 0xc3);
    }

    template< typename T >
    T parse_arith() {
        EndianConverter<T> converter;
        for( size_t j = 0; j < sizeof(T); ++j )
        {
            converter.value.bytes[j] = buffer[i];
            ++i;
        }

        if(!is_big_endian)
        {
            std::reverse(converter.value.bytes.begin(), converter.value.bytes.end());
        }

        return converter.value.packed;
    }

    template< typename T >
    std::string parse_string_impl(uint8_t, T bytes) {
        std::string res(&(buffer[i]), &(buffer[i+bytes]));
        i += bytes;
        return res;
    }

    template< typename T >
    std::string parse_string(uint8_t first_byte) {
        T const bytes = parse_arith<T>();
        return parse_string_impl<T>(first_byte, bytes);
    }

    template< typename T >
    MsgPack::array parse_array_impl(T bytes) {
        MsgPack::array res;
        for(T i = 0; i < bytes; ++i) {
            res.emplace_back(parse_msgpack(0));
        }

        return res;
    }

    template< typename T >
    MsgPack::array parse_array() {
        T const bytes = parse_arith<T>();
        return parse_array_impl<T>(bytes);
    }

    template< typename T >
    MsgPack::object parse_object_impl(uint8_t, T bytes) {
        MsgPack::object res;
        for(T i = 0; i < bytes; ++i) {
            MsgPack key = parse_msgpack(0);
            MsgPack value = parse_msgpack(0);
            res.insert(std::make_pair(std::move(key), std::move(value)));
        }

        return res;
    }

    template< typename T >
    MsgPack::object parse_object(uint8_t first_byte) {
        T const bytes = parse_arith<T>();
        return parse_object_impl<T>(first_byte, bytes);
    }

    template< typename T >
    MsgPack::binary parse_binary_impl(T bytes) {
        MsgPack::binary res;
        for(T j = 0; j < bytes; ++j) {
            res.push_back(buffer[i]);
            i++;
        }

        return res;
    }

    template< typename T >
    MsgPack::binary parse_binary() {
        T const bytes = parse_arith<T>();
        return parse_binary_impl<T>(bytes);
    }

    template< typename T >
    MsgPack::extension parse_extension() {
        const T bytes = parse_arith<T>();
        const uint8_t type = parse_arith<uint8_t>();
        const MsgPack::binary data =  parse_binary_impl<T>(bytes);
        return std::make_tuple(type, std::move(data));
    }

    uint8_t parse_pos_fixint(uint8_t first_byte) {
        return first_byte;
    }

    MsgPack::object parse_fixobject(uint8_t first_byte) {
        uint8_t const bytes = first_byte & 0x0f;
        return parse_object_impl<uint8_t>(first_byte, bytes);
    }

    MsgPack::array parse_fixarray(uint8_t first_byte) {
        uint8_t const bytes = first_byte & 0x0f;
        return parse_array_impl<uint8_t>(bytes);
    }

    std::string parse_fixstring(uint8_t first_byte) {
        uint8_t const bytes = first_byte & 0x1f;
        return parse_string_impl<uint8_t>(first_byte, bytes);
    }

    int8_t parse_neg_fixint(uint8_t first_byte) {
        return *reinterpret_cast<int8_t*>(&first_byte);
    }

    MsgPack::extension parse_fixext(uint8_t bytes) {
        const uint8_t type = parse_arith<uint8_t>();
        const MsgPack::binary data =  parse_binary_impl<uint8_t>(bytes);
        return std::make_tuple(type, std::move(data));
    }

    static const std::vector< std::tuple<uint8_t, uint8_t, std::function< MsgPack(MsgPackParser*, uint8_t) > > > parsers;

    /* parse_msgpack()
     *
     * Parse a JSON object.
     */
    MsgPack parse_msgpack(int depth) {
        if (depth > max_depth) {
            return fail("exceeded maximum nesting depth");
        }

        uint8_t first_byte = get_first_byte();
        if (failed) {
            return MsgPack();
        }

        for( auto const& parser : parsers ) {
            auto beg = std::get<0>(parser);
            auto end = std::get<1>(parser);
            if((beg <= first_byte) && (first_byte <= end)) {
                auto parser_func = std::get<2>(parser);
                return parser_func(this, first_byte);
            }
        }

        return MsgPack();
    }
};

const std::vector< std::tuple<uint8_t, uint8_t, std::function< MsgPack(MsgPackParser*, uint8_t) > > > MsgPackParser::parsers {
    { 0x00, 0x7f, [](MsgPackParser* that, uint8_t first_byte){ return MsgPack(that->parse_pos_fixint(first_byte)); } },
    { 0x80, 0x8f, [](MsgPackParser* that, uint8_t first_byte){ return MsgPack(that->parse_fixobject(first_byte)); } },
    { 0x90, 0x9f, [](MsgPackParser* that, uint8_t first_byte){ return MsgPack(that->parse_fixarray(first_byte)); } },
    { 0xa0, 0xbf, [](MsgPackParser* that, uint8_t first_byte){ return MsgPack(that->parse_fixstring(first_byte)); } },
    { 0xc0, 0xc0, [](MsgPackParser* that, uint8_t first_byte){ return MsgPack(that->parse_nil(first_byte)); } },
    { 0xc1, 0xc1, [](MsgPackParser* that, uint8_t first_byte){ return MsgPack(that->parse_invalid(first_byte)); } },
    { 0xc2, 0xc3, [](MsgPackParser* that, uint8_t first_byte){ return MsgPack(that->parse_bool(first_byte)); } },
    { 0xc4, 0xc4, [](MsgPackParser* that, uint8_t){ return MsgPack(that->parse_binary<uint8_t>()); } },
    { 0xc5, 0xc5, [](MsgPackParser* that, uint8_t){ return MsgPack(that->parse_binary<uint16_t>()); } },
    { 0xc6, 0xc6, [](MsgPackParser* that, uint8_t){ return MsgPack(that->parse_binary<uint32_t>()); } },
    { 0xc7, 0xc7, [](MsgPackParser* that, uint8_t){ return MsgPack(that->parse_extension<uint8_t>()); } },
    { 0xc8, 0xc8, [](MsgPackParser* that, uint8_t){ return MsgPack(that->parse_extension<uint16_t>()); } },
    { 0xc9, 0xc9, [](MsgPackParser* that, uint8_t){ return MsgPack(that->parse_extension<uint32_t>()); } },
    { 0xca, 0xca, [](MsgPackParser* that, uint8_t){ return MsgPack(that->parse_arith<float>()); } },
    { 0xcb, 0xcb, [](MsgPackParser* that, uint8_t){ return MsgPack(that->parse_arith<double>()); } },
    { 0xcc, 0xcc, [](MsgPackParser* that, uint8_t){ return MsgPack(that->parse_arith<uint8_t>()); } },
    { 0xcd, 0xcd, [](MsgPackParser* that, uint8_t){ return MsgPack(that->parse_arith<uint16_t>()); } },
    { 0xce, 0xce, [](MsgPackParser* that, uint8_t){ return MsgPack(that->parse_arith<uint32_t>()); } },
    { 0xcf, 0xcf, [](MsgPackParser* that, uint8_t){ return MsgPack(that->parse_arith<uint64_t>()); } },
    { 0xd0, 0xd0, [](MsgPackParser* that, uint8_t){ return MsgPack(that->parse_arith<int8_t>()); } },
    { 0xd1, 0xd1, [](MsgPackParser* that, uint8_t){ return MsgPack(that->parse_arith<int16_t>()); } },
    { 0xd2, 0xd2, [](MsgPackParser* that, uint8_t){ return MsgPack(that->parse_arith<int32_t>()); } },
    { 0xd3, 0xd3, [](MsgPackParser* that, uint8_t){ return MsgPack(that->parse_arith<int64_t>()); } },
    { 0xd4, 0xd4, [](MsgPackParser* that, uint8_t){ return MsgPack(that->parse_fixext(1)); } },
    { 0xd5, 0xd5, [](MsgPackParser* that, uint8_t){ return MsgPack(that->parse_fixext(2)); } },
    { 0xd6, 0xd6, [](MsgPackParser* that, uint8_t){ return MsgPack(that->parse_fixext(4)); } },
    { 0xd7, 0xd7, [](MsgPackParser* that, uint8_t){ return MsgPack(that->parse_fixext(8)); } },
    { 0xd8, 0xd8, [](MsgPackParser* that, uint8_t){ return MsgPack(that->parse_fixext(16)); } },
    { 0xd9, 0xd9, [](MsgPackParser* that, uint8_t first_byte){ return MsgPack(that->parse_string<uint8_t>(first_byte)); } },
    { 0xda, 0xda, [](MsgPackParser* that, uint8_t first_byte){ return MsgPack(that->parse_string<uint16_t>(first_byte)); } },
    { 0xdb, 0xdb, [](MsgPackParser* that, uint8_t first_byte){ return MsgPack(that->parse_string<uint32_t>(first_byte)); } },
    { 0xdc, 0xdc, [](MsgPackParser* that, uint8_t){ return MsgPack(that->parse_array<uint16_t>()); } },
    { 0xdd, 0xdd, [](MsgPackParser* that, uint8_t){ return MsgPack(that->parse_array<uint32_t>()); } },
    { 0xde, 0xde, [](MsgPackParser* that, uint8_t first_byte){ return MsgPack(that->parse_object<uint16_t>(first_byte)); } },
    { 0xdf, 0xdf, [](MsgPackParser* that, uint8_t first_byte){ return MsgPack(that->parse_object<uint32_t>(first_byte)); } },
    { 0xe0, 0xff, [](MsgPackParser* that, uint8_t first_byte){ return MsgPack(that->parse_neg_fixint(first_byte)); } }
};

}//namespace {

MsgPack MsgPack::parse(const std::vector<uint8_t> &in, string &err) {
    MsgPackParser parser { in, 0, err, false };
    MsgPack result = parser.parse_msgpack(0);

    return result;
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
