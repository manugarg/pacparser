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

from pacparser import _pacparser
import os
import re
import sys

_url_regex = re.compile('.*\:\/\/([^\/]+).*')

def init():
  """
  Initializes pacparser engine.
  """
  _pacparser.init()

def parse_pac(pacfile):
  """
  (Deprecated) Same as parse_pac_file.
  """
  parse_pac_file(pacfile)

def parse_pac_file(pacfile):
  """
  Reads the pacfile and evaluates it in the Javascript engine created by
  init().
  """
  try:
    f = open(pacfile)
    pac_script = f.read()
  except IOError:
    print('Could not read the pacfile: %s\n%s' % (pacfile, sys.exc_info()[1]))
    return
  f.close()
  _pacparser.parse_pac_string(pac_script)

def parse_pac_string(pac_script):
  """
  Evaluates pac_script in the Javascript engine created by init().
  """
  _pacparser.parse_pac_string(pac_script)

def find_proxy(url, host=None):
  """
  Finds proxy string for the given url and host. If host is not
  defined, it's extracted from the url.
  """
  if host is None:
    m = _url_regex.match(url)
    if not m:
      print('URL: %s is not a valid URL' % url)
      return None
    if len(m.groups()) is 1:
      host = m.groups()[0]
    else:
      print('URL: %s is not a valid URL' % url)
      return None
  return _pacparser.find_proxy(url, host)

def version():
  """
  Returns the compiled pacparser version.
  """
  return _pacparser.version()

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
    print('PAC file: %s doesn\'t exist' % pacfile)
    return None
  if host is None:
    m = _url_regex.match(url)
    if not m:
      print('URL: %s is not a valid URL' % url)
      return None
    if len(m.groups()) is 1:
      host = m.groups()[0]
    else:
      print('URL: %s is not a valid URL' % url)
      return None
  init()
  parse_pac(pacfile)
  proxy = find_proxy(url,host)
  cleanup()
  return proxy

def setmyip(ip_address):
  """
  Set my ip address. This is the IP address returned by myIpAddress()
  """
  _pacparser.setmyip(ip_address)

def enable_microsoft_extensions():
  """
  Enables Microsoft PAC extensions (dnsResolveEx, isResolvableEx,
  myIpAddressEx).
  """
  _pacparser.enable_microsoft_extensions()
