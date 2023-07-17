#pragma once

#include <map>
#include <memory>
#include <optional>
#include <string>
#include <tuple>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <vector>

namespace json {
// FOR EACH MACRO {{{
#define PARENS ()

#define EXPAND(...) EXPAND4(EXPAND4(EXPAND4(EXPAND4(__VA_ARGS__))))
#define EXPAND4(...) EXPAND3(EXPAND3(EXPAND3(EXPAND3(__VA_ARGS__))))
#define EXPAND3(...) EXPAND2(EXPAND2(EXPAND2(EXPAND2(__VA_ARGS__))))
#define EXPAND2(...) EXPAND1(EXPAND1(EXPAND1(EXPAND1(__VA_ARGS__))))
#define EXPAND1(...) __VA_ARGS__

#define FOR_EACH(macro, ...)                                                   \
    __VA_OPT__(EXPAND(FOR_EACH_HELPER(macro, __VA_ARGS__)))

#define FOR_EACH_HELPER(macro, a1, ...)                                        \
    macro(a1) __VA_OPT__(FOR_EACH_AGAIN PARENS(macro, __VA_ARGS__))

#define FOR_EACH_AGAIN() FOR_EACH_HELPER
// }}}
// REFLECTION {{{
template <typename T, T... S, typename F>
constexpr void for_sequence(std::integer_sequence<T, S...>, F &&f) {
    (static_cast<void>(f(std::integral_constant<T, S>{})), ...);
}

template <typename T>
constexpr auto properties() {}

template <typename Class, typename T>
struct Property
{
    using Type = T;
    const char *key;
    T Class::*value;

    constexpr Property(const char *key, T Class::*value) {
        this->key = key;
        this->value = value;
    }
};

#define REFLECT_PROPERTY(KEY) json::Property(#KEY, &_class::KEY),
#define REFLECT(CLASS, ...)                                                    \
    template <>                                                                \
    constexpr auto json::properties<CLASS>() {                                 \
        using _class = CLASS;                                                  \
        return std::tuple{FOR_EACH(REFLECT_PROPERTY, __VA_ARGS__)};            \
    }
// }}}
// CURSOR {{{
struct Cursor
{
    const std::string &string;
    int idx = 0;

    Cursor(const std::string &string) : string(string) {}

    void expect(char c) {
        if (next() != c) {
            throw std::exception();
        }
    }

    const char *c_str() {
        return string.c_str() + idx;
    }

    const char &peek() {
        return string[idx];
    }

    const char &peek(int i) {
        return string[idx + i];
    }

    const char &next() {
        return string[idx++];
    }

    void next(int i) {
        idx += i;
    }

    void skipWhitespace() {
        while (isspace(peek())) {
            next();
        }
    }

    std::string peekKeyword() {
        std::string keyword;
        if (isalpha(peek())) {
            int size = 0;
            do {
                keyword += peek(size++);
            } while (isalpha(peek(size)));
        }
        return keyword;
    }

    std::string getKeyword() {
        std::string keyword;
        if (isalpha(peek())) {
            do {
                keyword += next();
            } while (isalpha(peek()));
        }
        return keyword;
    }

    std::string substr(int length) {
        idx += length;
        return string.substr(idx - length, length);
    }
};
// }}}
// UTF-8 {{{
namespace utf8 {
inline int glyphLen(unsigned char ch) {
    if (ch < 128) {
        return 1;
    }
    else if (ch >> 5 == 0b110) {
        return 2;
    }
    else if (ch >> 4 == 0b1110) {
        return 3;
    }
    else if (ch >> 3 == 0b11110) {
        return 4;
    }
    throw std::exception();
}

inline int desurrogatePair(int cp1, int cp2) {
    return 0x10000 + (((cp1 - 0xd800) << 10) | (cp2 - 0xdc00));
}

inline void surrogatePair(int cp, int *s1, int *s2) {
    cp -= 0x10000;
    *s1 = 0xd800 | ((cp >> 10) & 0x3ff);
    *s2 = 0xdc00 | (cp & 0x3ff);
}

inline int parseCodepoint(const char *c) {
    int cp;
    char buf[5] = {0};
    for (int i = 0; i < 4; i++) {
        switch (*c) {
            case '0' ... '9':
            case 'a' ... 'f':
            case 'A' ... 'F':
                buf[i] = *c;
                break;
            default:
                throw std::exception();
        }
        c++;
    }
    sscanf(buf, "%x", &cp);
    return cp;
}

inline int codepointToBytes(int cp, char *buf) {
    switch (cp) {
        case 0x0000 ... 0x007F:
            buf[0] = cp;
            return 1;
        case 0x0080 ... 0x07FF:
            buf[0] = 0b11000000 | (0b00011111 & (cp >> 6));
            buf[1] = 0b10000000 | (0b00111111 & (cp >> 0));
            return 2;
        case 0x0800 ... 0xFFFF:
            buf[0] = 0b11100000 | (0b00001111 & (cp >> 12));
            buf[1] = 0b10000000 | (0b00111111 & (cp >> 6));
            buf[2] = 0b10000000 | (0b00111111 & (cp >> 0));
            return 3;
        case 0x10000 ... 0x10FFFF:
            buf[0] = 0b11110000 | (0b00000111 & (cp >> 18));
            buf[1] = 0b10000000 | (0b00111111 & (cp >> 12));
            buf[2] = 0b10000000 | (0b00111111 & (cp >> 6));
            buf[3] = 0b10000000 | (0b00111111 & (cp >> 0));
            return 4;
        default:
            throw std::exception();
    }
}

inline int bytesToCodepoint(const char *buf, int *cp) {
    int length = glyphLen(*buf);
    switch (length) {
        case 1:
            *cp = (((*(buf + 0)) & 0b00111111) << 0);
            break;
        case 2:
            *cp = (((*(buf + 0)) & 0b00011111) << 6) +
                (((*(buf + 1)) & 0b00111111) << 0);
            break;
        case 3:
            *cp = (((*(buf + 0)) & 0b00001111) << 12) +
                (((*(buf + 1)) & 0b00111111) << 6) +
                (((*(buf + 2)) & 0b00111111) << 0);
            break;
        case 4:
            *cp = (((*(buf + 0)) & 0b00000111) << 18) +
                (((*(buf + 1)) & 0b00111111) << 12) +
                (((*(buf + 2)) & 0b00111111) << 6) +
                (((*(buf + 3)) & 0b00111111) << 0);
            break;
        default:
            throw std::exception();
    }
    return length;
}

inline std::string escapeCodepoint(const char *str, int *offset) {
    int cp;
    *offset += utf8::bytesToCodepoint(str + *offset, &cp) - 1;
    char buf[16];
    if (cp < 0x10000) {
        snprintf(buf, sizeof(buf), "\\u%.4x", cp);
        return std::string(buf, 6);
    }
    else {
        int s1, s2;
        utf8::surrogatePair(cp, &s1, &s2);
        snprintf(buf, sizeof(buf), "\\u%.4x\\u%.4x", s1, s2);
        return std::string(buf, 12);
    }
}

inline std::string unescapeCodepoint(Cursor &cursor) {
    cursor.next();
    int cp = utf8::parseCodepoint(cursor.c_str());
    cursor.next(3);
    if (0xd800 <= cp && cp <= 0xdbff) {
        if (cursor.peek(1) != '\\' || cursor.peek(2) != 'u') {
            throw std::exception();
        }
        int cp2 = utf8::parseCodepoint(cursor.c_str() + 3);
        if (!(0xdc00 <= cp2 && cp2 <= 0xdfff)) {
            throw std::exception();
        }
        cp = utf8::desurrogatePair(cp, cp2);
        cursor.next(6);
    }
    if (cp > 0x10FFFF) {
        throw std::exception();
    }
    char buf[4];
    int size = utf8::codepointToBytes(cp, buf);
    return std::string(buf, size);
}
};
// }}}
// JSON BUILDERS {{{
class JsonArrayBuilder
{
    std::string m_buffer;

public:
    void add(std::string item) {
        m_buffer += ',';
        m_buffer += item;
    }

    const std::string &build() {
        if (!m_buffer.size()) {
            m_buffer = "[]";
            return m_buffer;
        }
        m_buffer[0] = '[';
        m_buffer += ']';
        return m_buffer;
    }
};

class JsonObjectBuilder
{
    std::string m_buffer;

public:
    void quotedKey(const std::string &key) {
        m_buffer += ',';
        m_buffer += '"';
        m_buffer += key;
        m_buffer += '"';
    }

    void key(const std::string &key) {
        m_buffer += ',';
        m_buffer += key;
    }

    void value(const std::string &value) {
        m_buffer += ':';
        m_buffer += value;
    }

    const std::string &build() {
        if (!m_buffer.size()) {
            m_buffer = "{}";
            return m_buffer;
        }
        m_buffer[0] = '{';
        m_buffer += '}';
        return m_buffer;
    }
};
// }}}
// JSON PARSERS {{{
class JsonArrayParser
{
    Cursor &m_cursor;
    bool m_first = true;

    void start() {
        m_cursor.skipWhitespace();
        m_cursor.expect('[');
    }

    void finish() {
        m_cursor.skipWhitespace();
        m_cursor.expect(']');
    }

public:
    JsonArrayParser(Cursor &cursor) : m_cursor(cursor) {
        start();
    }

    ~JsonArrayParser() {
        finish();
    }

    bool optionalNext() {
        m_cursor.skipWhitespace();
        if (m_cursor.peek() == ']') {
            return false;
        }
        m_cursor.skipWhitespace();
        if (!m_first) {
            m_cursor.expect(',');
        }
        m_cursor.skipWhitespace();
        m_first = false;
        return true;
    }

    void next() {
        if (!optionalNext()) {
            throw std::exception();
        }
    }
};

class JsonObjectParser
{
    Cursor &m_cursor;
    bool m_first = true;

    void start() {
        m_cursor.skipWhitespace();
        m_cursor.expect('{');
    }

    void finish() {
        m_cursor.skipWhitespace();
        m_cursor.expect('}');
    }

public:
    JsonObjectParser(Cursor &cursor) : m_cursor(cursor) {
        start();
    }

    ~JsonObjectParser() {
        finish();
    }

    bool optionalNext() {
        m_cursor.skipWhitespace();
        if (m_cursor.peek() == '}') {
            return false;
        }
        m_cursor.skipWhitespace();
        if (!m_first) {
            m_cursor.expect(',');
        }
        m_cursor.skipWhitespace();
        m_first = false;
        return true;
    }

    void next() {
        if (!optionalNext()) {
            throw std::exception();
        }
    }

    void value() {
        m_cursor.skipWhitespace();
        m_cursor.expect(':');
        m_cursor.skipWhitespace();
    }
};
// }}}
// DEFINITION {{{

// SERIALIZE/DESERIALIZE INTERFACE
template <typename T>
std::string serialize(const T &item);
template <typename T>
void deserialize(T item, Cursor &cursor);

// DESERIALIZE HELPERS
template <typename T>
void deserialize(T &item, const std::string &json);
template <typename T>
T deserialize(const std::string &json);

// STRUCTS
template <typename T> requires(std::is_class<T>())
std::string serialize(const T &item);
template <typename T> requires(std::is_class<T>())
void deserialize(T &item, Cursor &cursor);

// POINTERS
template <typename T>
std::string serialize(T *const &item);
template <typename T>
void deserialize(T *&item, Cursor &cursor);

// UNIQUE POINTERS
template <typename T>
std::string serialize(const std::unique_ptr<T> &item);
template <typename T>
void deserialize(std::unique_ptr<T> &item, Cursor &cursor);

// NUMBERS
template <typename T> requires(std::is_arithmetic<T>())
std::string serialize(const T &item);
template <typename T> requires(std::is_arithmetic<T>())
void deserialize(T &item, Cursor &cursor);

// ENUMS
template <typename T> requires(std::is_enum<T>())
std::string serialize(const T &item);
template <typename T> requires(std::is_enum<T>())
void deserialize(T &item, Cursor &cursor);

// OPTIONALS
template <typename T>
std::string serialize(const std::optional<T> &item);
template <typename T>
void deserialize(std::optional<T> &item, Cursor &cursor);

// VECTORS
template <typename T>
std::string serialize(const std::vector<T> &item);
template <typename T>
void deserialize(std::vector<T> &item, Cursor &cursor);

// BIT REFERENCE VECTORS
// std::vector<bool> does some __bit_reference magic behind the scenes
template <typename T>
std::string serialize(const std::__bit_const_reference<T> &item);
template <>
inline void deserialize(std::__bit_reference<std::vector<bool>> item, Cursor &cursor);

// STATIC ARRAYS
template <typename T, size_t Y>
std::string serialize(const T (&item)[Y]);
template <typename T, size_t Y>
void deserialize(T (&item)[Y], Cursor &cursor);

// PAIRS
template <typename T, typename Y>
std::string serialize(const std::pair<T, Y> &item);
template <typename T, typename Y>
void deserialize(std::pair<T, Y> &item, Cursor &cursor);

// TUPLES
template <typename... T>
std::string serialize(const std::tuple<T...> &item);
template <typename... T>
void deserialize(std::tuple<T...> &item, Cursor &cursor);

// MAPS
template <typename T, typename K>
std::string serializeMap(const T &item);
template <typename T, typename K>
void deserializeMap(T &item, Cursor &cursor);

template <typename K, typename V>
std::string serialize(const std::map<K, V> &item);
template <typename K, typename V>
void deserialize(std::map<K, V> &item, Cursor &cursor);

template <typename K, typename V>
std::string serialize(const std::unordered_map<K, V> &item);
template <typename K, typename V>
void deserialize(std::unordered_map<K, V> &item, Cursor &cursor);

// STRINGS
template <typename T>
static std::string serializeString(const T &item);
template <typename T>
void deserializeString(T &item, Cursor &cursor);

template <>
inline std::string serialize(const std::string &item);
template <>
inline void deserialize(std::string &item, Cursor &cursor);

template <>
inline std::string serialize(const char *const &item);
template <>
inline void deserialize(const char *&item, Cursor &cursor);

template <>
inline std::string serialize(char *const &item);
template <>
inline void deserialize(char *&item, Cursor &cursor);

// BOOLS
template <>
inline std::string serialize(const bool &item);
template <>
inline void deserialize(bool &item, Cursor &cursor);

// }}}
// IMPLEMENTATION {{{

// STRUCTS
template <typename T> requires(std::is_class<T>())
std::string serialize(const T &item) {
    JsonObjectBuilder jsonObject;
    constexpr auto props = properties<T>();
    constexpr auto size = std::tuple_size<decltype(props)>::value;
    for_sequence(std::make_index_sequence<size>{}, [&](auto i) {
        constexpr auto property = std::get<i>(properties<T>());
        jsonObject.quotedKey(property.key);
        jsonObject.value(serialize(item.*(property.value)));
    });
    return jsonObject.build();
}

template <typename T> requires(std::is_class<T>())
void deserialize(T &item, Cursor &cursor) {
    constexpr auto props = properties<T>();
    constexpr auto size = std::tuple_size<decltype(props)>::value;
    JsonObjectParser objectParser(cursor);
    while (objectParser.optionalNext()) {
        std::string key;
        deserialize(key, cursor);
        objectParser.value();
        for_sequence(std::make_index_sequence<size>{}, [&](auto i) {
            constexpr auto property = std::get<i>(properties<T>());
            if (strcmp(property.key, key.c_str()) == 0) {
                deserialize(item.*(property.value), cursor);
            }
        });
    }
}

// POINTERS
template <typename T>
std::string serialize(T *const &item) {
    if (item == nullptr) {
        return "null";
    }
    else {
        return serialize(*item);
    }
}

template <typename T>
void deserialize(T *&item, Cursor &cursor) {
    if (cursor.peekKeyword() == "null") {
        cursor.next(4);
        item = nullptr;
        return;
    }
    item = new T();
    deserialize(*item, cursor);
}

// UNIQUE POINTERS
template <typename T>
std::string serialize(const std::unique_ptr<T> &item) {
    if (item) {
        return serialize(*item);
    }
    else {
        return "null";
    }
}

template <typename T>
void deserialize(std::unique_ptr<T> &item, Cursor &cursor) {
    item = std::make_unique<T>();
    deserialize(*item, cursor);
}

// NUMBERS
template <typename T> requires(std::is_arithmetic<T>())
std::string serialize(const T &item) {
    return std::to_string(item);
}

template <typename T> requires(std::is_arithmetic<T>())
void deserialize(T &item, Cursor &cursor) {
    int length = 0;
    if (cursor.peek(length) == '-') {
        length++;
    }
    while (isdigit(cursor.peek(length))) {
        length++;
    }
    if constexpr (std::is_floating_point<T>()) {
        if (cursor.peek(length) == '.') {
            length++;
            while (isdigit(cursor.peek(length))) {
                length++;
            }
            if (tolower(cursor.peek(length)) == 'e') {
                length++;
                if (cursor.peek(length) == '-') {
                    length++;
                }
                while (isdigit(cursor.peek(length))) {
                    length++;
                }
            }
        }
    }
    std::string number = cursor.substr(length);
    try {
        if constexpr (std::is_floating_point<T>()) {
            item = (T)std::stold(number);
        }
        else if constexpr (std::is_signed<T>()) {
            item = (T)std::stoll(number);
        }
        else {
            item = (T)std::stoull(number);
        }
    }
    catch (const std::invalid_argument &) {
        throw std::exception();
    }
    catch (const std::out_of_range &) {
        throw std::exception();
    }
}

// ENUMS
template <typename T> requires(std::is_enum<T>())
std::string serialize(const T &item) {
    return std::to_string(item);
}

template <typename T> requires(std::is_enum<T>())
void deserialize(T &item, Cursor &cursor) {
    int value;
    deserialize(value, cursor);
    item = (T)value;
}

// OPTIONALS
template <typename T>
std::string serialize(const std::optional<T> &item) {
    if (item) {
        return serialize(*item);
    }
    else {
        return "null";
    }
}

template <typename T>
void deserialize(std::optional<T> &item, Cursor &cursor) {
    std::string keyword = cursor.peekKeyword();
    if (keyword == "null") {
        cursor.next(keyword.size());
        item.reset();
    }
    else {
        T elem;
        deserialize(elem, cursor);
        item = elem;
    }
}

// VECTORS
template <typename T>
std::string serialize(const std::vector<T> &item) {
    JsonArrayBuilder array;
    for (const auto &elem : item) {
        array.add(serialize(elem));
    }
    return array.build();
}

template <typename T>
void deserialize(std::vector<T> &item, Cursor &cursor) {
    item = std::vector<T>();
    JsonArrayParser arrayParser(cursor);
    while (arrayParser.optionalNext()) {
        item.push_back(T());
        deserialize(item.back(), cursor);
    }
}

// BIT REFERENCE VECTORS
template <typename T>
std::string serialize(const std::__bit_const_reference<T> &item) {
    return serialize((bool)item);
}

template <>
inline void deserialize(
    std::__bit_reference<std::vector<bool>> item, Cursor &cursor
) {
    bool value;
    deserialize<bool>(value, cursor);
    item = value;
}

// STATIC ARRAYS
template <typename T, size_t Y>
std::string serialize(const T (&item)[Y]) {
    JsonArrayBuilder array;
    for (int i = 0; i < Y; i++) {
        array.add(serialize(item[i]));
    }
    return array.build();
}

template <typename T, size_t Y>
void deserialize(T (&item)[Y], Cursor &cursor) {
    JsonArrayParser arrayParser(cursor);
    for (int i = 0; i < Y && arrayParser.optionalNext(); i++) {
        deserialize(item[i], cursor);
    }
}

// PAIRS
template <typename T, typename Y>
std::string serialize(const std::pair<T, Y> &item) {
    JsonArrayBuilder array;
    array.add(serialize(item.first));
    array.add(serialize(item.second));
    return array.build();
}

template <typename T, typename Y>
void deserialize(std::pair<T, Y> &item, Cursor &cursor) {
    item = std::pair<T, Y>();
    JsonArrayParser arrayParser(cursor);
    arrayParser.next();
    deserialize(item.first, cursor);
    arrayParser.next();
    deserialize(item.second, cursor);
}

// TUPLES
template <typename... T>
std::string serialize(const std::tuple<T...> &item) {
    JsonArrayBuilder array;
    constexpr auto size = std::tuple_size<std::tuple<T...>>::value;
    for_sequence(std::make_index_sequence<size>{}, [&](auto i) {
        array.add(serialize(std::get<i>(item)));
    });
    return array.build();
}

template <typename... T>
void deserialize(std::tuple<T...> &item, Cursor &cursor) {
    item = std::tuple<T...>();
    constexpr auto size = std::tuple_size<std::tuple<T...>>::value;
    JsonArrayParser arrayParser(cursor);
    for_sequence(std::make_index_sequence<size>{}, [&](auto i) {
        arrayParser.next();
        deserialize(std::get<i>(item), cursor);
    });
}

// MAPS
template <typename T, typename K>
std::string serializeMap(const T &item) {
    JsonObjectBuilder jsonObject;
    for (const auto &it : item) {
        constexpr bool isString = std::is_same<K, std::string>() ||
            std::is_same<K, char *>() || std::is_same<K, const char *>();
        if constexpr (isString) {
            jsonObject.key(serialize(it.first));
        }
        else {
            jsonObject.key(serialize(serialize(it.first)));
        }
        jsonObject.value(serialize(it.second));
    }
    return jsonObject.build();
}

template <typename T, typename K>
void deserializeMap(T &item, Cursor &cursor) {
    JsonObjectParser objectParser(cursor);
    while (objectParser.optionalNext()) {
        K key;
        constexpr bool isString = std::is_same<K, std::string>() ||
            std::is_same<K, char *>() || std::is_same<K, const char *>();
        if constexpr (isString) {
            deserialize(key, cursor);
        }
        else {
            std::string stringKey;
            deserialize(stringKey, cursor);
            key = deserialize<K>(stringKey);
        }
        objectParser.value();
        deserialize(item[key], cursor);
    }
}

template <typename K, typename V>
std::string serialize(const std::map<K, V> &item) {
    return serializeMap<std::map<K, V>, K>(item);
}

template <typename K, typename V>
void deserialize(std::map<K, V> &item, Cursor &cursor) {
    deserializeMap<std::map<K, V>, K>(item, cursor);
}

template <typename K, typename V>
std::string serialize(const std::unordered_map<K, V> &item) {
    return serializeMap<std::unordered_map<K, V>, K>(item);
}

template <typename K, typename V>
void deserialize(std::unordered_map<K, V> &item, Cursor &cursor) {
    deserializeMap<std::unordered_map<K, V>, K>(item, cursor);
}

// STRINGS
template <typename T>
static std::string serializeString(const T &item) {
    const char *str;
    size_t len;
    if constexpr (std::is_same<T, std::string>()) {
        str = item.c_str();
        len = item.size();
    }
    else {
        str = item;
        len = strlen(item);
    }
    std::string string;
    string += '"';
    for (int i = 0; i < len; i++) {
        switch (str[i]) {
            case -256 ... - 1: {
#ifndef JSON_ENCODE_ASCII
                string += str[i];
#else
                string += utf8::escapeCodepoint(str, &i);
#endif
                break;
            }
            case 0 ... 7:
            case 11 ... 12:
            case 14 ... 31:
            case 127:
                char buf[7];
                snprintf(buf, sizeof(buf), "\\u%.4x", (unsigned char)str[i]);
                string += std::string(buf, 6);
                break;
            case '\n':
                string += "\\n";
                break;
            case '\b':
                string += "\\b";
                break;
            case '\r':
                string += "\\r";
                break;
            case '\t':
                string += "\\t";
                break;
            case '"':
                string += "\\\"";
                break;
            case '\\':
                string += "\\\\";
                break;
            default:
                string += *(str + i);
                break;
        }
    }
    return string + '"';
}

template <typename T>
void deserializeString(T &item, Cursor &cursor) {
    if constexpr (std::is_pointer<T>()) {
        std::string keyword = cursor.getKeyword();
        if (keyword == "null") {
            item = nullptr;
            return;
        }
        else if (keyword.size()) {
            throw std::exception();
        }
    }
    cursor.expect('"');
    int length = 0;
    std::string string;
    while (cursor.peek() != '"') {
        if (cursor.peek() == '\\') {
            cursor.next();
            switch (cursor.peek()) {
                case '\\':
                    string += '\\';
                    break;
                case '"':
                    string += '"';
                    break;
                case 't':
                    string += '\t';
                    break;
                case 'n':
                    string += '\n';
                    break;
                case 'r':
                    string += '\r';
                    break;
                case 'b':
                    string += '\b';
                    break;
                case 'u': {
                    string += utf8::unescapeCodepoint(cursor);
                    break;
                }
                default:
                    throw std::exception();
            }
        }
        else {
            string += cursor.peek();
        }
        cursor.next();
    }
    cursor.expect('"');
    if constexpr (std::is_pointer<T>()) {
        item = new char[string.size()];
        memcpy((void *)item, string.c_str(), string.size());
    }
    else {
        item = string;
    }
}

template <>
inline std::string serialize(const std::string &item) {
    return serializeString(item);
}

template <>
inline std::string serialize(const char *const &item) {
    if (item == nullptr) {
        return "null";
    }
    return serializeString(item);
}

template <>
inline std::string serialize(char *const &item) {
    if (item == nullptr) {
        return "null";
    }
    return serializeString(item);
}

template <>
inline void deserialize(std::string &item, Cursor &cursor) {
    deserializeString(item, cursor);
}

template <>
inline void deserialize(const char *&item, Cursor &cursor) {
    deserializeString(item, cursor);
}

template <>
inline void deserialize(char *&item, Cursor &cursor) {
    deserializeString(item, cursor);
}

// BOOLS
template <>
inline std::string serialize(const bool &item) {
    return item ? "true" : "false";
}

template <>
inline void deserialize(bool &item, Cursor &cursor) {
    std::string keyword = cursor.getKeyword();
    if (keyword == "true") {
        item = true;
    }
    else if (keyword == "false") {
        item = false;
    }
    else {
        throw std::exception();
    }
}
// }}}
// DESERIALIZATION HELPERS IMPLEMENTATION {{{
template <typename T>
void deserialize(T &item, const std::string &json) {
    Cursor cursor(json);
    deserialize(item, cursor);
    cursor.skipWhitespace();
    if (cursor.peek()) {
        throw std::exception();
    }
}

template <typename T>
T deserialize(const std::string &json) {
    T item;
    deserialize(item, json);
    return item;
}
// }}}
// JSON PRETTIFIER {{{
class Prettifier
{
    int m_indent;

public:
    Prettifier() : m_indent(4) {}
    Prettifier(int indent) : m_indent(indent) {}

    void setIndent(int indent) {
        m_indent = indent;
    }

    std::string prettify(std::string json) {
        std::string prettyJson;
        int depth = 0;
        for (int i = 0; i < json.size(); i++) {
            switch (json[i]) {
                case ' ':
                case '\t':
                case '\n':
                    break;
                case '"':
                    prettyJson += json[i];
                    while (json[++i] != '"') {
                        prettyJson += json[i];
                        if (json[i] == '\\') {
                            prettyJson += json[++i];
                        }
                    }
                    prettyJson += json[i];
                    break;
                case '[':
                case '{':
                    prettyJson += json[i];
                    while (isspace(json[i + 1])) {
                        i++;
                    }
                    if (json[i + 1] == '}' || json[i + 1] == ']') {
                        prettyJson += json[++i];
                    }
                    else {
                        prettyJson += '\n';
                        depth++;
                        for (int j = 0; j < depth * m_indent; j++) {
                            prettyJson += ' ';
                        }
                    }
                    break;
                case ']':
                case '}':
                    depth--;
                    prettyJson += '\n';
                    for (int j = 0; j < depth * m_indent; j++) {
                        prettyJson += ' ';
                    }
                    prettyJson += json[i];
                    break;
                case ':':
                    prettyJson += json[i];
                    prettyJson += ' ';
                    break;
                case ',':
                    prettyJson += json[i];
                    prettyJson += '\n';
                    for (int j = 0; j < depth * m_indent; j++) {
                        prettyJson += ' ';
                    }
                    break;
                default:
                    prettyJson += json[i];
                    break;
            }
        }
        return prettyJson;
    }
};
// }}}
};
