#include <stdio.h>
#include "interface.h"

static const char* get_id(){return "world";}

static const char* get_desc(){return "print hello world";}
static const char* get_descv2(){return "This is get_descv2";}

static void print_message(void) {printf("Hello World\n");}


struct PluginInterface* get_plugin_interface(void) {
    static struct PluginInterface iface = {
        .get_id = get_id,
        .get_desc = get_descv2,
        .print_message = print_message
    };
    return &iface;
}
