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

import getopt
import glob
import os
import sys

def module_path(tests_dir):
  py_ver = '*'.join([str(x) for x in sys.version_info[0:2]])
  
  builddir = os.path.join(tests_dir, '..', 'src', 'pymod', 'build')
  print('Build dir: %s', builddir)
  print(os.listdir(builddir))
  
  return glob.glob(os.path.join(builddir, 'lib*%s' % py_ver))[0]
  
def runtests(pacfile, testdata, tests_dir):
  try:
    pacparser_module_path = module_path(tests_dir)
  except Exception:
    raise Exception('Tests failed. Could not determine pacparser path.')
  if 'DEBUG' in os.environ: print('Pacparser module path: %s' %
                                  pacparser_module_path)
  sys.path.insert(0, pacparser_module_path)

  try:
    import pacparser
  except ImportError:
    raise Exception('Tests failed. Could not import pacparser.')

  if 'DEBUG' in os.environ: print('Imported pacparser module: %s' %
                                  sys.modules['pacparser'])

  f = open(testdata)
  for line in f:
    comment = ''
    if '#' in line:
      comment = line.split('#', 1)[1]
      line = line.split('#', 1)[0].strip()
    if not line:
      continue
    if ('NO_INTERNET' in os.environ and os.environ['NO_INTERNET'] and
        'INTERNET_REQUIRED' in comment):
      continue
    if 'DEBUG' in os.environ: print(line)
    (params, expected_result) = line.strip().split('|')
    args = dict(getopt.getopt(params.split(), 'eu:c:')[0])
    if '-e' in args:
      pacparser.enable_microsoft_extensions()
    if '-c' in args:
      pacparser.setmyip(args['-c'])
    pacparser.init()
    pacparser.parse_pac_file(pacfile)
    result = pacparser.find_proxy(args['-u'])
    pacparser.cleanup()
    if result != expected_result:
      raise Exception('Tests failed. Got "%s", expected "%s"' % (result, expected_result))
  print('All tests were successful.')


def main():
  tests_dir = os.path.dirname(os.path.join(os.getcwd(), sys.argv[0]))
  pacfile = os.path.join(tests_dir, 'proxy.pac')
  testdata = os.path.join(tests_dir, 'testdata')
  runtests(pacfile, testdata, tests_dir)

if __name__ == '__main__':
  main()
