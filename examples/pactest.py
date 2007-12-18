#!/usr/bin/python2.5

from ctypes import *

pp = CDLL("libpacparser.so")

pp.pacparser_init()
pp.pacparser_parse_pac("wpad.dat")
proxy = pp.pacparser_find_proxy("http://www.manugarg.com", "www.manugarg.com")
print string_at(proxy)
pp.pacparser_cleanup()

# Or simply,
print string_at(pp.pacparser_just_find_proxy("examples/wpad.dat",
                                             "http://xyz.manugarg.com",
                                             "xyz.manugarg.com"))
