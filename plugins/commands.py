#
# PreBot IRC bot software
# (c) Michal Kuchta 2013
#

from src.event import handler, trigger

COMMAND_PREFIX="?"

class CommandEvent(object):
	def __init__(self, parent, command, text):
		self.parent = parent
		self.command = command
		self.text = text

@handler("irc.chanmsg")
@handler("irc.privmsg")
def onmsg(eventData, handlerData):
	if eventData.text.startswith(COMMAND_PREFIX):
		pos = eventData.text.find(" ")
		command = eventData.text[1:pos]
		text = eventData.text[1+len(command):].strip()
		trigger("command.%s" % command, CommandEvent(eventData, command, text))
