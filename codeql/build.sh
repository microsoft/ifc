#!/bin/sh

# Build file for CodeQL

cd ..

cmake --preset=ci-ubuntu \
-D CMAKE_BUILD_TYPE=Debug \
-D CMAKE_TOOLCHAIN_FILE= \
-D "CMAKE_MODULE_PATH:PATH=$PWD/cmake/find" \
|| exit $?

cmake --build build || exit $?
