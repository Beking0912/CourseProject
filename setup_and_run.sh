#!/bin/bash

# Step 1: Create and navigate to the build directory
mkdir build
cd build

# Step 2: Run CMake and make
cmake -DCMAKE_BUILD_TYPE=Debug ..
make

# Step 3: Navigate back to the parent directory
cd ..

# Step 4: Compile logPrint.c
cc -g -c logPrint.c

# Step 5: Compile fileX.c with the clang plugin
clang -fno-discard-value-names -fpass-plugin='build/skeleton/SkeletonPass.dylib' -c testOne.c -g

# Step 6: Link the object files and create the executable
cc -g testOne.o logPrint.o -o a.out

# Step 7: Run the executable
./a.out
