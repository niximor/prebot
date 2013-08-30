#
# PreBot IRC bot software
# (c) Michal Kuchta 2013
#

import logging
from singleton import Singleton


class Events:
    __metaclass__ = Singleton

    def __init__(self):
        self._events = {}
        self._toTrigger = []
        self._modules = {}
        self.log = logging.getLogger(__name__)

    def register(self, event, callback, data=None):
        if not event in self._events:
            self._events[event] = {}

        self._events[event][callback] = data

        # Register event to properly cleanup when module is
        # unloaded.
        modName = callback.__module__
        if modName not in self._modules:
            self._modules[modName] = []

        self._modules[modName].append((event, callback))

        self.log.debug("Registered new handler for event %s", event)

    def unregister(self, event, callback):
        if event in self._events and callback in self._events[event]:
            del self._events[event][callback]

            modName = callback.__module__
            if modName in self._modules:
                self._modules[modName].remove((event, callback))

            self.log.debug("Unregistered handler for event %s", event)

    def unregisterAll(self, module):
        if module in self._modules:
            for event, callback in self._modules[module]:
                self.unregister(event, callback)

    def trigger(self, event, data=None):
        self._toTrigger.append((event, data))

    def poll(self):
        for event, data in self._toTrigger:
            self.log.debug("Firing event %s", event)
            if event in self._events:
                for callback, handlerData in self._events[event].iteritems():
                    try:
                        callback(handlerData=handlerData, eventData=data)
                    except Exception, e:
                        logging.getLogger(callback.__module__).exception(e)

        self._toTrigger = []

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


def handler(event):
    """
        Decorator
    """
    def wrapper(f):
        ev = Events()
        ev.register(event, f)
        return f
    return wrapper
