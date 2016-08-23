# IRCbot build system - universal plugin makefile

CFLAGS += -I/usr/include/python2.5

# Name of library that will be generated. Don't modify unless you know
# what you are doing.
LIBNAME=$(PLUGIN).so

# Objects that will be linked into library.
OBJS=plugin.o py_api_plugin.o py_api_install.o py_api.o py_api_user.o py_api_channel.o pyplugin_events.o

all: $(LIBNAME)

$(LIBNAME): $(OBJS)
	$(CC) $(CFLAGS) -D'PLUGIN_NAME="$(basename $(LIBNAME))"' -lpython2.5 $(OBJS) -shared -o $(LIBNAME)

plugin.o: plugin.c interface.h
py_api.o: py_api.c interface.h
py_api_user.o: py_api_user.c interface.h
py_api_channel.o: py_api_channel.c interface.h
py_api_install.o: py_api_install.c interface.h
py_api_plugin.o: py_api_plugin.c interface.h
pyplugin_events.o: pyplugin_events.c interface.h

install:
	$(INSTALL) -D $(LIBNAME) $(PREFIX)/plugins/$(LIBNAME)

clean:
	rm -f *.o $(LIBNAME)

uninstall:
	rm -f $(PREFIX)/plugins/$(LIBNAME)

.PHONY: all install clean uninstall
