# Copyright Microsoft Corporation.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

include(CMakeFindDependencyMacro)
find_dependency(Microsoft.GSL)

include("${CMAKE_CURRENT_LIST_DIR}/Microsoft.IFCTargets.cmake")
