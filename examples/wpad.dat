// Go direct for plain hostnames and any host in .manugarg.com domain except
// for www and www.manugarg.com.
// Go via proxy for all other hosts.

function FindProxyForURL(url, host)
 {
 if ((isPlainHostName(host) ||
      dnsDomainIs(host, ".manugarg.com")) &&
      !localHostOrDomainIs(host, "www.manugarg.com"))
   return "DIRECT";
 else
   return "PROXY proxy1.manugarg.com:3128; PROXY proxy2.manugarg.com:3128; DIRECT";
 }
