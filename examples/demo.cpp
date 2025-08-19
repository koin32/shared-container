#include "kcontainer.hpp"
#include <iostream>

int main() {
    try {
        kcontainer::Container c = kcontainer::Container::create(1, 1024 * 1024); // 1MB
        std::cout << "Created container ID " << c.id() << " with size " << c.size() << "\n";
        kcontainer::Mapping m = c.map();
        if (m.ptr) {
            std::cout << "Mapped container at " << m.ptr << "\n";
            memset(m.ptr, 0, m.size);
            munmap(m.ptr, m.size);
        }
        c.close();
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
    return 0;
}