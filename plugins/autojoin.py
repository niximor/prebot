#
# PreBot IRC bot software
# (c) Michal Kuchta 2013
#

from event import handler
from storage import db

def init():
    c = db.cursor()
    c.execute("CREATE TABLE IF NOT EXISTS autojoin (network TEXT NULL DEFAULT NULL, channel TEXT);")


@handler("irc.connected")
def onConnect(eventData, handlerData):
    c = db.cursor()
    c.execute("SELECT channel FROM autojoin WHERE network IS NULL OR netwosk = %s", eventData.irc.network)

init()