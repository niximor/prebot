#ifndef _GNU_SOURCE
# define _GNU_SOURCE
#endif

#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

#include <pluginapi.h>
#include <io.h>
#include <plugins.h>

#ifndef PLUGIN_NAME
# define PLUGIN_NAME "linkfetcher"
#endif

typedef struct {
	PluginInfo *info;
	EVENT_HANDLER *onmessage;
} LinkFetcherPluginData;

typedef struct {
	char *target;
	PluginInfo *info;
	string s;
} LinkFetcherMessage;

void linkfetcher_read(Socket socket) {
	LinkFetcherMessage *dt = (LinkFetcherMessage *)socket->customData;

#define BUFF_SIZE 4096
	char buffer[BUFF_SIZE];
	int readed;
	memset(buffer, 0, BUFF_SIZE);
	while ((readed = read(socket->socketfd, buffer, BUFF_SIZE - 1)) > 0) {
		dynastring_appendstring(dt->s, buffer);
		memset(buffer, 0, BUFF_SIZE);
	}

	// Handle multi-line options.
	char *nl;
	char *oldpos = dynastring_getstring(dt->s);
	int newline = 0;
	while ((nl = strstr(oldpos, "\n")) != NULL) {
		*nl = '\0';
		irclib_message(dt->info->irc, dt->target, "%s", oldpos);
		oldpos = nl + 1;
		newline = 1;
	}

	if (newline) {
		string s2 = dynastring_init();
		dynastring_appendstring(s2, oldpos);
		dynastring_free(dt->s);
		dt->s = s2;
	}

	if (readed == 0) {
		socketpool_close(socket->pool, socket->socketfd);
	}
}

void linkfetcher_closed(Socket socket) {
	LinkFetcherMessage *dt = (LinkFetcherMessage *)socket->customData;
	irclib_message(dt->info->irc, dt->target, "%s", dynastring_getstring(dt->s));
	dynastring_free(dt->s);
}

void linkfetcher_message(EVENT *event) {
	LinkFetcherPluginData *dt = (LinkFetcherPluginData *)event->handlerData;
	IRCEvent_Message *message = (IRCEvent_Message *)event->customData;

	// Ignore private messages, only works on channels.
	if (!message->channel) return;

	char *str = strdup(message->message);

	char *url = strstr(str, "http://");
	if (!url) url = strstr(str, "https://");

	if (!url) return;

	char *end = strstr(url, " ");
	if (!end) end = strstr(url, ",");
	if (end) {
		*end = '\0';
	}

	printError(PLUGIN_NAME, "Fetch URL %s", url);

	int link[2];
	if (pipe(link) == -1) {
		return;
	}

	pid_t pid;
	if ((pid = fork()) == -1) {
		return;
	}

	if (pid == 0) {
	    dup2(link[1], STDOUT_FILENO);
		dup2(link[1], STDERR_FILENO);
	    close(link[0]);
	    execl("fetch.py", "fetch.py", url, (char *)0);
	} else {
	    close(link[1]);

		LinkFetcherMessage *msg = malloc(sizeof(LinkFetcherMessage));
		msg->target = strdup(message->channel);
		msg->info = dt->info;
		msg->s = dynastring_init();

		socketpool_add(dt->info->socketpool, link[0], linkfetcher_read, NULL, linkfetcher_closed, msg);
	}

	free(str);
}

void PluginInit(PluginInfo *info) {
	info->name = "Link fetcher";
	info->author = "Niximor";
	info->version = "1.0.0";

	LinkFetcherPluginData *dt = malloc(sizeof(LinkFetcherPluginData));
	memset(dt, 0, sizeof(LinkFetcherPluginData));
	dt->info = info;
	info->customData = dt;

	events_addEventListener(info->events, "onchannelmessage", linkfetcher_message, dt);
}

void PluginDone(PluginInfo *info) {
	LinkFetcherPluginData *dt = (LinkFetcherPluginData *)info->customData;
	if (!dt) return;

	if (dt->onmessage) {
		events_removeEventListener(dt->onmessage);
	}
	free(dt);
}

void PluginDeps(char **deps) {
	*deps = NULL;
}
