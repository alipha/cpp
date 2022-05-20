#include "system.h"

int main() {
    for (auto &i : SystemFactory::registered()) {
        i.second(); // this should print but doesn't because static initialization fiasco
    }
}

