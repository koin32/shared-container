#include "kcontainer.hpp"
#include "xml_config.hpp"
#include <iostream>
#include <cstring>
#include <sys/mman.h>
#include <iomanip>

int main() {
    std::cout << "================================================" << std::endl;
    std::cout << "🚀 XML Configuration Demo" << std::endl;
    std::cout << "================================================" << std::endl;

    try {
        // Проверяем доступность XML поддержки
        #ifdef HAVE_TINYXML2
        std::cout << "✅ XML support is enabled" << std::endl;
        
        // Попробуем распарсить конфигурацию
        try {
            auto config = kcontainer::parse_container_config("../containers_config.xml", 0x1001);
            std::cout << "📋 Configuration loaded successfully!" << std::endl;
            std::cout << "   Container: " << config.name << std::endl;
            std::cout << "   ID: 0x" << std::hex << config.id << std::dec << std::endl;
            std::cout << "   Size: " << config.total_size << " bytes" << std::endl;
            std::cout << "   Variables: " << config.variables.size() << std::endl;
            
        } catch (const std::exception& e) {
            std::cout << "⚠️  XML parsing test failed: " << e.what() << std::endl;
            std::cout << "💡 Create containers_config.xml file first" << std::endl;
        }
        #else
        std::cout << "❌ XML support is disabled" << std::endl;
        #endif

    } catch (const std::exception& e) {
        std::cerr << "❌ Error: " << e.what() << std::endl;
        return 1;
    }

    std::cout << "================================================" << std::endl;
    std::cout << "✅ XML demo completed" << std::endl;
    std::cout << "================================================" << std::endl;
    return 0;
}