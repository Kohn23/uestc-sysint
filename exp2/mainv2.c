#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <dlfcn.h>

typedef void (*print_func_t)(void);

int main() {
    const char *plugin_dir = "./plugins";
    DIR *dir;
    struct dirent *entry;

    dir = opendir(plugin_dir);
    if (dir == NULL) {
        fprintf(stderr, "无法打开插件目录: %s\n", plugin_dir);
        return 1;
    }

    while ((entry = readdir(dir)) != NULL) {
        const char *name = entry->d_name;
        size_t len = strlen(name);
        if (len < 3 || strcmp(name + len - 3, ".so") != 0) {
            continue; 
        }

        char libpath[512];
        snprintf(libpath, sizeof(libpath), "%s/%s", plugin_dir, name);

        void *handle = dlopen(libpath, RTLD_LAZY);
        if (!handle) {
            fprintf(stderr, "无法加载插件 %s: %s\n", libpath, dlerror());
            continue;
        }

        dlerror();

        print_func_t print_msg = (print_func_t) dlsym(handle, "print_message");
        const char *error = dlerror();
        if (error != NULL) {
            fprintf(stderr, "在插件 %s 中找不到 print_message: %s\n", libpath, error);
            dlclose(handle);
            continue;
        }

        print_msg();

        dlclose(handle);
    }

    closedir(dir);
    return 0;
}
