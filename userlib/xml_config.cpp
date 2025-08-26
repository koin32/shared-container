#include "xml_config.hpp"
#include "kcontainer.hpp"
#include <stdexcept>
#include <sstream>
#include <cctype>
#include <unordered_map>

#ifdef HAVE_TINYXML2
#include <tinyxml2.h>
using namespace tinyxml2;
#endif

namespace kcontainer {

// Вспомогательные функции для парсинга
namespace {

size_t parse_size(const std::string& s) {
    size_t num = 0;
    char suffix = 0;
    std::stringstream ss(s);
    ss >> num;
    if (!ss.eof()) ss >> suffix;
    size_t mul = 1;
    switch (std::toupper(suffix)) {
        case 'K': mul = 1024ull; break;
        case 'M': mul = 1024ull*1024; break;
        case 'G': mul = 1024ull*1024*1024; break;
        case 'T': mul = 1024ull*1024*1024*1024; break;
        case 0:   mul = 1; break;
        default: throw std::invalid_argument("bad size suffix");
    }
    return num * mul;
}

uint64_t parse_id(const std::string& s) {
    uint64_t val = 0;
    if (s.size() > 2 && s[0] == '0' && (s[1]=='x' || s[1]=='X')) {
        std::stringstream ss(s);
        ss >> std::hex >> val;
    } else {
        std::stringstream ss(s);
        ss >> val;
    }
    return val;
}

const std::unordered_map<std::string, VariableType> TYPE_MAPPING = {
    {"bool", VariableType::BOOL},
    {"int8_t", VariableType::INT8},
    {"uint8_t", VariableType::UINT8},
    {"int16_t", VariableType::INT16},
    {"uint16_t", VariableType::UINT16},
    {"int32_t", VariableType::INT32},
    {"uint32_t", VariableType::UINT32},
    {"int64_t", VariableType::INT64},
    {"uint64_t", VariableType::UINT64},
    {"float", VariableType::FLOAT},
    {"double", VariableType::DOUBLE},
    {"char", VariableType::CHAR}
};

} // namespace

size_t get_type_size(VariableType type) {
    switch (type) {
        case VariableType::BOOL:   return 1;
        case VariableType::INT8:   return 1;
        case VariableType::UINT8:  return 1;
        case VariableType::INT16:  return 2;
        case VariableType::UINT16: return 2;
        case VariableType::INT32:  return 4;
        case VariableType::UINT32: return 4;
        case VariableType::INT64:  return 8;
        case VariableType::UINT64: return 8;
        case VariableType::FLOAT:  return 4;
        case VariableType::DOUBLE: return 8;
        case VariableType::CHAR:   return 1;
        default: return 0;
    }
}

std::string variable_type_to_string(VariableType type) {
    for (const auto& pair : TYPE_MAPPING) {
        if (pair.second == type) return pair.first;
    }
    return "unknown";
}

VariableType string_to_variable_type(const std::string& type_str) {
    auto it = TYPE_MAPPING.find(type_str);
    if (it != TYPE_MAPPING.end()) return it->second;
    return VariableType::CUSTOM;
}

ContainerConfig parse_container_config(const std::string& xml_path, uint64_t container_id) {
#ifdef HAVE_TINYXML2
    XMLDocument doc;
    if (doc.LoadFile(xml_path.c_str()) != XML_SUCCESS) {
        throw std::runtime_error("Cannot load XML: " + xml_path);
    }

    ContainerConfig config;
    config.id = container_id;

    auto* root = doc.RootElement();
    if (!root || std::string(root->Name()) != "containers") {
        throw std::runtime_error("Invalid XML root element");
    }


    // Поиск контейнера по ID
    const char* id_attr = nullptr;
    for (auto* container_el = root->FirstChildElement("container"); 
         container_el; 
         container_el = container_el->NextSiblingElement("container")) {
        
        id_attr = container_el->Attribute("id");
        if (!id_attr) continue;

        // Используем функцию parse_id из анонимного namespace
        uint64_t current_id = parse_id(std::string(id_attr));
        if (current_id == container_id) {
            // Нашли нужный контейнер
            config.name = container_el->Attribute("name") ? container_el->Attribute("name") : "";
            const char* size_attr = container_el->Attribute("size");
            if (size_attr) {
                config.total_size = parse_size(std::string(size_attr));
            }

            // Парсим переменные
            size_t current_offset = 0;
            for (auto* var_el = container_el->FirstChildElement("variable"); 
                 var_el; 
                 var_el = var_el->NextSiblingElement("variable")) {
                
                VariableDefinition var;
                var.name = var_el->Attribute("name") ? var_el->Attribute("name") : "";
                
                const char* type_attr = var_el->Attribute("type");
                if (type_attr) {
                    var.type = string_to_variable_type(type_attr);
                    var.type_name = type_attr;
                    var.size = get_type_size(var.type);

                    // Обрабатываем массивы
                    const char* array_attr = var_el->Attribute("array");
                    if (array_attr && std::string(array_attr) == "true") {
                        var.is_array = true;
                        const char* elements_attr = var_el->Attribute("elements");
                        if (elements_attr) {
                            var.array_size = std::stoul(elements_attr);
                            var.size *= var.array_size;
                        }
                        
                        // Особый случай для массивов char (строк)
                        if (var.type == VariableType::CHAR) {
                            const char* elem_size_attr = var_el->Attribute("element_size");
                            if (elem_size_attr) {
                                var.element_size = std::stoul(elem_size_attr);
                                var.size = var.array_size * var.element_size;
                            }
                        }
                    }

                    var.offset = current_offset;
                    current_offset += var.size;
                    
                    config.variables.push_back(var);
                    config.variable_offsets[var.name] = var.offset;
                }
            }
            return config;
        }
    }

    throw std::runtime_error("Container with ID " + std::to_string(container_id) + " not found in XML");
#else
    throw std::runtime_error("XML support not available - TinyXML2 not found");
#endif
}

} // namespace kcontainer