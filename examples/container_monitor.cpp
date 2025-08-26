#include "kcontainer.hpp"
#include <iostream>
#include <iomanip>
#include <vector>

int main() {
    try {
        std::cout << "🔍 Монитор контейнеров kcontainer\n";
        std::cout << "==================================\n";
        
        // Проверяем несколько возможных контейнеров
        std::vector<uint64_t> container_ids = {123, 777, 9999, 1001, 1002, 1003};
        
        for (uint64_t id : container_ids) {
            auto stats = kcontainer::Container::get_stats(id);
            
            std::cout << "Контейнер " << std::setw(4) << id << ": ";
            if (stats.exists) {
                std::cout << "✅ Размер: " << std::setw(6) << stats.size 
                          << " байт, Процессов: " << stats.user_refs
                          << ", Ссылок: " << stats.kernel_refs;
            } else {
                std::cout << "❌ Не существует";
            }
            std::cout << "\n";
        }
        
        std::cout << "==================================\n";
        
    } catch (const std::exception& e) {
        std::cerr << "❌ Ошибка мониторинга: " << e.what() << "\n";
        return 1;
    }
    return 0;
}