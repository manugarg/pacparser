#!/usr/bin/python

import getopt
import glob
import os
import sys


def runtests(pacfile, testdata, tests_dir):
  try:
    pacparser_module_path = glob.glob(os.path.join(tests_dir, '..', 'pymod',
                                                   'build', 'lib*'))[0]
  except:
    print 'Tests failed. Could not determine pacparser path.\n%s' % str(e)
    return 1

  sys.path.insert(0, pacparser_module_path)

  try:
    import pacparser
  except ImportError, e:
    print 'Tests failed. Could not import pacparser.\n%s' % str(e)
    return 1

  f = open(testdata)
  for line in f:
    if line.startswith('#'):
      continue
    if 'DEBUG' in os.environ: print line
    (params, expected_result) = line.strip().split('|')
    args = dict(getopt.getopt(params.split(), 'eu:c:')[0])
    if '-e' in args:
      pacparser.enable_microsoft_extensions()
    if '-c' in args:
      pacparser.setmyip(args['-c'])
    pacparser.init()
    pacparser.parse_pac(pacfile)
    result = pacparser.find_proxy(args['-u'])
    pacparser.cleanup()
    if result != expected_result:
      print 'Tests failed. Got "%s", expected "%s"' % (result, expected_result)
      return 1
  print 'All tests were successful.'


def main():
  tests_dir = os.path.dirname(os.path.join(os.getcwd(), sys.argv[0]))
  pacfile = os.path.join(tests_dir, 'proxy.pac')
  testdata = os.path.join(tests_dir, 'testdata')
  runtests(pacfile, testdata, tests_dir)

if __name__ == '__main__':
  main()
