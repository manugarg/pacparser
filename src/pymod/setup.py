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

from distutils import sysconfig
from distutils.core import setup
from distutils.core import Extension

def main():
  # Use Makefile for windows. distutils doesn't work well with windows.
  if sys.platform == 'win32':
    pyVer = sysconfig.get_config_vars('VERSION')[0]
    pyDLL = 'C:\windows\system32\python%s.dll' % pyVer
    os.system('make -f Makefile.win32 %s PY_HOME="%s" PY_DLL="%s" PY_VER="%s"' %
              (' '.join(sys.argv[1:]), sys.prefix, pyDLL, pyVer))
    return

  pacparser_module = Extension(
      '_pacparser',
      include_dirs = ['../spidermonkey/js/src', '..'],
      sources = ['pacparser_py.c'],
      extra_objects = [
          '../pacparser.o',
          '../pacparser_dns.o',
          '../util.o',
          '../libjs.a'
      ], extra_link_args = os.getenv("C_ARES_LDFLAGS", "").split()
  )
  setup (name = 'pacparser',
         version = '1',
         description = 'Pacparser package',
         author = 'Manu Garg',
         author_email = 'manugarg@gmail.com',
         url = 'http://github.com/pacparser/pacparser',
         long_description = 'python library to parse proxy auto-config (PAC) '
                            'files.',
         license = 'LGPL',
         ext_package = 'pacparser',
         ext_modules = [pacparser_module],
         py_modules = ['pacparser.__init__'])

if __name__ == '__main__':
  main()
