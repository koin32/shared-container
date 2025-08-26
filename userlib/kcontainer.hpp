#pragma once
#include <cstddef>
#include <cstdint>
#include <string>
#include <memory>
#include <vector>

namespace kcontainer {

struct Mapping {
    void* ptr = nullptr;
    size_t size = 0;
};

class Container {
public:
    Container() = default;
    ~Container();

    // Создать контейнер (если нет) и открыть дескриптор для mmap
    static Container create(uint64_t id, size_t size);

    // Открыть существующий контейнер и получить fd
    static Container open(uint64_t id);

    // Получить mmap региона (RW)
    Mapping map();

    // Размер (если известен после create или info)
    size_t size() const { return size_; }

    // Текущий id контейнера
    uint64_t id() const { return id_; }

    // Закрыть fd, оставить контейнер существовать
    void close();

    // Запросить информацию у ядра
    bool info(size_t& size_out, uint32_t& user_refs_out, uint32_t& kernel_refs_out) const;

    // Статистика контейнера
    struct ContainerStats {
        uint64_t id;
        size_t size;
        uint32_t user_refs;
        uint32_t kernel_refs;
        bool exists;
    };

    // Принудительное удаление контейнера
    static bool force_destroy(uint64_t id);

    // Получение статистики контейнера
    static ContainerStats get_stats(uint64_t id);

    // Проверка существования контейнера
    static bool exists(uint64_t id);

private:
    explicit Container(uint64_t id, int fd, size_t size);

    uint64_t id_ = 0;
    int fd_ = -1;
    size_t size_ = 0;
};

// RAII обертка для автоматического управления контейнером
class ScopedContainer {
public:
    ScopedContainer(uint64_t id, size_t size = 0);
    ~ScopedContainer();
    
    Container& get() { return container_; }
    operator Container&() { return container_; }
    
private:
    Container container_;
    bool created_{false};
};

// Высокоуровневый помощник: создать набор контейнеров из XML
// Возвращает число успешно подготовленных контейнеров
int ensure_from_xml(const std::string& xml_path);

// Получить список контейнеров из XML без создания
struct ContainerSpec {
    uint64_t id;
    size_t size;
};
std::vector<ContainerSpec> parse_xml(const std::string& xml_path);

} // namespace kcontainer