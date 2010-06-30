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

from distutils.core import setup
from distutils.core import Extension

def main():
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
    return

  pacparser_module = Extension('_pacparser',
                               include_dirs = ['../spidermonkey/js/src', '..'],
                               sources = ['pacparser_py.c'],
                               extra_objects = ['../pacparser.o', '../libjs.a'])
  setup (name = 'pacparser',
         version = '1.1.1',
         description = 'Pacparser package',
         author = 'Manu Garg',
         author_email = 'manugarg@gmail.com',
         url = 'http://code.google.com/p/pacparser',
         long_description = 'python library to parse proxy auto-config (PAC) '
                           'files.',
         license = 'LGPL',
         ext_package = 'pacparser',
         ext_modules = [pacparser_module],
         py_modules = ['pacparser.__init__'])

if len(sys.argv) < 2:
  target = ''
else:
  target = sys.argv[1]

if __name__ == '__main__':
  main()
