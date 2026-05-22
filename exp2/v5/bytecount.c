#include <stdio.h>
#include <stdlib.h>
#include "interface.h"

static const char* get_id(void) {
    return "byte_count";
}

static const char* get_desc(void) {
    return "Count bytes in a file";
}

static int do_count(const char *filename) {
    FILE *f = fopen(filename, "rb");
    if (!f) return -1;

    fseek(f, 0, SEEK_END);
    long bytes = ftell(f);
    fclose(f);
    return (int)bytes;
}

struct PluginInterface* get_plugin_interface(void) {
    static struct PluginInterface iface = {
        .get_id = get_id,
        .get_desc = get_desc,
        .do_count = do_count
    };
    return &iface;
}
