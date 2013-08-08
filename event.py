#
# PreBot IRC bot software
# (c) Michal Kuchta 2013
#

import logging
import threading
from singleton import Singleton


class Events:
    __metaclass__ = Singleton

    def __init__(self):
        self._events = {}
        self._threadEvent = threading.Event()
        self._threadMutex = threading.Lock()
        self._toTrigger = []
        self.log = logging.getLogger(__name__)

    def register(self, event, callback, data=None):
        if not event in self._events:
            self._events[event] = {}

        self._events[event][callback] = data
        self.log.debug("Registered new handler for event %s", event)

    def unregister(self, event, callback):
        if event in self._events and callback in self._events[event]:
            del self._events[event][callback]
            self.log.debug("Unregistered handler for event %s", event)

    def trigger(self, event, data=None):
        self._threadMutex.acquire()
        self._toTrigger.append((event, data))
        self._threadEvent.set()
        self._threadMutex.release()

    def poll(self):
        self._threadEvent.wait(1.0)
        self._threadMutex.acquire()
        self._threadEvent.clear()
        
        for event, data in self._toTrigger:
            self.log.debug("Firing event %s", event)
            if event in self._events:
                for callback, handlerData in self._events[event].iteritems():
                    callback(handlerData=handlerData, eventData=data)

        self._toTrigger = []

        self._threadMutex.release()


def register(event, callback, data=None):
    ev = Events()
    ev.register(event, callback, data)


def unregister(event, callback):
    ev = Events()
    ev.unregister(event, callback)


def trigger(event, data=None):
    ev = Events()
    ev.trigger(event, data)

def poll():
    ev = Events()
    ev.poll()

def unlock():
    ev = Events()
    ev._threadEvent.set()