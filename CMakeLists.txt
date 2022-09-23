cmake_minimum_required (VERSION 3.14)
project (ifc-sdk)

# Fetch Microsoft GSL
include(FetchContent)

FetchContent_Declare(GSL
    GIT_REPOSITORY "https://github.com/microsoft/GSL"
    GIT_TAG "v3.1.0"
)
FetchContent_MakeAvailable(GSL)
#


if (MSVC)
add_compile_options(
    /permissive-   # Turn on strict language conformance
    /EHsc          # Turn on exception handling semantics
)
endif()

#find_package (GSL CONFIG REQUIRED)
#target_link_libraries(main PRIVATE Microsoft.GSL::GSL)
#link_libraries (${CURL_LIBRARIES})

# Add common properties
set(CMAKE_CXX_STANDARD 20)

add_definitions(-DVERSP_RELEASE=$<CONFIG:Release> -DVERSP_DEBUG=$<CONFIG:Debug> -DNOMINMAX)

include_directories (include)

add_subdirectory (src)
#add_subdirectory (samples)
