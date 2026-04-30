#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <cstring>
#include <fstream>
#include <iostream>

class CA_LL {
    int i;
public:
    CA_LL(int val):i(val){}
    CA_LL():i(0){}
    
    bool Serialize(const char* pFilePath);
    bool Deserialize(const char* pFilePath);

    friend std::ostream& operator<<(std::ostream& os, const CA_LL& obj); 
};

std::ostream& operator<<(std::ostream& os, const CA_LL& obj) {
    os << obj.i ;
    return os;
}

bool CA_LL::Serialize(const char* pFilePath) {
    int fd = open(pFilePath, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd < 0) return false;

    ssize_t written = write(fd, &i, sizeof(i));
    close(fd);
    return written == static_cast<ssize_t>(sizeof(i));
}

bool CA_LL::Deserialize(const char* pFilePath) {
    int fd = open(pFilePath, O_RDONLY);
    if (fd < 0) return false;

    ssize_t read_bytes = read(fd, &i, sizeof(i));
    close(fd);
    return read_bytes == static_cast<ssize_t>(sizeof(i));
}


void test(){
	{
		CA_LL a(2026);
		a.Serialize("v1_data");
	}
	{
		CA_LL b;
		b.Deserialize("v1_data");
		std::cout << b << std::endl;		
	}
}


int main(){
	test();
	return 0;
}
