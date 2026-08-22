# Copyright (C) 2007-2026 Manu Garg.
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
Command line tool to test PAC files. It provides the same interface as the
C pactester tool:

  pactester <-p pacfile> <-u url> [-h host] [-c client_ip] [-e]
  pactester <-p pacfile> <-f urlslist> [-c client_ip] [-e]
"""

import getopt
import sys

import pacparser

PACMAX = 1024 * 1024  # Max size of the PAC script (1 MiB)

PROGNAME = "pactester"

def usage():
  sys.stderr.write("\nUsage:  %s <-p pacfile> <-u url> [-h host] "
                   "[-c client_ip] [-e]\n" % PROGNAME)
  sys.stderr.write("        %s <-p pacfile> <-f urlslist> "
                   "[-c client_ip] [-e]\n\n" % PROGNAME)
  sys.stderr.write("Options:\n")
  sys.stderr.write("  -p pacfile   : PAC file to test (specify '-' to read "
                   "from standard input)\n")
  sys.stderr.write("  -u url       : URL to test for\n")
  sys.stderr.write("  -h host      : Host part of the URL\n")
  sys.stderr.write("  -c client_ip : client IP address (as returned by "
                   "myIpAddres() function\n")
  sys.stderr.write("                 in PAC files), defaults to IP address "
                   "on which it is running.\n")
  sys.stderr.write("  -e           : Deprecated: IPv6 extensions are enabled"
                   "by default now.\n")
  sys.stderr.write("  -f urlslist  : a file containing list of URLs to be "
                   "tested.\n")
  sys.stderr.write("  -v           : print version and exit\n")
  sys.exit(1)

def get_host_from_url(url):
  # Extract the host part of the URL, like the C pactester does. Prints an
  # error and returns None if the URL is not in a recognized form.
  idx = url.find(':')
  if idx < 0 or url[idx + 1:idx + 3] != '//':
    sys.stderr.write("pactester: Not a proper URL\n")
    return None
  start = idx + 3
  if start >= len(url) or url[start] in ('/', ':'):
    sys.stderr.write("pactester: Not a proper URL\n")
    return None
  for end in range(start, len(url)):
    if url[end] in ('/', ':'):
      break
  else:
    end = len(url)
  host = url[start:end]
  if not host:
    sys.stderr.write("pactester: Not a proper URL\n")
    return None
  return host

def main():
  argv = sys.argv[1:]
  if argv and argv[0] in ("--help", "--helpshort"):
    usage()

  pacfile = url = host = urlslist = client_ip = None

  try:
    opts, _ = getopt.getopt(argv, "evp:u:h:f:c:")
  except getopt.GetoptError:
    usage()

  for opt, arg in opts:
    if opt == "-v":
      print(pacparser.version())
      return 0
    elif opt == "-p":
      pacfile = arg
    elif opt == "-u":
      url = arg
    elif opt == "-h":
      host = arg
    elif opt == "-f":
      urlslist = arg
    elif opt == "-c":
      client_ip = arg
    elif opt == "-e":
      pass

  if not pacfile:
    sys.stderr.write("pactester: You didn't specify the PAC file\n")
    usage()
  if not url and not urlslist:
    sys.stderr.write("pactester: You didn't specify the URL\n")
    usage()

  try:
    pacparser.init()
  except Exception:
    sys.stderr.write("pactester: Could not initialize pacparser\n")
    return 1

  if pacfile == "-":
    script = sys.stdin.read()
    if len(script) > PACMAX:
      sys.stderr.write("Input file is too big. Maximum allowed size is: %d"
                       % PACMAX)
      pacparser.cleanup()
      return 1
    try:
      pacparser.parse_pac_string(script)
    except Exception:
      sys.stderr.write("pactester: Could not parse the pac script: %s\n"
                       % script)
      pacparser.cleanup()
      return 1
  else:
    try:
      pacparser.parse_pac_file(pacfile)
    except Exception:
      sys.stderr.write("pactester: Could not parse the pac file: %s\n"
                       % pacfile)
      pacparser.cleanup()
      return 1

  if client_ip:
    pacparser.setmyip(client_ip)

  if url:
    if not host:
      host = get_host_from_url(url)
      if not host:
        pacparser.cleanup()
        return 1
    try:
      proxy = pacparser.find_proxy(url, host)
    except Exception:
      sys.stderr.write("pactester: Problem in finding proxy for %s.\n" % url)
      pacparser.cleanup()
      return 1
    print(proxy)
    pacparser.cleanup()
    return 0

  if urlslist:
    try:
      fp = open(urlslist, "r")
    except IOError:
      sys.stderr.write("pactester: Could not open urlslist: %s" % urlslist)
      pacparser.cleanup()
      return 1
    for line in fp:
      u = line.lstrip(" \t")
      # Skip comment lines, echoing them as the C pactester does.
      if u.startswith("#"):
        sys.stdout.write(u)
        continue
      fields = u.split(None, 1)
      if not fields:
        continue
      u = fields[0]
      host = get_host_from_url(u)
      if not host:
        continue
      try:
        proxy = pacparser.find_proxy(u, host)
      except Exception:
        sys.stderr.write("pactester: Problem in finding proxy for %s.\n" % u)
        pacparser.cleanup()
        return 1
      if proxy:
        print("%s : %s" % (u, proxy))
    fp.close()
    pacparser.cleanup()
    return 0

  pacparser.cleanup()
  return 0

if __name__ == '__main__':
  sys.exit(main())
