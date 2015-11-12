cmake_minimum_required(VERSION 2.8.11)
set (CMAKE_BUILD_TYPE Debug)
project(K2MD1)

LIST(APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake_modules)

set( KSrc
#Add compilation units here
deps/flextGL/flextGL.c
src/main.cpp
src/context/freenect_types.cpp
src/graphics/glcontext.cpp
src/graphics/glfw_types.cpp
deps/stb/stb_image.h
deps/glm

)

set( KHeaders
#Add paths to headers here
${CMAKE_SOURCE_DIR}
deps/flextGL
deps/libfreenect2/include
deps/glfw/include
deps/stb

)

set(KLibs
#Add library linking options here

)

#Set include directories
include_directories(${KHeaders})

#Find the Freenect2 library using a CMake variable given by user
find_library(FREENECT2_LIBRARY NAMES "freenect2" PATHS ${CMAKE_CURRENT_BINARY_DIR})

find_package(GLFW3)
find_package(OpenGL)

if(NOT WIN32)
    find_package(X11)
    set (GLFWLibs Xrandr Xi GL m dl Xxf86vm X11 pthread Xinerama Xcursor ${X11_LIBRARIES})
endif()

message("Freenect2 at: ${FREENECT2_LIBRARY}")

#Create a target
add_executable(FreeBot ${KSrc})
#Link the target
target_link_libraries(FreeBot ${KLibs} ${FREENECT2_LIBRARY} ${GLFW3_LIBRARIES} ${GLFWLibs})

target_compile_features(FreeBot PUBLIC cxx_constexpr)