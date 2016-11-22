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
    static char b[9]; // bits + 1
    b[0] = '\0';

    int z;
    for (z = 256; z > 0; z >>= 1) // z = 2 ^ # of bits
    {
        strcat(b, ((x & z) == z) ? "1" : "0");
    }

    return b;
}

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


int getSectorValue(char * p, int i) {
	int offset = (i * 3) / 2;
	char * fat = p + 512; // skip first sector to reach FAT
	uint16_t value = 0;

	if ((i % 2) == 0) {
		int low = fat[offset + 1] & 0xff;
		int high = fat[offset] & 0xff;

			//printf("with i = %d entry (3*i/2): %d hex: %x entry %d (3*i/2)+1 %x \n", i, offset, fat[offset] & 0xff, offset+1, fat[offset + 1] & 0xff);
			//printf("lower %s\n", byte_to_binary(low));
			//printf("higher %s\n", byte_to_binary(high));
			value = value | low;
			//printf("result %s\n", byte_to_binary16(value));
			value = value << 8;
			//printf("result %s\n", byte_to_binary16(value));
			value = value & 0xF00;
			value = value | high;
			//printf("result %s\n", byte_to_binary16(value));

	} else {
		int low = fat[offset + 1] & 0xff;
		int high = fat[offset] & 0xff;
		
			//printf("with i = %d entry (3*i/2): %d hex: %x entry %d (3*i/2)+1 %x \n", i, offset, fat[offset] & 0xff, offset+1, fat[offset + 1] & 0xff);
			//printf("lower %s\n", byte_to_binary(low));
			//printf("higher %s\n", byte_to_binary(high));
			value = value | low;
			//printf("result %s\n", byte_to_binary16(value));
			value = value << 8;
			value = value & 0xFF00;
			value = value | high;
			value = value >> 4;
			//printf("result %s\n", byte_to_binary16(value));
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

int findFile(char * p, char * f_to_find, int * size) {
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
				*size = ((p_copy[i*32 + 28] & 0xff) | ((p_copy[i*32 + 29] & 0xff) << 8) | ((p_copy[i*32 + 30] & 0xff) << 16) | ((p_copy[i*32 + 31]  & 0xff) << 24));
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
	return getSectorValue(p, index);
}

void writeFile(char * fileName, char * p, int * error, int nextLogicalCluster, int size) {
	int n = nextLogicalCluster;
	FILE *write_ptr;
	char byte[1];
	write_ptr = fopen(fileName,"wb");  // w for write, b for binary
	
	while (checkNextLogicalCluster(n, error)) {
	 	int physicalCluster = 33 + n - 2;
	 	int i;
	 	for (i = 0; i < 512; i++) {
	 		byte[0] = (p[physicalCluster*512 + i] & 0xff);
	 		
	 		if (size > 0) {
	 			fwrite(byte, sizeof(byte), 1, write_ptr);
	 			size--;
	 		}
	 	}

		n = checkFAT(p, n);
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
	
	if (fd = open(argv[1], O_RDONLY)) {
		fstat(fd, &sf);
		p = mmap(NULL, sf.st_size, PROT_READ, MAP_SHARED, fd, 0);
		nextLogicalCluster = findFile(p, argv[2], &file_size);
		
		if (nextLogicalCluster == -1) { 
			printf("File not found\n");
			exit(EXIT_FAILURE);
		}
		
		writeFile(argv[2], p, &error, nextLogicalCluster, file_size);
		
		if (error == -1) { printf("Error: reserved cluster found"); close(fd); exit(EXIT_FAILURE); }
		if (error == -2) { printf("Error: bad cluster found"); close(fd); exit(EXIT_FAILURE); }
		
	}

	close(fd);
	munmap(p, sf.st_size);

	return 0;
}

