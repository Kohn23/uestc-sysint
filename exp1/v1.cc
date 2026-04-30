#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <string>
#include <iostream>

class CA_LL {
    std::string key;
    int val;
public:
    CA_LL(const std::string& k, int v) : key(k), val(v) {}
    CA_LL() : key(""), val(0) {}

    bool Serialize(const char* pFilePath);
    bool Deserialize(const char* pFilePath);

    friend std::ostream& operator<<(std::ostream& os, const CA_LL& obj);
};

std::ostream& operator<<(std::ostream& os, const CA_LL& obj) {
    os << "Key: " << obj.key << ", Val: " << obj.val;
    return os;
}

bool CA_LL::Serialize(const char* pFilePath) {
    int fd = open(pFilePath, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd < 0) return false;

    // 写入 key 的长度
    size_t len = key.size();
    if (write(fd, &len, sizeof(len)) != sizeof(len)) {
        close(fd);
        return false;
    }
    // 写入 key 的字符数据
    if (!key.empty() && write(fd, key.data(), len) != static_cast<ssize_t>(len)) {
        close(fd);
        return false;
    }
    // 写入 val
    if (write(fd, &val, sizeof(val)) != sizeof(val)) {
        close(fd);
        return false;
    }
    close(fd);
    return true;
}

bool CA_LL::Deserialize(const char* pFilePath) {
    int fd = open(pFilePath, O_RDONLY);
    if (fd < 0) return false;

    // 读取 key 的长度
    size_t len;
    if (read(fd, &len, sizeof(len)) != sizeof(len)) {
        close(fd);
        return false;
    }
    // 读取 key 的字符数据
    key.resize(len);
    if (len > 0 && read(fd, &key[0], len) != static_cast<ssize_t>(len)) {
        close(fd);
        return false;
    }
    // 读取 val
    if (read(fd, &val, sizeof(val)) != sizeof(val)) {
        close(fd);
        return false;
    }
    close(fd);
    return true;
}

void test() {
    {
        CA_LL a("example", 2026);
        a.Serialize("v1_data");
    }
    {
        CA_LL b;
        b.Deserialize("v1_data");
        std::cout << b << std::endl;
    }
}

int main() {
    test();
    return 0;
}
