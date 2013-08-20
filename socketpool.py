#
# PreBot IRC bot software
# (c) Michal Kuchta 2013
#

import select
import logging
import socket
from time import sleep
from sys import platform
from singleton import Singleton

isWin32 = platform == "win32"

class CallabkeSocket:
    def __init__(self, socket, callable):
        self._socket = socket
        self._callable = callable

    def fileno(self):
        return self._socket.fileno()


class SocketAction:
    def __init__(self, socket):
        self._socket = socket

    def run(self):
        """
            Called by pool when socket action can be performed
            (socket can be written).
        """
        return True


class WriteAction(SocketAction):
    def __init__(self, socket, data):
        SocketAction.__init__(self, socket)
        self._data = data

    def run(self):
        bytes = self._socket.send(self._data)

        log = logging.getLogger(__name__)
        #log.debug("<<< %s", self._data[0:bytes])

        if bytes == len(self._data):
            return True
        else:
            return False


class CloseAction(SocketAction):
    def run():
        self._socket.close()
        return True


class PoolSocket:
    def __init__(self, socket, rdcall=None, wrcall=None, othercall=None):
        self._socket = socket
        self._socket.setblocking(0)
        self.rdcall = rdcall
        self.wrcall = wrcall
        self.othercall = othercall
        self.queue = []
        self.processedData = ""

    def fileno(self):
        return self._socket.fileno()

    def enqueue(self, data):
        self.queue.append(WriteAction(self._socket, data))

    def readLine(self):
        pos = self.processedData.find("\n")

        # If not found, try to read next chunk.
        if pos == -1:
            self.processedData += self.readBuffer(4096)

            # Try again to find newline
            pos = self.processedData.find("\n")
        
        if pos > 0:
            dt = self.processedData[0:pos].replace("\r", "")
            self.processedData = self.processedData[pos + 1:]
            return dt
        else:
            return None


    def readBuffer(self, buffer):
        try:
            return self._socket.recv(buffer)
        except socket.error, e:
            # ignore EWOULDBLOCK call
            if e.errno == 11 or (isWin32 and e.errno == 10035):
                return ""
            else:
                raise

    def close(self, immediately=False):
        if immediately:
            self.queue = []
            self.rdcall = None
            self.wrcall = None
            self.othercall = None
            self._socket.close()
        else:
            self.queue.append(CloseAction(self, data))

    def needWrite(self):
        return self.wrcall is not None or self.queue

    def needRead(self):
        return self.rdcall is not None

    def needOther(self):
        return self.othercall is not None

    def callRead(self):
        if self.rdcall is not None:
            self.rdcall(self)

    def callWrite(self):
        if not self.queue:
            if self.wrcall is not None:
                self.wrcall(self)
        else:
            qa = self.queue[0]
            if qa.run():
                self.queue.remove(qa)

    def callOther(self):
        if self.othercall is not None:
            self.othercall(self)


class SocketPool:
    __metaclass__ = Singleton

    def __init__(self):
        self.sockets = {}
        
    def add(self, socket, read=None, write=None, other=None):
        if not socket in self.sockets:
            # Add new socket to pool
            s = PoolSocket(socket, rdcall=read, wrcall=write, othercall=other)
            self.sockets[socket] = s
        else:
            # Update socket in pool
            s = self.sockets[socket]
            s.rdcall = read
            s.wrcall = write
            s.othercall = other

        return s
        
    def remove(self, socket):
        if instanceof(socket, PoolSocket):
            socket = socket._socket

        if socket in self.sockets:
            del self.sockets[socket]

    def poll(self, timeout=1.0):
        rdsock = []
        wrsock = []
        othersock = []

        for socket, mysock in self.sockets.iteritems():
            if mysock.needRead():
                rdsock.append(mysock)

            if mysock.needWrite():
                wrsock.append(mysock)

            if mysock.needOther():
                othersock.append(mysock)

        if not rdsock and not wrsock and not othersock:
            sleep(1.0)
        else:
            rd, wr, oth = select.select(rdsock, wrsock, othersock, timeout)

            for sock in rd:
                sock.callRead()

            for sock in wr:
                sock.callWrite()

            for sock in oth:
                sock.callOther()
