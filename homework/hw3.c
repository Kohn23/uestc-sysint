#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>

typedef 


void parse_args(int argc, char *argv[], *options opts){


void mode_to_str(mode_t mode, char *str) {
    // file type 
    if (S_ISREG(mode))      str[0] = '-';
    else if (S_ISDIR(mode)) str[0] = 'd';
    else if (S_ISLNK(mode)) str[0] = 'l';
    else if (S_ISCHR(mode)) str[0] = 'c';
    else if (S_ISBLK(mode)) str[0] = 'b';
    else if (S_ISFIFO(mode))str[0] = 'p';
    else if (S_ISSOCK(mode))str[0] = 's';
    else                    str[0] = '?';

    // owner  
    str[1] = (mode & S_IRUSR) ? 'r' : '-';
    str[2] = (mode & S_IWUSR) ? 'w' : '-';
    str[3] = (mode & S_IXUSR) ? 'x' : '-';
    // group 
    str[4] = (mode & S_IRGRP) ? 'r' : '-';
    str[5] = (mode & S_IWGRP) ? 'w' : '-';
    str[6] = (mode & S_IXGRP) ? 'x' : '-';
    // other
    str[7] = (mode & S_IROTH) ? 'r' : '-';
    str[8] = (mode & S_IWOTH) ? 'w' : '-';
    str[9] = (mode & S_IXOTH) ? 'x' : '-';
    str[10] = '\0';
}


int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "用法: %s <文件名1> [文件名2] ...\n", argv[0]);
        return 1;
    }

    for (int i = 1; i < argc; i++) {
        struct stat st;
        if (stat(argv[i], &st) == -1) {
            perror("stat");
            continue;
        }

        char mode_str[11];
        mode_to_str(st.st_mode, mode_str);

        printf("%s %8ld %s\n", mode_str, (long)st.st_size, argv[i]);
    }
    return 0;
}
