#
# PreBot IRC bot software
# (c) Michal Kuchta 2013
#

import sqlite3

def dict_factory(cursor, row):
    d = {}
    for idx, col in enumerate(cursor.description):
        d[col[0]] = row[idx]
    return d

db = sqlite3.connect("data.db")
db.row_factory = dict_factory
