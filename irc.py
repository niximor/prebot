#
# PreBot IRC bot software
# (c) Michal Kuchta 2013
#

from types import *
import socket
import logging
import event
from socketpool import SocketPool
import re
from util import _networks


def _checkArgument(arg, args, requestedType):
        if arg not in args:
            raise KeyError("%s is mandatory configuration parameter." % arg)

        currentType = type(args[arg])

        # unicode and string are equal.
        if currentType == type(u"") and requestedType == type(""):
            currentType = type("")

        if currentType != requestedType:
            raise AttributeError("%s must be %s, but is %s." % (arg, requestedType, currentType))


class IrcServerEvent:
    def __init__(self, irc):
        self.irc = irc


class RawReceive(IrcServerEvent):
    def __init__(self, irc, line):
        IrcServerEvent.__init__(self, irc)
        self.line = line


class RawSend(IrcServerEvent):
    def __init__(self, irc, line):
        IrcServerEvent.__init__(self, irc)
        self.line = line


class ServerMessage(IrcServerEvent):
    def __init__(self, irc, sender, code, text):
        IrcServerEvent.__init__(self, irc)
        self.sender = sender
        self.code = code
        self.text = text


class Ping(IrcServerEvent):
    def __init__(self, irc, sender):
        IrcServerEvent.__init__(self, irc)
        self.sender = sender


class Error(IrcServerEvent):
    def __init__(self, irc, reason):
        IrcServerEvent.__init__(self, irc)
        self.reason = reason


class ChannelMessage(IrcServerEvent):
    def __init__(self, irc, sender, channel, text):
        IrcServerEvent.__init__(self, irc)
        self.sender = sender
        self.channel = channel
        self.text = text


class PrivateMessage(IrcServerEvent):
    def __init__(self, irc, sender, text):
        IrcServerEvent.__init__(self, irc)
        self.sender = sender
        self.text = text


class Join(IrcServerEvent):
    def __init__(self, irc, sender, channel):
        IrcServerEvent.__init__(self, irc)
        self.sender = sender
        self.channel = channel


class Part(IrcServerEvent):
    def __init__(self, irc, sender, channel, reason):
        IrcServerEvent.__init__(self, irc)
        self.sender = sender
        self.channel = channel
        self.reason = reason


class Quit(IrcServerEvent):
    def __init__(self, irc, sender, reason):
        IrcServerEvent.__init__(self, irc)
        self.sender = sender
        self.reason = reason

class Disconnected(IrcServerEvent):
    def __init__(self, irc):
        IrcServerEvent.__init__(self, irc)

class Connected(IrcServerEvent):
    def __init__(self, irc):
        IrcServerEvent.__init__(self, irc)


class UserInfo:
    addrRe = re.compile("(.*?)!(.*?)@(.*)")

    def __init__(self, nick, user=None, host=None):
        self.nick = nick
        self.user = user
        self.host = host
        self.channels = []

    def addChannel(self, channel):
        self.channels.append(channel)

    def remChannel(self, channel):
        self.channels.remove(channel)

    @staticmethod
    def parseAddress(address):
        """
            Return a 3-tuple containing nick, user and host.
        """
        match = UserInfo.addrRe.match(address)
        if match:
            return (match.groups(1), match.groups(2), match.groups(3))


class ChannelInfo:
    """
        Class holding info of one of channels that the connection knows of.
    """
    def __init__(self, name):
        self.name = name
        self.users = []

    def addUser(self, user):
        self.users.append(user)

    def remUser(self, user):
        self.users.remove(user)


class IrcConnection:
    """
        Class that handles one connection to one IRC network.
    """
    def __init__(self, **args):
        """
            Initializes connection to IRC server.

            Required arguments are:
            - host - hostname of IRC server
            - nick - list of nicks to use, or one nick to use

            Additional arguments can be:
            - port - port to use for connecting to IRC server (defaults to 6667)
            - user - username to use when registering to network (defaults to prebot)
            - realname - real name to use when registering to network (defaults to Prebot IRC Bot)
            - password - server password
        """

        _networks.append(self)
        self.log = logging.getLogger(__name__)

        _checkArgument("host", args, StringType)
        _checkArgument("nick", args, StringType)

        self.hostname = args["host"]
        self.currentNick = args["nick"]

        if "user" in args:
            _checkArgument("user", args, StringType)
            self.user = args["user"]
        else:
            self.user = "prebot"

        if "port" in args:
            _checkArgument("port", args, IntType)
            self.port = args["port"]
        else:
            self.port = 6667

        if "password" in args and args["password"] is not None:
            _checkArgument("password", args, StringType)
            self.password = args["password"]
        else:
            self.password = None

        if "realname" in args:
            _checkArgument("realname", args, StringType)
            self.realname = args["realname"]
        else:
            self.realname = "Prebot IRC Bot"

        self.connected = False
        self.registered = False
        self.props = {}
        self.users = {}
        self.channels = {}

    def close(self):
        if self.connected:
            self.socket.close(True)
            self.connected = False
            self.registered = False

    def socketRead(self, socket):
        while True:
            line = socket.readLine()
            if line is None:
                break

            self.log.debug(">>> %s", line)
            event.trigger("irc.rawreceive", RawReceive(self, line))
            self.processLine(line)

    def socketExtra(self, socket):
        socket.close(True)
        SocketPool().remove(socket)

        self.connected = False
        self.registered = False

        event.trigger("irc.disconnected", Disconnected(self))

    def parseLine(self, line):
        pos = line.find(" :")
        if pos == 0:
            pos = line.find(" :", pos + 1)

        return line[0:pos].split(" ") + [line[pos + 2:]]

    @staticmethod
    def isNumber(num):
        try:
            int(num)
            return True
        except ValueError:
            return False

    def processLine(self, line):
        parsed = self.parseLine(line)
        if parsed[0][0] == ":":
            # Cut off begining colon and keep hostname only.
            parsed[0] = parsed[0][1:]
            sender = parsed[0]
            cmd = parsed[1]
            args = parsed[2:]
        else:
            sender = None
            cmd = parsed[0]
            args = parsed[1:]

        eventName = None
        eventData = None

        # Update user database
        if sender is not None:
            self.updateUser(sender)

        # System message
        if self.isNumber(cmd):
            num = int(cmd)
            eventName = "irc.servermessage"
            eventData = ServerMessage(self, sender, num, args)

        elif cmd == "PRIVMSG":
            if args[0] == self.currentNick:
                # Private message
                eventName = "irc.privmsg"
                eventData = PrivateMessage(self, self.lookupUser(sender), args[1])

            elif args[0][0] in self.props["CHANTYPES"]:
                # Channel message
                eventName = "irc.chanmsg"
                eventData = ChannelMessage(self, self.lookupUser(sender), args[0], args[1])

            else:
                self.log.warn("Unknown message recipient: %s" % args[0])

            pass

        elif cmd == "NOTICE":
            # TODO: process notice
            pass

        elif cmd == "PING":
            eventName = "irc.ping"
            eventData = Ping(self, args[0])
        elif cmd == "ERROR":
            eventName = "irc.error"
            eventData = Error(self, args[0])

        if eventName is not None:
            event.trigger(eventName, eventData)

    def updateUser(self, address, oldNick=None):
        if UserInfo.addrRe.match(address):
            nick, user, host = UserInfo.parseAddress(address)
        else:
            nick = address
            user = None
            host = None

        if oldNick is None:
            oldNick = nick

        if nick not in self.users:
            self.users[oldNick] = UserInfo(nick, user, host)
        else:
            u = self.users[oldNick]
            if nick is not None:
                u.nick = nick

            if user is not None:
                u.user = user

            if host is not None:
                u.host = host

    def lookupUser(self, address):
        if UserInfo.addrRe.match(address):
            nick, user, host = UserInfo.parseAddress(address)
        else:
            nick = address

        if nick in self.users:
            return self.users[nick]
        else:
            return None

    def lookupChannel(self, channel):
        if channel in self.channels:
            return self.channels[channel]
        else:
            return None

    def connect(self):
        # Already connected...
        if self.connected:
            return

        self.registered = False
        self.props = {}
        self.users = {}
        self.channels = {}

        addresses = socket.getaddrinfo(self.hostname, self.port)

        for family, _, _, _, addr in addresses:
            self.socket = socket.socket(family)

            try:
                self.log.info("Connecting to %s:%s", addr[0], addr[1])
                self.socket.connect(addr)
                self.connected = True
                self.log.info("Connected.")
            except socket.error, msg:
                self.socket.close()
                self.socket = None
                self.log.info("Unable to connect: %s", str(msg))
                continue

            self.socket = SocketPool().add(self.socket, read=self.socketRead, other=self.socketExtra)
            break

        if self.password is not None:
            self.raw("PASS %s" % (self.password))

        self.nick(self.currentNick)
        self.raw("USER %s * * :%s" % (self.user, self.realname))

    def raw(self, msg):
        self.log.debug("<<< %s", msg)
        event.trigger("irc.rawsend", RawSend(self, msg))
        self.socket.enqueue("%s\r\n" % msg)

    @property
    def networkName(self):
        if "NETWORK" in self.props:
            return self.props["NETWORK"]
        else:
            return None

    @staticmethod
    @event.handler("irc.servermessage")
    def parseServerMessage(eventData, handlerData):
        self = eventData.irc
        if eventData.code == 5:
            # Parse ISUPPORT message
            for match in re.findall("([A-Z]+)(=([^\s]+))?", " ".join(eventData.text[:-1])):
                if match[1]:
                    self.props[match[0]] = match[2]
                else:
                    self.props[match[0]] = True

        elif eventData.code == 251:
            if not self.registered:
                self.registered = True
                event.trigger("irc.connected", Connected(eventData.irc))

    @staticmethod
    @event.handler("irc.connected")
    def connected(eventData, handlerData):
        self = eventData.irc
        self.log.info("Connected!")

    def join(self, channel):
        """
            Join channel
        """
        if not self.isJoined(channel):
            self.raw("JOIN %s" % channel)

    def part(self, channel, reason=""):
        """
            Part channel
        """
        if self.isJoined(channel):
            self.raw("PART %s :%s" % (channel, reason))

    def quit(self, reason=""):
        """
            Quit from IRC
        """
        self.quit("QUIT :%s" % (reason))

    def nick(self, newNick):
        """
            Change my nick
        """
        self.raw("NICK %s" % newNick)

    def message(self, target, message):
        """
            Send message
        """
        for line in message.split("\n"):
            self.raw("PRIVMSG %s :%s" % target, line)

    def isJoined(self, channel):
        return self.lookupChannel(channel) is not None
