# Copyright Microsoft Corporation.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

# This module is an opt-in find module _only_ for developers of the SDK who
# wish not to use vcpkg. To tell CMake about this find module, pass this file's
# directory via the CMAKE_MODULE_PATH command line option. This will satisfy a
# simple find_package(doctest REQUIRED) call by default.

include(FetchContent)

FetchContent_Declare(
    doctest
    GIT_REPOSITORY https://github.com/doctest/doctest.git
    GIT_TAG v2.4.11
    GIT_SHALLOW YES
)

FetchContent_MakeAvailable(doctest)

set(doctest_FOUND 1)
