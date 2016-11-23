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

/*
open(check) file to be copied in current dir
grab its file size & related info
char *src = mmap(file, ... file size)
open disk file
get disk size
char *dest = mmap(disk file, ... disk size)
check for free space in disk
Add file entry in disk root dir 
copy file from src->dest, reading sector by sector, update FAT table in the meantime

munmap(disk file)
munmap(file)

close disk file
close file
*/

int getSectorValue(char * p, int i) {
	int offset = (i * 3) / 2;
	char * fat = p + 512; // skip first sector to reach FAT
	uint16_t value = 0;

	if ((i % 2) == 0) {
		int low = fat[offset + 1] & 0xff;
		int high = fat[offset] & 0xff;
		value = value | low;
		value = value << 8;
		value = value & 0xF00;
		value = value | high;

	} else {
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

int getTotalNumberOfSectors(char * p) {
	return (int) (p[19] | p[20] << 8);
}

int getFreeSize(int totalNumberOfSectors, char * p) {
	int count = 1;
	int i;
	for (i = 2; i < totalNumberOfSectors; i++) {
		if (getSectorValue(p, i) == 0) {
			count++;
		}
	}
	return count * 512;
}

int findFirstAvailableRootEntry(char * p) {
	int i;
	char * p_copy = p + (19*512); // move to sector 19 (root directory)
	for (i = 0; i < 224; i++) {
		char c = p_copy[i*32];
		if ((c & 0xff) == 0x00) {
			return (19*512) + (i*32);
		}
	}
	return -1; 
}

int hasExtension(char * filename, int namelen) {
	int i;
	for (i = 0; i < namelen; i++) if (*(filename + i) == '.') return 1;
	return 0;
}

int findAvailFATEntry(char * p) {
	int i;
	int totalNumberOfSectors = (int) (p[19] | p[20] << 8);
	for (i = 2; i < totalNumberOfSectors; i++) {
		if (getSectorValue(p, i) == 0) break;
	}
	return i;
}


int addRootDirEntry(char * p, char * src, int offset, char * filename, int filesize) {
	int i;
	int ext_index;
	int namelen = strlen(filename);
	int hasExt = hasExtension(filename, namelen);
	
	// add filename
	if (namelen > 12) { printf("Error: filename too big"); exit(EXIT_FAILURE); }
	
	if (hasExt) {
		for (ext_index = 0; ext_index < namelen; ext_index++) if (filename[ext_index] == '.') break;
		for (i = 0; i < ext_index; i++) p[offset + i] = toupper(*(filename + i)) & 0xff;
		if (11 - ext_index != 3) {
			for (; i < 8; i++) p[offset + i] = 0x20; // insert spaces until reaching ext
		}
		int j;
		//for () 
	} else {
		for (i = 0; i < namelen; i++) p[offset + i] = toupper(*(filename + i)) & 0xff;
		for (; i < 11; i++) p[offset + i] = 0x20; // insert spaces until reaching end
	}
	
	// writing attributes
	p[offset + 11] = 0x00;
	
	// writing time and date
	time_t rawtime;
  struct tm *info;

  time(&rawtime);
  info = localtime(&rawtime);
  
  uint16_t seconds = info->tm_sec / 2;
  uint16_t minutes = info->tm_min;
  uint16_t hours = info->tm_hour;
  uint16_t day = info->tm_mday;
  uint16_t month = info->tm_mon;
  uint16_t year = info->tm_year - 80;
  
  uint8_t timebits = 0;
  timebits &= year;
  timebits = timebits << 4;
  timebits &= month;
  timebits = timebits << 4;
  timebits &= day;
  
  uint8_t datebits = 0;
  datebits &= year;
  datebits = datebits << 4;
  datebits &= month;
  datebits = datebits << 4;
  datebits &= day;
  
  // setting creation time
  p[offset + 14] = timebits & 0xff;
  timebits = timebits >> 8;
  p[offset + 15] = timebits & 0xff;
  
  // setting creation date
  p[offset + 16] = datebits & 0xff;
  datebits = datebits >> 8;
  p[offset + 17] = datebits & 0xff; 
  
  // setting last access date 
	p[offset + 18] = datebits & 0xff;
  datebits = datebits >> 8;
  p[offset + 19] = datebits & 0xff; 
  
  // setting last write time
  p[offset + 22] = timebits & 0xff;
  timebits = timebits >> 8;
  p[offset + 23] = timebits & 0xff;
  
  // setting last write date 
	p[offset + 24] = datebits & 0xff;
  datebits = datebits >> 8;
  p[offset + 25] = datebits & 0xff; 
  
  uint32_t fsize = filesize;
  
  // writing size 
  p[offset + 28] = fsize & 0xff;
  fsize = fsize >> 8;
  p[offset + 29] = fsize & 0xff;
  fsize = fsize >> 8;
  p[offset + 30] = fsize & 0xff;
  fsize = fsize >> 8;
  p[offset + 31] = fsize & 0xff;
  
  // find available fat entry 
  int16_t availFAT = findAvailFATEntry(p);
	
	p[offset + 26] = availFAT & 0xff;
  availFAT = availFAT >> 8;
  p[offset + 27] = availFAT & 0xff;
	
	return availFAT;
}

void writeToFATTable(char* p, int target, int src) {

}
 
void writeToDataArea(char * p, char * src, int size, int FATEntry) {
	int bytesWritten = 0;
	int i;
	
	while (bytesWritten < size) {
		int previousFATEntry = FATEntry;
		int physicalCluster = 33 + FATEntry - 2;
		for (i = 0; i < 512 ; i++, bytesWritten++) {
			if (bytesWritten == size) break;
			p[physicalCluster*512 + i] = src[i] & 0xff;
		}
		FATEntry = findAvailFATEntry(p);
		writeToFATTable(p, previousFATEntry, FATEntry);
	}
	
	writeToFATTable(p, previousFATEntry, 0);
}

int main (int argc, char *argv[]) {
	int fd;
	int fd2;
	struct stat sf2;
	struct stat sf;
	char * p;
	char * src;
	int src_size = 0;
	
	if (fd2 = open(argv[2], O_RDONLY)) {
		if (fd2 < 0) { printf("Error: File %s not found.", argv[2]); exit(EXIT_FAILURE); }
		fstat(fd2, &sf2);
		
		src_size = sf2.st_size;
		src = mmap(NULL, src_size, PROT_READ, MAP_SHARED, fd2, 0);
		
		if (fd = open(argv[1], O_RDONLY)) {
			fstat(fd, &sf);
			p = mmap(NULL, sf.st_size, PROT_WRITE, MAP_SHARED, fd, 0);
			if (src_size > getFreeSize(getTotalNumberOfSectors(p), p)) { printf("Error: Not enough free space in disk image."); exit(EXIT_FAILURE); }
			int offsetFirstAvailDir = findFirstAvailableRootEntry(p);
			int firstFATentry = addRootDirEntry(p, src, offsetFirstAvailDir, argv[1], src_size);
			
		}
	}
	
	munmap(src, src_size);
	munmap(p, sf.st_size);
	close(fd);
	close(fd2);
	
	return 0;
}
