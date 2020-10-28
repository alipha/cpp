#include "tokenize.hpp"
#include <iostream>


void test(const std::string &input, const std::string &delim, bool allowEmpty = true) {
    std::cout << "Input: " << input << '\n';
    std::cout << "Delim: " << delim << '\n';
    
    auto tokens = tokenize(input, delim, allowEmpty);
    
    for(auto &token : tokens) {
        std::cout << '"' << token << "\"\n";
    }
    
    std::cout << std::endl;
}


int main() {
    test("apple", "");
    test("apple", "", false);
    test("", "");
    test("", "", false);
    
    test("apple,banana,,carrot", ",");
    test(",apple,banana,,carrot,", ",");
    test("apple,banana,,carrot", ",", false);
    test(",apple,banana,,carrot,", ",", false);
    
    test("appleFOObananaFOOFOOcarrot", "FOO");
    test("FOOappleFOObananaFOOFOOcarrotFOO", "FOO");
    test("appleFOObananaFOOFOOcarrot", "FOO", false);
    test("FOOappleFOObananaFOOFOOcarrotFOO", "FOO", false);
}

