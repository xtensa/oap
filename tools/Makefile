#DEFS           = -D DEBUG 
LIBS           =
CC             = gcc 

OPTIMIZE = -Os
CFLAGS         = -c -g -Wall $(OPTIMIZE) ${DEFS}
LDFLAGS        = -lcurses 


all: oap_console oap_mitm oap_unit_testing
	@cat /dev/null 


oap_console.c: ../lib/oap.h 
	@cat /dev/null 

oap_console.o: oap_console.c ../lib/oap.h
	[ -f ../lib/oap.h.gch ] && rm ../lib/oap.h.gch
	$(CC) $(CFLAGS) $^ $(LIBS)

oap_console: oap_console.o
	$(CC) -o $@ $^ $(LIBS) $(LDFLAGS) 



oap_unit_testing.c: ../lib/oap.h
	@cat /dev/null 

oap_unit_testing.o: oap_unit_testing.c ../lib/oap.h 
	[ -f ../lib/oap.h.gch ] && rm ../lib/oap.h.gch
	$(CC) $(CFLAGS) $^ $(LIBS)

oap_unit_testing: oap_unit_testing.o
	$(CC) -o $@ $^ $(LIBS) $(LDFLAGS) 





oap_mitm.c: ../lib/oap.h 
	@cat /dev/null 

oap_mitm.o: oap_mitm.c ../lib/oap.h
	rm ../lib/oap.h.gch
	$(CC) $(CFLAGS) $^ $(LIBS)

oap_mitm: oap_mitm.o
	$(CC) -o $@ $^ $(LIBS) $(LDFLAGS) 




clean:
	rm -rf *.o oap_console oap_mitm oap_unit_testing
