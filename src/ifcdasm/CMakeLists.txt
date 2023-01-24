add_executable(ifcdasm
  main.cxx
  printer.cxx
)

target_link_libraries(ifcdasm
  PUBLIC
    ifc-dom
    ifc-reader
)

target_link_libraries(ifcdasm PRIVATE GSL)
