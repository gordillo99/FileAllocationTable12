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
#include "diskinfo.h"

// helper method used for debugging
// converting bytes to binary
const char *byte_to_binary(int x)
{
    static char b[17]; // bits + 1
    b[0] = '\0';

    int z;
    for (z = 65536; z > 0; z >>= 1) // z = 2 ^ # of bits
    {
        strcat(b, ((x & z) == z) ? "1" : "0");
    }

    return b;
}

void printRootInfo(char * p) {
	int count = 0;
	int i;
	char * p_copy = p + (19*512); // move to sector 19 (root directory)
	for (i = 0; i < 224; i++) {
		char c = p_copy[i*32];
		char type = ' ';
		uint32_t  size = 0;
		uint16_t time = 0;
		uint16_t date = 0;
		char name[] = "                    ";
		char createDate[20];
		
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
			
			// getting size
			size = ((p_copy[i*32 + 28] & 0xff) | ((p_copy[i*32 + 29] & 0xff) << 8) | ((p_copy[i*32 + 30] & 0xff) << 16) | ((p_copy[i*32 + 31]  & 0xff) << 24));
			
			// create time
			time = (p_copy[i*32 + 14] & 0xff) | ((p_copy[i*32 + 15] & 0xff) << 8);

			int hours = 0;
			int minutes = 0;
			//int seconds = 0;
			
			hours = (time & 0xF800) >> 11;
			minutes = (time & 0x7E0) >> 5;
			//seconds = time & 0x1f;
			
			//create date 
			date = (p_copy[i*32 + 16] & 0xff) | ((p_copy[i*32 + 17] & 0xff) << 8);
			
			int day = 0;
			int month = 0;
			int year = 0;
			
			year = (date & 0xFE00) >> 9;
			month = (date & 0x1E0) >> 5;
			day = date & 0x1f;
			
			sprintf(createDate, "%u:%u %u-%u-%u", hours, minutes, 1980 + year, month, day);
			
			printf("%c %u %s %s\n", type, size, name, createDate);
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
