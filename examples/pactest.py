#!/usr/bin/python

import pacparser

pacparser.init()
pacparser.parse_pac("wpad.dat")
proxy = pacparser.find_proxy("http://www.manugarg.com")
print(proxy)
pacparser.cleanup()

# Or simply,
print(pacparser.just_find_proxy("wpad.dat", "http://www2.manugarg.com"))
