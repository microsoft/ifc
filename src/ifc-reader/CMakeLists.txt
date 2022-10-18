# IFC Reader Library

add_library(ifc-reader STATIC 
	assert.cxx
	file.cxx
	operators.cxx
	reader.cxx
	sgraph.cxx
	sha256.cxx
	util.cxx)

target_link_libraries(ifc-reader PRIVATE GSL)
