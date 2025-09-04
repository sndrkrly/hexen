#!/bin/bash

if [ -d "build" ]; then
    echo "Deleting build directory"
    rm -rf build
fi

mkdir build
cd build

cmake ..
cmake --build .

if [ -f "bin/reif" ]; then
    ./bin/reif
else
    echo "Something went wrong, fatal error"
fi

read -p "Press enter to continue..."
