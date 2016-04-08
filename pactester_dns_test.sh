#!/bin/bash
# Test the 'pactester' utility (and with it, indirectly, the 'pacparser'
# C library).

set -u -e

. ./pactester_test_lib.sh || exit 1

#=== Option parsing ===#

has_ipv6_support=true
has_c_ares=false
while (($# > 0)); do
  case ${1} in
    --ipv6) has_ipv6_support=true;;
    --c-ares) has_c_ares=true;;
    *) die "invalid argument '${1}";;
  esac
  shift
done
readonly has_ipv6_support has_c_ares

if ${has_c_ares}; then
  # We want to use a fixed, well-known DNS resolver, the only
  # way to have reasonably predictable results.
  PACPARSER_COMMON_ARGS+=(-r "c-ares" -s "8.8.8.8")
fi

set -- # clear command line args

#=== Tests ===#

ok -E <<'EOT'
  var r = dnsResolve('localhost');
  if (r == '127.0.0.1')
    return 'OK';
  return 'KO -> ' + r;
EOT

# We cannot require "::1" to be there, as this test might run on
# machines where localhost do not resolve only to 127.0.0.1 -- and
# that even on IPv6-enabled machines.
ok -e <<EOT
  var r = dnsResolveEx('localhost');
  if (/^127\.0\.0\.1(;::1)?$/.test(r))
    return 'OK';
  return 'KO -> ' + r;
EOT

ok -E <<< 'return isResolvable("www.google.com") ? "OK" : "KO";'
ok -e <<< 'return isResolvableEx("mail.google.com") ? "OK" : "KO";'

declare -a addresses=(
  '127.0.0.1'
  '::1'
  '8.8.8.8'
  '74.125.138.129'
  '2a00:1450:400b:c01::81'
)

for addr in "${addresses[@]}"; do

  ok -E <<EOT
    var r = dnsResolve('${addr}');
    if (r == '${addr}')
      return 'OK';
    return 'KO -> ' + r;
EOT

  ok -e <<EOT
    var r = dnsResolveEx('${addr}');
    if (r == '${addr}')
      return 'OK';
    return 'KO -> ' + r;
EOT

done
unset addr addresses

for host in invalid i--dont--exist--really.google.com; do

  ok -E <<EOT
    var r = dnsResolve('${host}');
    if (r == null)
      return 'OK';
    return 'KO -> ' + r;
EOT

  ok -e <<EOT
    var r = dnsResolveEx('${host}');
    if (r == null)
      return 'OK';
    return 'KO -> ' + r;
EOT

done
unset h

ok -E <<EOT
  var r = dnsResolve('uberproxy4.l.google.com');
  if (/^${up_ip4_rx}$/.test(r))
    return 'OK';
  return 'KO -> ' + r;
EOT

ok -e <<EOT
  var r = dnsResolveEx('uberproxy4.l.google.com');
  if (/^${up_ip4_rx}$/.test(r))
    return 'OK';
  return 'KO -> ' + r;
EOT

ok -E <<EOT
  var r = dnsResolve('uberproxy6.l.google.com');
  if (/^${up_ip6_rx}$/.test(r))
    return 'OK';
  return 'KO -> ' + r;
EOT

ok -e <<EOT
  var r = dnsResolveEx('uberproxy6.l.google.com');
  if (/^${up_ip6_rx}$/.test(r))
    return 'OK';
  return 'KO -> ' + r;
EOT

ok -E <<EOT
  var r = dnsResolve('uberproxy.l.google.com');
  if (/^${up_ip4_rx}$/.test(r))
    return 'OK';
  return 'KO -> ' + r;
EOT

ok -e <<EOT
  var r = dnsResolveEx('uberproxy.l.google.com');
  if (/^${up_ip4_rx};${up_ip6_rx}$/.test(r))
    return 'OK';
  return 'KO -> ' + r;
EOT

# dnsResolveEx() on www.google.com should return several IPv4
# addresses (3 or more) and at least one IPv6 address.

ip_repeated_rx="${ip4_rx}(;${ip4_rx})*"
if ${has_ipv6_support}; then
  ip_repeated_rx+=";${ip6_rx}(;${ip6_rx})*"
fi

ok -e <<EOT
  var r = dnsResolveEx('www.google.com');
  if (/^${ip_repeated_rx}$/.test(r))
    return 'OK';
  return 'KO -> ' + r;
EOT

unset ip_repeated_rx

# But dnsResolve() should just return one IPv4 address. Both with
# Microsoft extensions enabled and with Microsoft extensions disabled.
for opt in '-E' '-e'; do

  ok ${opt} <<EOT
    var r = dnsResolve('www.google.com');
    if (/^${ip4_rx}$/.test(r))
      return 'OK';
    return 'KO -> ' + r;
EOT

done
unset opt

# IPv4-only hostnames.

ok -E <<EOT
  var r = dnsResolve('ipv4.l.google.com');
  if (/^${ip4_rx}$/.test(r))
    return 'OK';
  return 'KO -> ' + r;
EOT

ok -e <<EOT
  var r = dnsResolveEx('ipv4.l.google.com');
  if (/^(${ip4_rx};)*${ip4_rx}$/.test(r))
    return 'OK';
  return 'KO -> ' + r;
EOT

# IPv6-only hostnames.

if ${has_ipv6_support}; then

  ok -E <<EOT
    var r = dnsResolve('ipv6.l.google.com');
    if (/^${ip6_rx}$/.test(r))
      return 'OK';
    return 'KO -> ' + r;
EOT

  ok -e <<EOT
    var r = dnsResolveEx('ipv6.l.google.com');
    if (/^(${ip6_rx};)*${ip6_rx}$/.test(r))
      return 'OK';
    return 'KO -> ' + r;
EOT

fi # ${has_ipv6_support}

for h in facebook.com en.wikipedia.org; do

  ok -E <<EOT
    var r = dnsResolve('${h}');
    if (/^${ip4_rx}$/.test(r))
      return 'OK';
    return 'KO -> ' + r;
EOT

  ok -e <<EOT
    var r = dnsResolveEx('${h}');
    if (/^(${ip4_rx};)+${ip6_rx}(;${ip6_rx})*$/.test(r))
      return 'OK';
    return 'KO -> ' + r;
EOT

done
unset h

# Test the addition of DNS domains to append during DNS lookups.

if ${has_c_ares}; then

  ok -E -d l.google.com <<EOT
    return isResolvable('ipv6') ? 'OK' : 'KO';
EOT

  ok -E -d "" <<EOT
    r = dnsResolve('teams');
    if (r == null)
      return 'OK'
    else
      return 'KO -> ' + r;
EOT

  ok -d microsoft.com,corp.google.com <<EOT
    return isResolvable('critique') ? 'OK' : 'OK';
EOT

  ok -d microsoft.com,google.com <<EOT
    r = dnsResolve('critique');
    if (r == null)
      return 'OK'
    else
      return 'KO -> ' + r;
EOT

  ok -E -d wikipedia.org <<EOT
    return isResolvable('en') ? 'OK' : 'KO';
EOT

  ok -d googleplex.com <<EOT
    var r = dnsResolve('teams');
    if (/^$up_ip4_rx$/.test(r))
      return 'OK';
    return 'KO -> ' + r;
EOT

  ok -d corp.google.com,google.com <<EOT
    var r = dnsResolve('www');
    if (/^$up_ip4_rx$/.test(r))
      return 'OK';
    return 'KO -> ' + r;
EOT

fi # ${has_c_ares}

#=== Results ===#

declare_test_results_and_exit
