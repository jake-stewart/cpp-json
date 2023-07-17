#include <iostream>
#include <map>
#include <tuple>
#include <string>

#define JSON_ENCODE_ASCII

#include "../include/json.hpp"

json::Prettifier prettifier(4);

struct EmptyStruct {
    bool operator==(const EmptyStruct &rhs) {
        return true;
    }
};
REFLECT(EmptyStruct);

struct RealisticStruct {
    std::string string;
    int integer;
    float float1;
    float float2;
    std::vector<int> integers;
};
REFLECT(RealisticStruct, string, integer, float1, float2, integers);

struct Keyword {
    std::string value;
};

template<>
std::string json::serialize(const Keyword& s) {
    return s.value;
}

template<>
void json::deserialize(Keyword& s, Cursor &cursor) {
    s.value = cursor.getKeyword();
    if (s.value.size() == 0) {
        throw std::exception();
    }
}

struct MassiveStruct {
    std::string string1;
    std::string string2;
    std::string string3;
    int int1;
    int int2;
    int int3;
    float float1;
    float float2;
    float float3;
    double double1;
    double double2;
    double double3;
    EmptyStruct emptyStruct;
    std::vector<RealisticStruct> structs1;
    RealisticStruct structs2[3];
    std::map<std::string, RealisticStruct> structs3;
    std::optional<bool> optional1;
    std::optional<int> optional2;
    std::optional<float> optional3;
    std::optional<RealisticStruct> optional4;
    std::optional<RealisticStruct> optional5;
    Keyword keyword1;
    Keyword keyword2;
    std::tuple<int, int, float, std::string> tuple1;
    std::tuple<> tuple2;
    std::tuple<RealisticStruct, float> tuple3;
    std::tuple<std::string, const char *> tuple4;
    std::pair<int, float> pair1;
    std::pair<std::string, std::optional<std::string>> pair2;
    std::pair<const char*, std::map<std::string, std::string>> pair3;
};
REFLECT(
    MassiveStruct, string1, string2, string3, int1, int2, int3, float1, float2,
    float3, double1, double2, double3, emptyStruct, structs1, structs2,
    structs3, optional1, optional2, optional3, optional4, optional5, keyword1,
    keyword2, tuple1, tuple2, tuple3, tuple4, pair1, pair2, pair3
)

template <typename T>
struct Node {
    T value;
    Node<T> *next;
};
REFLECT(Node<int>, value, next);

template <typename T>
struct TreeNode {
    T value;
    std::vector<TreeNode<T>*> children;

    TreeNode() {}
    TreeNode(T value) {
        this->value = value;
    }

    void addChild(T value) {
        children.push_back(new TreeNode(value));
    }
};
REFLECT(TreeNode<std::string>, value, children);

template <typename T>
bool equals(const T& lhs, const T& rhs) {
    return lhs == rhs;
}

template <>
bool equals(const EmptyStruct &lhs, const EmptyStruct &rhs) {
    return true;
}

template <>
bool equals(const Keyword &lhs, const Keyword &rhs) {
    return lhs.value == rhs.value;
}

template <>
bool equals(const RealisticStruct &lhs, const RealisticStruct &rhs) {
        return equals(lhs.string, rhs.string)
            && equals(lhs.integer, rhs.integer)
            && equals(lhs.float1, rhs.float1)
            && equals(lhs.float2, rhs.float2)
            && equals(lhs.integers, rhs.integers);
}

template <>
bool equals(
    const MassiveStruct &lhs,
    const MassiveStruct &rhs
) {
    return true;
}

template <>
bool equals(
    const std::vector<const char *> &lhs,
    const std::vector<const char *> &rhs
) {
    if (lhs.size() != rhs.size()) {
        return false;
    }
    for (int i = 0; i < lhs.size(); i++) {
        if (strcmp(lhs[i], rhs[i]) != 0) {
            return false;
        }
    }
    return true;
}

template <typename T>
bool equals(const Node<T> &lhs, const Node<T> &rhs) {
    if (!equals(lhs.value, rhs.value)) {
        return false;
    }
    if (lhs.next == nullptr) {
        if (rhs.next != lhs.next) {
            return false;
        }
        return true;
    }
    return equals(*lhs.next, *rhs.next);
}

template <typename T>
bool equals(const TreeNode<T> &lhs, const TreeNode<T> &rhs) {
    if (!equals(lhs.value, rhs.value)) {
        printf("here\n");
        return false;
    }
    if (lhs.children.size() != rhs.children.size()) {
        printf("here2\n");
        return false;
    }
    for (int i = 0; i < lhs.children.size(); i++) {
        if (!equals(*lhs.children[i], *rhs.children[i])) {
            return false;
        }
    }
    return true;
}

template <typename T>
void test(std::string desc, T item, std::string expected) {
    printf("%-20s", desc.c_str());
    std::string serialized;
    std::string reserialized;
    try {
        serialized = json::serialize(item);
    }
    catch (const json::exception&) {
        printf("SERIALIZE EXCEPTION\n");
        return;
    }
    if (serialized != expected) {
        printf("SERIALIZE FAIL\n");
        printf("    %-15s %s\n", "expected", expected.c_str());
        printf("    %-15s %s\n", "serialized", serialized.c_str());
        return;
    }
    T deserialized;
    try {
        deserialized = json::deserialize<T>(serialized);
    }
    catch (const json::exception&) {
        printf("DESERIALIZE EXCEPTION\n");
        printf("    %-15s %s\n", "expected", expected.c_str());
        printf("    %-15s %s\n", "serialized", serialized.c_str());
        return;
    }
    if constexpr (!std::is_pointer<T>()) {
        if (!equals(deserialized, item)) {
            printf("DESERIALIZE FAIL\n");
            printf("    %-15s %s\n", "json", expected.c_str());
            return;
        }
    }

    try {
        reserialized = json::serialize(deserialized);
    }
    catch (const json::exception&) {
        printf("RESERIALIZE EXCEPTION\n");
        printf("    %-15s %s\n", "expected", expected.c_str());
        printf("    %-15s %s\n", "serialized", serialized.c_str());
        return;
    }
    if (reserialized != expected) {
        printf("RESERIALIZE FAIL\n");
        printf("    %-15s %s\n", "expected", expected.c_str());
        printf("    %-15s %s\n", "reserialized", reserialized.c_str());
        return;
    }

    std::string prettyJson = prettifier.prettify(serialized);
    try {
        deserialized = json::deserialize<T>(prettyJson);
    }
    catch (const json::exception) {
        printf("PRETTY DESERIALIZE EXCEPTION\n");
        printf("    %-15s %s\n", "json", expected.c_str());
        printf("    %-15s %s\n", "pretty", prettyJson.c_str());
        return;
    }
    if constexpr (!std::is_pointer<T>()) {
        if (!equals(deserialized, item)) {
            printf("PRETTY DESERIALIZE FAIL\n");
            printf("    %-15s %s\n", "json", expected.c_str());
            printf("    %-15s %s\n", "pretty", prettyJson.c_str());
            return;
        }
    }

    printf("PASS\n");
}

template <typename T, size_t Y>
void test(std::string desc, T (&item)[Y], std::string expected) {
    printf("%-20s", desc.c_str());
    std::string serialized;
    std::string reserialized;
    try {
        serialized = json::serialize(item);
    }
    catch (const json::exception&) {
        printf("SERIALIZE EXCEPTION\n");
        return;
    }
    if (serialized != expected) {
        printf("SERIALIZE FAIL\n");
        printf("    %-15s %s\n", "expected", expected.c_str());
        printf("    %-15s %s\n", "serialized", serialized.c_str());
        return;
    }
    T deserialized[Y];
    try {
        json::deserialize(deserialized, serialized);
    }
    catch (const json::exception&) {
        printf("DESERIALIZE EXCEPTION\n");
        printf("    %-15s %s\n", "expected", expected.c_str());
        printf("    %-15s %s\n", "serialized", serialized.c_str());
        return;
    }
    if constexpr (!std::is_pointer<T>()) {
        for (int i = 0; i < Y; i++) {
            if (deserialized[i] != item[i]) {
                printf("DESERIALIZE FAIL\n");
                printf("    %-15s %s\n", "expected", expected.c_str());
                printf("    %-15s %s\n", "reserialized", reserialized.c_str());
                return;
            }
        }
    }

    try {
        reserialized = json::serialize(deserialized);
    }
    catch (const json::exception&) {
        printf("RESERIALIZE EXCEPTION\n");
        printf("    %-15s %s\n", "expected", expected.c_str());
        printf("    %-15s %s\n", "serialized", serialized.c_str());
        return;
    }

    if (reserialized != expected) {
        printf("RESERIALIZE FAIL\n");
        printf("    %-15s %s\n", "expected", expected.c_str());
        printf("    %-15s %s\n", "reserialized", reserialized.c_str());
        return;
    }
    printf("PASS\n");
}

bool operator==(
    const std::vector<const char *> &lhs,
    const std::vector<const char *> &rhs
) {
    if (lhs.size() != rhs.size()) {
        printf("Wow\n");
        return false;
    }
    for (int i = 0; i < lhs.size(); i++) {
        if (strcmp(lhs[i], rhs[i]) != 0) {
            printf("Wow\n");
            return false;
        }
    }
    printf("Wow\n");
    return true;
}

void stringTest() {
    char buffer[4] = "foo";

    test<std::string>("std::string", "foo", "\"foo\"");
    test<std::string>("empty string", "", "\"\"");
    std::string stringWithNull("foo\0bar", 7);
    test<std::string>("string with null", stringWithNull, "\"foo\\u0000bar\"");
    test<char*>("char*", buffer, "\"foo\"");
    test("char[]", buffer, "[102,111,111,0]");
    test<const char*>("string escapes", "\"\r\n\b\t\"", "\"\\\"\\r\\n\\b\\t\\\"\"");
    test<std::string>(
        "unicode", "t🖕÷🔫🚬└💣→æ💎ï🗿🤖",
        "\"t\\ud83d\\udd95\\u00f7\\ud83d\\udd2b\\ud83d\\udeac\\u2514\\ud83d\\ud"
        "c"
        "a3\\u2192\\u00e6\\ud83d\\udc8e\\u00ef\\ud83d\\uddff\\ud83e\\udd16\""
    );
    std::string longString;
    for (int i = 0; i < 10000; i++) {
        longString += std::to_string(i) + ',';
    }
    test("long string", longString, '"' + longString + '"');
}

void charTest() {
    test("basic char", 'c', "99");
    test("tab char", '\t', "9");
    test("backspace char", '\b', "8");
    test("null char", '\0', "0");
    test<char>("negative char", -1, "-1");
}

void intTest() {
    test("basic int", 1024, "1024");
    test("negative int", -1024, "-1024");
    test("unsigned int", UINT_MAX, std::to_string(UINT_MAX));
}

void floatTest() {
    test("basic float", 2.5, "2.500000");
}

void arrayTest() {
    int intArray[9] = {1, 2, 3, 4, 5, 6, 7, 8, 9};
    test("int array", intArray, "[1,2,3,4,5,6,7,8,9]");
    char charArray[9] = {1, 2, 3, 4, 5, 6, 7, 8, 9};
    test("char array", charArray, "[1,2,3,4,5,6,7,8,9]");
    std::string stringArray[3] = {"foo", "bar", "baz"};
    test("string array", stringArray, "[\"foo\",\"bar\",\"baz\"]");
}

void vectorTest() {
    test("basic vector", std::vector<int> {
        {1, 2, 3, 4, 5, 6, 7, 8, 9}
    }, "[1,2,3,4,5,6,7,8,9]");
    test("empty vector", std::vector<int> {}, "[]");
    test("2d vector", std::vector<std::vector<int>> {
        {1, 2, 3}, {4, 5, 6}, {7, 8, 9}
    }, "[[1,2,3],[4,5,6],[7,8,9]]");
    test("string vector", std::vector<std::string> {
        "foo", "bar", "baz"}, "[\"foo\",\"bar\",\"baz\"]");
    test("char* vector", std::vector<const char *> {
        "foo", "bar", "baz"}, "[\"foo\",\"bar\",\"baz\"]");
    test("bool vector", std::vector<bool> {
        true, false, true}, "[true,false,true]");
    // test("vector*", new std::vector<bool> {
    //     true, false, true}, "[true,false,true]");
    test("vector*", new std::vector<int> {
        1, 2, 3}, "[1,2,3]");
}

void mapTest() {
    test(
        "basic map",
        std::map<std::string, std::string>{
            {"foo", "bar"},
            {"bar", "baz"},
            {"baz", "foo"}},
        "{\"bar\":\"baz\",\"baz\":\"foo\",\"foo\":\"bar\"}"
    );

    test(
        "int keys map",
        std::map<int, std::string>{
            {1, "bar"},
            {2, "baz"},
            {3, "foo"}},
        "{\"1\":\"bar\",\"2\":\"baz\",\"3\":\"foo\"}"
    );

    test(
        "bool map",
        std::map<std::string, bool>{
            {"bar", true},
            {"baz", false},
            {"foo", true}},
        "{\"bar\":true,\"baz\":false,\"foo\":true}"
    );

    test(
        "float map",
        std::map<float, double>{{1.10, 11.0}, {2.20, 22.0}, {3.30, 33.0}},
        "{\"1.100000\":11.000000,\"2.200000\":22.000000,\"3.300000\":33.000000}"
    );

    test(
        "extra bool map",
        std::map<bool, bool>{
            {false, false},
            {true, true}},
        "{\"false\":false,\"true\":true}"
    );

    test(
        "unordered map",
        std::unordered_map<std::string, std::string>{{"foo", "bar"}},
        "{\"foo\":\"bar\"}"
    );

    test(
        "2d map",
        std::map<std::string, std::map<int, std::string>>{
            {
                "foo",
                {{1, "bar"}, {2, "baz"}, {3, "foo"}},
            },
            {
                "bar",
                {{1, "bar"}, {2, "baz"}, {3, "foo"}},
            },
            {
                "baz",
                {{1, "bar"}, {2, "baz"}, {3, "foo"}},
            }},
        "{\"bar\":{\"1\":\"bar\",\"2\":\"baz\",\"3\":\"foo\"},\"baz\":{\"1\":"
        "\"bar\",\"2\":\"baz\",\"3\":\"foo\"},\"foo\":{\"1\":\"bar\",\"2\":"
        "\"baz\",\"3\":\"foo\"}}"
    );
}

void structTest() {
    EmptyStruct emptyStruct;
    test("empty struct", emptyStruct, "{}");

    RealisticStruct realisticStruct {
        "foo",
        100000,
        1.2345,
        1234.5,
        { 1, 2, 3, 4, 5, 6, 7, 8, 9 }
    };
    test(
        "realistic struct", realisticStruct,
        "{\"string\":\"foo\",\"integer\":100000,\"float1\":1.234500,"
        "\"float2\":1234.500000,\"integers\":[1,2,3,4,5,6,7,8,9]}"
    );
    
    Keyword keyword { "test" };
    test("custom funcs", keyword, "test");

    MassiveStruct massiveStruct {
        "foo", "bar", "baz", 1, 2, 3, 0.5, 1.5, 2.5, 10000, 20000, 30000, {},
        {
            {"foo", 100, 1.345, -134.5, {1, 6, 7, 8, 9}},
            {"foo", -200, 1.245, 124.5, {1, 2, 3, 8, 9}},
            {"foo", 300, -1.235, 123.5, {1, 2, 3, 8, 9}},
            {"foo", 400, 1.234, -234.5, {5, 6, 7, 8, 9}},
        },
        {
            {"foo", 1, 3.5345, 2.5, {1, 2, 3}},
            {"bar", 2, 2.5345, 4.5, {4, 5, 6}},
            {"baz", 3, 1.5345, 8.5, {4, 5, 6}},
        },
        {
            {"foo", {"foo", 1, 3.5345, 2.5, {1, 2, 3}}},
            {"bar", {"bar", 2, 2.5345, 4.5, {4, 5, 6}}},
            {"baz", {"baz", 3, 1.5345, 8.5, {4, 5, 6}}},
        },
        std::optional<bool>(),
        100,
        std::optional<float>(),
        std::optional<RealisticStruct>(),
        realisticStruct,
        { "test" },
        { "anotherTest" },
        {1, -2, 3.5, "foo"},
        {},
        {realisticStruct, 1.6},
        {"foo", "bar"},
        {-100, 1.5},
        {"foo bar baz", "baz bar foo"},
        {"foo bar baz", {{"baz bar foo", "foo bar baz"}}},
    };
    test(
        "massive struct", massiveStruct,
        "{\"string1\":\"foo\",\"string2\":\"bar\",\"string3\":\"baz\",\"int1\":"
        "1,\"int2\":2,\"int3\":3,\"float1\":0.500000,\"float2\":1.500000,"
        "\"float3\":2.500000,\"double1\":10000.000000,\"double2\":20000.000000,"
        "\"double3\":30000.000000,\"emptyStruct\":{},\"structs1\":[{\"string\":"
        "\"foo\",\"integer\":100,\"float1\":1.345000,\"float2\":-134.500000,"
        "\"integers\":[1,6,7,8,9]},{\"string\":\"foo\",\"integer\":-200,"
        "\"float1\":1.245000,\"float2\":124.500000,\"integers\":[1,2,3,8,9]},{"
        "\"string\":\"foo\",\"integer\":300,\"float1\":-1.235000,\"float2\":"
        "123.500000,\"integers\":[1,2,3,8,9]},{\"string\":\"foo\",\"integer\":"
        "400,\"float1\":1.234000,\"float2\":-234.500000,\"integers\":[5,6,7,8,"
        "9]}],\"structs2\":[{\"string\":\"foo\",\"integer\":1,\"float1\":3."
        "534500,\"float2\":2.500000,\"integers\":[1,2,3]},{\"string\":\"bar\","
        "\"integer\":2,\"float1\":2.534500,\"float2\":4.500000,\"integers\":[4,"
        "5,6]},{\"string\":\"baz\",\"integer\":3,\"float1\":1.534500,"
        "\"float2\":8.500000,\"integers\":[4,5,6]}],\"structs3\":{\"bar\":{"
        "\"string\":\"bar\",\"integer\":2,\"float1\":2.534500,\"float2\":4."
        "500000,\"integers\":[4,5,6]},\"baz\":{\"string\":\"baz\",\"integer\":"
        "3,\"float1\":1.534500,\"float2\":8.500000,\"integers\":[4,5,6]},"
        "\"foo\":{\"string\":\"foo\",\"integer\":1,\"float1\":3.534500,"
        "\"float2\":2.500000,\"integers\":[1,2,3]}},\"optional1\":null,"
        "\"optional2\":100,\"optional3\":null,\"optional4\":null,\"optional5\":"
        "{\"string\":\"foo\",\"integer\":100000,\"float1\":1.234500,\"float2\":"
        "1234.500000,\"integers\":[1,2,3,4,5,6,7,8,9]},\"keyword1\":test,"
        "\"keyword2\":anotherTest,\"tuple1\":[1,-2,3.500000,\"foo\"],"
        "\"tuple2\":[],\"tuple3\":[{\"string\":\"foo\",\"integer\":100000,"
        "\"float1\":1.234500,\"float2\":1234.500000,\"integers\":[1,2,3,4,5,6,"
        "7,8,9]},1.600000],\"tuple4\":[\"foo\",\"bar\"],\"pair1\":[-100,1."
        "500000],\"pair2\":[\"foo bar baz\",\"baz bar foo\"],\"pair3\":[\"foo "
        "bar baz\",{\"baz bar foo\":\"foo bar baz\"}]}"
    );
};

void linkedListTest() {
    Node<int> root;
    root.value = 10;
    root.next = new Node<int>();
    root.next->value = 20;
    root.next->next = new Node<int>();
    root.next->next->value = 30;
    root.next->next->next = new Node<int>();
    root.next->next->next->value = 40;
    root.next->next->next->next = new Node<int>();
    root.next->next->next->next->value = 40;
    test(
        "linked list", root,
        "{\"value\":10,\"next\":{\"value\":20,\"next\":{\"value\":30,\"next\":{"
        "\"value\":40,\"next\":{\"value\":40,\"next\":null}}}}}"
    );
}

void treeTest() {
    TreeNode<std::string> root("root");
    root.addChild("foo");
    root.addChild("bar");
    root.addChild("baz");

    root.children[0]->addChild("foo foo");
    root.children[0]->addChild("foo bar");
    root.children[0]->addChild("foo baz");

    root.children[1]->addChild("bar foo");
    root.children[1]->addChild("bar bar");
    root.children[1]->addChild("bar baz");

    root.children[2]->addChild("baz foo");
    root.children[2]->addChild("baz bar");
    root.children[2]->addChild("baz baz");

    root.children[0]->children[0]->addChild("foo foo foo");
    root.children[0]->children[0]->addChild("foo foo bar");
    root.children[0]->children[0]->addChild("foo foo baz");

    test(
        "tree", root,
        "{\"value\":\"root\",\"children\":[{\"value\":\"foo\",\"children\":[{"
        "\"value\":\"foo foo\",\"children\":[{\"value\":\"foo foo "
        "foo\",\"children\":[]},{\"value\":\"foo foo "
        "bar\",\"children\":[]},{\"value\":\"foo foo "
        "baz\",\"children\":[]}]},{\"value\":\"foo "
        "bar\",\"children\":[]},{\"value\":\"foo "
        "baz\",\"children\":[]}]},{\"value\":\"bar\",\"children\":[{\"value\":"
        "\"bar foo\",\"children\":[]},{\"value\":\"bar "
        "bar\",\"children\":[]},{\"value\":\"bar "
        "baz\",\"children\":[]}]},{\"value\":\"baz\",\"children\":[{\"value\":"
        "\"baz foo\",\"children\":[]},{\"value\":\"baz "
        "bar\",\"children\":[]},{\"value\":\"baz baz\",\"children\":[]}]}]}"
    );
}

void commentTest() {
    std::string string;
    int integer;
    float float1;
    float float2;
    std::vector<int> integers;
    printf("%-20s", "comment/whitespace");

    RealisticStruct realisticStruct;
    std::string
        serialized = "{\"string\"  /* comment\n/*/://\n\"foo bar\"// "
                     "comment\n,/*comment */ \"integer\"  /*comment  "
                     "*/: //comment\n  "
                     "/*comment*/42/**/,\"float1\"//\n:42.0/***comment    "
                     "\n\n****/,// \n \"float2\"/**//**//**/:\n  \n "
                     "4.2\n\n//comment}\n//comment[\n,\n\"integers\"\n//"
                     "\n:\n\n//comment\n/*comment*/[\n/*comment*/1\n,2 \n\n , "
                     "/**/ 3 /**/,//comment\n4\n/*comment]*/]} \n//comment";
    try {
        realisticStruct = json::deserialize<RealisticStruct>(serialized);
    }
    catch (const json::exception &ex) {
        printf("FAIL\n");
        return;
    }
    if (!equals(realisticStruct, {"foo bar", 42, 42.0, 4.2, {1, 2, 3, 4}})) {
        printf("FAIL\n");
        return;
    }
    printf("PASS\n");
}

int main() {
    if constexpr (false) {
        RealisticStruct realisticStruct;
        realisticStruct.string = "foo bar baz";
        realisticStruct.integer = 1;
        realisticStruct.float1 = 1.5;
        realisticStruct.float2 = 3.0;
        realisticStruct.integers = {10, 20, 30, 40, 50};

        std::string json = json::serialize(realisticStruct);
        printf("%s\n", json.c_str());
        printf("%s\n", prettifier.prettify(json).c_str());
        return 0;

        int total = 0;
        for (int i = 0; i < 5'000'000; i++) {
            std::string json = json::serialize(realisticStruct);
            RealisticStruct deserialized = json::deserialize<RealisticStruct>(json);
            total += deserialized.integer;
        }
        printf("%d\n", total);
    }
    else {
        stringTest();
        charTest();
        intTest();
        floatTest();
        vectorTest();
        arrayTest();
        mapTest();
        structTest();
        linkedListTest();
        treeTest();
        commentTest();
    }
}
