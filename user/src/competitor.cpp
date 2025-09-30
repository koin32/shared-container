#include "varser/varser.hpp"
#include <iostream>
#include <thread>
#include <chrono>
#include <atomic>
#include <csignal>
#include <random>

std::atomic<bool> running{true};

void signal_handler(int sig) {
    std::cout << "Competitor: Received signal " << sig << ", stopping..." << std::endl;
    running = false;
}

int main(int argc, char** argv) {
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    using namespace varser;
    
    std::string yamlPath = (argc > 1) ? argv[1] : "../examples/container.yaml.example";
    std::cout << "Competitor: Using YAML file: " << yamlPath << std::endl;
    
    auto c = ContainerManager::instance().load_from_yaml(yamlPath);
    if (!c) { 
        std::cerr << "Competitor: Load failed\n"; 
        return 1; 
    }

    if (!c->open()) { 
        std::cerr << "Competitor: Open failed\n"; 
        return 1; 
    }

    std::cout << "Competitor: Started. Press Ctrl+C to stop." << std::endl;
    
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(1, 100);
    
    while (running) {
        // Случайно либо читаем, либо пишем
        if (dis(gen) > 50) {
            // Чтение
            int64_t counter = 0;
            double temperature = 0.0;
            
            if (c->get<int64_t>("counter", counter) && c->get<double>("temperature", temperature)) {
                std::cout << "Competitor: READ counter = " << counter << ", temperature = " << temperature << "°C" << std::endl;
            }
        } else {
            // Запись случайных значений
            int64_t random_counter = dis(gen) * 10;
            double random_temp = 15.0 + dis(gen) * 0.2;
            
            if (c->set<int64_t>("counter", random_counter) && c->set<double>("temperature", random_temp)) {
                std::cout << "Competitor: WRITE counter = " << random_counter << ", temperature = " << random_temp << "°C" << std::endl;
            }
        }
        
        std::this_thread::sleep_for(std::chrono::milliseconds(800));
    }
    
    c->close();
    std::cout << "Competitor: Stopped." << std::endl;
    return 0;
}