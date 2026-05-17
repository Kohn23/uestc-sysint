#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <string>
#include <vector>
#include <iostream>
#include <stdexcept>
#include <functional>
#include <unordered_map>

class Serializable {
public:
    virtual ~Serializable() = default;

    virtual bool WriteToFd(int fd) const = 0;
    virtual bool ReadFromFd(int fd) = 0;

    virtual int GetTypeId() const = 0;

    static std::unordered_map<int, std::function<Serializable*()>> creators;
    static Serializable* Create(int typeId) { return creators[typeId](); }
};

std::unordered_map<int, std::function<Serializable*()>> Serializable::creators;

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

class CC_LL : public Serializable {
    std::string name;
    std::vector<double> values;
    int id;
public:
    CC_LL(const std::string& n, const std::vector<double>& v, int i)
        : name(n), values(v), id(i) {}
    CC_LL() : name(""), values(), id(0) {}

    int GetTypeId() const override { return 2; }

    bool WriteToFd(int fd) const override {
        size_t len = name.size();
        if (write(fd, &len, sizeof(len)) != sizeof(len)) return false;
        if (!name.empty() && write(fd, name.data(), len) != (ssize_t)len) return false;

        size_t vecSize = values.size();
        if (write(fd, &vecSize, sizeof(vecSize)) != sizeof(vecSize)) return false;
        for (double d : values) {
            if (write(fd, &d, sizeof(d)) != sizeof(d)) return false;
        }

        if (write(fd, &id, sizeof(id)) != sizeof(id)) return false;
        return true;
    }

    bool ReadFromFd(int fd) override {
        size_t len;
        if (read(fd, &len, sizeof(len)) != sizeof(len)) return false;
        name.resize(len);
        if (len > 0 && read(fd, &name[0], len) != (ssize_t)len) return false;

        size_t vecSize;
        if (read(fd, &vecSize, sizeof(vecSize)) != sizeof(vecSize)) return false;
        values.resize(vecSize);
        for (size_t i = 0; i < vecSize; ++i) {
            if (read(fd, &values[i], sizeof(double)) != sizeof(double)) return false;
        }

        if (read(fd, &id, sizeof(id)) != sizeof(id)) return false;
        return true;
    }

    friend std::ostream& operator<<(std::ostream& os, const CC_LL& obj) {
        os << "Name: " << obj.name << ", Id: " << obj.id << ", Values: [";
        for (size_t i = 0; i < obj.values.size(); ++i) {
            if (i > 0) os << ", ";
            os << obj.values[i];
        }
        os << "]";
        return os;
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

void test_v4() {
    Serializable::creators.emplace(0, []() -> Serializable* { return new CA_LL(); });
    Serializable::creators.emplace(1, []() -> Serializable* { return new CB_LL(); });
    Serializable::creators.emplace(2, []() -> Serializable* { return new CC_LL(); });

    CA_LL a1("first", 10);
    CB_LL b1("second", 3.14);
    std::vector<double> vals = {1.1, 2.2, 3.3};
    CC_LL c1("extra", vals, 42);

    std::vector<const Serializable*> input = {&a1, &b1, &c1};

    Serializer ser;
    if (!ser.Serialize("v4_data", input)) {
        std::cerr << "Serialize failed!\n";
        return;
    }

    std::vector<Serializable*> output;
    if (!ser.Deserialize("v4_data", output)) {
        std::cerr << "Deserialize failed!\n";
        return;
    }

    std::cout << "[v4] ";
    for (auto* obj : output) {
        if (auto* p = dynamic_cast<CA_LL*>(obj))
            std::cout << "{" << *p << "} ";
        else if (auto* p = dynamic_cast<CB_LL*>(obj))
            std::cout << "{" << *p << "} ";
        else if (auto* p = dynamic_cast<CC_LL*>(obj))
            std::cout << "{" << *p << "} ";
    }
    std::cout << '\n';

    Serializer::FreeSerialized(output);
}

int main() {
    test_v4();
    return 0;
}
