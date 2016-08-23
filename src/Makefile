# IRCBot build system (main source part)

# List of modules that are in their directories
MODULES=irclib config toolbox htable

# Objects that should be build into main binary
OBJS=dynastring.o events.o io.o main.o tokenizer.o socketpool.o plugins.o \
	timers.o

all: $(APPNAME)

$(APPNAME): $(OBJS) $(MODULES)
	$(CC) $(CFLAGS) -export-dynamic -rdynamic $(OBJS) -ldl $(addsuffix /*.o, $(MODULES)) -o $@

main.o: main.c main.h
events.o: events.c events.h
io.o: io.c io.h
tokenizer.o: tokenizer.c tokenizer.h
socketpool.o: socketpool.c socketpool.h
plugins.o: plugins.c plugins.h

$(MODULES):
	@$(MAKE) -w -C $@ $(MOD_MAKE_TARGET)

modules: $(MODULES)
	#@$(foreach DIR,$(MODULES),$(MAKE) -w -C $(DIR);)

install:
	$(INSTALL) -D $(APPNAME) $(PREFIX)/$(APPNAME)

modules-clean:
	$(MAKE) modules MOD_MAKE_TARGET=clean
#	@$(foreach DIR,$(MODULES),$(MAKE) -w -C $(DIR) clean;)

clean: modules-clean
	rm -f *.o $(APPNAME)

.PHONY: all install clean modules $(MODULES)
.EXPORT_ALL_VARIABLES:
