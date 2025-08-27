#include "kcontainer.hpp"
#include <stdexcept>
#include <system_error>
#include <vector>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <cstring>

extern "C" {
#include "../kernel/kcontainer_uapi.h"
}

namespace {

int dev_open()
{
    int fd = ::open("/dev/" KCONTAINER_DEV_NAME, O_RDWR);
    if (fd < 0) throw std::system_error(errno, std::generic_category(), "open /dev/kcontainer");
    return fd;
}

static size_t page_align(size_t n) {
    size_t p = sysconf(_SC_PAGESIZE);
    return ((n + p - 1) / p) * p;
}

}

namespace kcontainer {

Container::Container(uint64_t id, int fd, size_t size) : id_(id), fd_(fd), size_(size) {}

Container::~Container() { close(); }

Container Container::create(uint64_t id, size_t size)
{
    int ctl = dev_open();
    kc_create_req req{};
    req.id = id;
    req.size = size;
    req.flags = KC_FLAG_NONE;

    if (ioctl(ctl, KC_IOCTL_CREATE, &req) < 0) {
        int e = errno;
        ::close(ctl);
        throw std::system_error(e, std::generic_category(), "KC_IOCTL_CREATE");
    }

    int64_t fd = ioctl(ctl, KC_IOCTL_GET_FD, &id);
    int e = errno;
    ::close(ctl);
    if (fd < 0) throw std::system_error(e, std::generic_category(), "KC_IOCTL_GET_FD");

    return Container(id, (int)fd, size);
}

Container Container::open(uint64_t id)
{
    int ctl = dev_open();
    int64_t fd = ioctl(ctl, KC_IOCTL_GET_FD, &id);
    int e = errno;
    if (fd < 0) {
        ::close(ctl);
        throw std::system_error(e, std::generic_category(), "KC_IOCTL_GET_FD");
    }

    // Получаем информацию о контейнере для определения размера
    kc_info info{};
    info.id = id;
    size_t size = 0;
    if (ioctl(ctl, KC_IOCTL_INFO, &info) >= 0) {
        size = info.size;
    }
    
    ::close(ctl);
    return Container(id, (int)fd, size);
}

Mapping Container::map()
{
    Mapping m;
    if (fd_ < 0) return m;

    m.size = page_align(size_);
    m.ptr = mmap(nullptr, m.size, PROT_READ | PROT_WRITE, MAP_SHARED, fd_, 0);
    if (m.ptr == MAP_FAILED) {
        m.ptr = nullptr;
        m.size = 0;
    }
    return m;
}

void Container::close()
{
    if (fd_ >= 0) {
        ::close(fd_);
        fd_ = -1;
    }
}

bool Container::info(size_t& size_out, uint32_t& user_refs_out, uint32_t& kernel_refs_out) const
{
    if (fd_ < 0) return false;

    int ctl = dev_open();  // Нужно использовать control device для запроса информации
    kc_info info{};
    info.id = id_;
    
    if (ioctl(ctl, KC_IOCTL_INFO, &info) < 0) {
        ::close(ctl);
        return false;
    }
    
    size_out = info.size;
    user_refs_out = info.user_refs;
    kernel_refs_out = info.kernel_refs;
    
    ::close(ctl);
    return true;
}

bool Container::force_destroy(uint64_t id)
{
    int ctl = dev_open();
    if (ioctl(ctl, KC_IOCTL_FORCE_DESTROY, &id) < 0) {
        int e = errno;
        ::close(ctl);
        return false;
    }
    ::close(ctl);
    return true;
}

Container::ContainerStats Container::get_stats(uint64_t id)
{
    ContainerStats stats{};
    stats.id = id;
    
    try {
        int ctl = dev_open();
        kc_info info{};
        info.id = id;
        
        if (ioctl(ctl, KC_IOCTL_INFO, &info) >= 0) {
            stats.size = info.size;
            stats.user_refs = info.user_refs;
            stats.kernel_refs = info.kernel_refs;
            stats.exists = true;
        }
        ::close(ctl);
    } catch (...) {
        // Контейнер не существует
        stats.exists = false;
    }
    
    return stats;
}

bool Container::exists(uint64_t id)
{
    return get_stats(id).exists;
}

// RAII обертка
ScopedContainer::ScopedContainer(uint64_t id, size_t size) {
    if (size > 0) {
        try {
            container_ = Container::create(id, size);
            created_ = true;
        } catch (...) {
            // Пытаемся открыть существующий
            container_ = Container::open(id);
        }
    } else {
        container_ = Container::open(id);
    }
}

ScopedContainer::~ScopedContainer() {
    container_.close();
}

} // namespace kcontainer