#include <stdio.h>
#include "interface.h"

static const char* get_id(){
	return "china";
}

static const char* get_desc(){
	return "print hello china";
}


static void print_message(void) {
    printf("Hello China\n");
}

struct PluginInterface* get_plugin_interface(void) {
    static struct PluginInterface iface = {
        .get_id = get_id,
        .get_desc = get_desc,
        .print_message = print_message
    };
    return &iface;
}
