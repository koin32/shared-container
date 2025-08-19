#pragma once
#include <string>
#include <vector>
#include <cstdint>

namespace kcontainer {

struct ContainerSpec {
    uint64_t id;
    size_t size;
};

// Прочитать XML и гарантировать наличие описанных контейнеров
int ensure_from_xml(const std::string& xml_path);

// Получить список контейнеров из XML без создания
std::vector<ContainerSpec> parse_xml(const std::string& xml_path);

} // namespace kcontainer
