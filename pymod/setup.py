import sys
import os

if len(sys.argv) < 2:
  target = ''
else:
  target = sys.argv[1]

ver = sys.version[0:3]

if sys.platform.startswith("linux"):
  env = ''
  if 'LDFLAGS' in os.environ:
    env = '%s LDFLAGS="%s"' % (env, os.environ['LDFLAGS'])
  if 'CFLAGS' in os.environ:
    env = '%s CFLAGS="%s"' % (env, os.environ['CFLAGS'])
  os.system('%s make %s PY_VER="%s"' % (env, target, ver))

if sys.platform == 'win32':
  pydll = 'C:\windows\system32\python%s.dll' % ver.replace('.', '')
  os.system('make -f Makefile.win32 %s PY_HOME="%s" PY_DLL="%s"' %
            (target, sys.prefix, pydll))
