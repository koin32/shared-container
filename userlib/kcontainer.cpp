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


Container::Container(uint64_t id, int fd, size_t size): id_(id), fd_(fd), size_(size) {}


Container::~Container() { close(); }


Container Container::create(uint64_t id, size_t size)
{
int ctl = dev_open();
kc_create_req req{};
req.id = id;
req.size = size;
req.flags = KC_FLAG_NONE;
req.mode = 0666;


if (ioctl(ctl, KC_IOCTL_CREATE, &req) < 0) {
int e = errno;
::close(ctl);
throw std::system_error(e, std::generic_category(), "KC_IOCTL_CREATE");
}


// Получить fd контейнера
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
::close(ctl);
if (fd < 0) throw std::system_error(e,