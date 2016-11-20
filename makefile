.phony all:
all: diskinfo

diskinfo: diskinfo.c
	gcc diskinfo.c -o diskinfo -lreadline -lhistory -ggdb -pthread -lm

.PHONY clean:
clean:
	-rm -rf *.o *.exe
