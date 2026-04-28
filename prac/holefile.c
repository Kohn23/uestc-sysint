#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<fcntl.h>

#define HOLEFILE "holefile"

void test_trunc_lseek();

void test_trunc_lseek(){
	FILE fd = create(HOLEFILE,0666);
	const char* content = "holefile";
	write(fd, content, sizeof(*content));
	fflush(fd);
	close(fd);
	
	fd = open(HOLEFILE, O_RDWR|O_TRUNC);
	lseek(fd, 3, SEEK_SET);
	write(fd, content, sizeof(*content));
	fflush(fd);
	
}

