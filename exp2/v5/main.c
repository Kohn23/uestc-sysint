#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <dlfcn.h>
#include "interface.h"

int main(int argc, char *argv[]) {
 
    const char *plugin_dir = "./plugins";
    const char *mode = argv[1];
    int is_help = (strcmp(mode, "help") == 0);

    DIR *dir = opendir(plugin_dir);
    if (!dir) {
        perror("Cannot open plugin directory");
        return 1;
    }

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        size_t len = strlen(entry->d_name);
        if (len < 3 || strcmp(entry->d_name + len - 3, ".so") != 0)
            continue;

        char path[512];
        snprintf(path, sizeof(path), "%s/%s", plugin_dir, entry->d_name);

        void *handle = dlopen(path, RTLD_LAZY);
        if (!handle) {
            fprintf(stderr, "dlopen %s failed: %s\n", path, dlerror());
            continue;
        }

        dlerror();
        get_plugin_interface_func_t get_iface =
            (get_plugin_interface_func_t) dlsym(handle, "get_plugin_interface");
        const char *err = dlerror();
        if (err || !get_iface) {
            fprintf(stderr, "Plugin %s lacks get_plugin_interface\n", path);
            dlclose(handle);
            continue;
        }

        struct PluginInterface *plg = get_iface();

        if (is_help) {
            printf("%s: %s\n", plg->get_id(), plg->get_desc());
            dlclose(handle);
        } else {
            if (strcmp(plg->get_id(), mode) == 0) {
                int result = plg->do_count(argv[2]);
                if (result < 0)
                    printf("Error processing file.\n");
                else
                    printf("%s: %d\n", plg->get_id(), result);
                dlclose(handle);
                break;
            } else {
                dlclose(handle);
            }
        }
    }

    closedir(dir);
    
    return 0;
}
