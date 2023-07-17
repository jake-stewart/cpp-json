//
// compile me with `g++ -std=c++20 example.cpp -I .`
//

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
