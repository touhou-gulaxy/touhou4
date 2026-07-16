#!/bin/bash

# g++ -Iinclude -lstdc++ -std=c++20 -fvisibility=hidden -g -o ./process_recipe ./process.cpp
clang-18 -Iinclude -stdlib=libc++ -std=c++20 -fvisibility=hidden -g -o ./process_recipe ./process.cpp -lc++ -lc++abi
chmod +x ./process_recipe
./process_recipe
rm ./process_recipe
