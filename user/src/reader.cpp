#include "varser/varser.hpp"
#include <iostream>
#include <thread>
#include <chrono>
#include <atomic>
#include <csignal>

std::atomic<bool> running{true};

void signal_handler(int sig) {
    std::cout << "Reader: Received signal " << sig << ", stopping..." << std::endl;
    running = false;
}

int main(int argc, char** argv) {
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    using namespace varser;
    
    std::string yamlPath = (argc > 1) ? argv[1] : "../examples/container.yaml.example";
    std::cout << "Reader: Using YAML file: " << yamlPath << std::endl << std::flush;
    
    auto c = ContainerManager::instance().load_from_yaml(yamlPath);
    if (!c) { 
        std::cerr << "Reader: Load failed\n" << std::flush; 
        return 1; 
    }

    if (!c->open()) { 
        std::cerr << "Reader: Open failed\n" << std::flush; 
        return 1; 
    }

    std::cout << "Reader: Started. Press Ctrl+C to stop." << std::endl << std::flush;
    
    while (running) {
        int64_t counter = 0;
        double temperature = 0.0;
        
        // Читаем данные из контейнера
        if (!c->get<int64_t>("counter", counter)) {
            std::cerr << "Reader: Failed to get counter" << std::endl << std::flush;
        } else if (!c->get<double>("temperature", temperature)) {
            std::cerr << "Reader: Failed to get temperature" << std::endl << std::flush;
        } else {
            std::cout << "Reader: counter = " << counter << ", temperature = " << temperature << "°C" << std::endl << std::flush;
        }
        
        // Ждем 1 секунду (читаем чаще чем пишем)
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    
    c->close();
    std::cout << "Reader: Stopped." << std::endl << std::flush;
    return 0;
}