cmake_minimum_required(VERSION 2.8.11)

project(sqstdlib)

file(GLOB_RECURSE SOURCES ./*.cpp)
file(GLOB_RECURSE HEADERS ./*.h)

include_directories(../include)

add_library(sqstdlib STATIC ${SOURCES} ${HEADERS})
