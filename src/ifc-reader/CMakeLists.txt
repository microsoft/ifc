# IFC Reader Library

if(UNIX)
	set(PLATFORM_SPECIFIC_SOURCES ../sha256.cxx)
endif()

if(WIN32)
	set(PLATFORM_SPECIFIC_SOURCES ../hash_win.cxx)
	set(PLATFORM_SPECIFIC_LIBS bcrypt.lib)
endif()


add_library(ifc-reader STATIC 
	../assert.cxx
	../file.cxx
	operators.cxx
	reader.cxx
	../sgraph.cxx
	util.cxx
	${PLATFORM_SPECIFIC_SOURCES}
)

target_link_libraries(ifc-reader PRIVATE GSL ${PLATFORM_SPECIFIC_LIBS})
