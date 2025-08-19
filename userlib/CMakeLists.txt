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
if(NOT TinyXML2_FOUND)
    message(FATAL_ERROR "TinyXML2 not found. Please install libtinyxml2-dev or set TinyXML2_DIR or CMAKE_PREFIX_PATH.")
endif()
target_link_libraries(kcontainer PUBLIC TinyXML2::TinyXML2)