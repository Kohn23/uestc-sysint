#ifndef PLUGIN_INTERFACE_H
#define PLUGIN_INTERFACE_H

struct PluginInterface {
    const char* (*get_id)(void);
    const char* (*get_desc)(void);
    void (*print_message)(void);
};

typedef struct PluginInterface* (*get_interface_func_t)(void);

#endif
