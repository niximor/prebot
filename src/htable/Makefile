# IRCbot build system

CFLAGS+=-I../
OBJS=htval.o keyvalpair.o

all: $(OBJS)

htval.o: htval.c htval.h
keyvalpair.o: keyvalpair.c keyvalpair.h

clean:
	rm -f *.o