cmake_minimum_required(VERSION 3.10)
project(kcontainer CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

add_library(kcontainer SHARED
    kcontainer.cpp
    xml_config.cpp
)

target_include_directories(kcontainer PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../kernel
)

find_package(TinyXML2 REQUIRED)
target_link_libraries(kcontainer PUBLIC TinyXML2::TinyXML2)
