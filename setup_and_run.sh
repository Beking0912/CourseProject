#!/bin/bash

# Step 1: Create and navigate to the build directory
mkdir build
cd build

# Step 2: Run CMake and make with Debug flags
cmake -DCMAKE_BUILD_TYPE=Debug ..
make

# Step 3: Navigate back to the parent directory
cd ..

# Step 4: Compile rtlib.c with debug information and no optimization
cc -g -O0 -c rtlib.c

# Step 5: Compile fileX.c with the clang plugin, including debug information and no optimization
clang -fno-discard-value-names -fpass-plugin='build/skeleton/SkeletonPass.dylib' -c testTwo.c -g -O0

# Step 6: Link the object files and create the executable with debug information
cc -g testOne.o rtlib.o -o a.out

# Step 7: Run the executable
./a.out
