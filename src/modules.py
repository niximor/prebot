#
# PreBot IRC bot software
# (c) Michal Kuchta 2013
#

import sys
import logging
import event
from singleton import Singleton
from httpd import menulink, get


class ModuleInfo:
    def __init__(self, name, module):
        self.name = name
        self.module = module
        self.events = []

    def shutdown(self):
        """
            Destructor.
        """

        # Unregister event handlers for this module.
        event.Events().unregisterAll(self.name)

    def __del__(self):
        self.shutdown()


class Modules:
    __metaclass__ = Singleton

    def __init__(self):
        self.modules = {}
        self.unloaded = []
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
            event.trigger("module.loading", modName)
            module = __import__(modName)
            self.modules[modName] = ModuleInfo(modName, module)
            event.trigger("module.loaded", modName)
            self.log.info("Loaded module %s." % modName)

            try:
                self.unloaded.remove(modName)
            except ValueError:
                pass
        except ImportError, e:
            self.log.error("Unable to load module %s." % (modName, ))
            self.log.exception(e)

        # Restore old path
        sys.path = oldpath

    def reload(self, file):
        modName = self.__fileToModule(file)

        if modName not in self.modules:
            self.log.warn("Module %s not loaded." % modName)
            return

        oldpath = sys.path
        sys.path = ["plugins", "."]

        event.trigger("module.unloading", modName)
        self.modules[modName].shutdown()
        event.trigger("module.unloaded", modName)
        self.log.info("Unloaded module %s." % modName)

        try:
            event.trigger("module.loading", modName)
            reload(self.modules[modName].module)
            event.trigger("module.loaded", modName)
            self.log.info("Loaded module %s." % modName)
        except ImportError, e:
            self.log.error("Unable to load module %s: %s" % (modName, str(e)))

        sys.path = oldpath

    def unload(self, file):
        modName = self.__fileToModule(file)

        if not modName in self.modules:
            self.log.error("Module %s not loaded." % modName)
            return

        event.trigger("module.unloading", modName)
        self.modules[modName].shutdown()
        del self.modules[modName]
        self.unloaded.append(modName)
        event.trigger("module.unloaded", modName)
        self.log.info("Unloaded module %s." % modName)

    def list(self):
        return self.modules.keys()


def load(file):
    Modules().load(file)


def unload(file):
    Modules().unload(file)


def list():
    Modules().list()


@menulink("Plugins", "/plugins")
def web_index(req):
    plugins = []

    for mod in Modules().modules.itervalues():
        plugins.append({
            "name": mod.name,
            "loaded": mod.module is not None
        })

    for mod in Modules().unloaded:
        plugins.append({
            "name": mod,
            "loaded": False
        })

    req.render("plugins/index.html", {
        "plugins": plugins
    })


@get("/plugins.unload")
def web_unload(req):
    plugin = req.get.getfirst("plugin")
    unload(plugin)
    req.redirect("/plugins")


@get("/plugins.load")
def web_load(req):
    plugin = req.get.getfirst("plugin")
    load(plugin)
    req.redirect("/plugins")


@get("/plugins.reload")
def web_reload(req):
    plugin = req.get.getfirst("plugin")
    Modules().reload(plugin)
    req.redirect("/plugins")
