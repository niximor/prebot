#
# PreBot IRC bot software
# (c) Michal Kuchta 2013
#

import select
import threading
import logging
import socket
from time import sleep
from sys import platform

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
            Called by pool when socket action can be performed (socket can be written).
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
        self.queueMutex = threading.Lock()
        self.processedData = ""

    def fileno(self):
        return self._socket.fileno()

    def enqueue(self, data):
        self.queueMutex.acquire()
        self.queue.append(WriteAction(self._socket, data))
        self.queueMutex.release()

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
            self.queueMutex.acquire()
            self.queue = []
            self.queueMutex.release()
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
        self.queueMutex.acquire()
        if not self.queue:
            self.queueMutex.release()
            if self.wrcall is not None:
                self.wrcall(self)
        else:
            qa = self.queue[0]
            if qa.run():
                self.queue.remove(qa)
            self.queueMutex.release()

    def callOther(self):
        if self.othercall is not None:
            self.othercall(self)


class SocketPool:
    def __init__(self):
        self.sockets = {}
        self.mutex = threading.Lock()
        self.quitMutex = threading.Lock()
        
    def add(self, socket, read=None, write=None, other=None):
        self.mutex.acquire()
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
        self.mutex.release()

        return s
        
    def remove(self, socket):
        if instanceof(socket, PoolSocket):
            socket = socket._socket

        self.mutex.acquire()
        if socket in self.sockets:
            del self.sockets[socket]
        self.mutex.release()


    def poll(self):
        rdsock = []
        wrsock = []
        othersock = []

        self.mutex.acquire()
        for socket, mysock in self.sockets.iteritems():
            if mysock.needRead():
                rdsock.append(mysock)

            if mysock.needWrite():
                wrsock.append(mysock)

            if mysock.needOther():
                othersock.append(mysock)
        self.mutex.release()

        if not rdsock and not wrsock and not othersock:
            sleep(1.0)
        else:
            rd, wr, oth = select.select(rdsock, wrsock, othersock, 1.0)

            for sock in rd:
                sock.callRead()

            for sock in wr:
                sock.callWrite()

            for sock in oth:
                sock.callOther()


    def loop(self):
        self.quitMutex.acquire()
        self._quitLoop = False
        while not self._quitLoop:
            self.quitMutex.release()
            self.poll()
            self.quitMutex.acquire()
        self.quitMutex.release()


    def quitLoop(self):
        self.quitMutex.acquire()
        self._quitLoop = True
        self.quitMutex.release()
