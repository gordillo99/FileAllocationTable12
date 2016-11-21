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

void getFileName(char * p, int entry, char * name) {
	// building name
	int j;
	int i = 0; 
	for (j = 0; j < 8; j++) {
		if (p[entry*32 + j] != ' ') {
			name[i] = p[entry*32 + j];
			i++;
		}
	}
	name[i] = '.';
	i++;
	for (; j < 12; j++) {
		name[i] = p[entry*32 + j]; // move p_copy one back since name is ahead because of the .
		i++;
	}
	name[12] = '\0';
}

int findFile(char * p, char * f_to_find) {
	int fileFound = -1;
	int i;
	char * p_copy = p + (19*512); // move to sector 19 (root directory)
	char name[40];
	
	for (i = 0; i < 224; i++) {
		char c = p_copy[i*32];
		if ((c & 0xff) == 0x00) {
			break;
		}
		else if ((c & 0xff) != 0xe5 && !(p_copy[i*32 + 11] & 0x0F) && !(p_copy[i*32 + 11] & 0x08)) {
			getFileName(p_copy, i, name);
			if (strcmp(f_to_find, name) == 0)  {
				fileFound = (p_copy[i*32 + 26] & 0xff) | (p_copy[i*32 + 27] & 0xff) << 8;
				break;
			}
		}
	}
	return fileFound; 
}

int checkNextLogicalCluster(int next, int * error) {
	if (next == 0) return 0;
	else if (next >= 4080 && next <= 4086) {
		*error = -1;
		return 0;
	}
	else if (next == 4087) {
		*error = -2;
		return 0;
	}
	else if (next >= 4088 && next <= 4095) {
		return 0;
	} else {
		return 1;
	}
}

int checkFAT(char * p, int index) {
	printf("next sector value %d \n", getSectorValue(p, index));
	return getSectorValue(p, index);
}

void writeFile(char * fileName, char * p, int * error, int nextLogicalCluster) {
	FILE *write_ptr;
	char byte;
	write_ptr = fopen(fileName,"wb");  // w for write, b for binary
	
	 while (checkNextLogicalCluster(nextLogicalCluster, error)) {
	 	int physicalCluster = 33 + nextLogicalCluster - 2;
	 	int i;
	 	for (i = 0; i < 512; i++) {
	 		byte = p[physicalCluster*512 + i] & 0xff;
	 		fwrite(byte,sizeof(byte),1,write_ptr); 
	 	}
			
		nextLogicalCluster = checkFAT(p, nextLogicalCluster);
	}
	
	fclose(write_ptr);
}

int main (int argc, char *argv[]) {
	FILE * fp;
	int fd;
	struct stat sf;
	char * p;
	int nextLogicalCluster = -1;
	int error = 0;
	
	if (fd = open(argv[1], O_RDONLY)) {
		fstat(fd, &sf);
		p = mmap(NULL, sf.st_size, PROT_READ, MAP_SHARED, fd, 0);
		nextLogicalCluster = findFile(p, argv[2]);
		
		if (nextLogicalCluster == -1) { 
			printf("File not found\n");
			exit(EXIT_FAILURE);
		}
		
		printf("%d\n", nextLogicalCluster);
		
		writeFile(argv[2], p, &error, nextLogicalCluster);
		
		if (error == -1) { printf("Error: reserved cluster found"); close(fd); exit(EXIT_FAILURE); }
		if (error == -2) { printf("Error: bad cluster found"); close(fd); exit(EXIT_FAILURE); }
		
	}

	close(fd);

	return 0;
}

