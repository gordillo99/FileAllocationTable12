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

// gets file name for entry in directory
void getFileName(char * p, int entry, char * name) {

	// building name
	int j;
	int i = 0;
	
	// get name before extension
	for (j = 0; j < 8; j++) {
		if (p[entry*32 + j] != ' ') {
			name[i] = p[entry*32 + j];
			i++;
		}
	}
	
	if (p[entry*32 + 10] != ' ' && p[entry*32 + 9] != ' ' && p[entry*32 + 8] != ' ') { // if it has an extension
		name[i] = '.';
		i++;
	}
	
	// get rest of name
	for (; j < 12; j++) {
		if (p[entry*32 + j] != ' ') {
			name[i] = p[entry*32 + j]; // move p_copy one back since name is ahead because of the .
			i++;
		}
	}
	name[12] = '\0';
}

// checks through the root directory to see if file exists
int findFile(char * p, char * f_to_find, int * size) {
	int fileFound = -1;
	int i;
	char * p_copy = p + (19*512); // move to sector 19 (root directory)
	char name[40];
	int j;

	// changes name to uppercase
	for (j = 0; j < strlen(f_to_find); j++) f_to_find[j] = toupper(f_to_find[j]);
	
	// checks all the existing files
	for (i = 0; i < 224; i++) {
		char c = p_copy[i*32];
		if ((c & 0xff) == 0x00) { // returns if we reach end of files
			break;
		}
		else if ((c & 0xff) != 0xe5 && !(p_copy[i*32 + 11] & 0x0F) && !(p_copy[i*32 + 11] & 0x08)) {
			getFileName(p_copy, i, name);
			if (strcmp(f_to_find, name) == 0)  { // if it's the file we're looking for
				fileFound = (p_copy[i*32 + 26] & 0xff) | (p_copy[i*32 + 27] & 0xff) << 8;
				*size = ((p_copy[i*32 + 28] & 0xff) | ((p_copy[i*32 + 29] & 0xff) << 8) | ((p_copy[i*32 + 30] & 0xff) << 16) | ((p_copy[i*32 + 31]  & 0xff) << 24));
				break;
			}
		}
	}
	return fileFound; // return the first logical cluster 
}

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
		if (fd < 0) { printf("Error: File %s not found.", argv[1]); exit(EXIT_FAILURE); }
		if (fstat(fd, &sf) ==  -1) { printf("Error: couldn't get file stats."); exit(EXIT_FAILURE); };
		p = mmap(NULL, sf.st_size, PROT_READ, MAP_SHARED, fd, 0); // map disk file
		if ((int) *p == -1) { printf("Error: file mapping failed."); exit(EXIT_FAILURE); }		
		nextLogicalCluster = findFile(p, argv[2], &file_size);
		
		if (nextLogicalCluster == -1) { // error checking
			printf("File not found\n");
			exit(EXIT_FAILURE);
		}

		// write to new file
		writeFile(argv[2], p, &error, nextLogicalCluster, file_size);
		
		// error checking
		if (error == -1) { printf("Error: reserved cluster found"); close(fd); exit(EXIT_FAILURE); }
		if (error == -2) { printf("Error: bad cluster found"); close(fd); exit(EXIT_FAILURE); }
		
	}

	if (close(fd) == -1) {printf("Error: couldn't close file."); exit(EXIT_FAILURE);};
	munmap(p, sf.st_size);
	if (munmap(p, sf.st_size) == -1) { printf("Error: file mapping failed."); exit(EXIT_FAILURE); }

	return 0;
}

