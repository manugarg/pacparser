#!/bin/bash
# Test the 'pactester' utility (and with it, indirectly, the 'pacparser'
# C library).

set -u -e

. ./pactester_test_lib.sh || exit 1

# We want to make sure not to do any DNS query, local or remote.
PACPARSER_COMMON_ARGS+=(-r "none")

#=== Option parsing ===#

(($# == 0)) || die "this script doesn't take any command-line argument"

#=== Tests ===#

## Sanity check: DNS queries actually fail.
ok <<'EOT'
  var r = dnsResolve('www.google.com');
  return (r == null) ? 'OK': 'KO -> ' + r;
EOT

## Basics.

ok <<< 'return "OK"'
ok <<< 'if (true) { return "OK" }'
ok <<< 'if (false) { return "KO" } return "OK"'
ok <<< 'if (!true) return "KO"; else return "OK";'
ok <<< 'return "OK"'

## Regular expressions.

js_true  <<< '/oo+/.test("foobar")'
js_false <<< '/ff+/.test("foobar")'

js_true  <<< '/fo{2,4}b/.test("foobar")'
js_true  <<< '/fo{2,4}b/.test("fooobar")'
js_true  <<< '/fo{2,4}b/.test("foooobar")'
js_false <<< '/fo{2,4}b/.test("fobar")'
js_false <<< '/fo{2,4}b/.test("fooooobar")'

## Special PAC functions.

# shExpMatch
js_true  <<< 'shExpMatch("ab.cd", "*d")'
js_true  <<< 'shExpMatch("ab.cd", "*.cd")'
js_true  <<< 'shExpMatch("ab.cd", "??.cd")'
js_true  <<< 'shExpMatch("ab.cd", "a?.?d")'
js_true  <<< 'shExpMatch("ab.cd", "a???d")'
js_true  <<< 'shExpMatch("ab.cd", "a?*d")'
js_true  <<< 'shExpMatch("ab.cd", "ab.cd")'
js_true  <<< 'shExpMatch("ab.cd", "*ab.cd*")'
js_false <<< 'shExpMatch("ab.cd", "?ab.cd")'
js_false <<< 'shExpMatch("ab.cd", "ab.c?d")'
js_false <<< 'shExpMatch("ab.cd", "ab-cd")'

# isPlainHostName
js_true  <<< 'isPlainHostName("cs")'
js_true  <<< 'isPlainHostName("farg6--fg56jh")'
js_false <<< 'isPlainHostName("cs.corp")'
js_false <<< 'isPlainHostName("cs.corp.google.com")'

# dnsDomainIs
js_true  <<< 'dnsDomainIs("www.google.com", "www.google.com")'
js_true  <<< 'dnsDomainIs("www.google.com", "google.com")'
js_true  <<< 'dnsDomainIs("www.google.com", ".com")'
js_false <<< 'dnsDomainIs("www.google.com", "www.google.com.")'
js_false <<< 'dnsDomainIs("google.com", "www.google.com")'
js_false <<< 'dnsDomainIs("google.com", ".edu")'

# localHostOrDomainIs
js_true  <<< 'localHostOrDomainIs("www", "www.google.com")'
js_true  <<< 'localHostOrDomainIs("www.google", "www.google.com")'
js_true  <<< 'localHostOrDomainIs("www.google.com", "www.google.com")'
js_true  <<< 'localHostOrDomainIs("www", "www")'
js_true  <<< 'localHostOrDomainIs("www.edu", "www.edu")'
js_true  <<< 'localHostOrDomainIs("www", "www.edu")'
js_false <<< 'localHostOrDomainIs("www.google.com.", "www.google.com")'
js_false <<< 'localHostOrDomainIs("www.corp.google.com", "www.google.com")'
js_false <<< 'localHostOrDomainIs("www.google.com", "www")'
js_false <<< 'localHostOrDomainIs("www.edu", "www")'
js_false <<< 'localHostOrDomainIs("www.edu", "www.ed")'

# isInNet

js_true  <<< 'isInNet("1.2.3.4", "1.0.0.0", "255.0.0.0")'
js_true  <<< 'isInNet("1.2.3.4", "1.2.0.0", "255.255.0.0")'
js_true  <<< 'isInNet("1.2.3.4", "1.2.3.0", "255.255.255.0")'
js_true  <<< 'isInNet("1.2.3.4", "1.2.3.4", "255.255.255.255")'
js_true  <<< 'isInNet("1.2.3.4", "1.2.3.5", "255.255.255.254")'
js_true  <<< 'isInNet("1.2.3.4", "212.10.75.33", "0.0.0.0")'
js_false <<< 'isInNet("1.2.3.4", "1.0.0.0", "255.255.0.0")'
js_false <<< 'isInNet("1.2.3.4", "1.2.3.5", "255.255.255.255")'
js_false <<< 'isInNet("1.2.3.4", "1.2.3.6", "255.255.255.254")'

# isInNetEx (IPv4)
js_true  <<< 'isInNetEx("1.2.3.4", "1.0.0.0/8")'
js_true  <<< 'isInNetEx("1.2.3.4", "1.2.0.0/16")'
js_true  <<< 'isInNetEx("1.2.3.4", "1.2.3.0/24")'
js_true  <<< 'isInNetEx("1.2.3.4", "1.2.3.4/31")'
js_true  <<< 'isInNetEx("1.2.3.4", "1.2.3.5/31")'
js_true  <<< 'isInNetEx("1.2.3.4", "212.10.75.33/0")'
js_false <<< 'isInNetEx("1.2.3.4", "1.0.0.0/16")'
js_false <<< 'isInNetEx("1.2.3.4", "1.2.3.5/32")'
js_false <<< 'isInNetEx("1.2.3.4", "1.2.3.6/31")'

# isInNetEx (IPv6)
js_true  <<< 'isInNetEx("::", "2001:db8::1/0")'
js_true  <<< 'isInNetEx("ffff:ffff:ffff:ffff:ffff:ffff:ffff:ffff", "2001:db8::1/0")'
js_true  <<< 'isInNetEx("2001:db8::", "2001:db8::1/33")'
js_false <<< 'isInNetEx("2001:db8:8000::", "2001:db8::1/33")'
js_true  <<< 'isInNetEx("2001:db8:7fff:ffff:ffff:ffff:ffff:ffff", "2001:db8::1/33")'
js_false <<< 'isInNetEx("2001:db7:ffff:ffff:ffff:ffff:ffff:ffff", "2001:db8::1/33")'

# dnsDomainLevels
ok <<< 'r = dnsDomainLevels("foo"); return r == 0 ? "OK" : "KO -> " + r'
ok <<< 'r = dnsDomainLevels("foo.edu"); return r == 1 ? "OK" : "KO -> " + r'
ok <<'EOF'
  r = dnsDomainLevels("username.www.corp.google.com")
  return r == 4 ? "OK" : "KO -> " + r
EOF

# myIpAddress

ok -E <<'EOF'
  r = myIpAddress();
  return r == "127.0.0.1" ? "OK" : "KO -> " + r;
EOF

ipv4='1.2.3.4'
ok -E -c "${ipv4}" <<EOF
  r = myIpAddress();
  return r == "${ipv4}" ? "OK" : "KO -> " + r;
EOF
unset ipv4

# myIpAddressEx

ok -e <<'EOF'
  r = myIpAddressEx();
  return r == "127.0.0.1" ? "OK" : "KO -> " + r;
EOF

ipv6='2620:0:1040:c:a1d6:dd57:fc3a:609c'
ok -e -c "${ipv6}" <<EOF
  r = myIpAddressEx();
  return r == "${ipv6}" ? "OK" : "KO -> " + r;
EOF
unset ipv6

## Runtime/syntax errors are diagnosed.

ko 'Failed to evaluate the pac script' <<< 'return "OK;'
ko 'Failed to evaluate the pac script' <<< 'if(0'
ko 'ReferenceError: none is not defined' <<< 'return none.foo;'

## Valid URLs.

for url in 'http://foo' 'http://127.0.0.1:8080' 'http://[::1]'; do
  ok -u "${url}" <<< 'return "OK"'
done
unset url

## Invalid URLs.

for opt in '-e' '-E'; do
  for url in '' 'http' 'http:' 'http:/' 'http://' 'http://[::1'; do
    ko "pactester.c: Not a proper URL" -u "${url}" ${opt} <<< 'return "OK"'
  done
done
unset url opt

## "Microsoft extensions" functions.

for func in isResolvableEx dnsResolveEx myIpAddressEx isInNetEx; do

  case ${func} in
    myIpAddressEx) arg='';;
    isInNetEx) arg='"1.2.3.4", "1.0.0.0/8"';;
    *) arg='"1.2.3.4"';;
  esac

  ok -E <<EOT
    var t = typeof(${func});
    if (t == 'undefined')
      return 'OK';
    return 'KO -> ' + t;
EOT

  ok -e <<EOT
    var t = typeof(${func});
    if (t == 'function')
      return 'OK';
    return 'KO -> ' + t;
EOT

  ko "ReferenceError: ${func} is not defined" -E <<< "${func}(${arg})"
  ok -e <<< "return ${func}(${arg}) ? 'OK' : 'KO'"

done
unset func arg

do_test_status 0 -e <<'EOT'
  function FindProxyForURLEx(host, url) {
    return 'OK'
  }
EOT

do_test_status 1 -E <<'EOT'
  function FindProxyForURLEx(host, url) {
    return 'OK'
  }
EOT
(set -x && grep -E -e "ReferenceError: FindProxyForURL is not defined" $ERR) \
  || register_failure

## Make sure we don't try to send actual DNS queries for hostnames that
## are actually IP addresses.

declare -r -a IP_V4_ADDRESSES=(
  # Loopback IPs.
  '127.0.0.1'
  '127.34.253.52'
  # Made-up IP.
  '213.78.9.34'
  # IP of example.com
  '93.184.216.34'
  # IP of facebook.com
  '173.252.73.52'
)

declare -r -a IP_V6_ADDRESSES=(
  # Loopback IP.
  '::1'
  # Made-up IP.
  'dead:beef:1:7f09:4321:bead:2:1'
  # IP of example.com
  '2606:2800:220:1:248:1893:25c8:1946'
  # IP of facebook.com
  '2a03:2880:20:4f06:face:b00c:0:1'
)

declare -r -a IP_ADDRESSES=(
  "${IP_V4_ADDRESSES[@]}"
  "${IP_V6_ADDRESSES[@]}"
)

for ip4 in "${IP_V4_ADDRESSES[@]}"; do
  js_true -E <<< "isResolvable('${ip4}')"
done
unset ip4

for ip in "${IP_ADDRESSES[@]}"; do
  js_true -e <<< "isResolvableEx('${ip}')"
done
unset ip

for ip4 in "${IP_V4_ADDRESSES[@]}"; do
  ok -E <<EOT
    var ip = '${ip4}';
    var result = dnsResolve(ip)
    return result == ip ? 'OK' : 'KO -> ' + result;
EOT
done
unset ip4

for ip in "${IP_ADDRESSES[@]}"; do
  ok -e <<EOT
    var ip = '${ip}';
    var result = dnsResolveEx(ip)
    return result == ip ? 'OK' : 'KO -> ' + result;
EOT
done
unset ip

# Empty hostname should not cause DNS queries, and should
# resolve to null.

ok -E <<EOT
  var result = dnsResolve('')
  return result == null ? 'OK' : 'KO -> ' + result;
EOT

ok -e <<EOT
  var result = dnsResolveEx('')
  return result == null ? 'OK' : 'KO -> ' + result;
EOT

#=== Results ===#

declare_test_results_and_exit
