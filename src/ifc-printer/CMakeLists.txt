# Copyright Microsoft Corporation.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

add_executable(ifc-printer
  main.cxx
  printer.cxx
  ../assert.cxx
)

target_link_libraries(ifc-printer
  PUBLIC
    ifc-dom
    ifc-reader
)

target_link_libraries(ifc-printer PRIVATE GSL)

add_custom_command(
        TARGET ifc-printer POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
                ${CMAKE_CURRENT_SOURCE_DIR}/file.cxx.ifc
                ${CMAKE_CURRENT_BINARY_DIR}/file.cxx.ifc)