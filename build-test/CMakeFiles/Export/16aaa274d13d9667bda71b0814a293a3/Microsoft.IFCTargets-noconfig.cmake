#----------------------------------------------------------------
# Generated CMake target import file.
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "Microsoft.IFC::DOM" for configuration ""
set_property(TARGET Microsoft.IFC::DOM APPEND PROPERTY IMPORTED_CONFIGURATIONS NOCONFIG)
set_target_properties(Microsoft.IFC::DOM PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_NOCONFIG "CXX"
  IMPORTED_LOCATION_NOCONFIG "${_IMPORT_PREFIX}/lib/libifc-dom.a"
  )

list(APPEND _cmake_import_check_targets Microsoft.IFC::DOM )
list(APPEND _cmake_import_check_files_for_Microsoft.IFC::DOM "${_IMPORT_PREFIX}/lib/libifc-dom.a" )

# Import target "Microsoft.IFC::Core" for configuration ""
set_property(TARGET Microsoft.IFC::Core APPEND PROPERTY IMPORTED_CONFIGURATIONS NOCONFIG)
set_target_properties(Microsoft.IFC::Core PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_NOCONFIG "CXX"
  IMPORTED_LOCATION_NOCONFIG "${_IMPORT_PREFIX}/lib/libifc-reader.a"
  )

list(APPEND _cmake_import_check_targets Microsoft.IFC::Core )
list(APPEND _cmake_import_check_files_for_Microsoft.IFC::Core "${_IMPORT_PREFIX}/lib/libifc-reader.a" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
