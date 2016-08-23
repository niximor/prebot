# IRCbot build system - universal plugin makefile

# Name of library that will be generated. Don't modify unless you know
# what you are doing.
LIBNAME=$(PLUGIN).so

# Objects that will be linked into library.
OBJS=

all: $(LIBNAME)

$(LIBNAME): plugin.c
	$(CC) $(CFLAGS) -lm `xml2-config --cflags` -D'PLUGIN_NAME="$(basename $(LIBNAME))"' plugin.c $(OBJS) -shared -o $(LIBNAME) `xml2-config --libs` -lsqlite3

install:
	$(INSTALL) -D $(LIBNAME) $(PREFIX)/plugins/$(LIBNAME)

clean:
	rm -f *.o $(LIBNAME)

uninstall:
	rm -f $(PREFIX)/plugins/$(LIBNAME)

.PHONY: all install clean uninstall
