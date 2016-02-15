#!/bin/bash
# Test the 'pactester' utility (and with it, indirectly, the 'pacparser'
# C library).

set -u -e

. ./pactester_test_lib.sh || exit 1

# We want to make sure not to do any non-local network request, so use
# an invalid IP as the address of the DNS server.
# And some systems run (likely for caching reasons) a local DNS server
# listening on 127.0.0.1; so we use a somewhat more creative address.
PACPARSER_COMMON_ARGS+=(-s 127.1.2.3)

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
js_true   <<< 'localHostOrDomainIs("www", "www.google.com")'
js_true   <<< 'localHostOrDomainIs("www.google", "www.google.com")'
js_true   <<< 'localHostOrDomainIs("www.google.com", "www.google.com")'
js_true   <<< 'localHostOrDomainIs("www", "www")'
js_true   <<< 'localHostOrDomainIs("www.edu", "www.edu")'
js_true   <<< 'localHostOrDomainIs("www", "www.edu")'
js_false  <<< 'localHostOrDomainIs("www.google.com.", "www.google.com")'
js_false  <<< 'localHostOrDomainIs("www.corp.google.com", "www.google.com")'
js_false  <<< 'localHostOrDomainIs("www.google.com", "www")'
js_false  <<< 'localHostOrDomainIs("www.edu", "www")'
js_false  <<< 'localHostOrDomainIs("www.edu", "www.ed")'

# isInNet (notice: this could use more extensive tests...)
js_true  <<< 'isInNet("1.2.3.4", "1.0.0.0", "255.0.0.0")'
js_true  <<< 'isInNet("1.2.3.4", "1.2.0.0", "255.255.0.0")'
js_true  <<< 'isInNet("1.2.3.4", "1.2.3.0", "255.255.255.0")'
js_true  <<< 'isInNet("1.2.3.4", "1.2.3.4", "255.255.255.255")'
js_true  <<< 'isInNet("1.2.3.4", "1.2.3.5", "255.255.255.254")'
js_true  <<< 'isInNet("1.2.3.4", "212.10.75.33", "0.0.0.0")'
js_false <<< 'isInNet("1.2.3.4", "1.0.0.0", "255.255.0.0")'
js_false <<< 'isInNet("1.2.3.4", "1.2.3.5", "255.255.255.255")'
js_false <<< 'isInNet("1.2.3.4", "1.2.3.5", "255.255.255.255")'
js_false <<< 'isInNet("1.2.3.4", "1.2.3.6", "255.255.255.254")'

# dnsDomainLevels.
ok <<< 'r = dnsDomainLevels("foo"); return r == 0 ? "OK" : "KO -> " + r'
ok <<< 'r = dnsDomainLevels("foo.edu"); return r == 1 ? "OK" : "KO -> " + r'
ok <<'EOF'
  r = dnsDomainLevels("username.www.corp.google.com")
  return r == 4 ? "OK" : "KO -> " + r
EOF

## Syntax errors are diagnosed.

ko 'SyntaxError: unterminated string literal' <<< 'return "OK;'
ko 'ReferenceError: none is not defined' <<< 'return none.foo;'

## Valid URLs.

for url in 'http://foo' 'http://127.0.0.1:8080' 'http://[::1]'; do
  ok -u "${url}" <<< 'return "OK"'
done
unset url

## Invalid URLs.

for url in '' 'http' 'http:' 'http:/' 'http://' 'http://[::1'; do
  ko "pactester.c: Not a proper URL" -u "${url}" <<< 'return "OK"'
done
unset url

## "Microsoft extensions" functions.

for f in isResolvableEx dnsResolveEx myIpAddressEx; do

  case $f in
    myIpAddressEx) x='';;
    *) x='"0.0.0.0"';;
  esac

  ok <<EOT
    var t = typeof($f);
    if (t == 'undefined')
      return 'OK';
    return 'KO -> ' + t;
EOT

  ok -e <<EOT
    var t = typeof($f);
    if (t == 'function')
      return 'OK';
    return 'KO -> ' + t;
EOT

  ko "ReferenceError: $f is not defined" <<< "$f($x)"
  ok -e <<< "$f($x); return 'OK'"

done
unset f x

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
  js_true <<< "isResolvable('${ip4}')"
done
unset ip4

for ip in "${IP_ADDRESSES[@]}"; do
  js_true -e <<< "isResolvableEx('${ip}')"
done
unset ip

for ip4 in "${IP_V4_ADDRESSES[@]}"; do
  ok <<EOT
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

#=== Results ===#

declare_test_results_and_exit
