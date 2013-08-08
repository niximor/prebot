#
# PreBot IRC bot software
# (c) Michal Kuchta 2013
#

from types import *
import socket
import logging
from socketpool import SocketPool
from threading import Thread

def _checkArgument(arg, args, requestedType):
        if arg not in args:
            raise KeyError("%s is mandatory configuration parameter." % arg)

        currentType = type(args[arg])
        if currentType != requestedType:
            raise AttributeError("%s must be %s, but is %s." % (arg, requestedType, currentType))


class IrcServerEvent:
    pass

class ServerMessage(IrcServerEvent):
    def __init__(self, sender, code, text):
        self.sender = sender
        self.code = code
        self.text = text

class Error(IrcServerEvent):
    def __init__(self, sender, reason):
        self.sender = sender
        self.reason = reason

class ChannelMessage(IrcServerEvent):
    def __init__(self, sender, channel, text):
        self.sender = sender
        self.channel = channel
        self.text = text

class PrivateMessage(IrcServerEvent):
    def __init__(self, sender, text):
        self.sender = sender
        self.text = text

class Join(IrcServerEvent):
    def __init__(self, sender, channel):
        self.sender = sender
        self.channel = channel

class Part(IrcServerEvent):
    def __init__(self, sender, channel, reason):
        self.sender = sender
        self.channel = channel
        self.reason = reason

class Quit(IrcServerEvent):
    def __init__(self, sender, reason):
        self.sender = sender
        self.reason = reason

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
        self.socketpool = SocketPool()
        self.thread = Thread(target=self.threadLoop)
        self.thread.start()

    def close(self):
        if self.connected:
            self.socket.close(True)

        if self.thread.is_alive():
            self.socketpool.quitLoop()
            self.thread.join()

    def threadLoop(self):
        self.socketpool.loop()

    def socketRead(self, socket):
        while True:
            line = socket.readLine()
            if line is None:
                break

            self.log.debug(">>> %s", line)
            self.processLine(line)

    def socketExtra(self, socket):
        pass

    def processLine(self, line):


    def connect(self):
        # Already connected...
        if self.connected:
            return

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

            self.socket = self.socketpool.add(self.socket, read=self.socketRead, other=self.socketExtra)
            break

        if self.password is not None:
            self.raw("PASS %s" % (self.password))

        self.raw("NICK %s" % self.nicks[0])
        self.raw("USER %s * * :%s" % (self.user, self.realname))

    def raw(self, msg):
        self.log.debug("<<< %s", msg)
        self.socket.enqueue("%s\r\n" % msg)
