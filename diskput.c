#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <math.h>
#include <ctype.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <stdint.h>

/*
open(check) file to be copied in current dir
grab its file size & related info
char *src = mmap(file, ... file size)
open disk file
get disk size
char *dest = mmap(disk file, ... disk size)
check for free space in disk
Add file entry in disk root dir 
copy file from src->dest, reading sector by sector, update FAT table in the meantime

munmap(disk file)
munmap(file)

close disk file
close file
*/

int main (int argc, char *argv[]) {
	int fd;
	int fd2;
	struct stat sf2;
	struct stat sf;
	char * p;
	char * src;
	int src_size = 0;
	
	if (fd2 = open(argv[2], O_RDONLY)) {
		if (fd2 < 0) { printf("Error: File %s could not be opened.", argv[2]); exit(EXIT_FAILURE); }
		fstat(fd2, &sf2);
		
		src_size = sf.st_size;
		src = mmap(NULL, sf.st_size, PROT_READ, MAP_SHARED, fd2, 0);
		
		if (fd = open(argv[1], O_RDONLY)) {
			fstat(fd, &sf);
			p = mmap(NULL, sf.st_size, PROT_READ, MAP_SHARED, fd, 0);

		}
	}
	
	

	close(fd);
	close(fd2);

	return 0;
}
