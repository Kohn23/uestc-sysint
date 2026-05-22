#include <stdio.h>
#include "interface.h"

static const char* get_id(){
	return "canada";
}

static const char* get_desc(){
	return "print hello canada";
}


static void print_message(void) {
    printf("Hello Canada\n");
}

struct PluginInterface* get_plugin_interface(void) {
    static struct PluginInterface iface = {
        .get_id = get_id,
        .get_desc = get_desc,
        .print_message = print_message
    };
    return &iface;
}
