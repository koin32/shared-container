#include "kcontainer.hpp"
#include <iostream>
#include <vector>

int main() {
    std::cout << "================================================" << std::endl;
    std::cout << "🚀 Create from XML Demo" << std::endl;
    std::cout << "================================================" << std::endl;

    std::cout << "This demo would create containers from XML configuration" << std::endl;
    std::cout << "XML configuration file: ../containers_config.xml" << std::endl;
    
    #ifdef HAVE_TINYXML2
    std::cout << "✅ XML support is available" << std::endl;
    #else
    std::cout << "❌ XML support is not available" << std::endl;
    #endif

    std::cout << "================================================" << std::endl;
    std::cout << "✅ Create from XML demo completed" << std::endl;
    std::cout << "================================================" << std::endl;
    return 0;
}