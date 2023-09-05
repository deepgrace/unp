#!/bin/bash

dst=/tmp
path=example

flags=(-std=c++23 -Wall -O3 -Os -s -I . -I ../include)

cd ${path}

for file in *.cpp; do
    bin=${dst}/${file/.cpp/}

    g++ "${flags[@]}" -o ${bin} ${file}
    strip ${bin}
done

echo Please check the executables at ${dst}
