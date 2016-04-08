import unittest
import pacparser

# Hostname and IP for Honest DNS.
HDNS_HOSTNAME = 'google-public-dns-a.google.com'
HDNS_IP = '8.8.8.8'

class TestPacParser(unittest.TestCase):

  def do_test(self, pac_string, url='http://invalid', expected='OK',
              setup=None, raw_pac=False):
    if setup:
      setup()
    pacparser.init()
    if not raw_pac:
      pac_string = '''
function findProxyForURL(url, host) {
  return %s
}''' % pac_string
    pacparser.parse_pac_string(pac_string)
    got = pacparser.find_proxy(url)
    pacparser.cleanup()
    self.assertEqual(expected, got)

  def test_my_ip(self):
    self.do_test(
        'myIpAddress()',
        setup=(lambda: pacparser.setmyip('1.2.3.4')),
        expected='1.2.3.4')

  def test_hostname_extraction_from_url(self):
    self.do_test(
        pac_string='host',
        url='http://www.google.com/?q=http://www.yahoo.com',
        expected='www.google.com')

  def test_dns_resolver_hostname(self):
    self.do_test(
        pac_string='dnsResolve("%s")' % HDNS_HOSTNAME,
        expected=HDNS_IP)

  def test_dns_resolver_ip(self):
    self.do_test(
        pac_string='dnsResolve("%s")' % HDNS_IP,
        expected=HDNS_IP)

  def test_dns_resolver_hostname_none(self):
    self.do_test(
        pac_string='dnsResolve("%s")' % HDNS_HOSTNAME,
        setup=(lambda: pacparser.set_dns_resolver_variant('none')),
        expected='null')

  def test_dns_resolver_ip_none(self):
    self.do_test(
        pac_string='dnsResolve("%s")' % HDNS_IP,
        setup=(lambda: pacparser.set_dns_resolver_variant('none')),
        expected=HDNS_IP)


if __name__ == '__main__':
  unittest.main()
