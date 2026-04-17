#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <string.h>
#include <sys/stat.h>

void list_dir(const char *path, int level) {
    DIR *dir = opendir(path);
    if (!dir) {
        perror("opendir");
        return;
    }

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            continue;

        for (int i = 0; i < level; i++) printf("  ");

        if (entry->d_type == DT_DIR) {
            printf("[dir] %s\n", entry->d_name);
            
	    char subpath[1024];
            snprintf(subpath, sizeof(subpath), "%s/%s", path, entry->d_name);
            list_dir(subpath, level + 1);
        } else if (entry->d_type == DT_REG) {
            printf("[file] %s\n", entry->d_name);
        } else {
            printf("[other] %s\n", entry->d_name);
        }
    }
    closedir(dir);
}

int main(int argc, char *argv[]) {
    const char *dir = (argc > 1) ? argv[1] : ".";
    printf("root: %s\n\n", dir);
    list_dir(dir, 0);
    return 0;
}
