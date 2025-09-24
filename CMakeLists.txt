# Copyright Microsoft Corporation.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

cmake_minimum_required(VERSION 3.28)

# Corresponds to https://github.com/microsoft/ifc-spec
set(ifc_spec_version 0.43)

project(
    ifc-sdk
    VERSION "${ifc_spec_version}.5"
    LANGUAGES CXX
)

option(DEVELOPER_MODE "Enable project options and targets for developers of ifc-sdk" OFF)
mark_as_advanced(DEVELOPER_MODE)
include(CMakeDependentOption)

find_package(Microsoft.GSL REQUIRED)

# Those pesky min/max macros.
if(WIN32)
  add_compile_definitions(NOMINMAX)
endif()

# By default we don't want the Visual Studio default "sources" and "headers" folders, the following put's everything directly in the project
if (${CMAKE_GENERATOR} MATCHES "^Visual Studio")
    source_group( " "  REGULAR_EXPRESSION .*)
endif()


# The `reader` component.
add_library(
    ifc-reader STATIC
    src/file.cxx
    src/sgraph.cxx
    src/input-file.cxx
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
target_compile_features(ifc-reader PUBLIC cxx_std_23)
target_include_directories(ifc-reader PUBLIC "\$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>")

# The `dom` component.
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
    src/ifc-dom/common.hxx
)
add_library(Microsoft.IFC::DOM ALIAS ifc-dom)
set_property(TARGET ifc-dom PROPERTY EXPORT_NAME DOM)
target_link_libraries(ifc-dom PUBLIC ifc-reader)
target_compile_features(ifc-dom PUBLIC cxx_std_23)
target_include_directories(ifc-dom PUBLIC "\$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>")

# The `ifc` tool executable.
add_executable(
  ifc
  src/tools/ifc.cxx
)
add_executable(Microsoft.IFC::Tool ALIAS ifc)
set_property(TARGET ifc PROPERTY EXPORT_NAME Tool)
target_compile_features(ifc PUBLIC cxx_std_23)
target_link_libraries(ifc ifc-reader)
target_include_directories(ifc PUBLIC "\$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>")

# The IFC SDK comprises the `reader`, the `dom`, and the tool.
add_library(SDK INTERFACE)
add_library(Microsoft.IFC::SDK ALIAS SDK)
target_link_libraries(SDK INTERFACE ifc-reader ifc-dom)

cmake_dependent_option(BUILD_PRINTER "Build the ifc-printer executable" OFF "NOT DEVELOPER_MODE" ON)
if(BUILD_PRINTER)
  add_executable(
      ifc-printer
      src/ifc-printer/main.cxx
      src/ifc-printer/printer.cxx
      src/ifc-printer/printer.hxx
      src/assert.cxx
  )
  target_link_libraries(ifc-printer PRIVATE ifc-dom ifc-reader)
  target_compile_features(ifc-printer PRIVATE cxx_std_23)
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

# Add include headers
add_custom_target(ifc-include SOURCES
    include/ifc/abstract-sgraph.hxx
    include/ifc/assertions.hxx
    include/ifc/basic-types.hxx
    include/ifc/file.hxx
    include/ifc/index-utils.hxx
    include/ifc/operators.hxx
    include/ifc/pathname.hxx
    include/ifc/pp-forms.hxx
    include/ifc/reader.hxx
    include/ifc/source-word.hxx
    include/ifc/tooling.hxx
    include/ifc/underlying.hxx
    include/ifc/util.hxx
    include/ifc/version.hxx
    include/ifc/dom/node.hxx
)

set( SolutionItems
    .clang-format
    .gitignore
    CMakePresets.json
    LICENSE.txt
    README.md
    vcpkg.json)
set_property(DIRECTORY APPEND PROPERTY VS_SOLUTION_ITEMS ${SolutionItems})
source_group("Solution Items" FILES ${SolutionItems})
