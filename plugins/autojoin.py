#
# PreBot IRC bot software
# (c) Michal Kuchta 2013
#

from event import handler
from storage import db
from httpd import get, post, menulink
from util import _networks


def init():
    c = db.cursor()
    c.execute("CREATE TABLE IF NOT EXISTS autojoin (network TEXT NULL DEFAULT NULL, channel TEXT, UNIQUE (network, channel));")
    db.commit()


@handler("irc.connected")
def onConnect(eventData, handlerData):
    irc = eventData.irc

    c = db.cursor()
    c.execute("SELECT channel FROM autojoin WHERE network IS NULL OR network = ?", (eventData.irc.networkName, ))
    for row in c.fetchall():
        irc.join(row["channel"])


@menulink("Autojoin", "/autojoin")
def web_main(req):
    # List networks (only connected ones)
    networks = []
    for irc in _networks:
        if irc.networkName:
            networks.append(irc.networkName)

    # List current channels
    channels = []
    c = db.cursor()
    c.execute("SELECT channel, network FROM autojoin ORDER BY network ASC, channel ASC")
    for row in c.fetchall():
        channels.append(row)

    req.render("autojoin/index.html", {
        "networks": networks,
        "channels": channels
    })


@post("/autojoin.add")
def web_add(req):
    network = req.post.getfirst("network")
    channel = req.post.getfirst("channel")

    if network == "":
        network = None

    c = db.cursor()
    c.execute("INSERT OR IGNORE INTO autojoin (network, channel) VALUES (?, ?)", (network, channel))
    db.commit()

    for irc in _networks:
        if irc.registered and (network is None or irc.networkName == network) and not irc.isJoined(channel):
            irc.join(channel)

    req.redirect("/autojoin")


@get("/autojoin.remove")
def web_remove(req):
    network = req.get.getfirst("network")
    channel = req.get.getfirst("channel")

    if network == u"":
        network = None

    c = db.cursor()
    if network is not None:
        c.execute("DELETE FROM autojoin WHERE network = ? AND channel = ?", (network, channel))
    else:
        c.execute("DELETE FROM autojoin WHERE channel = ? AND network IS NULL", (channel, ))
    db.commit()
    req.redirect("/autojoin")

init()
