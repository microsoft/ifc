# IFC Reader Library

add_library(ifc-reader STATIC 
	assert.cxx
	file.cxx
	io.cxx
	filesystem.cxx
	operators.cxx
	reader.cxx
	sgraph.cxx
	util.cxx)

target_link_libraries(ifc-reader PRIVATE GSL)
