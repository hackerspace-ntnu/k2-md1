cmake_minimum_required(VERSION 2.8.11)
project(K2MD1)

set(KSrc

src/test.cpp

)

include_directories(${PROJECT_SOURCE_DIR})

add_executable(FreeBot ${KSrc})

set(KLibs


)

target_link_libraries(FreeBot ${KLibs})
