#
# PreBot IRC bot software
# (c) Michal Kuchta 2013
#

from storage import db
from main import _networks
from irc import IrcConnection

def addServer(host, username, realname, port=6667, password=None):
    c = db.cursor()
    c.execute("SELECT 1 FROM servers WHERE host = %s AND port = %s", (host, port))
    if c.fetchone():
        c.execute("UPDATE servers SET user = %s, realname = %s, password = %s WHERE host = %s AND port = %s", (username, realname, password, host, port))
    else:
        c.execute("INSERT INTO servers (host, port, user, realname, password) VALUES (%s, %s, %s, %s, %s)", (host, port, username, realname, password))

        irc = IrcConnection(
            host=host,
            port=port,
            nick=listNicks(),
            user=username,
            realname=realname,
            password=password
        )
        irc.connect()

    c.commit()

def enableServer(host, port):
    c = db.cursor()
    c.execute("SELECT 1 FROM servers WHERE host = %s AND port = %s AND enabled = 0", (host, port))

    if c.fetchone():
        c.execute("UPDATE servers SET enabled = 1 WHERE host = %s AND port = %s", (host, port))
        c.commit()

        # Connect to IRC if not connected.
        found = False
        for network in _networks.values():
            if network.hostname == host and network.port == port:
                found = True

        if not found:
            # Connect, not connected.
            c.execute("SELECT host, port, user, password, realname FROM servers WHERE host = %s AND port = %s AND enabled = 1", (host, port))
            row = c.fetchone()

            irc = IrcConnection(
                host=row["host"],
                port=row["port"],
                nick=listNicks(),
                user=row["user"],
                realname=row["realname"],
                password=row["password"]
            )

            irc.connect()


def disableServer(host, port):
    c = db.cursor()
    c.execute("SELECT 1 FROM servers WHERE host = %s AND port = %s AND enabled = 1", (host, port))

    if c.fetchone():
        c.execute("UPDATE servers SET enabled = 0 WHERE host = %s AND port = %s", (host, port))
        c.commit()

        # Disconnect from IRC if connected.
        for network in _networks.values():
            if network.hostname == host and network.port == port:
                network.close()

def delServer(host, port):
    c = db.cursor()
    c.execute("DELETE FROM servers WHERE host = %s AND port = %s", (host, port))
    c.commit()

    # Disconnect from IRC if connected.
    for network in _networks.values():
        if network.hostname == host and network.port == port:
            network.close()

def addNick(nick):
    c = db.cursor()
    c.execute("INSERT OR IGNORE INTO nicks (nick) VALUES (%s)", (nick))
    c.commit()

def delNick(nick):
    c = db.cursor()
    c.execute("DELETE FROM nicks WHERE nick = %s", (nick))
    c.commit()

def listNicks():
    c = db.cursor()
    c.execute("SELECT nick FROM nicks WHERE enabled = 1")
    return [row["nick"] for row in c.fetchall()]