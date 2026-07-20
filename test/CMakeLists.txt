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

if (WIN32)
  # Only enabled for MSVC for now.
  add_executable(ifc-basic basic.cxx)
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

# End-to-end test for the `ifc archive` / `ifc extract` subcommands.  The member IFCs are
# produced here with MSVC (GCC and Clang cannot emit IFCs), so -- like ifc-basic -- this test is
# Windows-only.  The members are built now; at test time only the ifc tool and cmake run, so no
# MSVC environment is needed then.
if (WIN32 AND TARGET ifc)
  set(archive_work "${CMAKE_CURRENT_BINARY_DIR}/archive-test")
  file(MAKE_DIRECTORY "${archive_work}")
  add_custom_command(
    OUTPUT "${archive_work}/m.ifc"
    COMMAND ${CMAKE_CXX_COMPILER} /nologo /std:c++20 /ifcOutputm.ifc /c "${CMAKE_CURRENT_SOURCE_DIR}/m.ixx"
    DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/m.ixx"
    WORKING_DIRECTORY "${archive_work}")
  add_custom_command(
    OUTPUT "${archive_work}/n.ifc"
    COMMAND ${CMAKE_CXX_COMPILER} /nologo /std:c++20 /ifcOutputn.ifc /c "${CMAKE_CURRENT_SOURCE_DIR}/n.ixx"
    DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/n.ixx"
    WORKING_DIRECTORY "${archive_work}")
  add_custom_target(archive-test-ifcs ALL DEPENDS "${archive_work}/m.ifc" "${archive_work}/n.ifc")

  add_test(
    NAME ifc-archive
    COMMAND ${CMAKE_COMMAND}
      "-DIFC_TOOL=$<TARGET_FILE:ifc>"
      "-DWORK=${archive_work}"
      -P "${CMAKE_CURRENT_SOURCE_DIR}/archive-roundtrip.cmake")
endif()
