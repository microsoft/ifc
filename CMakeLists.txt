cmake_minimum_required (VERSION 3.14)
project (ifc-sdk)

# Fetch Microsoft GSL
include(FetchContent)

FetchContent_Declare(GSL
    GIT_REPOSITORY "https://github.com/microsoft/GSL"
    GIT_TAG "v3.1.0"
)
FetchContent_MakeAvailable(GSL)

if (MSVC)
add_compile_options(
    /permissive-   # Turn on strict language conformance
    /EHsc          # Turn on exception handling semantics
)
endif()

# Add common properties
set(CMAKE_CXX_STANDARD 20)

add_definitions(-DVERSP_RELEASE=$<CONFIG:Release> -DVERSP_DEBUG=$<CONFIG:Debug> -DNOMINMAX)

include_directories (include)

add_subdirectory (src)
