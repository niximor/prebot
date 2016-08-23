# IRCbot build system

CFLAGS+=-I../
OBJS=irclib.o irc_commands.o irc_parser.o irc_address.o irc_channels.o \
	irc_users.o
HEADS=irclib.h irc_events.h irc_functions.h

all: $(OBJS)

irclib.o: irclib.c $(HEADS)
irc_commands.o: irc_commands.c $(HEADS)
irc_parser.o: irc_parser.c $(HEADS)
irc_address.o: irc_address.c $(HEADS)
irc_channels.o: irc_channels.c $(HEADS)
irc_users.o: irc_users.c $(HEADS)

clean:
	rm -f *.o