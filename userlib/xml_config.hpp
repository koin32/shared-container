#pragma once
#include <string>
#include <vector>
#include <cstdint>
#include "kcontainer.hpp" 

namespace kcontainer {

// Прочитать XML и гарантировать наличие описанных контейнеров
int ensure_from_xml(const std::string& xml_path);

// Получить список контейнеров из XML без создания
std::vector<ContainerSpec> parse_xml(const std::string& xml_path);

} // namespace kcontainer