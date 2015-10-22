cmake_minimum_required(VERSION 2.8.11)
project(K2MD1)

LIST(APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake_modules)

set( KSrc
#Add compilation units here
deps/glad/src/glad.c
src/firstkinect.cpp
src/context/freenect_types.cpp

)

set( KHeaders
#Add paths to headers here
${CMAKE_SOURCE_DIR}
deps/glad/include
deps/libfreenect2/include
deps/glfw/include

)

set(KLibs
#Add library linking options here

)

#Set include directories
include_directories(${KHeaders})

#Find the Freenect2 library using a CMake variable given by user
find_library(FREENECT2_LIBRARY NAMES "freenect2" PATHS ${CMAKE_CURRENT_BINARY_DIR})

message("Freenect2 at: ${FREENECT2_LIBRARY}")

#Create a target
add_executable(FreeBot ${KSrc})
#Link the target
target_link_libraries(FreeBot ${KLibs} ${FREENECT2_LIBRARY})
