import sys
import os

if len(sys.argv) < 2:
  target = "build"
else:
  target = sys.argv[1]

if sys.version.startswith("2.4"):
  pydll = "C:\windows\system32\python24.dll"
elif sys.version.startswith("2.5"):
  pydll = "C:\windows\system32\python25.dll"

os.system('make -f Makefile.win32 %s PY_HOME=\"%s\" PY_DLL=\"%s\"' %
          (target, sys.prefix, pydll))
