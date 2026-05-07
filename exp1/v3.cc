#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <string>
#include <vector>
#include <iostream>
#include <stdexcept>

class Serializable {
public:
    virtual ~Serializable() = default;

    virtual bool WriteToFd(int fd) const = 0;
    virtual bool ReadFromFd(int fd) = 0;

    virtual int GetTypeId() const = 0;

    // 新增子类时，这里添加case
    static Serializable* Create(int typeId); 
};

class CA_LL : public Serializable {
    std::string key;
    int val;
public:
    CA_LL(const std::string& k, int v) : key(k), val(v) {}
    CA_LL() : key(""), val(0) {}

    int GetTypeId() const override { return 0; }

    bool WriteToFd(int fd) const override {
        size_t len = key.size();
        if (write(fd, &len, sizeof(len)) != sizeof(len)) return false;
        if (!key.empty() && write(fd, key.data(), len) != (ssize_t)len) return false;
        if (write(fd, &val, sizeof(val)) != sizeof(val)) return false;
        return true;
    }

    bool ReadFromFd(int fd) override {
        size_t len;
        if (read(fd, &len, sizeof(len)) != sizeof(len)) return false;
        key.resize(len);
        if (len > 0 && read(fd, &key[0], len) != (ssize_t)len) return false;
        if (read(fd, &val, sizeof(val)) != sizeof(val)) return false;
        return true;
    }

    friend std::ostream& operator<<(std::ostream& os, const CA_LL& obj) {
        return os << "Key: " << obj.key << ", Val: " << obj.val;
    }
};

class CB_LL : public Serializable {
    std::string description;
    double value;
public:
    CB_LL(const std::string& desc, double v) : description(desc), value(v) {}
    CB_LL() : description(""), value(0.0) {}

    int GetTypeId() const override { return 1; }

    bool WriteToFd(int fd) const override {
        size_t len = description.size();
        if (write(fd, &len, sizeof(len)) != sizeof(len)) return false;
        if (!description.empty() && write(fd, description.data(), len) != (ssize_t)len) return false;
        if (write(fd, &value, sizeof(value)) != sizeof(value)) return false;
        return true;
    }

    bool ReadFromFd(int fd) override {
        size_t len;
        if (read(fd, &len, sizeof(len)) != sizeof(len)) return false;
        description.resize(len);
        if (len > 0 && read(fd, &description[0], len) != (ssize_t)len) return false;
        if (read(fd, &value, sizeof(value)) != sizeof(value)) return false;
        return true;
    }

    friend std::ostream& operator<<(std::ostream& os, const CB_LL& obj) {
        return os << "Desc: " << obj.description << ", Value: " << obj.value;
    }
};

class Serializer {
public:
    bool Serialize(const char* pFilePath,
                   const std::vector<const Serializable*>& v) {
        int fd = open(pFilePath, O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (fd < 0) return false;

        size_t count = v.size();
        if (write(fd, &count, sizeof(count)) != sizeof(count)) {
            close(fd); return false;
        }

        for (const auto* obj : v) {
            int typeId = obj->GetTypeId();
            if (write(fd, &typeId, sizeof(typeId)) != sizeof(typeId) ||
                !obj->WriteToFd(fd)) {
                close(fd); return false;
            }
        }

        close(fd);
        return true;
    }

    bool Deserialize(const char* pFilePath,
                     std::vector<Serializable*>& v) {
        int fd = open(pFilePath, O_RDONLY);
        if (fd < 0) return false;

        size_t count;
        if (read(fd, &count, sizeof(count)) != sizeof(count)) {
            close(fd); return false;
        }

        v.clear();
        v.reserve(count);
        for (size_t i = 0; i < count; ++i) {
            int typeId;
            if (read(fd, &typeId, sizeof(typeId)) != sizeof(typeId)) {
                Cleanup(v); close(fd); return false;
            }

            try {
                Serializable* obj = Serializable::Create(typeId);
                if (!obj->ReadFromFd(fd)) {
                    delete obj;
                    Cleanup(v);
                    close(fd);
                    return false;
                }
                v.push_back(obj);
            } catch (const std::exception&) {
                Cleanup(v);
                close(fd);
                return false;
            }
        }

        close(fd);
        return true;
    }

    static void FreeSerialized(std::vector<Serializable*>& v) {
        for (auto* p : v) delete p;
        v.clear();
    }

private:
    static void Cleanup(std::vector<Serializable*>& v) {
        FreeSerialized(v);
    }
};

Serializable* Serializable::Create(int typeId) {
        switch (typeId) {
            case 0:  return new CA_LL();
            case 1:  return new CB_LL();
            default:
                throw std::runtime_error("Unknown type ID: " +
                                         std::to_string(typeId));
    }
}

void test_v3() {
    CA_LL a1("first", 10);
    CB_LL b1("second", 3.14);
    CA_LL a2("third", 20);

    std::vector<const Serializable*> input = {&a1, &b1, &a2};

    Serializer ser;
    if (!ser.Serialize("v3_data", input)) {
        std::cerr << "Serialize failed!\n";
        return;
    }

    std::vector<Serializable*> output;
    if (!ser.Deserialize("v3_data", output)) {
        std::cerr << "Deserialize failed!\n";
        return;
    }

    std::cout << "[v3] ";
    for (auto* obj : output) {
        if (auto* p = dynamic_cast<CA_LL*>(obj))
            std::cout << "{" << *p << "} ";
        else if (auto* p = dynamic_cast<CB_LL*>(obj))
            std::cout << "{" << *p << "} ";
    }
    std::cout << '\n';

    Serializer::FreeSerialized(output);
}

int main() {
    test_v3();
    return 0;
}
