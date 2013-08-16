#
# PreBot IRC bot software
# (c) Michal Kuchta 2013
#

import sys
import logging
import event
from singleton import Singleton

class ModuleInfo:
    def __init__(self, name, module):
        self.name = name
        self.module = module
        self.events = []

    def __del__(self):
        """
            Destructor.
        """

        # Unregister event handlers for this module.
        for event, callback in self.events:
            events.unregister(event, callback)

        del self.module


class Modules:
    __metaclass__ = Singleton

    def __init__(self):
        self.modules = {}
        self.log = logging.getLogger(__name__)

    def __fileToModule(self, file):
        if (file.endswith(".py")):
            file = file[0:-3]
        return file.replace("/", ".").replace("\\", ".")

    def load(self, file):
        # Import module
        modName = self.__fileToModule(file)

        if modName in self.modules:
            self.log.warn("Module %s already loaded." % modName)
            return

        self.log.debug("Loading module %s." % modName)

        # Only load plugins from specified folder.
        oldpath = sys.path
        sys.path = ["plugins"]

        try:
            module = __import__(modName)
            self.modules[modName] = ModuleInfo(modName, module)
            self.log.info("Loaded module %s." % modName)
        except ImportError, e:
            self.log.error("Unable to load module %s: %s" % (modName, str(e)))

        # Restore old path
        sys.path = oldpath

    def unload(self, file):
        modName = self.__fileToModule(file)

        if not modName in self.modules:
            self.log.error("Module %s not loaded." % modName)
            return

        del self.modules[modName]

    def list(self):
        return self.modules.keys()

def load(file):
    Modules().load(file)

def unload(file):
    Modules().unload(file)

def list():
    Modules().list()