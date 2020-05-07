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
from setuptools import setup
from distutils.core import Extension

def main():
  pacparser_version = os.environ.get('PACPARSER_VERSION', '1.3.7')

  extra_objects = ['pacparser.o', 'libjs.a']
  libraries = []
  extra_link_args = []
  if sys.platform == 'win32':
    extra_objects = ['pacparser.o', 'js.lib']
    libraries = ['ws2_32']
    if 'mingw32' in sys.argv or '--compiler=mingw32' in sys.argv:
      extra_link_args = ['-static-libgcc']
    else:
      extra_objects.extend(['libgcc.a', 'legacy_stdio_definitions.lib'])

  pacparser_module = Extension('_pacparser',
                               include_dirs = ['../spidermonkey/js/src', '..'],
                               sources = ['pacparser_py.c'],
                               libraries = libraries,
                               extra_link_args = extra_link_args,
                               extra_objects = extra_objects)
  setup (name = 'pacparser',
         version = pacparser_version,
         description = 'Pacparser package',
         author = 'Manu Garg',
         author_email = 'manugarg@gmail.com',
         url = 'http://github.com/pacparser/pacparser',
         long_description = 'python library to parse proxy auto-config (PAC) '
                            'files.',
         package_data = {'': ['pacparser.o', 'libjs.a']},
         license = 'LGPL',
         ext_package = 'pacparser',
         ext_modules = [pacparser_module],
         py_modules = ['pacparser.__init__'])

if __name__ == '__main__':
  main()
