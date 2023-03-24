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
