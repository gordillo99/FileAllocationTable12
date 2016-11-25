.phony all:
all: diskinfo disklist diskget diskput

diskinfo: diskinfo.c 
	gcc diskinfo.c utilities.c -o diskinfo -lreadline -lhistory -ggdb -pthread -lm
	
disklist: disklist.c 
	gcc disklist.c utilities.c -o disklist -lreadline -lhistory -ggdb -pthread -lm
	
diskget: diskget.c 
	gcc diskget.c utilities.c -o diskget -lreadline -lhistory -ggdb -pthread -lm
	
diskput: diskput.c 
	gcc diskput.c utilities.c -o diskput -lreadline -lhistory -ggdb -pthread -lm

.PHONY clean:
clean:
	-rm -rf *.o *.exe
