#include "kcontainer.hpp"
#include <iostream>
#include <cstring>
#include <sys/mman.h>
#include <unistd.h>
#include <thread>

int main() {
    try {
        std::cout << "📖 ПРОЦЕСС-ЧИТАТЕЛЬ (PID: " << getpid() << ")\n";
        
        // Ждем немного, чтобы writer успел создать контейнер
        std::cout << "⏳ Ожидание создания контейнера...\n";
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        
        // Пытаемся открыть контейнер несколько раз
        kcontainer::Container container;
        int attempts = 5;
        
        for (int i = 0; i < attempts; ++i) {
            try {
                container = kcontainer::Container::open(777);
                std::cout << "✅ Контейнер ID: 777 успешно открыт\n";
                
                // Получаем информацию (ИСПРАВЛЕНО)
                size_t size;
                uint32_t user_refs, kernel_refs;
                if (container.info(size, user_refs, kernel_refs)) {
                    std::cout << "📊 Пользовательских процессов: " << user_refs << "\n";
                }
                break;
            } catch (const std::exception& e) {
                if (i == attempts - 1) {
                    throw;
                }
                std::cout << "⌛ Контейнер еще не готов, ждем... (" << i+1 << "/" << attempts << ")\n";
                std::this_thread::sleep_for(std::chrono::milliseconds(200));
            }
        }
        
        kcontainer::Mapping mapping = container.map();
        if (mapping.ptr) {
            // Читаем данные от писателя
            std::cout << "📖 Прочитано из общей памяти: " << (char*)mapping.ptr << "\n";
            
            // Отвечаем писателю
            const char* response = "Привет от процесса-читателя!";
            std::memcpy(mapping.ptr, response, std::strlen(response) + 1);
            std::cout << "📝 Отправлен ответ писателю\n";
            
            // Даем время писателю прочитать ответ
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
            
            munmap(mapping.ptr, mapping.size);
        }
        
        container.close();
        std::cout << "✅ Читатель завершил работу\n";
        
    } catch (const std::exception& e) {
        std::cerr << "❌ Ошибка читателя: " << e.what() << "\n";
        return 1;
    }
    return 0;
}