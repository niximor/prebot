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
from util import listNicks, getFirstNick, _networks, quit
from httpd import HTTPServer

log = logging.getLogger(__name__)


def shutdown():
    interrupt_main()
    event.unlock()


def main():
    handler = logging.StreamHandler(stream=sys.stderr)
    handler.setFormatter(logging.Formatter(
      "%(asctime)s "
      "[%(process)s] "
      "%(name)s "
      "%(levelname)s: "
      "%(message)s "
      "{%(filename)s:%(lineno)d}"))
    logging.root.addHandler(handler)
    logging.root.setLevel(logging.DEBUG)

    #signal.signal(signal.SIGINT, sighandler)

    log.info("Starting...")

    SocketPool()

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
              "id INTEGER PRIMARY KEY AUTOINCREMENT, "
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
              "sort INT, "
              "UNIQUE (nick)"
              ")")

    nick = getFirstNick()
    if nick is not None:
        c.execute(
          "SELECT host, port, user, password, realname "
          "FROM servers "
          "WHERE enabled = 1")
        gotServer = False
        for row in c.fetchall():
            # Connect to IRC to all specified networks.
            irc = IrcConnection(
                host=row["host"],
                port=row["port"],
                nick=nick,
                user=row["user"],
                realname=row["realname"],
                password=row["password"]
            )

            irc.connect()
            gotServer = True

        if not gotServer:
            log.info("No servers configured.")
    else:
        log.info("No nicks configured.")

    httpd = HTTPServer(('0.0.0.0', 31331))

    try:
        while True:
            event.poll()
            SocketPool().poll()
    except KeyboardInterrupt:
        pass

    log.info("Shutdown.")
    for irc in _networks:
        irc.close()

    del httpd

if __name__ == "__main__":
    main()
