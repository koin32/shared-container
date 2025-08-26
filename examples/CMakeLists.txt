cmake_minimum_required(VERSION 3.10)
project(examples CXX)

set(CMAKE_CXX_STANDARD 17)

# Поиск TinyXML2
find_package(tinyxml2 REQUIRED)

# Основное демо
add_executable(demo1 demo1.cpp)
target_include_directories(demo1 PRIVATE ${CMAKE_SOURCE_DIR}/../userlib)
target_link_directories(demo1 PRIVATE ${CMAKE_SOURCE_DIR}/../userlib/build)
target_link_libraries(demo1 kcontainer tinyxml2::tinyxml2)

# Межпроцессное взаимодействие
add_executable(ipc_writer ipc_writer.cpp)
target_include_directories(ipc_writer PRIVATE ${CMAKE_SOURCE_DIR}/../userlib)
target_link_directories(ipc_writer PRIVATE ${CMAKE_SOURCE_DIR}/../userlib/build)
target_link_libraries(ipc_writer kcontainer tinyxml2::tinyxml2)

add_executable(ipc_reader ipc_reader.cpp)
target_include_directories(ipc_reader PRIVATE ${CMAKE_SOURCE_DIR}/../userlib)
target_link_directories(ipc_reader PRIVATE ${CMAKE_SOURCE_DIR}/../userlib/build)
target_link_libraries(ipc_reader kcontainer tinyxml2::tinyxml2)

# Многопоточное демо
add_executable(demo3 demo3.cpp)
target_include_directories(demo3 PRIVATE ${CMAKE_SOURCE_DIR}/../userlib)
target_link_directories(demo3 PRIVATE ${CMAKE_SOURCE_DIR}/../userlib/build)
target_link_libraries(demo3 kcontainer tinyxml2::tinyxml2)

# Новые демо: Persistence и мониторинг
add_executable(persistence_demo persistence_demo.cpp)
target_include_directories(persistence_demo PRIVATE ${CMAKE_SOURCE_DIR}/../userlib)
target_link_directories(persistence_demo PRIVATE ${CMAKE_SOURCE_DIR}/../userlib/build)
target_link_libraries(persistence_demo kcontainer tinyxml2::tinyxml2)

add_executable(container_monitor container_monitor.cpp)
target_include_directories(container_monitor PRIVATE ${CMAKE_SOURCE_DIR}/../userlib)
target_link_directories(container_monitor PRIVATE ${CMAKE_SOURCE_DIR}/../userlib/build)
target_link_libraries(container_monitor kcontainer tinyxml2::tinyxml2)

# XML демонстрация
add_executable(xml_config_demo xml_config_demo.cpp)
target_include_directories(xml_config_demo PRIVATE ${CMAKE_SOURCE_DIR}/../userlib)
target_link_directories(xml_config_demo PRIVATE ${CMAKE_SOURCE_DIR}/../userlib/build)
target_link_libraries(xml_config_demo kcontainer tinyxml2::tinyxml2)

# Утилита создания контейнеров из XML
add_executable(create_from_xml create_from_xml.cpp)
target_include_directories(create_from_xml PRIVATE ${CMAKE_SOURCE_DIR}/../userlib)
target_link_directories(create_from_xml PRIVATE ${CMAKE_SOURCE_DIR}/../userlib/build)
target_link_libraries(create_from_xml kcontainer tinyxml2::tinyxml2)