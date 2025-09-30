#include "varser/varser.hpp"
#include <iostream>
#include <thread>
#include <chrono>
#include <atomic>
#include <csignal>

std::atomic<bool> running{true};

void signal_handler(int sig) {
    std::cout << "Writer: Received signal " << sig << ", stopping..." << std::endl;
    running = false;
}

int main(int argc, char** argv) {
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    using namespace varser;
    
    std::string yamlPath = (argc > 1) ? argv[1] : "../examples/container.yaml.example";
    std::cout << "Writer: Using YAML file: " << yamlPath << std::endl << std::flush;
    
    auto c = ContainerManager::instance().load_from_yaml(yamlPath);
    if (!c) { 
        std::cerr << "Writer: Load failed\n" << std::flush; 
        return 1; 
    }

    if (!c->open()) { 
        std::cerr << "Writer: Open failed\n" << std::flush; 
        return 1; 
    }

    std::cout << "Writer: Started. Press Ctrl+C to stop." << std::endl << std::flush;
    
    int64_t counter = 0;
    double temperature = 20.0;
    
    while (running) {
        // Записываем данные в контейнер
        if (!c->set<int64_t>("counter", counter)) {
            std::cerr << "Writer: Failed to set counter" << std::endl << std::flush;
        } else {
            std::cout << "Writer: Set counter = " << counter << std::endl << std::flush;
        }
        
        if (!c->set<double>("temperature", temperature)) {
            std::cerr << "Writer: Failed to set temperature" << std::endl << std::flush;
        } else {
            std::cout << "Writer: Set temperature = " << temperature << "°C" << std::endl << std::flush;
        }
        
        // Увеличиваем значения для следующей итерации
        counter++;
        temperature += 0.5;
        if (temperature > 30.0) temperature = 20.0;
        
        // Ждем 2 секунды
        std::this_thread::sleep_for(std::chrono::seconds(2));
    }
    
    c->close();
    std::cout << "Writer: Stopped." << std::endl << std::flush;
    return 0;
}