#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>

int main() {
    int fd = open("test", O_RDWR | O_APPEND | O_TRUNC, 0644);
    if (fd == -1) return 1;

    write(fd, "First word", 10);

    lseek(fd, 0, SEEK_SET);
    char buf[100]={'\0'};
    ssize_t n = read(fd, buf, 100);
    printf("read returns %ld bytes at: ^%s \n", n, buf);

    write(fd, "Second word", 12);

    n = read(fd, buf, 100);
    printf("read returns %ld\n", n);  
	
    lseek(fd, 0, SEEK_SET);
    n = read(fd, buf, 100);
    printf("read returns %ld bytes at: ^%s \n", n, buf);
    
    close(fd);
    return 0;
}
