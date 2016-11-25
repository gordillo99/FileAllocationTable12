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
