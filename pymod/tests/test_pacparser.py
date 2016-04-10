"""Tests for pacparser python wrapper."""

import os
import unittest
import pacparser

# Hostname and IP for Honest DNS.
HDNS_BASENAME = 'google-public-dns-a'
HDNS_FQDN = 'google-public-dns-a.google.com'
HDNS_IP = '8.8.8.8'

class TestPacParser(unittest.TestCase):

  def do_test(self, pac_string, url='http://invalid', expected='OK',
              setup=None, raw_pac=False, msg=None):
    if setup:
      setup()
    pacparser.init()
    if not raw_pac:
      pac_string = '''
function findProxyForURL(url, host) {
  return %s
}''' % pac_string
    try:
      pacparser.parse_pac_string(pac_string)
      got = pacparser.find_proxy(url)
    finally:
      pacparser.cleanup()
    self.assertEqual(expected, got, msg)

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
        pac_string='dnsResolve("%s")' % HDNS_FQDN,
        expected=HDNS_IP)

  def test_dns_resolver_ip(self):
    self.do_test(
        pac_string='dnsResolve("%s")' % HDNS_IP,
        expected=HDNS_IP)

  def test_dns_resolver_hostname_none(self):
    self.do_test(
        pac_string='dnsResolve("%s")' % HDNS_FQDN,
        setup=(lambda: pacparser.set_dns_resolver_variant('none')),
        expected='null')

  def test_dns_resolver_ip_none(self):
    self.do_test(
        pac_string='dnsResolve("%s")' % HDNS_IP,
        setup=(lambda: pacparser.set_dns_resolver_variant('none')),
        expected=HDNS_IP)

  def test_enable_microsoft_extensions(self):
    # No need for setup, should be enabled by default.
    self.do_test(
        setup=(lambda: pacparser.enable_microsoft_extensions()),
        pac_string='typeof(dnsResolveEx)',
        expected='function')

  def test_disable_microsoft_extensions(self):
    self.do_test(
        setup=(lambda: pacparser.disable_microsoft_extensions()),
        pac_string='typeof(dnsResolveEx)',
        expected='undefined')

  def test_change_microsoft_extensions(self):
    # No need for setup, should be enabled by default.
    self.do_test(
        pac_string='typeof(isInNetEx)',
        expected='function')
    # Disable explicitly.
    self.do_test(
        setup=(lambda: pacparser.disable_microsoft_extensions()),
        pac_string='typeof(isInNetEx)',
        expected='undefined')
    # Should be enabled again.
    self.do_test(
        pac_string='typeof(isInNetEx)',
        expected='function')

  def test_is_in_net(self):
    for ip, net, mask, result in [
        ('1.2.3.4', '1.0.0.0', '255.0.0.0', 'true'),
        ('1.2.3.4', '1.2.3.5', '255.255.255.254', 'true'),
        ('1.2.3.4', '1.2.3.4', '255.255.255.255', 'true'),
        ("1.2.3.4", "1.0.0.0", "255.255.0.0", 'false'),
        ("1.2.3.4", "1.2.3.5", "255.255.255.255", 'false'),
        ("1.2.3.4", "1.2.3.5", "255.255.255.255", 'false'),
        ("1.2.3.4", "1.2.3.6", "255.255.255.254", 'false'),
    ]:
      self.do_test(
          setup=(lambda: pacparser.disable_microsoft_extensions()),
          pac_string='isInNet("%s", "%s", "%s")' % (ip, net, mask),
          expected=result)

  def test_is_in_net_ex(self):
    for ip, net_and_mask, result in [
        ('1.2.3.4', '1.0.0.0/8', 'true'),
        ('1.2.3.4', '1.2.0.0/16', 'true'),
        ('1.2.3.4', '1.2.3.0/24', 'true'),
        ('1.2.3.4', '1.2.3.4/31', 'true'),
        ('1.2.3.4', '1.2.3.5/31', 'true'),
        ('1.2.3.4', '1.0.0.0/16', 'false'),
        ('1.2.3.4', '1.2.3.5/32', 'false'),
        ('1.2.3.4', '1.2.3.6/31', 'false'),
        ('::', '2001:db8::1/0', 'true'),
        ('ffff:ffff:ffff:ffff:ffff:ffff:ffff:ffff', '2001:db8::1/0', 'true'),
        ('2001:db8::', '2001:db8::1/33', 'true'),
        ('2001:db8:8000::', '2001:db8::1/33', 'false'),
        ('2001:db8:7fff:ffff:ffff:ffff:ffff:ffff', '2001:db8::1/33', 'true'),
        ('2001:db7:ffff:ffff:ffff:ffff:ffff:ffff', '2001:db8::1/33', 'false'),
    ]:
      self.do_test(
          pac_string='isInNetEx("%s", "%s")' % (ip, net_and_mask),
          expected=result)

  @unittest.skipIf(os.getenv('ENABLE_C_ARES', 'yes') == 'no',
                   'requires c-ares integration')
  def test_set_dns_servers(self):

    def make_setup(servers):
      def setup_func():
        pacparser.set_dns_resolver_variant('c-ares')
        pacparser.set_dns_servers(servers)
      return setup_func

    self.do_test(
        setup=make_setup('127.34.253.89'),
        pac_string='dnsResolve("%s")' % HDNS_FQDN,
        expected='null')
    self.do_test(
        setup=make_setup(HDNS_IP),
        pac_string='dnsResolve("%s")' % HDNS_FQDN,
        expected=HDNS_IP)
    self.do_test(
        setup=make_setup(HDNS_IP),
        pac_string='dnsResolve("%s")' % HDNS_FQDN,
        expected=HDNS_IP)

  @unittest.skipIf(os.getenv('ENABLE_C_ARES', 'yes') == 'no',
                   'requires c-ares integration')
  def test_set_dns_domains(self):

    def make_setup(domains):
      def setup_func():
        pacparser.set_dns_resolver_variant('c-ares')
        pacparser.set_dns_domains(domains)
      return setup_func

    google = 'google.com.'
    fake = 'i-do-not-exists.fakedomain'

    self.do_test(
        setup=make_setup(google),
        pac_string='dnsResolve("%s")' % HDNS_BASENAME,
        expected=HDNS_IP)
    self.do_test(
        setup=make_setup(google),
        pac_string='dnsResolve("%s.")' % HDNS_BASENAME,
        expected='null')
    self.do_test(
        setup=make_setup(fake),
        pac_string='dnsResolve("%s")' % HDNS_BASENAME,
        expected='null')
    self.do_test(
        setup=make_setup(fake),
        pac_string='dnsResolve("%s")' % HDNS_FQDN,
        expected=HDNS_IP)
    self.do_test(
        setup=make_setup(','.join([fake, google])),
        pac_string='dnsResolve("%s")' % HDNS_BASENAME,
        expected=HDNS_IP)


if __name__ == '__main__':
  unittest.main()
