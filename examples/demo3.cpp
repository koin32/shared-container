#include "kcontainer.hpp"
#include <iostream>
#include <vector>
#include <thread>
#include <cstring>
#include <sys/mman.h>

void worker(uint64_t container_id, const std::string& message) {
    try {
        kcontainer::Container container = kcontainer::Container::open(container_id);
        
        // Получаем информацию (ИСПРАВЛЕНО)
        size_t size;
        uint32_t user_refs, kernel_refs;
        container.info(size, user_refs, kernel_refs);
        std::cout << "Контейнер " << container_id << " имеет " << user_refs << " пользователей\n";
        
        kcontainer::Mapping mapping = container.map();
        
        if (mapping.ptr) {
            std::memcpy(mapping.ptr, message.c_str(), message.size() + 1);
            std::cout << "Поток записал в контейнер " << container_id 
                      << ": " << (char*)mapping.ptr << "\n";
            munmap(mapping.ptr, mapping.size);
        }
        container.close();
    } catch (const std::exception& e) {
        std::cerr << "Ошибка в потоке: " << e.what() << "\n";
    }
}

int main() {
    try {
        std::cout << "=== Многопоточная демонстрация ===\n";
        
        // Создаем несколько контейнеров
        std::vector<uint64_t> container_ids = {1001, 1002, 1003};
        std::vector<std::string> messages = {
            "Сообщение из потока 1",
            "Сообщение из потока 2", 
            "Сообщение из потока 3"
        };
        
        // Создаем контейнеры
        for (auto id : container_ids) {
            kcontainer::Container::create(id, 4096);
            std::cout << "Создан контейнер ID: " << id << "\n";
        }
        
        // Запускаем потоки для работы с контейнерами
        std::vector<std::thread> threads;
        for (size_t i = 0; i < container_ids.size(); ++i) {
            threads.emplace_back(worker, container_ids[i], messages[i]);
        }
        
        // Ждем завершения потоков
        for (auto& thread : threads) {
            thread.join();
        }
        
        // Читаем результаты из основного потока
        for (auto id : container_ids) {
            kcontainer::Container container = kcontainer::Container::open(id);
            kcontainer::Mapping mapping = container.map();
            
            if (mapping.ptr) {
                std::cout << "Контейнер " << id << " содержит: " 
                          << (char*)mapping.ptr << "\n";
                munmap(mapping.ptr, mapping.size);
            }
            container.close();
        }
        
        std::cout << "=== Многопоточная демонстрация завершена ===\n";
        
    } catch (const std::exception& e) {
        std::cerr << "❌ Ошибка: " << e.what() << "\n";
        return 1;
    }
    return 0;
}