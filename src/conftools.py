#
# PreBot IRC bot software
# (c) Michal Kuchta 2013
#

from storage import db
from irc import IrcConnection
from util import listNicks, getFirstNick, _networks


def addServer(host, username, realname, port=6667, password=None, id=None):
    c = db.cursor()
    c.execute(
        "SELECT id, host, port "
        "FROM servers "
        "WHERE "
        "   (? IS NOT NULL AND id = ?) "
        "   OR (host = ? AND port = ?)", (id, id, host, port))
    row = c.fetchone()
    if row:
        id = row["id"]
        c.execute(
            "UPDATE servers "
            "SET "
            "   host = ?, "
            "   port = ?, "
            "   user = ?, "
            "   realname = ?, "
            "   password = ? "
            "WHERE "
            "   id = ?",
            (host, port, username, realname, password, host, port, id))

        # If host and port changed, reconnect.
        if (row["host"] != host or row["port"] != port):
            # Stop old connection.
            for irc in _networks:
                if irc.hostname == row["host"] and irc.port == row["port"]:
                    irc.quit("Changing servers...")
                    _networks.remove(irc)

            nick = getFirstNick()

            if nick is not None:
                # Create new one.
                irc = IrcConnection(
                    host=host,
                    port=port,
                    nick=nick,
                    user=username,
                    realname=realname,
                    password=password
                )
                irc.connect()
    else:
        c.execute(
            "INSERT INTO servers "
            "(host, port, user, realname, password) "
            "VALUES (?, ?, ?, ?, ?)",
            (host, port, username, realname, password))

        nick = getFirstNick()
        if nick is not None:
            irc = IrcConnection(
                host=host,
                port=port,
                nick=nick,
                user=username,
                realname=realname,
                password=password
            )
            irc.connect()

    db.commit()


def enableServer(id):
    c = db.cursor()
    c.execute(
        "SELECT host, port "
        "FROM servers "
        "WHERE "
        "   id = ? "
        "   AND enabled = 0", (id, ))

    row = c.fetchone()
    if row:
        c.execute(
            "UPDATE servers "
            "SET "
            "   enabled = 1 "
            "WHERE "
            "   id = ?", (id, ))
        db.commit()

        # Connect to IRC if not connected.
        found = False
        for network in _networks:
            if network.hostname == row["host"] and network.port == row["port"]:
                found = True

        if not found:
            # Connect, not connected.
            c.execute(
                "SELECT host, port, user, password, realname "
                "FROM servers "
                "WHERE "
                "   id = ? "
                "   AND enabled = 1", (id, ))
            row = c.fetchone()

            nick = getFirstNick()
            if nick is not None:
                irc = IrcConnection(
                    host=row["host"],
                    port=row["port"],
                    nick=nick,
                    user=row["user"],
                    realname=row["realname"],
                    password=row["password"]
                )

                irc.connect()


def disableServer(id):
    c = db.cursor()
    c.execute(
        "SELECT host, port "
        "FROM servers "
        "WHERE "
        "   id = ? "
        "   AND enabled = 1", (id, ))

    row = c.fetchone()
    if row:
        c.execute("UPDATE servers SET enabled = 0 WHERE id = ?", (id, ))
        db.commit()

        # Disconnect from IRC if connected.
        for network in _networks:
            if network.hostname == row["host"] and network.port == row["port"]:
                network.close()
                _networks.remove(network)


def delServer(id):
    c = db.cursor()
    c.execute("SELECT host, port FROM servers WHERE id = ?", (id, ))
    row = c.fetchone()
    if row:
        c.execute("DELETE FROM servers WHERE id = ?", (id, ))
        db.commit()

        # Disconnect from IRC if connected.
        for network in _networks:
            if network.hostname == row["host"] and network.port == row["port"]:
                network.close()
                _networks.remove(network)


def addNick(nick):
    c = db.cursor()
    c.execute("SELECT MAX(sort) AS sort FROM nicks")
    row = c.fetchone()
    if row and row["sort"] is not None:
        sort = row["sort"] + 1
    else:
        sort = 1

    c.execute(
        "INSERT OR IGNORE INTO nicks "
        "(nick, sort) "
        "VALUES (?, ?)", (nick, sort))
    db.commit()


def delNick(nick):
    c = db.cursor()
    c.execute("DELETE FROM nicks WHERE nick = ?", (nick))

    # Update sorting
    c.execute("SELECT nick FROM nicks ORDER BY sort ASC")
    sort = 1
    for row in c.fetchall():
        c.execute(
            "UPDATE nicks "
            "SET "
            "   sort = ? "
            "WHERE "
            "   nick = ?", (row["nick"], sort))
        sort += 1

    db.commit()


def moveUp(nick):
    c = db.cursor()
    c.execute("SELECT sort FROM nicks WHERE nick = ?", (nick, ))
    row = c.fetchone()
    if row:
        if row["sort"] > 1:
            c.execute(
                "UPDATE nicks "
                "SET "
                "   sort = sort + 1 "
                "WHERE "
                "   sort = ?", (row["sort"] - 1, ))
            c.execute(
                "UPDATE nicks "
                "SET "
                "   sort = sort - 1 "
                "WHERE "
                "   nick = ?", (nick, ))

    db.commit()


def moveDown(nick):
    c = db.cursor()
    c.execute("SELECT sort FROM nicks WHERE nick = ?", (nick, ))
    row = c.fetchone()
    if row:
        c.execute("SELECT MAX(sort) AS sort FROM nicks")
        if row["sort"] < c.fetchone()["sort"]:
            c.execute(
                "UPDATE nicks "
                "SET "
                "   sort = sort - 1 "
                "WHERE "
                "   sort = ?", (row["sort"] + 1, ))
            c.execute(
                "UPDATE nicks "
                "SET "
                "   sort = sort + 1 "
                "WHERE "
                "   nick = ?", (nick, ))

    db.commit()
