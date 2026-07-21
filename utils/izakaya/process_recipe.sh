#!/bin/bash

# g++ -Iinclude -lstdc++ -std=c++20 -fvisibility=hidden -g -o ./process_recipe ./process.cpp
clang-18 -Iinclude -stdlib=libc++ -std=c++20 -fvisibility=hidden -O3 -Wall -o ./process_recipe ./process.cpp -lc++ -lc++abi
chmod +x ./process_recipe
./process_recipe
# valgrind --leak-check=full --show-leak-kinds=all ./process_recipe
rm ./process_recipe
cp ./izakaya_machine_trans.map ./izakaya_machine_trans.map.md
pandoc ./izakaya_machine_trans.map.md --pdf-engine=xelatex -V mainfontoptions="Path=../../gfx/fonts/" -V mainfont="spth_ui_fonts.ttf" -o ../../../izakaya_machine_trans.map.pdf # -V CJKmainfont="FZCuYuan-M03"
rm ./izakaya_machine_trans.map.md
