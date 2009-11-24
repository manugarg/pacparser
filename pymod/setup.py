# Copyright (C) 2007 Manu Garg.
# Author: Manu Garg <manugarg@gmail.com>
#
# pacparser is a library that provides methods to parse proxy auto-config
# (PAC) files. Please read README file included with this package for more
# information about this library.
#
# pacparser is free software; you can redistribute it and/or
# modify it under the terms of the GNU Lesser General Public
# License as published by the Free Software Foundation; either
# version 2.1 of the License, or (at your option) any later version.

# pacparser is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# Lesser General Public License for more details.

# You should have received a copy of the GNU Lesser General Public
# License along with this library; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301,
# USA.

"""
Wrapper script around python module Makefiles. This script take care of
identifying python setup and setting up some environment variables needed by
Makefiles.
"""

import sys
import os

if len(sys.argv) < 2:
  target = ''
else:
  target = sys.argv[1]

ver = sys.version[0:3]
print sys.platform
if sys.platform.startswith("linux") or sys.platform == "darwin":
  os.system('make %s PY_VER="%s"' % (target, ver))

if sys.platform == 'win32':
  #install target is used to just install compiled files.
  if target == 'install':
    import shutil
    install_path = '%s\Lib\site-packages\pacparser' % sys.prefix
    if os.path.isdir(install_path):
      shutil.rmtree(install_path)
    shutil.copytree('.', install_path)
  else:
    pydll = 'C:\windows\system32\python%s.dll' % ver.replace('.', '')
    os.system('make -f Makefile.win32 %s PY_HOME="%s" PY_DLL="%s"' %
              (target, sys.prefix, pydll))
