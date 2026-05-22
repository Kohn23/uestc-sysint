#ifndef INTERFACE_H
#define INTERFACE_H

struct PluginInterface {
    const char* (*get_id)(void);
    const char* (*get_desc)(void);
    int (*do_count)(const char *filename);
};

typedef struct PluginInterface* (*get_plugin_interface_func_t)(void);

#endif
