/**
 * Plugin that tries to regain back stolen nick.
 */

#include <pluginapi.h>
#include <toolbox/tb_string.h>

typedef struct {
	Timer tmnick;
} KeepNickPluginData;

bool keepnick_timer(Timer t) {
	PluginInfo *info = (PluginInfo *)t->customData;

	const char *targetNick = config_getvalue_string(info->config, "irc:nickname", NULL);

	if (info->irc->status == IRC_CONNECTED && targetNick != NULL && !eq(info->irc->nickname, targetNick)) {
		irclib_sendraw(info->irc, "NICK %s", targetNick);
	}

	return true;
}

void PluginInit(PluginInfo *info) {
	KeepNickPluginData *dt = malloc(sizeof(KeepNickPluginData));
	dt->tmnick = timers_add(TM_TIMEOUT, 30, keepnick_timer, info);
	info->customData = dt;
}

void PluginDone(PluginInfo *info) {
	KeepNickPluginData *dt = (KeepNickPluginData *)info->customData;
	timers_remove(dt->tmnick);
	free(dt);
}
