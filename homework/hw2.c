#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <string.h>
#include <sys/stat.h>

static int last_dir = 0;

int is_last_entry(DIR *dir) {
    long pos = telldir(dir);
    struct dirent *next;
    int last = 1;
    while ((next = readdir(dir)) != NULL) {
        if (strcmp(next->d_name, ".") != 0 && strcmp(next->d_name, "..") != 0) {
            last = 0;
            break;
        }
    }
    seekdir(dir, pos);
    return last;
}

void list_dir(const char *path, int level) {
    
	
    DIR *dir = opendir(path);
    if (!dir) {
        perror("opendir");
        return;
    }

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            continue;
	

        for (int i = 1; i <= level; i++){
	       if((last_dir & (1 << i))==0) printf("   |");
	       else printf("    ");
	}


        if (entry->d_type == DT_DIR) {
            printf("- %s\n", entry->d_name);
            
	    char subpath[1024];
            snprintf(subpath, sizeof(subpath), "%s/%s", path, entry->d_name);
            
	    if(is_last_entry(dir)) last_dir |= (1 << level);

	    list_dir(subpath, level + 1);


        } else if (entry->d_type == DT_REG) {
            printf("  %s\n", entry->d_name);
        } else {
            printf("  %s\n", entry->d_name);
        }
    }
    closedir(dir);
}

int main(int argc, char *argv[]) {
    const char *dir = (argc > 1) ? argv[1] : ".";
    printf(".:%s\n", dir);
    list_dir(dir, 1);
    return 0;
}
