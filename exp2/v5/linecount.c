#include <stdio.h>
#include <stdlib.h>
#include "interface.h"

static const char* get_id(void) {
    return "line_count";
}

static const char* get_desc(void) {
    return "Count lines in a text file";
}

static int do_count(const char *filename) {
    FILE *f = fopen(filename, "r");
    if (!f) return -1;

    int lines = 0;
    int ch;
    while ((ch = fgetc(f)) != EOF) {
        if (ch == '\n') lines++;
    }
    fclose(f);
    return lines;
}

struct PluginInterface* get_plugin_interface(void) {
    static struct PluginInterface iface = {
        .get_id = get_id,
        .get_desc = get_desc,
        .do_count = do_count
    };
    return &iface;
}
