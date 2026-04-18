#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#include <pwd.h>
#include <grp.h>
#include <string.h>
#include <errno.h>



#define MAX_FILE_COUNT 1024
#define MAX_PATH_LEN 4096


#define OPT_A (1 << 0)	// -a
#define OPT_L (1 << 1)	// -l

#define HAS_OPT(flag, opt) (((flag)&(opt))!=0)


void parse_args(int argc, char* argv[], char* files[], int* file_count, int* flag){
	*file_count = 0;
	*flag = 0;

	int opt = 0;
	while((opt = getopt(argc, argv, "al")) != -1){
		switch(opt){
			case 'a': 
				*flag |= OPT_A;
				break;
			case 'l':
				*flag |= OPT_L;
				break;
			default:
				fprintf(stderr, "valid options: -a|-l");
				exit(EXIT_FAILURE);
		}
	}
	
	for (int i = optind; i < argc && *file_count < MAX_FILE_COUNT; i++) {
        	files[(*file_count)++] = argv[i];
    	}
    	
}



void mode_to_str(mode_t mode, char *str) {
	// file type 
	if (S_ISREG(mode))      str[0] = '-';
    	else if (S_ISDIR(mode)) str[0] = 'd';
    	else if (S_ISLNK(mode)) str[0] = 'l';
    	else if (S_ISCHR(mode)) str[0] = 'c';
    	else if (S_ISBLK(mode)) str[0] = 'b';
    	else if (S_ISFIFO(mode))str[0] = 'p';
    	else if (S_ISSOCK(mode))str[0] = 's';
    	else                    str[0] = '?';

    	// owner  
    	str[1] = (mode & S_IRUSR) ? 'r' : '-';
    	str[2] = (mode & S_IWUSR) ? 'w' : '-';
    	str[3] = (mode & S_IXUSR) ? 'x' : '-';
    	// group 
    	str[4] = (mode & S_IRGRP) ? 'r' : '-';
    	str[5] = (mode & S_IWGRP) ? 'w' : '-';
    	str[6] = (mode & S_IXGRP) ? 'x' : '-';
    	// other
    	str[7] = (mode & S_IROTH) ? 'r' : '-';
    	str[8] = (mode & S_IWOTH) ? 'w' : '-';
    	str[9] = (mode & S_IXOTH) ? 'x' : '-';
    	str[10] = '\0';
}

const char* get_user_name(uid_t uid) {
    struct passwd *pw = getpwuid(uid);
    return pw ? pw->pw_name : "?";
}

const char* get_group_name(gid_t gid) {
    struct group *gr = getgrgid(gid);
    return gr ? gr->gr_name : "?";
}

void print_entry_long_format(const char* entry_name, struct stat* st){
	char mode[11];
	mode_to_str(st->st_mode,mode);

	const char* user = get_user_name(st->st_uid);
    	const char* group = get_group_name(st->st_gid);


	printf("%s %s %s %ld %s\n", mode, user, group, st->st_size, entry_name);
}

void print_entry(const char* dir_path, const char* entry_name, int flag){
	char path[MAX_PATH_LEN];
	if (dir_path) {
     		snprintf(path, sizeof(path), "%s/%s", dir_path, entry_name);
    	} else {
        	strncpy(path, entry_name, sizeof(path) - 1);
    	}

	struct stat st;
    	if (lstat(path, &st) == -1) {
        	perror(entry_name);
        	return;
    	}


	if(HAS_OPT(flag, OPT_L)){
		print_entry_long_format(entry_name, &st);
	}else{
		printf("%s \n", entry_name);
	}	
}

void list_dir(const char* dir_path, int flag){
	DIR *dir = opendir(dir_path);
    	if (!dir) {
        	fprintf(stderr, "myls: cannot access '%s': %s\n", dir_path, strerror(errno));
        	return;
    	}

    	struct dirent *entry;
    	
	while ((entry = readdir(dir)) != NULL) {
		if(!HAS_OPT(flag, OPT_A) && entry->d_name[0] == '.') continue;
        	print_entry(dir_path, entry->d_name, flag);
    	}

    	closedir(dir);
}


int main(int argc, char *argv[]) {
	int flag = 0;
    	int file_count = 0;
	char* files[MAX_FILE_COUNT];

    	
	parse_args(argc, argv, files, &file_count, &flag);

	if(file_count==0){
		files[0] = ".";
		file_count = 1;
	}

	for(int i = 0; i < file_count; ++i){
		struct stat st;
		if (lstat(files[i], &st) == -1) {
            		perror(files[i]);
            		continue;
        	}
		
		if(S_ISDIR(st.st_mode)){
			list_dir(files[i],flag);
		}else{
			print_entry(NULL, files[i], flag);
		}
	}

    	return 0;
}
