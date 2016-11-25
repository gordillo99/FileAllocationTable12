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

// checks to see if we should keep going (based on the entry's value)
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
	else if (next >= 4088 && next <= 4095) return 0;
	else return 1;
}

// gets the value for the given FAT entry
int checkFAT(char * p, int index) {
	return getSectorValue(p, index);
}

// writes to the file in the unix directory
void writeFile(char * fileName, char * p, int * error, int nextLogicalCluster, int size) {
	int n = nextLogicalCluster;
	FILE *write_ptr;
	char byte[1];
	write_ptr = fopen(fileName,"wb");  // w for write, b for binary
	
	while (checkNextLogicalCluster(n, error)) { // while the fat table tells us to keep on going...
	 	int physicalCluster = 33 + n - 2;
	 	int i;
	 	for (i = 0; i < 512; i++) {
	 		byte[0] = (p[physicalCluster*512 + i] & 0xff);
	 		
	 		if (size > 0) { // run only until reaching size of file
	 			fwrite(byte, sizeof(byte), 1, write_ptr); // write byte by byte
	 			size--;
	 		}
	 	}

		n = checkFAT(p, n); // move on to next FAT entry
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
	int file_size = 0;
	
	if (fd = open(argv[1], O_RDONLY)) { // open disk file
		if (fd < 0) { printf("Error: File %s not found.\n", argv[1]); exit(EXIT_FAILURE); }
		if (fstat(fd, &sf) ==  -1) { printf("Error: couldn't get file stats.\n"); exit(EXIT_FAILURE); };
		p = mmap(NULL, sf.st_size, PROT_READ, MAP_SHARED, fd, 0); // map disk file
		if ((int) *p == -1) { printf("Error: file mapping failed.\n"); exit(EXIT_FAILURE); }		
		nextLogicalCluster = findFile(p, argv[2], &file_size);
		
		if (nextLogicalCluster == -1) { // error checking
			printf("File not found\n");
			exit(EXIT_FAILURE);
		}

		// write to new file
		writeFile(argv[2], p, &error, nextLogicalCluster, file_size);
		
		// error checking
		if (error == -1) { printf("Error: reserved cluster found\n"); close(fd); exit(EXIT_FAILURE); }
		if (error == -2) { printf("Error: bad cluster found\n"); close(fd); exit(EXIT_FAILURE); }
		
	}

	if (close(fd) == -1) {printf("Error: couldn't close file.\n"); exit(EXIT_FAILURE);};
	if (munmap(p, sf.st_size) == -1) { printf("Error: file mapping failed.\n"); exit(EXIT_FAILURE); }

	return 0;
}

