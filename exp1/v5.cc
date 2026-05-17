#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <string>
#include <vector>
#include <iostream>
#include <stdexcept>
#include <functional>
#include <unordered_map>
#include <cstring>

class Writer {
public:
    virtual ~Writer() = default;
    virtual bool write(const void* buf, size_t len) = 0;
};

class Reader {
public:
    virtual ~Reader() = default;
    virtual bool read(void* buf, size_t len) = 0;
};

class FileWriter : public Writer {
    int fd_;
    bool owner_;
public:
    explicit FileWriter(int fd, bool owner = false) : fd_(fd), owner_(owner) {}
    FileWriter(const char* path, int flags = O_WRONLY | O_CREAT | O_TRUNC, mode_t mode = 0644) {
        fd_ = ::open(path, flags, mode);
        if (fd_ < 0) throw std::runtime_error("open file failed");
        owner_ = true;
    }
    ~FileWriter() override { if (owner_ && fd_ >= 0) ::close(fd_); }

    bool write(const void* buf, size_t len) override {
        const char* data = static_cast<const char*>(buf);
        ssize_t written = 0;
        while (written < static_cast<ssize_t>(len)) {
            ssize_t ret = ::write(fd_, data + written, len - written);
            if (ret <= 0) return false;
            written += ret;
        }
        return true;
    }
};

class FileReader : public Reader {
    int fd_;
    bool owner_;
public:
    explicit FileReader(int fd, bool owner = false) : fd_(fd), owner_(owner) {}
    FileReader(const char* path) {
        fd_ = ::open(path, O_RDONLY);
        if (fd_ < 0) throw std::runtime_error("open file failed");
        owner_ = true;
    }
    ~FileReader() override { if (owner_ && fd_ >= 0) ::close(fd_); }

    bool read(void* buf, size_t len) override {
        char* data = static_cast<char*>(buf);
        ssize_t read_bytes = 0;
        while (read_bytes < static_cast<ssize_t>(len)) {
            ssize_t ret = ::read(fd_, data + read_bytes, len - read_bytes);
            if (ret <= 0) return false;
            read_bytes += ret;
        }
        return true;
    }
};

class MemoryWriter : public Writer {
    std::vector<char> data_;
public:
   bool write(const void* buf, size_t len) override {
        const char* src = static_cast<const char*>(buf);
        data_.insert(data_.end(), src, src + len);
        return true;
    }
    const std::vector<char>& getData() const { return data_; }
};

class MemoryReader : public Reader {
    const std::vector<char>& data_;
    size_t pos_;
public:
    explicit MemoryReader(const std::vector<char>& data) : data_(data), pos_(0) {}
    bool read(void* buf, size_t len) override {
        if (pos_ + len > data_.size()) return false;
        std::memcpy(buf, data_.data() + pos_, len);
        pos_ += len;
        return true;
    }
};

class SocketWriter : public Writer {
    int sockfd_;
public:
    explicit SocketWriter(int sockfd) : sockfd_(sockfd) {}
    bool write(const void* buf, size_t len) override {
        const char* data = static_cast<const char*>(buf);
        ssize_t sent = 0;
        while (sent < static_cast<ssize_t>(len)) {
            ssize_t ret = ::send(sockfd_, data + sent, len - sent, 0);
            if (ret <= 0) return false;
            sent += ret;
        }
        return true;
    }
};

class SocketReader : public Reader {
    int sockfd_;
public:
    explicit SocketReader(int sockfd) : sockfd_(sockfd) {}
    bool read(void* buf, size_t len) override {
        char* data = static_cast<char*>(buf);
        ssize_t recvd = 0;
        while (recvd < static_cast<ssize_t>(len)) {
            ssize_t ret = ::recv(sockfd_, data + recvd, len - recvd, 0);
            if (ret <= 0) return false;
            recvd += ret;
        }
        return true;
    }
};

class Serializable {
public:
    virtual ~Serializable() = default;

    virtual bool WriteToWriter(Writer& writer) const = 0;
    virtual bool ReadFromReader(Reader& reader) = 0;

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

    bool WriteToWriter(Writer& writer) const override {
        size_t len = key.size();
        if (!writer.write(&len, sizeof(len))) return false;
        if (!key.empty() && !writer.write(key.data(), len)) return false;
        if (!writer.write(&val, sizeof(val))) return false;
        return true;
    }

    bool ReadFromReader(Reader& reader) override {
        size_t len;
        if (!reader.read(&len, sizeof(len))) return false;
        key.resize(len);
        if (len > 0 && !reader.read(&key[0], len)) return false;
        if (!reader.read(&val, sizeof(val))) return false;
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

    bool WriteToWriter(Writer& writer) const override {
        size_t len = description.size();
        if (!writer.write(&len, sizeof(len))) return false;
        if (!description.empty() && !writer.write(description.data(), len)) return false;
        if (!writer.write(&value, sizeof(value))) return false;
        return true;
    }

    bool ReadFromReader(Reader& reader) override {
        size_t len;
        if (!reader.read(&len, sizeof(len))) return false;
        description.resize(len);
        if (len > 0 && !reader.read(&description[0], len)) return false;
        if (!reader.read(&value, sizeof(value))) return false;
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

    bool WriteToWriter(Writer& writer) const override {
        size_t len = name.size();
        if (!writer.write(&len, sizeof(len))) return false;
        if (!name.empty() && !writer.write(name.data(), len)) return false;

        size_t vecSize = values.size();
        if (!writer.write(&vecSize, sizeof(vecSize))) return false;
        for (double d : values) {
            if (!writer.write(&d, sizeof(d))) return false;
        }

        if (!writer.write(&id, sizeof(id))) return false;
        return true;
    }

    bool ReadFromReader(Reader& reader) override {
        size_t len;
        if (!reader.read(&len, sizeof(len))) return false;
        name.resize(len);
        if (len > 0 && !reader.read(&name[0], len)) return false;

        size_t vecSize;
        if (!reader.read(&vecSize, sizeof(vecSize))) return false;
        values.resize(vecSize);
        for (size_t i = 0; i < vecSize; ++i) {
            if (!reader.read(&values[i], sizeof(double))) return false;
        }

        if (!reader.read(&id, sizeof(id))) return false;
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
    bool Serialize(Writer& writer, const std::vector<const Serializable*>& v) {
        size_t count = v.size();
        if (!writer.write(&count, sizeof(count))) return false;

        for (const auto* obj : v) {
            int typeId = obj->GetTypeId();
            if (!writer.write(&typeId, sizeof(typeId))) return false;
            if (!obj->WriteToWriter(writer)) return false;
        }
        return true;   
    }

    bool Deserialize(Reader& reader, std::vector<Serializable*>& v) {
        size_t count;
        if (!reader.read(&count, sizeof(count))) return false;

        v.clear();
        v.reserve(count);
        for (size_t i = 0; i < count; ++i) {
            int typeId;
            if (!reader.read(&typeId, sizeof(typeId))) {
                Cleanup(v);
                return false;
            }

            try {
                Serializable* obj = Serializable::Create(typeId);
                if (!obj->ReadFromReader(reader)) {
                    delete obj;
                    Cleanup(v);
                    return false;
                }
                v.push_back(obj);
            } catch (const std::exception&) {
                Cleanup(v);
                return false;
            }
        }
        return true;
    }

    bool Serialize(const char* pFilePath, const std::vector<const Serializable*>& v) {
        try {
            FileWriter writer(pFilePath);
            return Serialize(writer, v);
        } catch (...) {
            return false;
        }
    }

    bool Deserialize(const char* pFilePath, std::vector<Serializable*>& v) {
        try {
            FileReader reader(pFilePath);
            return Deserialize(reader, v);
        } catch (...) {
            return false;
        }
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

void test_file() {
    Serializable::creators.emplace(0, []() -> Serializable* { return new CA_LL(); });
    Serializable::creators.emplace(1, []() -> Serializable* { return new CB_LL(); });
    Serializable::creators.emplace(2, []() -> Serializable* { return new CC_LL(); });

    CA_LL a1("first", 10);
    CB_LL b1("second", 3.14);
    std::vector<double> vals = {1.1, 2.2, 3.3};
    CC_LL c1("extra", vals, 42);

    std::vector<const Serializable*> input = {&a1, &b1, &c1};

    Serializer ser;
    if (!ser.Serialize("v5_data", input)) {
        std::cerr << "File serialize failed!\n";
        return;
    }

    std::vector<Serializable*> output;
    if (!ser.Deserialize("v5_data", output)) {
        std::cerr << "File deserialize failed!\n";
        return;
    }

    std::cout << "[File] ";
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

void test_memory() {
    Serializable::creators.emplace(0, []() -> Serializable* { return new CA_LL(); });
    Serializable::creators.emplace(1, []() -> Serializable* { return new CB_LL(); });
    Serializable::creators.emplace(2, []() -> Serializable* { return new CC_LL(); });

    CA_LL a1("mem_key", 123);
    CB_LL b1("mem_desc", 2.718);
    std::vector<double> vals = {0.1, 0.2};
    CC_LL c1("mem_name", vals, 99);

    std::vector<const Serializable*> input = {&a1, &b1, &c1};

    Serializer ser;
    MemoryWriter memWriter;
    if (!ser.Serialize(memWriter, input)) {
        std::cerr << "Memory serialize failed!\n";
        return;
    }

    MemoryReader memReader(memWriter.getData());
    std::vector<Serializable*> output;
    if (!ser.Deserialize(memReader, output)) {
        std::cerr << "Memory deserialize failed!\n";
        return;
    }

    std::cout << "[Memory] ";
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

void test_network() {
    int sv[2];
    if (socketpair(AF_UNIX, SOCK_STREAM, 0, sv) != 0) {
        std::cerr << "socketpair failed\n";
        return;
    }

    Serializable::creators.emplace(0, []() -> Serializable* { return new CA_LL(); });
    Serializable::creators.emplace(1, []() -> Serializable* { return new CB_LL(); });
    Serializable::creators.emplace(2, []() -> Serializable* { return new CC_LL(); });

    CA_LL a1("net_key", 777);
    CB_LL b1("net_desc", 1.414);
    std::vector<double> vals = {9.9, 8.8};
    CC_LL c1("net_name", vals, 66);

    std::vector<const Serializable*> input = {&a1, &b1, &c1};

    Serializer ser;
    SocketWriter writer(sv[0]);
    if (!ser.Serialize(writer, input)) {
        std::cerr << "Network serialize failed!\n";
        close(sv[0]); close(sv[1]);
        return;
    }

    SocketReader reader(sv[1]);
    std::vector<Serializable*> output;
    if (!ser.Deserialize(reader, output)) {
        std::cerr << "Network deserialize failed!\n";
        close(sv[0]); close(sv[1]);
        return;
    }

    std::cout << "[Network] ";
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
    close(sv[0]); close(sv[1]);
}

int main() {
    test_file();
    test_memory();
    test_network();
    return 0;
}
