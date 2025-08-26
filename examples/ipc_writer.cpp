#include "kcontainer.hpp"
#include <iostream>
#include <cstring>
#include <sys/mman.h>
#include <unistd.h>

int main() {
    try {
        std::cout << "🖊️  ПРОЦЕСС-ПИСАТЕЛЬ (PID: " << getpid() << ")\n";
        
        // Создаем общий контейнер
        kcontainer::Container container = kcontainer::Container::create(777, 4096);
        std::cout << "✅ Общий контейнер ID: 777 создан, размер: " << container.size() << " байт\n";
        
        // Получаем информацию о контейнере (ИСПРАВЛЕНО)
        size_t size;
        uint32_t user_refs, kernel_refs;
        if (container.info(size, user_refs, kernel_refs)) {
            std::cout << "📊 Пользовательских процессов: " << user_refs << "\n";
        }
        
        kcontainer::Mapping mapping = container.map();
        if (mapping.ptr) {
            // Записываем данные в общую память
            const char* data = "Данные от процесса-писателя!";
            std::memcpy(mapping.ptr, data, std::strlen(data) + 1);
            std::cout << "📝 Записано в общую память: " << (char*)mapping.ptr << "\n";
            
            // Ждем, чтобы reader успел прочитать
            std::cout << "⏳ Ожидание 2 секунды...\n";
            sleep(2);
            
            // Читаем ответ от reader-а (если он успел записать)
            std::cout << "📖 Ответ от reader-а: " << (char*)mapping.ptr << "\n";
            
            munmap(mapping.ptr, mapping.size);
        }
        
        container.close();
        std::cout << "✅ Писатель завершил работу\n";
        
    } catch (const std::exception& e) {
        std::cerr << "❌ Ошибка писателя: " << e.what() << "\n";
        return 1;
    }
    return 0;
}