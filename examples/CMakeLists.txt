cmake_minimum_required(VERSION 3.10)
project(demo CXX)

set(CMAKE_CXX_STANDARD 17)

add_executable(demo demo.cpp)
target_include_directories(demo PRIVATE ${CMAKE_SOURCE_DIR}/../userlib)
target_link_libraries(demo PRIVATE kcontainer)