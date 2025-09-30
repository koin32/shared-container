#pragma once
#include <string>
#include <vector>
#include <memory>
#include <cstdint>

namespace varser {

enum class VarType {
    INT32, INT64, UINT8, UINT64, FLOAT, DOUBLE, STRING, BLOB
};

struct VarDesc {
    std::string name;
    VarType type;
    uint32_t size{0}; // for string/blob
};

struct ContainerDesc {
    std::string name;
    std::string lock_policy;
    std::vector<VarDesc> vars;
};

class Container {
public:
    Container(ContainerDesc desc);
    ~Container();

    bool register_with_kernel(); // calls IOCTL REGISTER
    bool open(); // OPEN_CONTAINER
    bool close(); // CLOSE_CONTAINER

    template<typename T>
    bool set(const std::string &varname, const T &value);

    template<typename T>
    bool get(const std::string &varname, T &out);

private:
    bool container_exists(); // Добавьте эту строку
    
    struct Impl;
    std::unique_ptr<Impl> p;
};

class ContainerManager {
public:
    static ContainerManager &instance();
    std::shared_ptr<Container> load_from_yaml(const std::string &path);
private:
    ContainerManager();
};

} // namespace varser