# Copyright (C) 2007-2022 Manu Garg.
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
import glob
import os
import platform
import re
import shutil
import sys

from unittest.mock import patch
import distutils
from setuptools import setup, Extension

import distutils.cmd

def build_dir():
  return os.path.join(os.path.dirname(os.path.join(os.getcwd(), sys.argv[0])), 'build')

def module_path():
  py_ver = '.'.join([str(x) for x in sys.version_info[0:2]])
  return glob.glob(os.path.join(build_dir(), 'lib*%s' % py_ver))[0]


def pacparser_version(pacparser_module_path=''):
  if not os.path.exists(build_dir()):
    return '1.0.0'

  if not pacparser_module_path:
    pacparser_module_path = module_path()

  print(pacparser_module_path)
  sys.path.insert(0, pacparser_module_path)
  import pacparser
  ver = pacparser.version()
  # Strip first 'v' and last part from git provided versions.
  # For example, v1.3.8-12-g231 becomes v1.3.8-12.
  ver = re.sub(r'^v?([\d]+\.[\d]+\.[\d]+(-[\d]+)).*$', '\\1', ver)
  # 1.3.8-12 becomes 1.3.8.dev12
  return ver.replace('-', '.dev')


class DistCmd(distutils.cmd.Command):
  """Build pacparser python distribution."""

  description = 'Build pacparser python distribution.'
  user_options = []

  def initialize_options(self):
    pass

  def finalize_options(self):
    pass

  def run(self):
    py_ver = '.'.join([str(x) for x in sys.version_info[0:2]])
    pacparser_module_path = module_path()
    pp_ver = pacparser_version(pacparser_module_path)

    mach = platform.machine().lower()
    if mach == 'x86_64':
      mach = 'amd64'
    dist = 'pacparser-python%s-%s-%s-%s' % (
      py_ver.replace('.',''), pp_ver, platform.system().lower(), mach)

    if os.path.exists(dist):
      shutil.rmtree(dist)
    os.mkdir(dist)
    shutil.copytree(os.path.join(pacparser_module_path, 'pacparser'),
                    dist+'/pacparser',
                    ignore=shutil.ignore_patterns('*pycache*'))


@patch('distutils.cygwinccompiler.get_msvcr')
def main(patched_func):
  python_home = os.path.dirname(sys.executable)

  extra_objects = ['../pacparser.o', '../spidermonkey/libjs.a']
  libraries = []
  extra_link_args = []
  if sys.platform == 'win32':
    extra_objects = ['../pacparser.o', '../spidermonkey/js.lib']
    libraries = ['ws2_32']
    # python_home has vcruntime140.dll
    patched_func.return_value =  ['vcruntime140']
    extra_link_args = ['-static-libgcc', '-L'+python_home]

  pacparser_module = Extension('_pacparser',
                               include_dirs = ['../spidermonkey/js/src', '..'],
                               sources = ['pacparser_py.c'],
                               libraries = libraries,
                               extra_link_args = extra_link_args,
                               extra_objects = extra_objects)
  setup (
        cmdclass={
            'dist': DistCmd,
        },
        name = 'pacparser',
        version = pacparser_version(),
        description = 'Pacparser package',
        author = 'Manu Garg',
        author_email = 'manugarg@gmail.com',
        url = 'http://github.com/manugarg/pacparser',
        long_description = ('python library to parse proxy auto-config (PAC) '
                            'files.'),
        package_data = {'': ['pacparser.o', 'libjs.a']},
        license = 'LGPL',
        ext_package = 'pacparser',
        ext_modules = [pacparser_module],
        py_modules = ['pacparser.__init__'])

if __name__ == '__main__':
  main()
