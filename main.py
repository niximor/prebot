#!/usr/bin/env python

#
# PreBot IRC bot software
# (c) Michal Kuchta 2013
#

import logging
import sys
import event
import signal
from thread import interrupt_main

from irc import IrcConnection

_networks = {}
quit = False
log = logging.getLogger("main")

def getConn(networkName):
    """
        Return connection to a given IRC network.
    """
    return _networks[networkName]


def onConnected(eventData=None):
    """
        Fired when connection to IRC network is successfully registered.
        Used to add connection to list of networks.
    """
    _networks[eventData["irc"].networkName] = eventData["irc"]


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

    # TODO: Init config
    event.register("irc.connect", onConnected)

    irc = IrcConnection(
        host="10.10.1.1",
        port=6668,
        nick=["gcm", "gcm_", "gcm__"],
        user="gcm",
        realname="GCM IRC Bot"
    )

    irc.connect()

    try:
        while True:
            event.poll()
    except KeyboardInterrupt:
        pass

    log.info("Shutdown.")
    irc.close()

if __name__ == "__main__":
    main()
