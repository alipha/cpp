#!/bin/bash
g++-9.1 -std=c++17 -W -Wall -pedantic -o example main.cpp shapes/shape_registry.cpp properties/property_registry.cpp functions/*
