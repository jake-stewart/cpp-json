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
#include <cstring>

namespace json {
// CONSTEXPR UTILS {{{
template<typename Test, template<typename...> class Ref>
struct is_specialization : std::false_type {};

template<template<typename...> class Ref, typename... Args>
struct is_specialization<Ref<Args...>, Ref>: std::true_type {};

template <typename T, size_t n>
constexpr size_t array_size(const T (&)[n]) {
    return n;
}
// }}}
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
// JSON EXCEPTION {{{
struct exception {
    std::string description;
    int idx;
    exception(std::string description, int idx) {
        this->description = description;
        this->idx = idx;
    }
};
// }}}
// CURSOR {{{
struct Cursor
{
    const std::string &string;
    int idx = 0;

    Cursor(const std::string &string) : string(string) {}

    void expect(char c) {
        if (next() != c) {
            std::string desc = std::string("expected '") + c + "'";
            desc += std::string(" but got '") + peek(-1) + "'";
            throw exception(desc, idx);
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

    void skipWhitespaceAndComments() {
        while (true) {
            if (isspace(peek())) {
                next();
                continue;
            }
            if (peek() != '/') {
                break;
            }
            next();
            if (peek() == '/') {
                next();
                while (peek() && peek() != '\n') {
                    next();
                }
            }
            else if (peek() == '*') {
                next();
                while (peek()) {
                    if (peek() == '*' && peek(1) == '/') {
                        next(2);
                        break;
                    }
                    next();
                }
            }
            else {
                break;
            }
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
            throw exception("expected utf-8 surrogate pair", cursor.idx);
        }
        int cp2 = utf8::parseCodepoint(cursor.c_str() + 3);
        if (!(0xdc00 <= cp2 && cp2 <= 0xdfff)) {
            throw exception("invalid utf-8 surrogate pair", cursor.idx);
        }
        cp = utf8::desurrogatePair(cp, cp2);
        cursor.next(6);
    }
    if (cp > 0x10FFFF) {
        throw exception("invalid utf-8 codepoint", cursor.idx);
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

public:
    JsonArrayParser(Cursor &cursor) : m_cursor(cursor) {}

    void start() {
        m_cursor.skipWhitespaceAndComments();
        m_cursor.expect('[');
    }

    void finish() {
        m_cursor.skipWhitespaceAndComments();
        m_cursor.expect(']');
    }

    bool optionalNext() {
        m_cursor.skipWhitespaceAndComments();
        if (m_cursor.peek() == ']') {
            return false;
        }
        m_cursor.skipWhitespaceAndComments();
        if (!m_first) {
            m_cursor.expect(',');
        }
        m_cursor.skipWhitespaceAndComments();
        m_first = false;
        return true;
    }

    void next() {
        if (!optionalNext()) {
            throw exception("not enough elements in array", m_cursor.idx);
        }
    }
};

class JsonObjectParser
{
    Cursor &m_cursor;
    bool m_first = true;

public:
    JsonObjectParser(Cursor &cursor) : m_cursor(cursor) {}

    void start() {
        m_cursor.skipWhitespaceAndComments();
        m_cursor.expect('{');
    }

    void finish() {
        m_cursor.skipWhitespaceAndComments();
        m_cursor.expect('}');
    }

    bool optionalNext() {
        m_cursor.skipWhitespaceAndComments();
        if (m_cursor.peek() == '}') {
            return false;
        }
        m_cursor.skipWhitespaceAndComments();
        if (!m_first) {
            m_cursor.expect(',');
        }
        m_cursor.skipWhitespaceAndComments();
        m_first = false;
        return true;
    }

    void next() {
        if (!optionalNext()) {
            throw exception("not enough elements in object", m_cursor.idx);
        }
    }

    void value() {
        m_cursor.skipWhitespaceAndComments();
        m_cursor.expect(':');
        m_cursor.skipWhitespaceAndComments();
    }
};
// }}}
// SERIALIZE {{{
template <typename T>
std::string serialize(const T &item);

template <typename T>
std::string serializeUniquePointer(const std::unique_ptr<T> &item) {
    if (item) {
        return serialize(*item);
    }
    else {
        return "null";
    }
}

template <typename T>
std::string serializeOptional(const std::optional<T> &item) {
    if (item) {
        return serialize(*item);
    }
    else {
        return "null";
    }
}

template <typename T, typename Y>
std::string serializePair(const std::pair<T, Y> &item) {
    JsonArrayBuilder array;
    array.add(serialize(item.first));
    array.add(serialize(item.second));
    return array.build();
}

template <typename ...T>
std::string serializeTuple(const std::tuple<T...> &item) {
    JsonArrayBuilder array;
    constexpr auto size = std::tuple_size<std::tuple<T...>>::value;
    for_sequence(std::make_index_sequence<size>{}, [&](auto i) {
        array.add(serialize(std::get<i>(item)));
    });
    return array.build();
}

inline std::string serializeBoolVector(const std::vector<bool> &item) {
    JsonArrayBuilder array;
    for (const auto &elem : item) {
        array.add(elem ? "true" : "false");
    }
    return array.build();
}

template <typename T>
std::string serializeVector(const std::vector<T> &item) {
    JsonArrayBuilder array;
    for (const auto &elem : item) {
        array.add(serialize(elem));
    }
    return array.build();
}

template <typename T>
std::string serializeMap(const T &item) {
    using KeyType = typename std::decay<decltype(item.begin()->first)>::type;
    JsonObjectBuilder jsonObject;
    for (const auto &it : item) {
        constexpr bool isString = std::is_same<KeyType, std::string>().value ||
            std::is_same<KeyType, char *>().value ||
            std::is_same<KeyType, const char *>().value;
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

template <typename T>
std::string serializeString(const T &item) {
    const char *str;
    size_t len;
    if constexpr (std::is_same<T, std::string>().value) {
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
        switch ((unsigned char)str[i]) {
            case 128 ... 255: {
#ifndef JSON_ENCODE_ASCII
                string += str[i];
#else
                try {
                    string += utf8::escapeCodepoint(str, &i);
                }
                catch (const std::exception&) {
                    throw exception("invalid utf-8 codepoint", 0);
                }
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

inline std::string serializeBool(const bool &item) {
    return item ? "true" : "false";
}

template <typename T, size_t N>
std::string serializeArray(const T(&item)[N]) {
    JsonArrayBuilder array;
    for (int i = 0; i < N; i++) {
        array.add(serialize(item[i]));
    }
    return array.build();
}

template <typename T>
std::string serializePointer(const T &item) {
    if (item == nullptr) {
        return "null";
    }
    else {
        return serialize(*item);
    }
}

template <typename T>
std::string serializeEnum(const T &item) {
    return std::to_string(item);
}

template <typename T>
std::string serializeNumber(const T &item) {
    return std::to_string(item);
}

inline std::string serializeChar(const char &item) {
    unsigned char value = item;
    return std::to_string(value);
}

template <typename T>
std::string serializeClass(const T& item) {
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

template <typename T>
std::string serialize(const T &item) {
    if constexpr (is_specialization<T, std::unique_ptr>().value) {
        return serializeUniquePointer(item);
    }
    else if constexpr (is_specialization<T, std::optional>().value) {
        return serializeOptional(item);
    }
    else if constexpr (is_specialization<T, std::pair>().value) {
        return serializePair(item);
    }
    else if constexpr (is_specialization<T, std::tuple>().value) {
        return serializeTuple(item);
    }
    else if constexpr (std::is_same<T, std::vector<bool>>().value) {
        return serializeBoolVector(item);
    }
    else if constexpr (is_specialization<T, std::vector>().value) {
        return serializeVector(item);
    }
    else if constexpr (is_specialization<T, std::map>().value) {
        return serializeMap(item);
    }
    else if constexpr (is_specialization<T, std::unordered_map>().value) {
        return serializeMap(item);
    }
    else if constexpr (std::is_same<T, std::string>().value) {
        return serializeString(item);
    }
    else if constexpr (std::is_same<T, char *>().value) {
        return serializeString(item);
    }
    else if constexpr (std::is_same<T, const char *>().value) {
        return serializeString(item);
    }
    else if constexpr (std::is_array<T>().value) {
        return serializeArray(item);
    }
    else if constexpr (std::is_same<T, bool>().value) {
        return serializeBool(item);
    }
    else if constexpr (std::is_pointer<T>().value) {
        return serializePointer(item);
    }
    else if constexpr (std::is_enum<T>().value) {
        return serializeEnum(item);
    }
    else if constexpr (std::is_same<T, char>().value) {
        return serializeChar(item);
    }
    else if constexpr (std::is_arithmetic<T>().value) {
        return serializeNumber(item);
    }
    else if constexpr (std::is_class<T>().value) {
        return serializeClass(item);
    }
}
// }}}
// DESERIALIZE {{{
template <typename T>
void deserialize(T &item, Cursor &cursor);

template <typename T>
void deserialize(T &item, const std::string &json);

template <typename T>
T deserialize(const std::string &json);

template <typename T>
void deserializeUniquePointer(std::unique_ptr<T> &item, Cursor &cursor) {
    item = std::make_unique<T>();
    deserialize(*item, cursor);
}

template <typename T>
void deserializeOptional(std::optional<T> &item, Cursor &cursor) {
    std::string keyword = cursor.peekKeyword();
    if (keyword == "null") {
        cursor.next(keyword.size());
        item.reset();
    }
    else {
        T tmp;
        deserialize(tmp, cursor);
        item = tmp;
    }
}

template <typename T, typename Y>
void deserializePair(std::pair<T, Y> &item, Cursor &cursor) {
    item = std::pair<T, Y>();
    JsonArrayParser arrayParser(cursor);
    arrayParser.start();
    arrayParser.next();
    deserialize(item.first, cursor);
    arrayParser.next();
    deserialize(item.second, cursor);
    arrayParser.finish();
}

template <typename ...T>
void deserializeTuple(std::tuple<T...> &item, Cursor &cursor) {
    item = std::tuple<T...>();
    constexpr auto size = std::tuple_size<std::tuple<T...>>::value;
    JsonArrayParser arrayParser(cursor);
    arrayParser.start();
    for_sequence(std::make_index_sequence<size>{}, [&](auto i) {
        arrayParser.next();
        deserialize(std::get<i>(item), cursor);
    });
    arrayParser.finish();
}

inline void deserializeBoolVector(std::vector<bool> &item, Cursor &cursor) {
    item = std::vector<bool>();
    JsonArrayParser arrayParser(cursor);
    arrayParser.start();
    while (arrayParser.optionalNext()) {
        bool result;
        deserialize(result, cursor);
        item.push_back(result);
    }
    arrayParser.finish();
}

template <typename T>
void deserializeVector(std::vector<T> &item, Cursor &cursor) {
    item = std::vector<T>();
    JsonArrayParser arrayParser(cursor);
    arrayParser.start();
    while (arrayParser.optionalNext()) {
        item.push_back(T());
        deserialize(item.back(), cursor);
    }
    arrayParser.finish();
}

template <typename T>
void deserializeMap(T &item, Cursor &cursor) {
    using KeyType = typename std::decay<decltype(item.begin()->first)>::type;
    JsonObjectParser objectParser(cursor);
    objectParser.start();
    while (objectParser.optionalNext()) {
        KeyType key;
        constexpr bool isString = std::is_same<KeyType, std::string>().value ||
            std::is_same<KeyType, char *>().value ||
            std::is_same<KeyType, const char *>().value;
        if constexpr (isString) {
            deserialize(key, cursor);
        }
        else {
            std::string stringKey;
            deserialize(stringKey, cursor);
            deserialize<KeyType>(key, stringKey);
        }
        objectParser.value();
        deserialize(item[key], cursor);
    }
    objectParser.finish();
}

template <typename T>
void deserializeString(T &item, Cursor &cursor) {
    if constexpr (std::is_pointer<T>().value) {
        std::string keyword = cursor.getKeyword();
        if (keyword == "null") {
            item = nullptr;
            return;
        }
        else if (keyword.size()) {
            throw exception("invalid keyword '" + keyword + "'", cursor.idx);
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
                    try {
                        string += utf8::unescapeCodepoint(cursor);
                    }
                    catch (const std::exception&) {
                        throw exception("invalid utf-8 codepoint", cursor.idx);
                    }
                    break;
                }
                default:
                    throw exception("invalid escape character", cursor.idx);
            }
        }
        else {
            string += cursor.peek();
        }
        cursor.next();
    }
    cursor.expect('"');
    if constexpr (std::is_pointer<T>().value) {
        string += '\0';
        item = new char[string.size()];
        memcpy((void *)item, string.c_str(), string.size());
    }
    else {
        item = string;
    }
}

inline void deserializeBool(bool &item, Cursor &cursor) {
    std::string keyword = cursor.getKeyword();
    if (keyword == "true") {
        item = true;
    }
    else if (keyword == "false") {
        item = false;
    }
    else {
        throw exception("invalid keyword '" + keyword + "'", cursor.idx);
    }
}

template <typename T, size_t N>
void deserializeArray(T(&item)[N], Cursor &cursor) {
    JsonArrayParser arrayParser(cursor);
    arrayParser.start();
    for (int i = 0; i < N && arrayParser.optionalNext(); i++) {
        deserialize(item[i], cursor);
    }
    arrayParser.finish();
}

template <typename T>
void deserializePointer(T &item, Cursor &cursor) {
    using Type = typename std::remove_pointer<T>::type;
    if (cursor.peekKeyword() == "null") {
        cursor.next(4);
        item = nullptr;
        return;
    }
    item = new Type();
    deserialize(*item, cursor);
}

template <typename T>
void deserializeEnum(T &item, Cursor &cursor) {
    int value;
    deserialize(value, cursor);
    item = (T)value;
}

template <typename T>
void deserializeNumber(T &item, Cursor &cursor) {
    int length = 0;
    if (cursor.peek(length) == '-') {
        length++;
    }
    while (isdigit(cursor.peek(length))) {
        length++;
    }
    if constexpr (std::is_floating_point<T>().value) {
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
        if constexpr (std::is_floating_point<T>().value) {
            item = (T)std::stold(number);
        }
        else if constexpr (std::is_signed<T>().value) {
            item = (T)std::stoll(number);
        }
        else {
            item = (T)std::stoull(number);
        }
    }
    catch (const std::invalid_argument &) {
        throw exception("invalid number", cursor.idx);
    }
    catch (const std::out_of_range &) {
        throw exception("invalid number", cursor.idx);
    }
}

inline void deserializeChar(char &item, Cursor &cursor) {
    unsigned char value;
    deserializeNumber<unsigned char>(value, cursor);
    item = value;
}

template <typename T>
void deserializeClass(T &item, Cursor &cursor) {
    constexpr auto props = properties<T>();
    constexpr auto size = std::tuple_size<decltype(props)>::value;
    JsonObjectParser objectParser(cursor);
    objectParser.start();
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
    objectParser.finish();
}

template <typename T>
void deserialize(T &item, Cursor &cursor) {
    if constexpr (is_specialization<T, std::unique_ptr>().value) {
        deserializeUniquePointer(item, cursor);
    }
    else if constexpr (is_specialization<T, std::optional>().value) {
        deserializeOptional(item, cursor);
    }
    else if constexpr (is_specialization<T, std::pair>().value) {
        deserializePair(item, cursor);
    }
    else if constexpr (is_specialization<T, std::tuple>().value) {
        deserializeTuple(item, cursor);
    }
    else if constexpr (std::is_same<T, std::vector<bool>>().value) {
        deserializeBoolVector(item, cursor);
    }
    else if constexpr (is_specialization<T, std::vector>().value) {
        deserializeVector(item, cursor);
    }
    else if constexpr (is_specialization<T, std::map>().value) {
        deserializeMap(item, cursor);
    }
    else if constexpr (is_specialization<T, std::unordered_map>().value) {
        deserializeMap(item, cursor);
    }
    else if constexpr (std::is_same<T, std::string>().value) {
        deserializeString(item, cursor);
    }
    else if constexpr (std::is_same<T, char *>().value) {
        deserializeString(item, cursor);
    }
    else if constexpr (std::is_same<T, const char *>().value) {
        deserializeString(item, cursor);
    }
    else if constexpr (std::is_same<T, bool>().value) {
        deserializeBool(item, cursor);
    }
    else if constexpr (std::is_array<T>().value) {
        deserializeArray(item, cursor);
    }
    else if constexpr (std::is_pointer<T>().value) {
        deserializePointer(item, cursor);
    }
    else if constexpr (std::is_enum<T>().value) {
        deserializeEnum(item, cursor);
    }
    else if constexpr (std::is_same<T, char>().value) {
        deserializeChar(item, cursor);
    }
    else if constexpr (std::is_arithmetic<T>().value) {
        deserializeNumber(item, cursor);
    }
    else if constexpr (std::is_class<T>().value) {
        deserializeClass(item, cursor);
    }
}
// }}}
// DESERIALIZATION HELPERS IMPLEMENTATION {{{
template <typename T>
void deserialize(T &item, const std::string &json) {
    Cursor cursor(json);
    deserialize(item, cursor);
    cursor.skipWhitespaceAndComments();
    if (cursor.peek()) {
        throw exception("expected EOF", cursor.idx);
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
