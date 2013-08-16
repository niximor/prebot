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

def _checkArgument(arg, args, requestedType):
        if arg not in args:
            raise KeyError("%s is mandatory configuration parameter." % arg)

        currentType = type(args[arg])
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

##
# Class that handles one connection to one IRC network.
#
class IrcConnection:
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

        self.log = logging.getLogger(__name__)

        _checkArgument("host", args, StringType)

        try:
            _checkArgument("nick", args, ListType)
        except AttributeError:
            _checkArgument("nick", args, StringType)
            args["nick"] = [args["nick"]]

        self.hostname = args["host"]
        self.nicks = args["nick"]

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

        if "password" in args:
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
        pos = line.find(":")
        if pos == 0:
            pos = line.find(":", pos + 1)

        return line[0:pos - 1].split(" ") + [line[pos + 1:]]

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
            # It is incomming message

            # Cut off begining colon and keep hostname only.
            parsed[0] = parsed[0][1:]
            sender = parsed[0]
            cmd = parsed[1]
            args = parsed[2:]

            eventName = None
            eventData = None

            # System message
            if self.isNumber(cmd):
                num = int(cmd)
                eventName = "irc.servermessage"
                eventData = ServerMessage(self, sender, num, args)
                
            elif cmd == "PRIVMSG":
                # TODO: process privmsg
                pass

            elif cmd == "NOTICE":
                # TODO: process notice
                pass

            elif cmd == "MODE":
                # TODO: parse mode
                pass

            if eventName is not None:
                event.trigger(eventName, eventData)

        else:
            # Event to raise
            eventName = None
            eventData = None

            # It is incomming command
            cmd = parsed[0]
            args = parsed[1:]
            if cmd == "PING":
                eventName = "irc.ping"
                eventData = Ping(self, args[0])
            elif cmd == "ERROR":
                eventName = "irc.error"
                eventData = Error(self, args[0])

            if eventName is not None:
                event.trigger(eventName, eventData)

    def connect(self):
        # Already connected...
        if self.connected:
            return

        self.registered = False
        self.props = {}

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

        self.raw("NICK %s" % self.nicks[0])
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
            for match in re.findall("([A-Z]+)(=([^[:space:]]))?", " ".join(eventData.text[:-1])):
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
