cmake_minimum_required(VERSION 2.8.6)

project(sqrew)

list(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake")
include(CXXFeatures)

use_cxx11()

add_subdirectory(extern/squirrel)

file(GLOB_RECURSE SOURCES ./src/*.cpp)
file(GLOB_RECURSE HEADERS ./include/*.h)

include_directories(include ${SQUIRREL_INCLUDE})

add_library(sqrew STATIC ${SOURCES} ${HEADERS})
target_link_libraries(sqrew squirrel)
target_link_libraries(sqrew sqstdlib)

add_executable(test ./test/test.cpp)
target_link_libraries(test sqrew)
