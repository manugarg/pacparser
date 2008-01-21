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
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA

"""
Python module to parse pac files. Look at project's homepage
http://code.google.com/p/pacparser for more information.
"""

__author__ = 'manugarg@gmail.com (Manu Garg)'
__copyright__ = 'Copyright (C) 2008 Manu Garg'
__license__ = 'LGPL'

import _pacparser
import os
import re

url_regex = re.compile('(http[s]?|ftp)\:\/\/([^\/]+).*')

def init():
  """
  Initialize pacparser engine.
  """
  _pacparser.init()

def parse_pac(pacfile):
  """
  Parse pacfile in the Javascript engine created by init().
  """
  _pacparser.parse_pac(pacfile)

def find_proxy(url, host=None):
  """
  Finds proxy string for the given url and host. If host is not
  defined, it's extracted from url.
  """
  if host is None:
    matches = url_regex.findall(url)[0]
    if len(matches) is not 2:
      print 'URL: %s is not a valid URL' % url
      return None
    host = matches[1]
  return _pacparser.find_proxy(url, host)

def cleanup():
  """
  Destroys pacparser engine.
  """
  _pacparser.cleanup()

def just_find_proxy(pacfile, url, host=None):
  """
  This function is a wrapper around init, parse_pac, find_proxy
  and cleanup. This is the function to call if you want to find
  proxy just for one url.
  """
  if os.path.isfile(pacfile):
    pass
  else:
    print 'PAC file: %s doesn\'t exist' % pacfile
    return None
  if host is None:
    matches = url_regex.findall(url)[0]
    if len(matches) is not 2:
      print 'URL: %s is not a valid URL' % url
      return None
    host = matches[1]
  init()
  parse_pac(pacfile)
  proxy = find_proxy(url,host)
  cleanup()
  return proxy
