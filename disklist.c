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
			int diff = 0;
			for (j = 0; j < 8; j++) {
				if (p_copy[i*32 + j] == ' ') break; 
				name[j] = p_copy[i*32 + j];
			}
			if (p_copy[i*32 + 8] != ' ' && p_copy[i*32 + 9] != ' ' && p_copy[i*32 + 10] != ' ') {	
				name[j] = '.';
				j++;
				diff = 1;

				int local_j = j;
				int space_counter = 0;
				for (; j < 12; j++) {
					if (p_copy[i*32 + j - diff] == ' ') space_counter++; 
				}				
				j =  local_j;
				for (; j < 12; j++) {
					if (p_copy[i*32 + j - diff] != ' ') name[j - space_counter] = p_copy[i*32 + j - diff]; // move p_copy one back since name is ahead because of the .
				}
			}

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
			
			// masking and shifting
			year = (date & 0xFE00) >> 9;
			month = (date & 0x1E0) >> 5;
			day = date & 0x1f;
			
			sprintf(createDate, "%04u-%02u-%02u %02u:%02u", 1980 + year, month, day, hours, minutes); // save format to variable
			
			printf("%c %10u %20s %s\n", type, size, name, createDate); // display results

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
	munmap(p, sf.st_size);

	return 0;
}
