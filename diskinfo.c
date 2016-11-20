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

void getOSName(char * p, char * osName) {
	int i;
	for (i = 0; i < 8; i++) {
		osName[i] = p[3 + i];
	}
}

int getTotalNumberOfSectors(char * p) {
	return (int) (p[19] | p[20] << 8);
}

int getSectorValue(char * p, int i) {
	int offset = (i * 3) / 2;
	char * fat = p + 512; // skip first sector to reach FAT
	int value = fat[offset + 0] | (fat[offset + 1] << 8) | (fat[offset + 2] << 16);

	if ((i % 2) == 0) {
		value &= 0xfff;
	} else {
		value >>= 12;
	}

	return value;
}

int getFreeSize(int totalNumberOfSectors, char * p) {
	int count = 0;
	int i;
	for (i = 2; i < totalNumberOfSectors; i++) {
		if (getSectorValue(p, i) == 0) {
			count++;
		}
	}
	return count * 512;
}

void getLabel(char * p, char * label) {
	int i;
	for (i = 0; i < 11; i++) {
		*(label + i) = p[43 + i];
	}
}

int countDirectoriesInRoot(char * p) {
	int count = 0;
	int i;
	char * p_copy = p + (19*512); // move to sector 19 (root directory)
	for (i = 0; i < 224; i++) {
		char c = p_copy[i*32];
		if (c == 0x00) break;
		else if (c != 0xe5) count++;
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
	
	if (fd = open(argv[1], O_RDONLY)) {
		fstat(fd, &sf);
		p = mmap(NULL, sf.st_size, PROT_READ, MAP_SHARED, fd, 0);
		size = getTotalNumberOfSectors(p);
		getOSName(p, osname);
		getLabel(p, label);
		
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
	close(fd);

	return 0;
}


