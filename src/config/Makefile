# IRCbot build system

CFLAGS+=-I../
OBJS=config.o config_parser.o config_lookup.o config_get.o config_set.o

all: $(OBJS)

config.o: config.c config.h
config_parser.o: config_parser.c config.h
config_lookup.o: config_lookup.c config.h
config_get.o: config_get.c config.h
config_set.o: config_set.c config.h

clean:
	rm -f *.o

.PHONY: all