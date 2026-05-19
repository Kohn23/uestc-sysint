#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <dlfcn.h>

typedef const char* (*get_id_func_t)(void);
typedef const char* (*get_desc_func_t)(void);
typedef void (*print_func_t)(void);

int main(int argc, char *argv[]) {
    const char *plugin_dir = "./plugins";
    const char *mode = argv[1];
    
    struct dirent *entry;
    DIR *dir = opendir(plugin_dir);
    if (dir == NULL) {
	fprintf(stderr, "无法打开插件目录: %s\n", plugin_dir);
	return 1;
    }

    int is_help = (strcmp(mode,"help")==0);

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

        if(is_help){
		get_id_func_t get_id = (get_id_func_t) dlsym(handle,"get_id");
		const char *id_err = dlerror();
		get_desc_func_t get_desc = (get_desc_func_t) dlsym(handle,"get_desc");
		const char *desc_err = dlerror();

		if(desc_err||id_err){
			fprintf(stderr, "missing interface:%s\n ", libpath);
		}else{
			const char *id = get_id();
			const char *desc = get_desc();
			printf("%s: %s\n", id, desc);
		}
		dlclose(handle);
	}else{
		get_id_func_t get_id = (get_id_func_t) dlsym(handle,"get_id");
		const char *err = dlerror();
		if(err){
			fprintf(stderr, "can't get id:%s", libpath);
		}

		const char* id = get_id();
		if(strcmp(id,mode)==0){
			print_func_t print_msg = (print_func_t) dlsym(handle, "print_message");
                	err = dlerror();
                	if (err) {
                    		fprintf(stderr, "missing functional interface: %s\n", libpath);
                	} else {
                    		print_msg();
                	}
			dlclose(handle);
			break;
		}else{
			dlclose(handle);
		}
	}
    }

    closedir(dir);

    return 0;
}
    

