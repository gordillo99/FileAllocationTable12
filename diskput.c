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
#include "utilities.h"

// find the first available entry in the root
int findFirstAvailableRootEntry(char * p) {
	int i;
	char * p_copy = p + (19*512); // move to sector 19 (root directory)
	for (i = 0; i < 224; i++) {
		char c = p_copy[i*32];
		// if we hit zero, this is the next avail entry
		if ((c & 0xff) == 0x00) return (19*512) + (i*32);
	}
	return -1; // if not found
}

// returns true if the file has an extension (has . in name)
int hasExtension(char * filename, int namelen) {
	int i;
	for (i = 0; i < namelen; i++) if (*(filename + i) == '.') return 1;
	return 0;
}

// finds the first available FAT entry
int findAvailFATEntry(char * p) {
	int i = 2;
	int totalNumberOfSectors = getTotalNumberOfSectors(p);
	
	// go on until you hit 0
	for (; i < totalNumberOfSectors; i++) if (getSectorValue(p, i) == 0) break;
	return i;
}

// finds the second available FAT entry
int findNextAvailFATEntry(char * p) {
	int i = 2;
	int totalNumberOfSectors = (int) getTotalNumberOfSectors(p);
	int flag = 0;
	
	for (; i < totalNumberOfSectors; i++) {
		if (getSectorValue(p, i) == 0 && flag) break;
		if (getSectorValue(p, i) == 0) flag = 1;
	}
	return i;
}

// adds the root directory info about the file 
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

		// if spaces are needed, add them
		if (11 - ext_index > 3) {
			for (; i < 8; i++) { p[offset + i] = 0x20; } // insert spaces until reaching ext
		}

		int j;
		// write the extension
		for (j = ext_index + 1; j < strlen(filename); j++, i++) p[offset + i] = toupper(*(filename + j)) & 0xff;
	} else {
		for (i = 0; i < namelen; i++) { p[offset + i] = toupper(*(filename + i)) & 0xff; }
		for (; i < 11; i++) { p[offset + i] = 0x20; } // insert spaces until reaching end
	}
	
	// writing attributes
	p[offset + 11] = 0x00;
	
	// writing time and date
	time_t rawtime;
  struct tm *info;

	// getting local time
  time(&rawtime);
  info = localtime(&rawtime);
  
	// manipulating variables to fit requirements
  uint16_t seconds = info->tm_sec / 2;
  uint16_t minutes = info->tm_min;
  uint16_t hours = info->tm_hour;
  uint16_t day = info->tm_mday;
  uint16_t month = info->tm_mon + (int) 1;
  uint16_t year = info->tm_year - (int) 80;
  
	// formatting time as required
  uint16_t timebits = 0;
  timebits |= hours;
  timebits = timebits << 6;
  timebits |= minutes;
  timebits = timebits << 5;
  timebits |= seconds;
  
	// formatting date as required
  uint16_t datebits = 0;
  datebits |= year;
  datebits = datebits << 4;
  datebits |= month;
  datebits = datebits << 5;
  datebits |= day;

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
  int16_t fatEntry = availFAT;
	p[offset + 26] = availFAT & 0xff;
  availFAT = availFAT >> 8;
  p[offset + 27] = availFAT & 0xff;
  
	return fatEntry;
}

// writes the target value into the FAT entry number src
void writeToFATTable(char* p, int target, int src) {

	// bit manipulation to enter info in 12 bits
	if (target % 2 == 0) { // if target is even
		uint8_t low = src & 0xff;
		uint8_t high = (src >> 8) & 0xf;
		p[512 + (3*target)/2] = low & 0xff;
		uint8_t previoushigh = p[512 + (3*target)/2 + 1];
		previoushigh &= 0xf0;
		previoushigh |= high;
		p[512 + (3*target)/2 + 1] = previoushigh & 0xff;
	} else { // if target is odd
		uint8_t high = (src >> 4) & 0xff;
		uint8_t low = src & 0xf;
		p[512 + (3*target)/2 + 1] = high & 0xff;
		uint8_t previouslow = p[512 + (3*target)/2];
		previouslow &= 0xf;
		low = (low << 4);
		previouslow = previouslow | low;
		p[512 + (3*target)/2] = previouslow & 0xff;
	}
}
 
// writes to data area
void writeToDataArea(char * p, char * src, int size, int FATEntry) {
	int bytesWritten = 0;
	int i;
	int previousFATEntry = -1;

	while (bytesWritten < size) { // while we haven't written all the bytes in the file
		previousFATEntry = FATEntry;
		int physicalCluster = 33 + FATEntry - 2; // map to physical address
		int refPoint = bytesWritten;
		for (i = 0; i < 512 ; i++, bytesWritten++) {
			if (bytesWritten == size) { break; } // break if done
			p[physicalCluster*512 + i] = src[i + refPoint] & 0xff;
		}

		if (bytesWritten == size) { break; } // break from outer loop when done
		FATEntry = findNextAvailFATEntry(p); // find next FAT entry
		writeToFATTable(p, (uint16_t) previousFATEntry, (uint16_t) FATEntry); // update FAT table
	}
	
	// indicate this is the last entry for the file
	writeToFATTable(p, previousFATEntry, 4095);
}

int main (int argc, char *argv[]) {
	int fd;
	int fd2;
	struct stat sf2;
	struct stat sf;
	char * p;
	char * src;
	int src_size = 0;
	
	if (fd2 = open(argv[2], O_RDONLY)) { // opens file
		
		// error checking
		if (fd2 < 0) { printf("Error: File %s not found.", argv[2]); exit(EXIT_FAILURE); }
		if (fstat(fd2, &sf2) ==  -1) { printf("Error: couldn't get file stats."); exit(EXIT_FAILURE); };
		
		src_size = sf2.st_size;
		src = mmap(NULL, src_size, PROT_READ, MAP_SHARED, fd2, 0); // maps file
		if ((int) *src == -1) { printf("Error: file mapping failed."); exit(EXIT_FAILURE); }		

		if (fd = open(argv[1], O_RDWR)) { // opens disk file
			if (fstat(fd, &sf) ==  -1) { printf("Error: couldn't get file stats."); exit(EXIT_FAILURE); };
			p = mmap(NULL, sf.st_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0); // maps disk file
			if ((int) *p == -1) { printf("Error: file mapping failed."); exit(EXIT_FAILURE); }		
			int totalNumSectors = getTotalNumberOfSectors(p); // gets total number of sectors
			if (src_size > getFreeSize(totalNumSectors, p)) { printf("Error: Not enough free space in disk image."); exit(EXIT_FAILURE); }
			int offsetFirstAvailDir = findFirstAvailableRootEntry(p); // finds first available root entry for new file
			int firstFATentry = addRootDirEntry(p, src, offsetFirstAvailDir, argv[2], src_size); // writes the root dir entry for new file
			writeToDataArea(p, src, src_size, firstFATentry); // writes file data to data region
		}
	}
	
	if (munmap(src, src_size) == -1) { printf("Error: file mapping failed."); exit(EXIT_FAILURE); }
	if (munmap(p, sf.st_size) == -1) { printf("Error: file mapping failed."); exit(EXIT_FAILURE); }
	if (close(fd) == -1) {printf("Error: couldn't close file."); exit(EXIT_FAILURE);};
	if (close(fd2) == -1) {printf("Error: couldn't close file."); exit(EXIT_FAILURE);};
	
	return 0;
}
