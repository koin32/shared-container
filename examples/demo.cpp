#include "kcontainer.hpp"
#include <iostream>
#include <cstring>
#include <sys/mman.h>

int main() {
    try {
        std::cout << "=== Демонстрация работы kcontainer ===\n";
        
        // 1. Создаем контейнер размером 1MB
        kcontainer::Container container = kcontainer::Container::create(123, 1024 * 1024);
        std::cout << "✅ Создан контейнер ID: " << container.id() 
                  << ", размер: " << container.size() << " байт\n";
        
        // 2. Маппим контейнер в память
        kcontainer::Mapping mapping = container.map();
        if (mapping.ptr) {
            std::cout << "✅ Контейнер отображен по адресу: " << mapping.ptr 
                      << ", размер mapping: " << mapping.size << " байт\n";
            
            // 3. Работаем с данными в контейнере
            // Записываем данные
            const char* message = "Hello from kcontainer!";
            std::memcpy(mapping.ptr, message, std::strlen(message) + 1);
            std::cout << "✅ Данные записаны в контейнер\n";
            
            // Читаем данные
            std::cout << "📖 Прочитанные данные: " << (char*)mapping.ptr << "\n";
            
            // 4. Освобождаем mapping
            munmap(mapping.ptr, mapping.size);
            std::cout << "✅ Mapping освобожден\n";
        }
        
        // 5. Закрываем контейнер (но он продолжает существовать в ядре)
        container.close();
        std::cout << "✅ Дескриптор контейнера закрыт\n";
        
        // 6. Открываем существующий контейнер
        kcontainer::Container reopened = kcontainer::Container::open(123);
        std::cout << "✅ Контейнер переоткрыт, размер: " << reopened.size() << " байт\n";
        
        // 7. Получаем информацию о контейнере
        size_t size;
        uint32_t refcnt;
        if (reopened.info(size, refcnt)) {
            std::cout << "📊 Информация о контейнере:\n";
            std::cout << "   - Размер: " << size << " байт\n";
            std::cout << "   - Количество ссылок: " << refcnt << "\n";
        }
        
        reopened.close();
        std::cout << "=== Демонстрация завершена ===\n";
        
    } catch (const std::exception& e) {
        std::cerr << "❌ Ошибка: " << e.what() << "\n";
        return 1;
    }
    return 0;
}