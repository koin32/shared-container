#include "kcontainer.hpp"
#include <iostream>
#include <cstring>
#include <sys/mman.h>
#include <unistd.h>
#include <sys/wait.h>

void process_worker(uint64_t container_id, int process_num) {
    try {
        std::cout << "🚀 Процесс " << process_num << " запущен (PID: " << getpid() << ")\n";
        
        // Каждый процесс открывает тот же контейнер
        kcontainer::Container container = kcontainer::Container::open(container_id);
        
        // Получаем информацию о контейнере
        size_t size;
        uint32_t user_refs, kernel_refs;
        container.info(size, user_refs, kernel_refs);
        std::cout << "📊 Процесс " << process_num << ": пользователей=" << user_refs 
                  << ", ядерных ссылок=" << kernel_refs << "\n";
        
        kcontainer::Mapping mapping = container.map();
        
        if (mapping.ptr) {
            // Читаем и обновляем счетчик
            int* counter = static_cast<int*>(mapping.ptr);
            int old_value = *counter;
            *counter = old_value + 1;
            
            std::cout << "🔢 Процесс " << process_num << ": счетчик " 
                      << old_value << " -> " << *counter << "\n";
            
            // Ждем чтобы другие процессы успели поработать
            sleep(1);
            
            munmap(mapping.ptr, mapping.size);
        }
        
        // Закрываем контейнер, но он продолжает существовать!
        container.close();
        
        std::cout << "✅ Процесс " << process_num << " завершен\n";
        
    } catch (const std::exception& e) {
        std::cerr << "❌ Ошибка в процессе " << process_num << ": " << e.what() << "\n";
    }
}

int main() {
    try {
        std::cout << "=== Демонстрация Persistence ===\n";
        
        const uint64_t SHARED_CONTAINER_ID = 9999;
        const size_t CONTAINER_SIZE = sizeof(int);
        
        // Создаем или открываем общий контейнер
        kcontainer::Container container = kcontainer::Container::create(SHARED_CONTAINER_ID, CONTAINER_SIZE);
        std::cout << "✅ Создан общий контейнер ID: " << container.id() << "\n";
        
        // Инициализируем счетчик
        kcontainer::Mapping mapping = container.map();
        if (mapping.ptr) {
            int* counter = static_cast<int*>(mapping.ptr);
            *counter = 0;
            std::cout << "🔢 Счетчик инициализирован: " << *counter << "\n";
            munmap(mapping.ptr, mapping.size);
        }
        
        // Получаем информацию до запуска процессов
        size_t size;
        uint32_t user_refs, kernel_refs;
        container.info(size, user_refs, kernel_refs);
        std::cout << "📊 До процессов: пользователей=" << user_refs 
                  << ", ядерных ссылок=" << kernel_refs << "\n";
        
        // Закрываем основной дескриптор, но контейнер остается!
        container.close();
        std::cout << "📌 Основной процесс закрыл дескриптор, контейнер сохраняется\n";
        
        // Запускаем несколько процессов
        const int NUM_PROCESSES = 3;
        std::vector<pid_t> pids;
        
        for (int i = 0; i < NUM_PROCESSES; ++i) {
            pid_t pid = fork();
            if (pid == 0) {
                // Дочерний процесс
                process_worker(SHARED_CONTAINER_ID, i + 1);
                exit(0);
            } else if (pid > 0) {
                pids.push_back(pid);
            } else {
                std::cerr << "❌ Ошибка создания процесса\n";
            }
        }
        
        // Ждем завершения всех процессов
        for (pid_t pid : pids) {
            waitpid(pid, nullptr, 0);
        }
        
        // Проверяем, что контейнер все еще существует
        if (kcontainer::Container::exists(SHARED_CONTAINER_ID)) {
            std::cout << "🎉 Контейнер все еще существует после всех процессов!\n";
            
            // Открываем и проверяем финальное значение
            kcontainer::Container final_container = kcontainer::Container::open(SHARED_CONTAINER_ID);
            
            // Получаем информацию после процессов
            final_container.info(size, user_refs, kernel_refs);
            std::cout << "📊 После процессов: пользователей=" << user_refs 
                      << ", ядерных ссылок=" << kernel_refs << "\n";
            
            kcontainer::Mapping final_mapping = final_container.map();
            
            if (final_mapping.ptr) {
                int* final_counter = static_cast<int*>(final_mapping.ptr);
                std::cout << "🏁 Финальное значение счетчика: " << *final_counter << "\n";
                munmap(final_mapping.ptr, final_mapping.size);
            }
            
            final_container.close();
        } else {
            std::cout << "❌ Контейнер был удален\n";
        }
        
        std::cout << "=== Демонстрация завершена ===\n";
        
    } catch (const std::exception& e) {
        std::cerr << "❌ Ошибка: " << e.what() << "\n";
        return 1;
    }
    return 0;
}