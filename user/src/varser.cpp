#include "varser/varser.hpp"
#include <yaml-cpp/yaml.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <cstring>
#include <iostream>
#include <memory>
#include <vector>
#include <algorithm>
#include <errno.h> // Добавьте этот include
#include "varser_ioctl.h"

using namespace varser;

struct Container::Impl {
    ContainerDesc desc;
    int fd{-1};
    bool opened{false};
    Impl(const ContainerDesc &d): desc(d) {}
};

Container::Container(ContainerDesc desc)
    : p(std::make_unique<Impl>(desc)) {}

Container::~Container() {
    if (p->opened) close();
}

static uint8_t mapVarType(VarType t) {
    switch (t) {
        case VarType::INT32: return VARSER_TYPE_INT32;
        case VarType::INT64: return VARSER_TYPE_INT64;
        case VarType::UINT8: return VARSER_TYPE_UINT8;
        case VarType::UINT64: return VARSER_TYPE_UINT64;
        case VarType::FLOAT: return VARSER_TYPE_FLOAT;
        case VarType::DOUBLE: return VARSER_TYPE_DOUBLE;
        case VarType::STRING: return VARSER_TYPE_STRING;
        case VarType::BLOB: return VARSER_TYPE_BLOB;
    }
    return VARSER_TYPE_INT32;
}

bool Container::container_exists() {
    int fd = ::open("/dev/varser", O_RDWR);
    if (fd < 0) {
        perror("open /dev/varser");
        return false;
    }
    
    char buf[4096];
    memset(buf, 0, sizeof(buf));
    if (ioctl(fd, VARSER_IOC_LIST_CONTAINERS, buf) < 0) {
        perror("ioctl LIST_CONTAINERS");
        ::close(fd);
        return false;
    }
    
    ::close(fd);
    
    // Ищем имя контейнера в списке
    std::string containers_list(buf);
    return containers_list.find(p->desc.name) != std::string::npos;
}

bool Container::register_with_kernel() {
    // Сначала проверяем, существует ли контейнер
    if (container_exists()) {
        std::cout << "Container '" << p->desc.name << "' already exists, skipping registration\n";
        return true;
    }
    
    int fd = ::open("/dev/varser", O_RDWR);
    if (fd < 0) {
        perror("open /dev/varser");
        return false;
    }
    struct varser_register reg;
    memset(&reg, 0, sizeof(reg));
    strncpy(reg.container_name, p->desc.name.c_str(), VARSER_MAX_CONTAINER_NAME-1);
    reg.var_count = std::min<uint32_t>(p->desc.vars.size(), VARSER_MAX_VARS);
    for (uint32_t i = 0; i < reg.var_count; ++i) {
        const VarDesc &vd = p->desc.vars[i];
        strncpy(reg.vars[i].name, vd.name.c_str(), VARSER_MAX_VAR_NAME-1);
        reg.vars[i].type = mapVarType(vd.type);
        reg.vars[i].size = vd.size;
    }
    if (ioctl(fd, VARSER_IOC_REGISTER, &reg) != 0) {
        perror("ioctl REGISTER");
        ::close(fd);
        return false;
    }
    ::close(fd);
    std::cout << "Container '" << p->desc.name << "' registered successfully\n";
    return true;
}

bool Container::open() {
    if (p->opened) return true;
    p->fd = ::open("/dev/varser", O_RDWR);
    if (p->fd < 0) { perror("open /dev/varser"); return false; }
    char name[VARSER_MAX_CONTAINER_NAME] = {};
    strncpy(name, p->desc.name.c_str(), sizeof(name)-1);
    if (ioctl(p->fd, VARSER_IOC_OPEN_CONTAINER, name) != 0) {
        perror("ioctl OPEN_CONTAINER");
        ::close(p->fd);
        p->fd = -1;
        return false;
    }
    p->opened = true;
    return true;
}

bool Container::close() {
    if (!p->opened) return true;
    if (ioctl(p->fd, VARSER_IOC_CLOSE_CONTAINER) != 0) {
        perror("ioctl CLOSE_CONTAINER");
    }
    ::close(p->fd);
    p->fd = -1;
    p->opened = false;
    return true;
}

template<typename T>
bool Container::set(const std::string &varname, const T &value) {
    if (!p->opened && !open()) return false;
    struct varser_var_access access;
    memset(&access,0,sizeof(access));
    strncpy(access.container_name, p->desc.name.c_str(), VARSER_MAX_CONTAINER_NAME-1);
    strncpy(access.var_name, varname.c_str(), VARSER_MAX_VAR_NAME-1);
    access.buf_size = sizeof(T);
    T tmp = value;
    access.user_buf = (uintptr_t)&tmp;
    if (ioctl(p->fd, VARSER_IOC_SET_VAR, &access) != 0) {
        perror("ioctl SET_VAR");
        return false;
    }
    return true;
}

template<typename T>
bool Container::get(const std::string &varname, T &out) {
    if (!p->opened && !open()) return false;
    struct varser_var_access access;
    memset(&access,0,sizeof(access));
    strncpy(access.container_name, p->desc.name.c_str(), VARSER_MAX_CONTAINER_NAME-1);
    strncpy(access.var_name, varname.c_str(), VARSER_MAX_VAR_NAME-1);
    access.buf_size = sizeof(T);
    access.user_buf = (uintptr_t)&out;
    if (ioctl(p->fd, VARSER_IOC_GET_VAR, &access) != 0) {
        perror("ioctl GET_VAR");
        return false;
    }
    return true;
}

/* explicit instantiations for common types used in examples */
template bool Container::set<int64_t>(const std::string&, const int64_t&);
template bool Container::get<int64_t>(const std::string&, int64_t&);
template bool Container::set<double>(const std::string&, const double&);
template bool Container::get<double>(const std::string&, double&);

ContainerManager &ContainerManager::instance() {
    static ContainerManager mgr;
    return mgr;
}

ContainerManager::ContainerManager() {}

std::shared_ptr<Container> ContainerManager::load_from_yaml(const std::string &path) {
    try {
        YAML::Node root = YAML::LoadFile(path);
        ContainerDesc desc;
        desc.name = root["container"].as<std::string>();
        desc.lock_policy = root["lock_policy"].as<std::string>("per_variable_rw");
        
        if (!root["variables"]) {
            std::cerr << "No 'variables' section in YAML file: " << path << std::endl;
            return nullptr;
        }
        
        for (const auto &n : root["variables"]) {
            VarDesc vd;
            vd.name = n["name"].as<std::string>();
            std::string t = n["type"].as<std::string>();
            
            if (t == "int32") vd.type = VarType::INT32;
            else if (t == "int64") vd.type = VarType::INT64;
            else if (t == "uint8") vd.type = VarType::UINT8;
            else if (t == "uint64") vd.type = VarType::UINT64;
            else if (t == "float") vd.type = VarType::FLOAT;
            else if (t == "double") vd.type = VarType::DOUBLE;
            else if (t == "string") { 
                vd.type = VarType::STRING; 
                vd.size = n["size"] ? n["size"].as<uint32_t>() : 256; 
            }
            else if (t == "blob") { 
                vd.type = VarType::BLOB; 
                vd.size = n["size"].as<uint32_t>(); 
            }
            else { 
                std::cerr << "Unknown type: " << t << " for variable " << vd.name << std::endl;
                vd.type = VarType::INT32;
            }
            
            desc.vars.push_back(vd);
        }
        
        auto c = std::make_shared<Container>(desc);
        if (!c->register_with_kernel()) {
            std::cerr << "Failed to register container\n";
            return nullptr;
        }
        return c;
    } catch (const YAML::Exception &e) {
        std::cerr << "YAML parsing error in " << path << ": " << e.what() << std::endl;
        return nullptr;
    } catch (const std::exception &e) {
        std::cerr << "Error loading YAML file " << path << ": " << e.what() << std::endl;
        return nullptr;
    }
}