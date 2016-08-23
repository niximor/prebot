# IRCbot build system - universal plugin makefile

# Name of library that will be generated. Don't modify unless you know
# what you are doing.
LIBNAME=$(PLUGIN).so
CFLAGS+=-D'PLUGIN_NAME="$(basename $(LIBNAME))"'

# Objects that will be linked into library.
OBJS=plugin.o users_telnet_admin.o users_telnet_login.o users_list.o users_interface.o

all: $(LIBNAME)

$(LIBNAME): $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -shared -o $(LIBNAME)

plugin.o: plugin.c interface.h
users_telnet_admin.o: users_telnet_admin.c interface.h
users_telnet_login.o: users_telnet_login.c interface.h
users_list.o: users_list.c interface.h
users_interface.o: users_interface.c interface.h

install:
	$(INSTALL) -D $(LIBNAME) $(PREFIX)/plugins/$(LIBNAME)

clean:
	rm -f *.o $(LIBNAME)

uninstall:
	rm -f $(PREFIX)/plugins/$(LIBNAME)

.PHONY: all install clean uninstall
