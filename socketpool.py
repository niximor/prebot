#
# PreBot IRC bot software
# (c) Michal Kuchta 2013
#

import select

class CallabkeSocket:
    def __init__(self, socket, callable):
        self._socket = socket
        self._callable = callable

    def fileno(self):
        return self._socket.fileno()



class SocketPool:
    def __init__(self):
        self.rdsock = []
        self.wrsock = []
        self.othersock = []

    def add(self, socket, read=None, write=None, other=None):
        if read is not None:
            self.rdsock.append(CallableSocket(socket, read))

        if write is not None:
            self.wrsock.append(CallableSocket(socket, write))

        if other is not None:
            self.othersock.append(CallableSocket(socket, other))

    def poll():
        rd, wr, other = select.select(self.rdsock, self.wrsock, self.othersock)
        
        for item in rd:
            item()

        for item in wr:
            item()

        for item in other:
            item()
