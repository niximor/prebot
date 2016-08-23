# IRCbot build system

CFLAGS+=-I../
OBJS=dirs.o wildcard.o tb_rand.o tb_string.o

all: $(OBJS)

dirs.o: dirs.c dirs.h
wildcard.o: wildcard.c wildcard.h
tb_rand.o: tb_rand.c tb_rand.h
tb_string.o: tb_string.c tb_string.h

clean:
	rm -f *.o
