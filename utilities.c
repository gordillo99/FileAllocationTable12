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
#include <time.h>

// gets the sector value for the FAT entry
int getSectorValue(char * p, int i) {
	int offset = (i * 3) / 2;
	char * fat = p + 512; // skip first sector to reach FAT
	uint16_t value = 0;

	if ((i % 2) == 0) { // for even entries
		int low = fat[offset + 1] & 0xff;
		int high = fat[offset] & 0xff;
		value = value | low;
		value = value << 8;
		value = value & 0xF00;
		value = value | high;

	} else { // for odd entries
		int low = fat[offset + 1] & 0xff;
		int high = fat[offset] & 0xff;
		value = value | low;
		value = value << 8;
		value = value & 0xFF00;
		value = value | high;
		value = value >> 4;
	}

	return value;
}

// gets total number of sectors in disk
int getTotalNumberOfSectors(char * p) {
	return (int) (p[19] | p[20] << 8);
}

// gets free size in disk
int getFreeSize(int totalNumberOfSectors, char * p) {
	int count = 0;
	int i;
	// start at 2 since the first 2 entries are reserved
	for (i = 2; i < totalNumberOfSectors; i++) if (getSectorValue(p, i) == 0) count++;
	return count * 512;
}

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


// helper methods used for debugging
// converting bytes to binary (8 bits)
const char *byte_to_binary(int x)
{
    static char b[9]; // bits + 1
    b[0] = '\0';

    int z;
    for (z = 256; z > 0; z >>= 1) // z = 2 ^ # of bits
    {
        strcat(b, ((x & z) == z) ? "1" : "0");
    }

    return b;
}

// converting bytes to binary (16 bits)
const char *byte_to_binary16(int x)
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
