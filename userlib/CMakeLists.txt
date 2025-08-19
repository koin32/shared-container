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

# Гибкий поиск TinyXML2
option(FORCE_NO_XML "Force disable XML support" OFF)

if(NOT FORCE_NO_XML)
    find_package(TinyXML2 QUIET)
endif()

if(TinyXML2_FOUND)
    message(STATUS "Found TinyXML2: ${TinyXML2_INCLUDE_DIRS}")
    target_link_libraries(kcontainer PUBLIC TinyXML2::TinyXML2)
    target_compile_definitions(kcontainer PUBLIC -DHAVE_TINYXML2)
else()
    message(STATUS "TinyXML2 not found, building without XML support")
    target_compile_definitions(kcontainer PUBLIC -DNO_XML_SUPPORT)
endif()