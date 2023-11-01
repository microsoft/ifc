# Copyright Microsoft Corporation.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

cmake_minimum_required(VERSION 3.26)

project(ifc-sdk-tests CXX)

if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
  find_package(Microsoft.IFC REQUIRED)
endif()

find_package(doctest CONFIG REQUIRED)

add_executable(ifc-test main.cxx)

target_compile_features(ifc-test PRIVATE cxx_std_23)

# Libs for ifc-test
target_link_libraries(ifc-test PRIVATE Microsoft.IFC::SDK)

add_executable(ifc-basic basic.cxx)

if (WIN32)
  # MSVC-specific testing
  # Test IFCs
  add_custom_target(test-ifcs
    COMMAND
      ${CMAKE_CXX_COMPILER}
      /nologo
      /std:c++20
      /ifcOutput${CMAKE_BINARY_DIR}/m.ifc
      /c
      ${CMAKE_CURRENT_SOURCE_DIR}/m.ixx)

  add_dependencies(ifc-basic test-ifcs)

  target_compile_features(ifc-basic PRIVATE cxx_std_23)

  target_compile_definitions(ifc-basic PRIVATE IFC_FILE="${CMAKE_BINARY_DIR}/m.ifc")

  # Libs for ifc-basic
  target_link_libraries(ifc-basic PRIVATE Microsoft.IFC::SDK)
  target_link_libraries(ifc-basic PRIVATE doctest::doctest)
endif()

enable_testing()

add_test(NAME ifc-test COMMAND ifc-test)

if (WIN32)
  add_test(NAME ifc-basic COMMAND ifc-basic)
endif()
