#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <dlfcn.h>
#include "interface.h"

int main(int argc, char *argv[]) {
    
    const char *plugin_dir = "./plugins";
    const char *mode = argv[1];

    DIR *dir = opendir(plugin_dir);
    if (dir == NULL) {
        fprintf(stderr, "cannot open dir: %s\n", plugin_dir);
        return 1;
    }

    int is_help = (strcmp(mode, "help") == 0);

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        const char *name = entry->d_name;
        size_t len = strlen(name);
        if (len < 3 || strcmp(name + len - 3, ".so") != 0)
            continue;

        char libpath[512];
        snprintf(libpath, sizeof(libpath), "%s/%s", plugin_dir, name);

        void *handle = dlopen(libpath, RTLD_LAZY);
        if (!handle) {
            fprintf(stderr, "cannot load %s: %s\n", libpath, dlerror());
            continue;
        }

        dlerror();

        get_interface_func_t get_iface = 
            (get_interface_func_t) dlsym(handle, "get_plugin_interface");
        const char *err = dlerror();
        if (err || !get_iface) {
            fprintf(stderr, "missing get_plugin_interface: %s\n", libpath);
            dlclose(handle);
            continue;
        }

        struct PluginInterface *plugin = get_iface();

        if (is_help) {
            printf("%s: %s\n", plugin->get_id(), plugin->get_desc());
            dlclose(handle);
        } else {
            if (strcmp(plugin->get_id(), mode) == 0) {
                plugin->print_message();
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
