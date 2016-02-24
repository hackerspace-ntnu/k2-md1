project ( k2md1 )
cmake_minimum_required( VERSION 2.8 )

set ( CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin/ )
set ( CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib/ )

# set ( FREENECT2_ROOT_DIR )

set ( BAT_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/deps )

set ( FREENECT2_INCLUDE_DIR "${FREENECT2_ROOT_DIR}/include/" )
set ( FREENECT2_LIBRARY_DIR "${FREENECT2_ROOT_DIR}/build/lib/" )

set ( DEFAULT_LIBS )

list ( APPEND DEFAULT_LIBS freenect2 X11 )

include_directories (
	${FREENECT2_INCLUDE_DIR}
	${BAT_INCLUDE_DIR}
	include
	)
link_directories (
	${FREENECT2_LIBRARY_DIR} )

add_subdirectory ( src )
