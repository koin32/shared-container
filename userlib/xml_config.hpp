#pragma once
#include <string>
#include <vector>
#include <cstdint>
#include <map>
#include "kcontainer.hpp"

namespace kcontainer {

// Типы данных
enum class VariableType {
    BOOL,
    INT8, UINT8,
    INT16, UINT16, 
    INT32, UINT32,
    INT64, UINT64,
    FLOAT, DOUBLE,
    CHAR,
    STRUCT,
    CUSTOM
};

struct VariableDefinition {
    std::string name;
    VariableType type;
    std::string type_name; // Для структур и кастомных типов
    bool is_array;
    size_t array_size;
    size_t element_size; // Для массивов char
    size_t offset; // Смещение в контейнере
    size_t size;   // Общий размер переменной
};

struct StructDefinition {
    std::string name;
    std::vector<VariableDefinition> fields;
    size_t total_size;
};

struct ContainerConfig {
    uint64_t id;
    std::string name;
    size_t total_size;
    std::vector<VariableDefinition> variables;
    std::vector<StructDefinition> structs;
    std::map<std::string, size_t> variable_offsets;
};

// Функции для работы с XML конфигурацией
ContainerConfig parse_container_config(const std::string& xml_path, uint64_t container_id);
std::vector<ContainerConfig> parse_all_containers_config(const std::string& xml_path);

// Вспомогательные функции
size_t get_type_size(VariableType type);
std::string variable_type_to_string(VariableType type);
VariableType string_to_variable_type(const std::string& type_str);

// Создание контейнеров из конфигурации
int create_containers_from_config(const std::string& xml_path);

} // namespace kcontainer