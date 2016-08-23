# IRCbot build system (modules part)

CFLAGS+=-I../../src

all: $(PLUGINS)

$(PLUGINS):
	@$(MAKE) -w -C $@ PLUGIN=$@ $(PLUG_MAKE_TARGET)

install:
	@$(MAKE) PLUG_MAKE_TARGET=install
#	@$(foreach DIR, $(PLUGINS), $(MAKE) -w -C $(DIR) PLUGIN=$(DIR) install;)

clean:
	@$(MAKE) PLUG_MAKE_TARGET=clean
#	@$(foreach DIR, $(PLUGINS), $(MAKE) -w -C $(DIR) PLUGIN=$(DIR) clean;)

uninstall:
	@$(MAKE) PLUG_MAKE_TARGET=uninstall
#	@$(foreach DIR, $(PLUGINS), $(MAKE) -w -C $(DIR) PLUGIN=$(DIR) uninstall;)

.PHONY: all install clean uninstall $(PLUGINS)
.EXPORT_ALL_VARIABLES: