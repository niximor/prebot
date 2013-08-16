#!/usr/bin/env python

#
# PreBot IRC bot software
# (c) Michal Kuchta 2013
#

import logging
import sys
import event
import os
import glob
from thread import interrupt_main
from modules import Modules
from socketpool import SocketPool
from storage import db
from irc import IrcConnection
from conftools import listNicks

_networks = {}
quit = False
log = logging.getLogger("main")

def getConn(networkName):
    """
        Return connection to a given IRC network.
    """
    return _networks[networkName] if networkName in _networks else None


@event.handler("irc.connected")
def onConnected(eventData, handlerData):
    """
        Fired when connection to IRC network is successfully registered.
        Used to add connection to list of networks.
    """
    _networks[eventData.irc.networkName] = eventData.irc

@event.handler("irc.disconnected")
def onDisconnected(eventData, handlerData):
    networkName = eventData.irc.networkName
    if networkName in _networks:
        del _networks[networkName]


def shutdown():
    interrupt_main()
    event.unlock()


def main():
    handler = logging.StreamHandler(stream=sys.stderr)
    handler.setFormatter(logging.Formatter("%(asctime)s [%(process)s] %(name)s %(levelname)s: %(message)s {%(filename)s:%(lineno)d}"))
    logging.root.addHandler(handler)
    logging.root.setLevel(logging.DEBUG)

    #signal.signal(signal.SIGINT, sighandler)

    log.info("Starting...")

    # Load modules
    m = Modules()
    pluginPath = os.path.normpath("plugins")
    for file in glob.glob(os.path.join(pluginPath, "*.py")):
        file = file.replace(pluginPath, "", 1)

        if file.startswith("/") or file.startswith("\\"):
            file = file[1:]

        m.load(file)

    c = db.cursor()
    c.execute("CREATE TABLE IF NOT EXISTS servers ("
        "enabled INT DEFAULT 1, "
        "host TEXT, "
        "port INT, "
        "user TEXT, "
        "password TEXT NULL, "
        "realname TEXT, "
        "UNIQUE (host,port)"
    ")")

    c.execute("CREATE TABLE IF NOT EXISTS nicks ("
        "nick TEXT, "
        "enabled INT DEFAULT 1, "
        "UNIQUE (nick)"
    ")")

    c.execute("SELECT host, port, user, password, realname FROM servers WHERE enabled = 1")
    for row in c.fetchall():
        # Connect to IRC to all specified networks.
        irc = IrcConnection(
            host=row["host"],
            port=row["port"],
            nick=listNicks(),
            user=row["user"],
            realname=row["realname"],
            password=row["password"]
        )

        irc.connect()

    try:
        while True:
            event.poll()
            SocketPool().poll()
    except KeyboardInterrupt:
        pass

    log.info("Shutdown.")
    irc.close()

if __name__ == "__main__":
    main()
