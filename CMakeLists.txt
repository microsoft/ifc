# Copyright Microsoft Corporation.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

cmake_minimum_required(VERSION 3.14)

# Corresponds to https://github.com/microsoft/ifc-spec
set(ifc_spec_version 0.43)

project(
    ifc-sdk
    VERSION "${ifc_spec_version}.1"
    LANGUAGES CXX
)

option(DEVELOPER_MODE "Enable project options and targets for developers of ifc-sdk" OFF)
mark_as_advanced(DEVELOPER_MODE)
include(CMakeDependentOption)

find_package(Microsoft.GSL REQUIRED)

add_library(
    ifc-reader STATIC
    src/file.cxx
    src/sgraph.cxx
    src/ifc-reader/operators.cxx
    src/ifc-reader/reader.cxx
    src/ifc-reader/util.cxx
)
add_library(Microsoft.IFC::Core ALIAS ifc-reader)
set_property(TARGET ifc-reader PROPERTY EXPORT_NAME Core)
if(WIN32)
  target_compile_definitions(ifc-reader PRIVATE NOMINMAX)
  target_link_libraries(ifc-reader PRIVATE bcrypt)
  target_sources(ifc-reader PRIVATE src/hash_win.cxx)
else()
  target_sources(ifc-reader PRIVATE src/sha256.cxx)
endif()
target_link_libraries(ifc-reader PUBLIC Microsoft.GSL::GSL)
target_compile_features(ifc-reader PUBLIC cxx_std_20)
target_include_directories(ifc-reader PUBLIC "\$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>")

add_library(
    ifc-dom STATIC
    src/ifc-dom/charts.cxx
    src/ifc-dom/decls.cxx
    src/ifc-dom/exprs.cxx
    src/ifc-dom/literals.cxx
    src/ifc-dom/names.cxx
    src/ifc-dom/sentences.cxx
    src/ifc-dom/stmts.cxx
    src/ifc-dom/syntax.cxx
    src/ifc-dom/types.cxx
)
add_library(Microsoft.IFC::DOM ALIAS ifc-dom)
set_property(TARGET ifc-dom PROPERTY EXPORT_NAME DOM)
target_link_libraries(ifc-dom PUBLIC ifc-reader)
target_compile_features(ifc-dom PUBLIC cxx_std_20)
target_include_directories(ifc-reader PUBLIC "\$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>")

add_library(SDK INTERFACE)
add_library(Microsoft.IFC::SDK ALIAS SDK)
target_link_libraries(SDK INTERFACE ifc-reader ifc-dom)

cmake_dependent_option(BUILD_PRINTER "Build the ifc-printer executable" OFF "NOT DEVELOPER_MODE" ON)
if(BUILD_PRINTER)
  add_executable(
      ifc-printer
      src/ifc-printer/main.cxx
      src/ifc-printer/printer.cxx
      src/assert.cxx
  )
  target_link_libraries(ifc-printer PRIVATE ifc-dom ifc-reader)
  target_compile_features(ifc-printer PRIVATE cxx_std_20)
endif()

if(NOT CMAKE_SKIP_INSTALL_RULES)
  include(cmake/install-rules.cmake)
endif()

if(NOT DEVELOPER_MODE)
  return()
endif()

include(CTest)
if(BUILD_TESTING)
  add_subdirectory(test)
endif()
