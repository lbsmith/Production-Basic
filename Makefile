CC=gcc
#CFLAGS=-O3 -Wall -I../gc
CFLAGS=-ggdb -Wall -I../gc
LIBS=-lm -lreadline -ltermcap

OBJECTS=src/basic.o src/storage.o src/stack.o src/token.o src/executor.o src/runtime.o \
		src/byteswap.o src/tables.o src/lister.o src/utility.o src/functions.o

basic: $(OBJECTS)
#	$(CC) -pg -o basic $(OBJECTS) $(LIBS) ../gc/gc.a
	$(CC) -o basic $(OBJECTS) $(LIBS) src/gc/.libs/libgc.a

clean: 
	rm -f $(OBJECTS) basic

basic.o: basic.c basic.h errors.h opcodes.h

storage.o: storage.c basic.h errors.h opcodes.h

stack.o: stack.c basic.h errors.h opcodes.h

token.o: token.c basic.h errors.h opcodes.h

executor.o: executor.c basic.h errors.h opcodes.h

runtime.o: runtime.c basic.h errors.h opcodes.h

lister.o: lister.c basic.h errors.h opcodes.h
 
byteswap.o: byteswap.c basic.h errors.h opcodes.h

tables.o: tables.c basic.h errors.h opcodes.h

utility.o: utility.c basic.h errors.h opcodes.h

functions.o: functions.c basic.h errors.h opcodes.h
