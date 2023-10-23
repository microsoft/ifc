# Copyright Microsoft Corporation.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

cmake_minimum_required(VERSION 3.27)

project(ifc-sdk-tests CXX)

if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
  find_package(Microsoft.IFC REQUIRED)
endif()

add_executable(ifc-test main.cxx)
target_link_libraries(ifc-test PRIVATE Microsoft.IFC::SDK)

enable_testing()

add_test(NAME ifc-test COMMAND ifc-test)
