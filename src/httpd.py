#
# PreBot IRC bot software
# (c) Michal Kuchta 2013
#

from BaseHTTPServer import BaseHTTPRequestHandler
import SocketServer
from socketpool import SocketPool
import logging
import urlparse
import util
from jinja2 import Environment, FileSystemLoader, contextfunction, Undefined
from jinja2.utils import missing
from StringIO import StringIO
import string
import random


@contextfunction
def get_context(c):
    return c


class QueryString:
    def __init__(self, qs=""):
        self.data = urlparse.parse_qsl(qs, True)
        self.data = map(lambda (k, v): (k, unicode(v, "utf-8")), self.data)

    def getfirst(self, name, notfound=None):
        for n, v in self.data:
            if n == name:
                return v

        return notfound

    def getlist(self, name):
        out = []
        for n, v in self.data:
            if n == name:
                out.append(v)

        return out


class PersistentStorage:
    def __init__(self):
        self.data = {}

    @staticmethod
    def __getId():
        return "".join(random.choice(
            string.ascii_uppercase
            + string.ascii_lowercase
            + string.digits) for x in range(32))

    def store(self, data):
        id = None
        while True:
            id = self.__getId()
            if not id in self.data:
                break

        self.data[id] = data
        return id

    def load(self, key):
        if key in self.data:
            out = self.data[key]
            del self.data[key]
            return out
        else:
            return None


class EasyUndefined(Undefined):
    def __getattr__(self, name):
        if name[:2] == '__':
            raise AttributeError(name)
        return self.fail()

    def _fail_with_undefined_error(self, *args, **kwargs):
        if self._undefined_hint is None:
            if self._undefined_obj is missing:
                hint = '%r is undefined' % self._undefined_name
            elif not isinstance(self._undefined_name, string_types):
                hint = '%s has no element %r' % (
                    object_type_repr(self._undefined_obj),
                    self._undefined_name
                )
            else:
                hint = '%r has no attribute %r' % (
                    object_type_repr(self._undefined_obj),
                    self._undefined_name
                )
        else:
            hint = self._undefined_hint

        logging.getLogger(__name__).warn(hint)

    def fail(self):
        self._fail_with_undefined_error()
        return ""

    __add__ = __radd__ = __mul__ = __rmul__ = __div__ = __rdiv__ = \
        __truediv__ = __rtruediv__ = __floordiv__ = __rfloordiv__ = \
        __mod__ = __rmod__ = __pos__ = __neg__ = __call__ = \
        __getitem__ = __lt__ = __le__ = __gt__ = __ge__ = __int__ = \
        __float__ = __complex__ = __pow__ = __rpow__ = \
        fail

    def __eq__(self, other):
        return type(self) is type(other)

    def __ne__(self, other):
        return not self.__eq__(other)

    def __hash__(self):
        return id(type(self))

    def __str__(self):
        return u''

    def __len__(self):
        return 0

    def __iter__(self):
        if 0:
            yield None

    def __nonzero__(self):
        return False

    def __repr__(self):
        return 'Undefined'


class Request:
    g = {}
    persistent = PersistentStorage()
    env = Environment(
        loader=FileSystemLoader(["tpl/"]),
        lstrip_blocks=True,
        trim_blocks=True,
        line_statement_prefix="%",
        line_comment_prefix="##",
        block_start_string="{%",
        block_end_string="%}",
        variable_start_string="${",
        variable_end_string="}",
        autoescape=True,
        undefined=EasyUndefined
    )

    def __init__(self, request, url, path, get=QueryString(), post=QueryString()):
        self.request = request
        self.url = url
        self.path = path
        self.get = get
        self.post = post

    def render(self, template, data={}, statusCode=200, contentType="text/html; charset=utf-8"):
        # Prepare data
        data.update(Request.g)

        p = self.get.getfirst("p")
        if p is None:
            self.post.getfirst("p")

        if p is not None:
            pers = self.persistent.load(p)
            if pers is not None:
                data.update(pers)

        # Generate template
        try:
            tpl = Request.env.get_template(template)
            tpl.globals["context"] = get_context
            rendered = tpl.render(**data)

            self.request.send_response(statusCode)
            self.request.send_header("Content-Type", contentType)
            self.request.end_headers()
            self.request.wfile.write(rendered.encode("utf-8"))
        except Exception, e:
            logging.getLogger(__name__).exception(e)
            self.request.send_error(500)

    def redirect(self, url):
        self.request.send_response(302)
        self.request.send_header("Location", url)
        self.request.end_headers()


class RequestHandler(BaseHTTPRequestHandler):
    dispatch_table = {
        "GET": {},
        "POST": {}
    }

    def do_GET(self):
        result = urlparse.urlparse(self.path)
        if result.path in RequestHandler.dispatch_table["GET"]:
            req = Request(self, self.path, result.path,
                          get=QueryString(result.query))

            RequestHandler.dispatch_table["GET"][result.path](req)
        else:
            self.send_error(404, "Not found.")

    def do_POST(self):
        result = urlparse.urlparse(self.path)
        if result.path in RequestHandler.dispatch_table["POST"]:
            length = int(self.headers.getheader('content-length'))
            req = Request(self, self.path, result.path,
                          get=QueryString(result.query),
                          post=QueryString(self.rfile.read(length)))

            RequestHandler.dispatch_table["POST"][result.path](req)
        else:
            self.send_error(404, "Not found.")

    def log_error(self, format, *args):
        logging.getLogger(__name__).error("%s - %s" %
            (self.client_address[0], format % args))

    def log_request(self, code='-', size='-'):
        logging.getLogger(__name__).info("%s - %s %s %s" %
            (self.client_address[0], self.requestline, str(code), str(size)))


class HTTPServer(SocketServer.TCPServer):
    allow_reuse_address = True

    def __init__(self, address):
        SocketServer.TCPServer.__init__(self, address, RequestHandler)

    def server_activate(self):
        SocketServer.TCPServer.server_activate(self)
        SocketPool().add(self.socket, self.socket_ready, None, self.socket_term)

    def socket_ready(self, socket):
        self._handle_request_noblock()

    def socket_term(self, socket):
        self.shutdown()


def get(uri):
    """
        Decorator
    """
    def wrapper(f):
        RequestHandler.dispatch_table["GET"][uri] = f
        return f
    return wrapper


def post(uri):
    """
        Decorator
    """
    def wrapper(f):
        RequestHandler.dispatch_table["POST"][uri] = f
        return f
    return wrapper


def menulink(name, uri, **kwargs):
    """
        Decorator
    """
    def wrapper(f):
        RequestHandler.dispatch_table["GET"][uri] = f

        if not "menu" in Request.g:
            Request.g["menu"] = []

        menuItem = {"name": name, "uri": uri}
        menuItem.update(kwargs)
        Request.g["menu"].append(menuItem)
        return f
    return wrapper
