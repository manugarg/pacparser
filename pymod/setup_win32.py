import sys
import os

py_home=sys.prefix
os.system("make -f Makefile.win32 %s PY_HOME=%s" % (sys.argv[1], py_home))
