# cpp-json
reflection in c++ for compile-time json de/serialization implemented as a single header library.
requires **c++20**.

```cpp
std::string serializedJson = json::serialize(myStruct);
MyStruct deserialized = json::deserialize<MyStruct>(serializedJson);
```

### public domain
do whatever you want with this header file. use it, modify it, sell it. you do not need to credit me in any way.

### supported datatypes:
- strings: `std::string`, `const char *` and `char *`
- arrays: `std::vector`, and `T[]`
- maps: `std::map` and `std::unordered_map`
- pointers: `T*`, `std::unique_ptr`
- all arithmetic types (`int`, `unsigned long long`, `double`, `char`, etc)
- various STL types: `std::tuple`, `std::pair`, `std::optional`, `std::queue`, `std::deque`, `std::list`, `std::set`, `std::unordered_set`
- enums
- classes and structs via `REFLECT`
- can be extended via template specializations.

#### freeing pointers
if you are smart and use `std::vector`, `std::unique_ptr`, and `std::string`, then you don't need to worry about memory management. you can skip this section.

pointers are not treated as arrays. they are expected to reference a single value.
when populating a pointer with `json::deserialize()`, the data is created using `new` and so should be freed with `delete`.
`const char *` and `char *` are treated differently. they are expected to point to strings. they are created using `new []` and so should be deleted with `delete []`.

#### exceptions
if any problems occur during serialization or deserialization, a `json::exception` is thrown.
the `json::exception` is a struct containing a short description in `std::string desc` and the json index where it was located in `int idx`.

# example usage with serializing/deserializing structs
```c++
#include <iostream>
#include "json.hpp"

struct Person {
    std::string name;
    int age;
    std::vector<std::string> friends;
};

// use the REFLECT() macro to make the json serializer aware of the Person type
REFLECT(Person, name, age, friends);

int main() {
    Person person = {"joe", 20, {"ben", "sam"}};

    // create a json string representing the person    
    std::string personJson = json::serialize(person);

    // prettify and print the json string to stdout
    json::Prettifier prettifier;
    std::cout << prettifier.prettify(personJson) << std::endl;

    // construct a new person from the person json
    Person person2 = json::deserialize<Person>(personJson);

    std::cout << person2.name << std::endl;

    return 0;
}

```

# design decisions
- the `json::Prettifier` class is used for prettifying the json, instead of having the prettifying capability built into the `json::serialize()` function. this is to keep the `json::serialize()` signature simple. You can create your own `json::serialize()` function specializations from outside of the header file. the signature is simply `std::string json::serialize(const T& item);` (very beautiful).
- enums are treated as integers. this seems to be common practice and i do not want to force the user to uglify their enum declarations just so reflection works.
- only public fields can be serialized. this is also common practice yet you can normally force them to be serialized. i do not see the point of allowing private fields to be serialized since it breaks the idea of encapsulation.
