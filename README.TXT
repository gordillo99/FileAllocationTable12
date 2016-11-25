To compile code, have the following files in the same directory:

diskget.c
diskinfo.c
disklist.c
diskput.c
utilities.c
utilities.h
makefile

Then, run make in the directory where all the files are.
The program can now be run with one of the following (depending on what you want to do):

./diskinfo disk
./disklist disk
./diskget disk file
./diskput disk file
