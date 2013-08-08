#
# PreBot IRC bot software
# (c) Michal Kuchta 2013
#

from types import *
import socket
import logging


def _checkArgument(arg, args, requestedType):
        if arg not in args:
            raise KeyError("%s is mandatory configuration parameter." % arg)

        currentType = type(args[arg])
        if currentType != requestedType:
            raise AttributeError("%s must be %s, but is %s." % (arg, requestedType, currentType))


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

            break

        self.raw("NICK %s" % self.nicks[0])
        self.raw("USER %s * * :%s" % (self.user, self.realname))

        if self.password is not None:
            self.raw("PASSWORD %s" % (self.password))


    def raw(self, msg):
        self.socket.sendall(msg)
