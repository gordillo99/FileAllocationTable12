int getSectorValue(char * p, int i);
int getTotalNumberOfSectors(char * p);
const char *byte_to_binary(int x);
const char *byte_to_binary16(int x);
int getFreeSize(int totalNumberOfSectors, char * p);
int findFile(char * p, char * f_to_find, int * size);
void getFileName(char * p, int entry, char * name);
