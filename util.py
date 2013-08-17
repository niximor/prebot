#
# PreBot IRC bot software
# (c) Michal Kuchta 2013
#

from storage import db
import re
import socket

_networks = []
quit = False


def listNicks():
    c = db.cursor()
    c.execute("SELECT nick FROM nicks ORDER BY sort ASC")
    return [row["nick"] for row in c.fetchall()]


def getFirstNick():
    c = db.cursor()
    c.execute("SELECT nick FROM nicks ORDER BY sort ASC LIMIT 1")
    row = c.fetchone()
    if row:
        return row["nick"]
    else:
        return None


def isValidIp(ip):
    try:
        socket.inet_aton(ip)
        return True
    except socket.error, e:
        print "%s: %s" % (ip, e)
        return False


def isValidHostname(hostname):
    if len(hostname) > 255:
        return False
    if hostname[-1] == ".":
        hostname = hostname[:-1]  # strip exactly one dot from the right, if present
    allowed = re.compile("(?!-)[A-Z\d-]{1,63}(?<!-)$", re.IGNORECASE)
    return all(allowed.match(x) for x in hostname.split("."))
