cmake_minimum_required(VERSION 2.8.11)
project(K2MD1)

set(KSrc

src/firstkinect.cpp

)

set(KHeaders

${CMAKE_SOURCE_DIR}
deps/libfreenect2/include

)

include_directories(${KHeaders})

add_executable(FreeBot ${KSrc} ${KHeaders})

set(KLibs


)

target_link_libraries(FreeBot ${KLibs})
