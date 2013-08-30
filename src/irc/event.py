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

class UserMode(IrcServerEvent):
    def __init__(self, irc, sender, mode):
        IrcServerEvent.__init__(self, irc)
        self.sender = sender
        self.mode = mode

class ChannelMode(IrcServerEvent):
    def __init__(self, irc, sender, channel, mode):
        IrcServerEvent.__init__(self, irc)
        self.sender = sender
        self.channel = channel
        self.mode = mode