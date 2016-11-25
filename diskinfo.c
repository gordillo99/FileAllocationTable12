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
#include "utilities.h"

// gets OS name from boot sector
void getOSName(char * p, char * osName) {
	int i;
	for (i = 0; i < 8; i++) osName[i] = p[3 + i];
}

// gets label name from boot sector
void getLabel(char * p, char * label) {
	int i;
	for (i = 0; i < 11; i++) *(label + i) = p[43 + i];
}

// counts how many directories are in the root directory
int countDirectoriesInRoot(char * p) {
	int count = 0;
	int i;
	char * p_copy = p + (19*512); // move to sector 19 (root directory)
	for (i = 0; i < 224; i++) {
		char c = p_copy[i*32];
		if ((c & 0xff) == 0x00) break; // return if we reach the end
		else if ((c & 0xff) != 0xe5 && !(p_copy[i*32 + 11] & 0x0F) && !(p_copy[i*32 + 11] & 0x08)) count++; // check not free (0xef) and no long invalid long name (0x0f) and not volume label (0x08)
	}
	return count; 
}

int main (int argc, char *argv[]) {
	FILE * fp;
	int fd;
	struct stat sf;
	char *p;
	char *osname = malloc(sizeof(char)*8);
	char *label = malloc(sizeof(char)*11);
	int size;
	int diskFreeSize;
	
	if (fd = open(argv[1], O_RDONLY)) { // open disk file
		if (fd < 0) { printf("Error: File %s not found.", argv[1]); exit(EXIT_FAILURE); }
		if (fstat(fd, &sf) ==  -1) { printf("Error: couldn't get file stats."); exit(EXIT_FAILURE); };
		p = mmap(NULL, sf.st_size, PROT_READ, MAP_SHARED, fd, 0); // map disk file
		if ((int) *p == -1) { printf("Error: file mapping failed."); exit(EXIT_FAILURE); }		
		size = getTotalNumberOfSectors(p);
		
		// grab boot sector info		
		getOSName(p, osname);
		getLabel(p, label);
		
		// display and grab remaining info
		printf("OS Name: %s\n", osname);
		printf("Label of the disk: %s\n", label);
		printf("Total size of the disk: %i bytes\n", size * 512);
		printf("Free size of the disk: %i bytes\n", getFreeSize(size, p));
		printf("==============\n");
		printf("The number of files in the root directory (not including subdirectories): %d\n", countDirectoriesInRoot(p));
		printf("==============\n");
		printf("Number of FAT copies: %i\n", (int) p[16]);
		printf("Sectors per FAT: %i\n", (int) p[22]);
	}

	free(osname);
	free(label);
	if (close(fd) == -1) {printf("Error: couldn't close file."); exit(EXIT_FAILURE);};
	munmap(p, sf.st_size);
	if (munmap(p, sf.st_size) == -1) { printf("Error: file mapping failed."); exit(EXIT_FAILURE); }

	return 0;
}


