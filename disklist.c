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

int main (int argc, char *argv[]) {
	FILE * fp;
	int fd;
	struct stat sf;
	char * p;
	
	if (fd = open(argv[1], O_RDONLY)) {
		fstat(fd, &sf);
		p = mmap(NULL, sf.st_size, PROT_READ, MAP_SHARED, fd, 0);
	}

	close(fd);

	return 0;
}

void printRootInfo(char * p) {
	/*int count = 0;
	int i = 0;
	int index = 19*512 + (i*32);
	char c = p[index] & 0xff;
	while (i = 0; i < 224; i++) {
		char type = ' ';
		char size[] = "          ";
		char name = "                    ";
		char createDate = "                    ";

		if (c == 0x00) break;
		else if (c != 0xe5) {
			int j;
			for (j = 0; j < 8; j++) name[j] = p[index + j];
			name[j++] = '.';
			for (; j < 11; j++) name[j] = p[index + j];
			for (j = 0; j < 8; j++) name[j] = p[index + j];
			//type = p[index
			printf("%c %s %s %s", type, size, name, createDate);
		}
		i++;
		index = 19*512 + (i*32);
		c = p[index] & 0xff;
	}*/
}

