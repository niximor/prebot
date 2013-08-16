#
# PreBot IRC bot software
# (c) Michal Kuchta 2013
#

from event import handler

@handler("irc.ping")
def pingReply(eventData, handlerData):
    eventData.irc.raw("PONG :%s" % eventData.sender)