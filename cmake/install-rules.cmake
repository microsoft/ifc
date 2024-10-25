# Copyright Microsoft Corporation.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

include(CMakePackageConfigHelpers)
include(GNUInstallDirs)

if(BUILD_PRINTER)
    install(
        TARGETS ifc-printer
        DESTINATION "${CMAKE_INSTALL_BINDIR}"
        COMPONENT Printer
    )
endif()

# find_package(<package>) call for consumers to find this project
set(package Microsoft.IFC)

# everything else is dev only
set(CMAKE_INSTALL_DEFAULT_COMPONENT_NAME Development)

install(
    DIRECTORY include/
    DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
)

install(
    TARGETS ifc
    RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
)

install(
    TARGETS ifc-dom ifc-reader SDK
    EXPORT Microsoft.IFCTargets
    INCLUDES DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
)

write_basic_package_version_file(
    "${package}ConfigVersion.cmake"
    COMPATIBILITY SameMajorVersion
)

# Allow package maintainers to freely override the path for the configs
set(
    IFC_INSTALL_CMAKEDIR "${CMAKE_INSTALL_LIBDIR}/cmake/${package}"
    CACHE PATH "CMake package config location relative to the install prefix"
)
mark_as_advanced(IFC_INSTALL_CMAKEDIR)

install(
    FILES cmake/install-config.cmake
    DESTINATION "${IFC_INSTALL_CMAKEDIR}"
    RENAME "${package}Config.cmake"
)

install(
    FILES "${PROJECT_BINARY_DIR}/${package}ConfigVersion.cmake"
    DESTINATION "${IFC_INSTALL_CMAKEDIR}"
)

install(
    EXPORT Microsoft.IFCTargets
    NAMESPACE Microsoft.IFC::
    DESTINATION "${IFC_INSTALL_CMAKEDIR}"
)

include(CPack)
