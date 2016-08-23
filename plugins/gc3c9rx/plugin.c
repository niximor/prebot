#ifndef _GNU_SOURCE
# define _GNU_SOURCE
#endif

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/wait.h>

#include <pluginapi.h>
#include <events.h>
#include <toolbox/tb_string.h>
#include <toolbox/linkedlist.h>
#include <timers.h>

#include <ctype.h>
#include <string.h>

#include <signal.h>

typedef struct {
	PluginInfo *info;
	EVENT_HANDLER *onmessage;
	EVENT_HANDLER *onquery;
	EVENT_HANDLER *onjoin;
	EVENT_HANDLER *onnick;
} GC3C9RXPluginData;

bool gc3c9rx_timer(Timer timer) {
	IRCLib_Connection *irc = (IRCLib_Connection *)timer->customData;
	irclib_message(irc, "#GC3C9RX", "Tip dne: Pokud chceme nekoho na IRC oslovit, zacneme svou zpravu jeho nickem nasledovanym dvojteckou.");

	return false; // Do not repeat.
}

void gc3c9rx_message(EVENT *event) {
	IRCEvent_Message *message = (IRCEvent_Message *)event->customData;
	GC3C9RXPluginData *data = (GC3C9RXPluginData *)event->handlerData;

	// Work only on #GC3C9RX.
	char *chan = message->channel;
	if (chan != NULL) {
		for (size_t i = 0; i < strlen(chan); i++) {
			chan[i] = tolower(chan[i]);
		}
	}

	if (chan == NULL || strcmp(chan, "#gc3c9rx") != 0) {
		return;
	}

	char *recipient = message->address->nick;

	char cmpBuff[128];
	sprintf(cmpBuff, "%s:", message->sender->nickname);

	if (strncmp(message->message, cmpBuff, strlen(cmpBuff)) == 0) {
		if (strncmp(message->address->nick, "XS4-", 4) == 0) {
			irclib_message(message->sender, message->channel,
				"%s: S anonymnimi identitami se nebavim. Zmen si nick na neco smysluplneho pomoci /nick <tvujnick>. Idealne tvuj gc.com nick :)",
				message->address->nick);
			return;
		}

		int rpipe[2], wpipe[2];
		pipe(rpipe);
		pipe(wpipe);

		char out[1024];

		int res = vfork();

		if (res == 0) {
			close(wpipe[1]);
			close(rpipe[0]);
			dup2(wpipe[0], STDIN_FILENO);
			dup2(rpipe[1], STDOUT_FILENO);
			close(wpipe[0]);
			close(rpipe[1]);
			execl("/usr/bin/iconv", "/usr/bin/iconv", "-f", "utf8", "-t", "ascii//translit", (char *)NULL);
			_exit(0);
		} else if (res > 0) {
			close(wpipe[0]);
			close(rpipe[1]);

			FILE *iconv_in = fdopen(wpipe[1], "w");
			FILE *iconv_out = fdopen(rpipe[0], "r");

			//int childExitStatus;
			//waitpid(res, &childExitStatus, 0);

			if (iconv_in && iconv_out) {
				fprintf(iconv_in, "%s", message->message);
				fclose(iconv_in);

				fgets(out, 1023, iconv_out);
				fclose(iconv_out);
			} else {
				perror("fdopen");
				if (strlen(message->message) < 1023) {
					memcpy(out, message->message, strlen(message->message) + 1);
				} else {
					memcpy(out, message->message, 1023);
					out[1023] = '\0';
				}
			}
		} else {
			perror("fork");
			if (strlen(message->message) < 1023) {
				memcpy(out, message->message, strlen(message->message) + 1);
			} else {
				memcpy(out, message->message, 1023);
				out[1023] = '\0';
			}
		}

		for (size_t i = 0; i < strlen(out); i++) {
			out[i] = tolower(out[i]);
		}

		int has_please = 0;
		int has_coords = 0;
		int has_polite = 0;
		int has_want = 0;
		int has_final = 0;

		if (strstr(out, "souradnice") != NULL) has_coords = 1;
		else if (strstr(out, "souradnic") != NULL) has_coords = 1;
		else if (strstr(out, "souradky") != NULL) has_coords = 1;

		if (strstr(out, "prosim") != NULL) has_please = 1;
		else if (strstr(out, "poprosit") != NULL) has_please = 1;

		if (strstr(out, "slusne") != NULL) has_polite = 1;

		if (strstr(out, "zadam") != NULL) has_want = 1;
		if (strstr(out, "pozadat") != NULL) has_want = 1;

		if (strstr(out, "final") != NULL) has_final = 1;

		if (has_coords && has_please) {
			FILE *fr = fopen(config_getvalue_string(data->info->config, "gc3c9rx:message", "gc3c9rx.txt"), "r");
		
			if (fr) {
				char buff[1024];
				memset(buff, 0, 1024);

				if (fread(buff, sizeof(char), 1024, fr) > 1023) {
					buff[1023] = '\0';
				}

				irclib_message(message->sender, recipient, "%s", buff);
				irclib_message(message->sender, message->channel,
					"%s: Perfektni, hledej v okne se soukromymi zpravami.",
					message->address->nick);

				fclose(fr);
			} else {
				irclib_message(message->sender, message->channel,
					"Bot error! No coordinates given (%s).", strerror(errno));
			}
		} else {
			if (has_coords) {
				irclib_message(message->sender, message->channel,
					"%s: Souradnice? Ty bych mel, jo... ale chtel bych slyset kouzelne slovicko...", message->address->nick);
			} else if (has_please) {
				irclib_message(message->sender, message->channel,
					"%s: Prosis pekne, ale musis taky rict o co prosis.", message->address->nick);
			} else if (has_final) {
				irclib_message(message->sender, message->channel,
					"%s: Pro finalku si musis dojit sam... Ja ti muzu dat jen jeji souradnice...", message->address->nick);
			} else if (has_want && has_polite) {
				irclib_message(message->sender, message->channel,
					"%s: A kouzelne slovicko zni...?", message->address->nick);
			} else {
				irclib_message(message->sender, message->channel,
					"%s: Sama voda, kouzelna formule zni jinak!", message->address->nick); 
			}
		}
	} else if (strstr(message->message, message->sender->nickname) != NULL) {
		static int counter = 0;

		if (++counter % 5 == 0) {
			timers_add(TM_TIMEOUT, 15, gc3c9rx_timer, message->sender);
		}

		irclib_message(message->sender, message->channel,
			"hmm, jako bych ve vetru zaslechl sve jmeno...");
	}
}

void gc3c9rx_query(EVENT *event) {
	IRCEvent_Message *message = (IRCEvent_Message *)event->customData;

	IRCLib_User user = irclib_find_user(message->sender->userStorage, message->address->nick);
	IRCLib_UserChannel chan = user->first;

	while (chan != NULL) {
		if (eq(chan->channel->name, "#gc3c9rx")) {
			irclib_message(
				message->sender,
				message->address->nick,
				"Tady se odemne nic nedozvis. Pokud chces souradnice, musis mi psat verejne na kanal #GC3C9RX."
			);

			break;
		}

		chan = chan->next;
	}
}

void gc3c9rx_join(EVENT *event) {
	IRCEvent_JoinPart *join = (IRCEvent_JoinPart *)event->customData;
	if (strncmp(join->address->nick, "XS4-", 4) != 0 && strcmp(join->channel, "#gc3c9rx") == 0) {
		irclib_message(join->sender, join->channel, "%s: oh hai! \\o_", join->address->nick);
	}
}

void gc3c9rx_nick(EVENT *event) {
	IRCEvent_NickChange *nick = (IRCEvent_NickChange *)event->customData;

	// Change from XS4-* to something better...
	if (strncmp(nick->address->nick, "XS4-", 4) == 0 && strncmp(nick->newnick, "XS4-", 4) != 0) {
		IRCLib_User user = irclib_find_user(nick->sender->userStorage, nick->address->nick);
		if (user == NULL) {
			user = irclib_find_user(nick->sender->userStorage, nick->newnick);
		}

		// Only if user is on #gc3c9rx.
		if (user != NULL) {
			ll_loop(user, channel) {
				if (strcmp(channel->channel->name, "#gc3c9rx") == 0){
					irclib_message(nick->sender, "#gc3c9rx", "%s: on hai! \\o_", nick->newnick);
					break;
				}
			}
		}
	}
}

void PluginInit(PluginInfo *info) {
	info->name = "GC3C9RX Solver";
	info->author = "Niximor";
	info->version = "1.0.1";

	GC3C9RXPluginData *plugData = malloc(sizeof(GC3C9RXPluginData));
	plugData->info = info;

	plugData->onmessage = events_addEventListener(info->events,
		"onchannelmessage", gc3c9rx_message, plugData);
	plugData->onquery = events_addEventListener(info->events,
		"onprivatemessage", gc3c9rx_query, plugData);
	plugData->onjoin = events_addEventListener(info->events,
		"onjoin", gc3c9rx_join, plugData);
	plugData->onnick = events_addEventListener(info->events,
		"onnick", gc3c9rx_nick, plugData);

	info->customData = plugData;

	// Do not collect return values from childrens,
	// to allow to successfully terminate them.
	signal(SIGCHLD, SIG_IGN);
}

void PluginDone(PluginInfo *info) {
	GC3C9RXPluginData *plugData = (GC3C9RXPluginData *)info->customData;
	events_removeEventListener(plugData->onmessage);
	events_removeEventListener(plugData->onquery);
	events_removeEventListener(plugData->onjoin);
	events_removeEventListener(plugData->onnick);

	free(plugData);
}
