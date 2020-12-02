#include "cleanup.hpp"

#include <iostream>
#include <cstdio>


void example() {
    FILE *fp = std::fopen("foo.txt", "r");
    cleanup close_fp = [fp]() { std::fclose(fp); };
    
    char line[1000];
    while(fgets(line, sizeof line, fp)) {
        std::cout << line << std::endl;
    }
}


int main() {
    std::cout << "hi" << std::endl;
    {
        cleanup c = []() { std::cout << "Called!" << std::endl; };
        std::cout << "hello" << std::endl;
    }
    std::cout << "bye" << std::endl;
}
