#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <string>
#include <vector>
#include <iostream>

class CA_LL {
    std::string key;
    int val;
public:
    CA_LL(const std::string& k, int v) : key(k), val(v) {}
    CA_LL() : key(""), val(0) {}

    bool Serialize(const char* pFilePath) const;
    bool Deserialize(const char* pFilePath);

    // 新增:提取write/read fd的子逻辑
    bool WriteToFd(int fd) const;
    bool ReadFromFd(int fd);

    friend std::ostream& operator<<(std::ostream& os, const CA_LL& obj);
};

std::ostream& operator<<(std::ostream& os, const CA_LL& obj) {
    os << "Key: " << obj.key << ", Val: " << obj.val;
    return os;
}

bool CA_LL::WriteToFd(int fd) const {
    size_t len = key.size();
    if (write(fd, &len, sizeof(len)) != sizeof(len))
        return false;
    if (!key.empty() && write(fd, key.data(), len) != static_cast<ssize_t>(len))
        return false;
    if (write(fd, &val, sizeof(val)) != sizeof(val))
        return false;
    return true;
}

bool CA_LL::ReadFromFd(int fd) {
    size_t len;
    if (read(fd, &len, sizeof(len)) != sizeof(len))
        return false;
    key.resize(len);
    if (len > 0 && read(fd, &key[0], len) != static_cast<ssize_t>(len))
        return false;
    if (read(fd, &val, sizeof(val)) != sizeof(val))
        return false;
    return true;
}

bool CA_LL::Serialize(const char* pFilePath) const {
    int fd = open(pFilePath, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd < 0) return false;
    bool ok = WriteToFd(fd);
    close(fd);
    return ok;
}

bool CA_LL::Deserialize(const char* pFilePath) {
    int fd = open(pFilePath, O_RDONLY);
    if (fd < 0) return false;
    bool ok = ReadFromFd(fd);
    close(fd);
    return ok;
}

class SerializerForCA_LLs {
public:
    bool Serialize(const char* pFilePath, const std::vector<CA_LL>& v);
    bool Deserialize(const char* pFilePath, std::vector<CA_LL>& v);
};

bool SerializerForCA_LLs::Serialize(const char* pFilePath, const std::vector<CA_LL>& v) {
    int fd = open(pFilePath, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd < 0) return false;

    // 写入元素个数
    size_t count = v.size();
    if (write(fd, &count, sizeof(count)) != sizeof(count)) {
        close(fd);
        return false;
    }

    // 依次写入每个 CA_LL 对象
    for (const auto& obj : v) {
        if (!obj.WriteToFd(fd)) {
            close(fd);
            return false;
        }
    }
    close(fd);
    return true;
}

bool SerializerForCA_LLs::Deserialize(const char* pFilePath, std::vector<CA_LL>& v) {
    int fd = open(pFilePath, O_RDONLY);
    if (fd < 0) return false;

    // 读取元素个数
    size_t count;
    if (read(fd, &count, sizeof(count)) != sizeof(count)) {
        close(fd);
        return false;
    }

    v.clear();
    v.reserve(count);
    for (size_t i = 0; i < count; ++i) {
        CA_LL obj;
        if (!obj.ReadFromFd(fd)) {
            close(fd);
            return false;
        }
        v.push_back(std::move(obj));
    }
    close(fd);
    return true;
}

void test_v1() {
    {
        CA_LL a("example", 2026);
        a.Serialize("v1_data");
    }
    {
        CA_LL b;
        b.Deserialize("v1_data");
        std::cout << "[v1]" << b << std::endl;
    }
}

void test_v2(){
    {
        std::vector<CA_LL> vec = {
            {"alpha", 100},
            {"beta",  200},
            {"gamma", 300}
        };
        SerializerForCA_LLs s;
        s.Serialize("v2_data", vec);
    }
    {
        std::vector<CA_LL> vec;
        SerializerForCA_LLs s;
        s.Deserialize("v2_data", vec);
        std::cout << "[v2] ";
        for (const auto& obj : vec)
            std::cout << "{" << obj << "} ";
        std::cout << std::endl;
    }
}
    

int main() {
    test_v1();
    test_v2();
    return 0;
}
