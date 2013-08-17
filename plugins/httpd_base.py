#
# PreBot IRC bot software
# (c) Michal Kuchta 2013
#

from httpd import get, post, menulink, Request, RequestHandler
import event
from storage import db
import util
import modules
import conftools


def cleanModule(modName):
    # Do a little trick here. If module is in unloaded, nothing happens.
    # But if module is not there, we are reloading, so we need to do a cleanup.
    if modName in modules.Modules().unloaded:
        return

    toDel = []

    for uri, f in RequestHandler.dispatch_table["GET"].iteritems():
        if f.__module__ == modName:
            toDel.append(uri)

            # Clean menu links
            for item in Request.g["menu"]:
                if item["uri"] == uri:
                    Request.g["menu"].remove(item)

    for uri in toDel:
        del RequestHandler.dispatch_table["GET"][uri]

    toDel = []

    for uri, f in RequestHandler.dispatch_table["POST"].iteritems():
        if f.__module__ == modName:
            toDel.append(uri)

            # Clean menu links
            for item in Request.g["menu"]:
                if item["uri"] == uri:
                    Request.g["menu"].remove(item)

    for uri in toDel:
        del RequestHandler.dispatch_table["POST"][uri]


@event.handler("module.unloading")
def mod_unloaded(eventData, handlerData):
    modName = eventData
    cleanModule(modName)


@get("/")
def root_page(req):
    req.render("index.html")


@get("/style.css")
def css(req):
    req.render("style.css", contentType="text/css")


@menulink("Settings", "/settings")
def settings(req):
    req.render("settings.html", data={
        "nicks": util.listNicks()
    })


@get("/nick.remove")
def nick_remove(req):
    conftools.delNick(req.get.getfirst("nick"))
    req.redirect("/settings")


@get("/nick.moveup")
def nick_up(req):
    conftools.moveUp(req.get.getfirst("nick"))
    req.redirect("/settings")


@get("/nick.movedown")
def nick_down(req):
    conftools.moveDown(req.get.getfirst("nick"))
    req.redirect("/settings")


@post("/nick.add")
def nick_add(req):
    conftools.addNick(req.post.getfirst("nick"))
    req.redirect("/settings")


@menulink("Servers", "/servers")
def servers(req):
    servers = []
    c = db.cursor()
    c.execute("SELECT id, host, port, user, password, realname, enabled FROM servers ORDER BY host ASC, port ASC")
    for row in c.fetchall():
        row["status"] = ""

        if row["enabled"]:
            # Try to find matching IRC connection.
            for irc in util._networks:
                if irc.hostname == row["host"] and irc.port == row["port"]:
                    if irc.connected and irc.registered:
                        row["status"] = "Connected."
                    elif irc.connected:
                        row["status"] = "Connecting..."
                    else:
                        row["status"] = "Disconnected."

        servers.append(row)

    req.render("servers.html", data={
        "servers": servers,
        "formData": {
            "host": "",
            "port": 6667,
            "user": "",
            "password": "",
            "realname": ""
        }
    })


@post("/servers.add")
def server_add(req):
    formData = {
        "host": req.post.getfirst("host"),
        "port": req.post.getfirst("port"),
        "user": req.post.getfirst("user"),
        "password": req.post.getfirst("password"),
        "realname": req.post.getfirst("realname")
    }

    print formData

    errors = {}

    if formData["host"] is None or formData["host"] == "":
        errors["host"] = "Host is required."
    elif not util.isValidHostname(formData["host"]) and not util.isValidIp(formData["host"]):
        errors["host"] = "Host is invalid."

    try:
        formData["port"] = int(formData["port"])
        if formData["port"] < 1 or formData["port"] > 65535:
            raise ValueError("")
    except ValueError:
        errors["port"] = "Port must be number between 1 and 65535."

    if formData["user"] is None or formData["user"] == "":
        errors["user"] = "User is required."

    if formData["realname"] is None or formData["realname"] == "":
        errors["realname"] = "Real name is required."

    # Password is not mandatory.
    if formData["password"] == "":
        password = None

    if not errors:
        conftools.addServer(formData["host"], formData["user"],
                            formData["realname"], port=formData["port"],
                            password=password)
        req.redirect("/servers")
    else:
        id = req.persistent.store({
            "formData": formData,
            "errors": errors
        })
        req.redirect("/servers?p=%s" % id)


@get("/server.disable")
def server_disable(req):
    id = req.get.getfirst("id")
    conftools.disableServer(id)
    req.redirect("/servers")


@get("/server.enable")
def server_enable(req):
    id = req.get.getfirst("id")
    conftools.enableServer(id)
    req.redirect("/servers")


@get("/server.remove")
def server_remove(req):
    id = req.get.getfirst("id")
    conftools.delServer(id)
    req.redirect("/servers")
