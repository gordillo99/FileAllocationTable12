.phony all:
all: diskinfo disklist diskget diskput

diskinfo: diskinfo.c
	gcc diskinfo.c -o diskinfo -lreadline -lhistory -ggdb -pthread -lm
	
disklist: disklist.c
	gcc disklist.c -o disklist -lreadline -lhistory -ggdb -pthread -lm
	
diskget: diskget.c
	gcc diskget.c -o diskget -lreadline -lhistory -ggdb -pthread -lm
	
diskput: diskput.c
	gcc diskput.c -o diskput -lreadline -lhistory -ggdb -pthread -lm

.PHONY clean:
clean:
	-rm -rf *.o *.exe
