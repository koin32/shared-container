#include "xml_config.hpp"
#include "kcontainer.hpp"
#include <tinyxml2.h>
#include <stdexcept>
#include <system_error>
#include <sstream>
#include <cctype>

using namespace tinyxml2;

namespace {

// "100K", "64M", "1G" → bytes
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

} // namespace

namespace kcontainer {

std::vector<ContainerSpec> parse_xml(const std::string& xml_path) {
    XMLDocument doc;
    if (doc.LoadFile(xml_path.c_str()) != XML_SUCCESS)
        throw std::runtime_error("cannot load XML: " + xml_path);

    auto* root = doc.RootElement();
    if (!root || std::string(root->Name()) != "containers")
        throw std::runtime_error("bad XML root");

    std::vector<ContainerSpec> out;
    for (auto* el = root->FirstChildElement("container"); el; el = el->NextSiblingElement("container")) {
        const char* idtxt = el->Attribute("id");
        const char* sizetxt = el->Attribute("size");
        if (!idtxt || !sizetxt) continue;
        ContainerSpec spec;
        spec.id = parse_id(idtxt);
        spec.size = parse_size(sizetxt);
        out.push_back(spec);
    }
    return out;
}

int ensure_from_xml(const std::string& xml_path) {
    auto specs = parse_xml(xml_path);
    int ok = 0;
    for (auto& s : specs) {
        try {
            Container::create(s.id, s.size);
            ++ok;
        } catch(...) {
            // уже создан или ошибка
        }
    }
    return ok;
}

} // namespace kcontainer
