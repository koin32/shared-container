cmake_minimum_required(VERSION 3.16)
project(varser_user LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
find_package(yaml-cpp REQUIRED)

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../kernel
)

add_library(varser STATIC src/varser.cpp)
target_link_libraries(varser PUBLIC yaml-cpp)

# Основной демо
add_executable(varser_demo src/main.cpp)
target_link_libraries(varser_demo PRIVATE varser)

# Мультипроцессные демо
add_executable(writer src/writer.cpp)
target_link_libraries(writer PRIVATE varser)

add_executable(reader src/reader.cpp)
target_link_libraries(reader PRIVATE varser)

add_executable(competitor src/competitor.cpp)
target_link_libraries(competitor PRIVATE varser)