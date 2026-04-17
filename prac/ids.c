#define _GNU_SOURCE

#include<unistd.h>
#include<sys/stat.h>
#include<stdio.h>
#include<stdlib.h>
#include<fcntl.h>


void printresids();
void printst(const char*);
	

int main(int argc, char *argv[]){
	
	
	const char* filepath;
	if(argc == 2) {
		filepath = argv[1];
		printst(filepath);

	}

	printresids();

	return 0;
}


void printresids(){
	uid_t ruid, euid, suid;
	getresuid(&ruid, &euid, &suid);
	printf("ruid:%d, euid:%d, suid:%d\n",ruid, euid, suid);
}

void printst(const char* filepath){
	struct stat st;
	if(stat(filepath,&st)==-1){
		perror("stat");
		exit(EXIT_FAILURE);
	}
	
	printf("st_uid:%d, st_mode_12:%o\n", st.st_uid, st.st_mode & 07777);
}

	
