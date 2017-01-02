# IRCbot build system

# Name of executable file that will be produced
APPNAME=prebot

# List of plugins you want to compile
#PLUGINS=autojoin telnet telnet_socket_driver telnet_commands commands users \
	dummytalk
PLUGINS=autojoin dummytalk users telnet telnet_socket_driver telnet_commands commands gpxtell gc3c9rx calc seen pivo keepnick kofola linkfetcher

# Installation prefix
PREFIX=$(CURDIR)

# Installer
INSTALL=install

# Get date of build
NOW=`date +"%Y%m%d"`
SVN_VERSION=\"svn$(NOW)\"

# C compiler settings
CC=gcc
CFLAGS=-std=gnu99 -Wall -Wextra -Werror -g -fPIC -I. -O0

# Compile all as SVN build
all: CFLAGS+=-DSVN_VERSION="$(SVN_VERSION)"
all: src plugins

# Compile as stable build, without SVN version info
dist: src plugins

src:
	@$(MAKE) -w -C src

plugins:
	@$(MAKE) -w -C plugins

install: src-install plugins-install

clean: src-clean plugins-clean

src-install:
	@$(MAKE) -w -C src install

src-clean:
	@$(MAKE) -w -C src clean

plugins-install:
	@$(MAKE) -w -C plugins install

plugins-uninstall:
	@$(MAKE) -w -C plugins uninstall

plugins-clean:
	@$(MAKE) -w -C plugins clean

$(APPNAME): src src-install

run:
	@./$(APPNAME)

valgrind:
	@valgrind --leak-check=full --suppressions=support/valgrind-suppressions.txt ./$(APPNAME)

uninstall: plugins-uninstall
	rm -f $(PREFIX)/$(APPNAME)

fullclean: clean uninstall

docker: dist install
	docker build -t registry.lan.gcm.cz/prebot:latest . && docker push registry.lan.gcm.cz/prebot:latest

.PHONY: all src plugins install run valgrind src-install plugins-install \
	run valgrind clean src-clean plugins-clean uninstall fullclean \
	plugins-uninstall dist
.EXPORT_ALL_VARIABLES:
