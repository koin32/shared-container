cmake_minimum_required(VERSION 3.10)
project(demo CXX)

set(CMAKE_CXX_STANDARD 17)

add_executable(demo demo.cpp)

# Указываем правильные пути для include и библиотеки
target_include_directories(demo PRIVATE 
    ${CMAKE_SOURCE_DIR}/../userlib
)

# Указываем путь к библиотеке и саму библиотеку
target_link_directories(demo PRIVATE
    ${CMAKE_SOURCE_DIR}/../userlib/build
)

target_link_libraries(demo PRIVATE kcontainer)