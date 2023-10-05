# Copyright Microsoft Corporation.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

# This module is an opt-in find module _only_ for developers of the SDK who
# wish not to use vcpkg. To tell CMake about this find module, pass this file's
# directory via the CMAKE_MODULE_PATH command line option. This will satisfy a
# simple find_package(Microsoft.GSL REQUIRED) call by default.
# Do not use for release builds, because the GSL targets will also be added to
# the install rules of the SDK.

include(FetchContent)

FetchContent_Declare(
    Microsoft.GSL
    GIT_REPOSITORY https://github.com/microsoft/GSL.git
    GIT_TAG v4.0.0
    GIT_SHALLOW YES
)

# required to satisfy install(EXPORT) in the SDK
set(GSL_INSTALL 1)
if(CMAKE_SKIP_INSTALL_RULES)
  set(GSL_INSTALL 0)
endif()

FetchContent_MakeAvailable(Microsoft.GSL)

set(Microsoft.GSL_FOUND 1)
