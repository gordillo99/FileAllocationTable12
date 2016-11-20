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
#include "diskinfo.h"

void printRootInfo(char * p) {
	int count = 0;
	int i;
	char * p_copy = p + (19*512); // move to sector 19 (root directory)
	for (i = 0; i < 224; i++) {
		char c = p_copy[i*32];
		char type = ' ';
		char size[] = "          ";
		char name[] = "                    ";
		char createDate[] = "                    ";
		
		if (c == 0x00) break; // if not empty
		else if (c != 0xe5 && !(p_copy[i*32 + 11] & 0x0F) && !(p_copy[i*32 + 11] & 0x08)) { // check not free (0xef) and no long invalid long name (0x0f) and not volume label (0x08)
			
			// building name
			int j;
			for (j = 0; j < 8; j++) name[j] = p_copy[i*32 + j];
			name[j] = '.';
			j++;
			for (; j < 12; j++) name[j] = p_copy[i*32 + j - 1]; // move p_copy one back since name is ahead because of the .
			
			// file or directory code
			if ((p_copy[i*32 + 11] == 0x10)) type = 'D';
			else type = 'F';
			
			printf("%c %s %s %s\n", type, size, name, createDate);
		}
	}
}

int main (int argc, char *argv[]) {
	FILE * fp;
	int fd;
	struct stat sf;
	char * p;
	
	if (fd = open(argv[1], O_RDONLY)) {
		fstat(fd, &sf);
		p = mmap(NULL, sf.st_size, PROT_READ, MAP_SHARED, fd, 0);
		printRootInfo(p);
	}

	close(fd);

	return 0;
}
