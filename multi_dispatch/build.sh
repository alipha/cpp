#!/bin/bash
g++-9.1 -std=c++17 -W -Wall -pedantic -c functions/contains.cpp functions/display.cpp functions/compute_property.cpp
g++-9.1 -std=c++17 -W -Wall -pedantic -o example *.o main.cpp shapes/shape_registry.cpp properties/property_registry.cpp
