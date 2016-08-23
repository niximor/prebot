#!/usr/bin/env python

import sys
from urllib2 import urlopen, HTTPError, URLError, Request
from urlparse import urlparse
from BeautifulSoup import BeautifulSoup, BeautifulStoneSoup
import re
import socket

import googl

responses = {
    300: ('Multiple Choices',
          'Object has several resources -- see URI list'),
    301: ('Moved Permanently', 'Object moved permanently -- see URI list'),
    302: ('Found', 'Object moved temporarily -- see URI list'),
    303: ('See Other', 'Object moved -- see Method and URL list'),
    304: ('Not Modified',
          'Document has not changed since given time'),
    305: ('Use Proxy',
          'You must use proxy specified in Location to access this '
          'resource.'),
    307: ('Temporary Redirect',
          'Object moved temporarily -- see URI list'),

    400: ('Bad Request',
          'Bad request syntax or unsupported method'),
    401: ('Unauthorized',
          'No permission -- see authorization schemes'),
    402: ('Payment Required',
          'No payment -- see charging schemes'),
    403: ('Forbidden',
          'Request forbidden -- authorization will not help'),
    404: ('Not Found', 'Nothing matches the given URI'),
    405: ('Method Not Allowed',
          'Specified method is invalid for this server.'),
    406: ('Not Acceptable', 'URI not available in preferred format.'),
    407: ('Proxy Authentication Required', 'You must authenticate with '
          'this proxy before proceeding.'),
    408: ('Request Timeout', 'Request timed out; try again later.'),
    409: ('Conflict', 'Request conflict.'),
    410: ('Gone',
          'URI no longer exists and has been permanently removed.'),
    411: ('Length Required', 'Client must specify Content-Length.'),
    412: ('Precondition Failed', 'Precondition in headers is false.'),
    413: ('Request Entity Too Large', 'Entity is too large.'),
    414: ('Request-URI Too Long', 'URI is too long.'),
    415: ('Unsupported Media Type', 'Entity body in unsupported format.'),
    416: ('Requested Range Not Satisfiable',
          'Cannot satisfy request range.'),
    417: ('Expectation Failed',
          'Expect condition could not be satisfied.'),

    500: ('Internal Server Error', 'Server got itself in trouble'),
    501: ('Not Implemented',
          'Server does not support this operation'),
    502: ('Bad Gateway', 'Invalid responses from another server/proxy.'),
    503: ('Service Unavailable',
          'The server cannot process the request due to a high load'),
    504: ('Gateway Timeout',
          'The gateway server did not receive a timely response'),
    505: ('HTTP Version Not Supported', 'Cannot fulfill request.'),
}

API_KEY="AIzaSyCB8O0NhCBcV27BG1itZ0RsMeI59SsrM3c"

try:
    url = sys.argv[1]

    #print "DBG: %s" % sys.argv[1]
    req = Request(url)
    #req.add_header("Host", req.get_host())
    req.add_header("User-Agent", "Opera/9.80 (X11; Linux x86_64) Presto/2.12.388 Version/12.16")
    req.add_header("Accept", "text/html, application/xml;q=0.9, application/xhtml+xml, image/png, image/webp, image/jpeg, image/gif, image/x-xbitmap, */*;q=0.1")
    req.add_header("Accept-Language", "cs,en-US;q=0.9,en;q=0.8")
    req.add_header("Accept-Encoding", "deflate")
    req.add_header("Cache-Control", "no-cache")
    req.add_header("Connection", "Keep-Alive")

    gotException = None

    try:
        response = urlopen(req)
        html = response.read()
    except HTTPError, e:
        gotException = e
        html = e.read()

    if not url.startswith("http://goo.gl"):
        client = googl.Googl(API_KEY)
        try:
            shorten_url = client.shorten(url)
            shorten_url = shorten_url.get("id", None)

            if shorten_url is not None:
                shorten_url = shorten_url.encode("utf-8", "ignore")
        except Exception, e:
            shorten_url = None
    else:
        shorten_url = None

    if html.find("<title") >= 0:
        try:
            parsed_html = BeautifulSoup(html, convertEntities=BeautifulStoneSoup.HTML_ENTITIES)
            title = unicode(parsed_html.title.string)
            if title is not None and title != "":
                title = re.sub(r"\s+", " ", title).encode("utf-8", "ignore").strip()
                if shorten_url is not None:
                    print "%s :: %s" % (title, shorten_url)
                else:
                    print "%s" % (title, )
            else:
                #print "DBG: Empty title."
                if shorten_url is not None:
                    print u"%s" % shorten_url
        except Exception, e:
            print "Error: %s" % e
            raise
    else:
        if gotException is not None:
            e = gotException
            print "HTTP Error %d (%s)" % (e.code, e.reason)
        else:
            #print "DBG: No HTML: %s." % (html, )
            pass

# Ignore unknown URL
except URLError, e:
    print "Error: %s" % e.reason

except socket.error, e:
    print "Error: %s" % e
