#DEFS           = -D DEBUG 
LIBS           =
CC             = gcc 

OPTIMIZE = -Os
CFLAGS         = -c -g -Wall $(OPTIMIZE) ${DEFS}
LDFLAGS        = -lcurses


all: oap_console oap_mitm
	@cat /dev/null 


oap_console.c: ../lib/oap.h 
	@cat /dev/null 

oap_console.o: oap_console.c 
	$(CC) $(CFLAGS) $^ $(LIBS)

oap_console: oap_console.o
	$(CC) $(LDFLAGS) -o $@ $^ $(LIBS)





oap_mitm.c: ../lib/oap.h 
	@cat /dev/null 

oap_mitm.o: oap_mitm.c 
	$(CC) $(CFLAGS) $^ $(LIBS)

oap_mitm: oap_mitm.o
	$(CC) $(LDFLAGS) -o $@ $^ $(LIBS)




clean:
	rm -rf *.o oap_console oap_mitm
