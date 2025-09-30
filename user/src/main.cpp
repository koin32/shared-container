#include "varser/varser.hpp"
#include <iostream>
#include <chrono>
#include <thread>

int main(int argc, char** argv) {
    using namespace varser;
    
    std::string yamlPath = (argc > 1) ? argv[1] : "../examples/example.yaml";
    std::cout << "Using YAML file: " << yamlPath << std::endl;
    
    auto c = ContainerManager::instance().load_from_yaml(yamlPath);
    if (!c) { std::cerr << "Load failed\n"; return 1; }

    if (!c->open()) { std::cerr << "Open failed\n"; return 1; }

    int64_t val = 0;
    if (!c->get<int64_t>("counter", val)) {
        std::cerr << "get failed\n";
    } else {
        std::cout << "counter initial = " << val << "\n";
    }

    if (!c->set<int64_t>("counter", (int64_t)42)) {
        std::cerr << "set failed\n";
    } else {
        std::cout << "set counter = 42\n";
    }

    if (!c->get<int64_t>("counter", val)) {
        std::cerr << "get failed\n";
    } else {
        std::cout << "counter now = " << val << "\n";
    }

    c->close();
    return 0;
}