LIBNAME=$(PLUGIN).so
OBJS=plugin.o
all: $(LIBNAME)
$(LIBNAME): $(OBJS)
	$(CC) $(CFLAGS) -D'PLUGIN_NAME="$(basename $(LIBNAME))"' $(OBJS) -shared  -o $(LIBNAME)

plugin.o: plugin.c

install:
	$(INSTALL) -D $(LIBNAME) $(PREFIX)/plugins/$(LIBNAME)

clean:
	rm -f *.o $(LIBNAME)

uninstall:
	rm -f $(PREFIX)/plugins/$(LIBNAME)

.PHONY: all install clean uninstall
