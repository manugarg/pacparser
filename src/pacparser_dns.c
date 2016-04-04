// Copyright (C) 2007 Manu Garg.
// Authors: Stefano Lattarini <stefano.lattarini@gmail.com>,
//          Manu Garg <manugarg@gmail.com>
//
// DNS-related function to be used in the pacparser library.
// Used by other routines which implement the PAC builtins dnsResolve(),
// dnsResolveEx(), myIpAddress(), myIpAddressEx().
//
// Pacparser is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published
// by the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// Pacparser is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with Pacparser.  If not, see <http://www.gnu.org/licenses/>.

#include "pacparser_dns.h"
#include "util.h"

#ifdef XP_UNIX
#  include <arpa/inet.h>  // for inet_pton
#  include <sys/socket.h>  // for AF_INET
#  include <netdb.h>
#endif

#ifdef _WIN32
#ifdef __MINGW32__
// MinGW enables definition of getaddrinfo et al only if WINVER >= 0x0501.
#define WINVER 0x0501
#endif
#include <winsock2.h>
#include <ws2tcpip.h>
#endif

//------------------------------------------------------------------------------

char *
pacparser_get_my_ip_address(pacparser_resolve_host_func resolve_host_func,
                        int all_ips)
{
  char *ipaddr;
  // According to the gethostname(2) manpage, SUSv2  guarantees that
  // "Host names are limited to 255 bytes".
  char name[256];
  if (gethostname(name, sizeof(name)) < 0 ||
      (ipaddr = resolve_host_func(name, all_ips)) == NULL) {
    ipaddr = strdup("127.0.0.1");
  }
  return ipaddr;
}

//------------------------------------------------------------------------------

// DNS "resolution" of literal IPs only.

static int
is_ip_address(const char *str)
{
  char ipaddr4[INET_ADDRSTRLEN], ipaddr6[INET6_ADDRSTRLEN];
  return (inet_pton(AF_INET, str, &ipaddr4) > 0 ||
          inet_pton(AF_INET6, str, &ipaddr6) > 0);
}

char *
pacparser_resolve_host_literal_ips(const char *hostname, int all_ips)
{
  (void)all_ips;  // shut up linter
  return is_ip_address(hostname) ? strdup(hostname) : NULL;
}

//------------------------------------------------------------------------------

// DNS resolutions via the getaddrinfo(3) function.

static int
addrinfo_len(const struct addrinfo *ai)
{
  int i = 0;
  while (ai != NULL) {
    i++;
    ai = ai->ai_next;
  }
  return i;
}

char *
pacparser_resolve_host_getaddrinfo(const char *hostname, int all_ips)
{
  struct addrinfo *ai[2] = {NULL, NULL};

#ifdef _WIN32
  // On windows, we need to initialize the winsock dll first.
  WSADATA WsaData;
  WSAStartup(MAKEWORD(2,0), &WsaData);
#endif

  struct addrinfo hints;
  memset(&hints, 0, sizeof(struct addrinfo));
  hints.ai_socktype = SOCK_STREAM;

  int ips_count = 0;

  // First, IPv4 addresses (if any).
  hints.ai_family = AF_INET;
  if (getaddrinfo(hostname, NULL, &hints, &ai[0]) == 0) {
    ips_count += addrinfo_len(ai[0]);
  }

  // Then, IPv6 addresses (if any, and only if needed).
  if (all_ips || !ips_count) {
    hints.ai_family = AF_INET6;
    if (getaddrinfo(hostname, NULL, &hints, &ai[1]) == 0) {
      ips_count += addrinfo_len(ai[1]);
    }
  }

  if (!all_ips && ips_count)
    ips_count = 1;

  // Add one for terminating NULL.
  const char **ips = calloc(ips_count + 1, sizeof(char **));
  int i = 0, k;

  // First format the IPv4 addrinfos (if any), then the IPv6 ones (if any).
  for (k = 0; k < 2; k++) {
    for (; ai[k] != NULL; ai[k] = ai[k]->ai_next) {
      // This is large enough to contain either an IPv4 and IPv6 address.
      char ipaddr[INET6_ADDRSTRLEN];
      getnameinfo(ai[k]->ai_addr, ai[k]->ai_addrlen, ipaddr, sizeof(ipaddr),
                  NULL, 0, NI_NUMERICHOST);
      ips[i++] = strdup(ipaddr);
      if (i >= ips_count)
        goto resolve_host_done;
    }
  }

resolve_host_done:
  ips[i] = NULL;
#ifdef _WIN32
  WSACleanup();
#endif
  // On failed resolution, we want to return null, not the empty string.
  char *retval = *ips ? join_string_list(ips, ";") : NULL;
  // No memory leaks: free the ip addresses and the pointers to them.
  deep_free_string_list(ips);
  // And we are done.
  return retval;
}

//------------------------------------------------------------------------------

// DNS resolutions via the feature-rich c-ares third party library.

// TODO(slattarini): refactor stuff to use print_error() here (will have to be
// ripped out from pacparser.c)

#define print_err(...) \
    do { \
      fprintf(stderr, "%s: %s: ", __FILE__, __FUNCTION__); \
      fprintf(stderr, __VA_ARGS__); \
      fprintf(stderr, "\n"); \
    } while (0)

#define pacparser_no_c_ares(funcname) \
  fprintf(stderr, "function %s requires c-ares integration to be enabled " \
          "at compile time", funcname)

static int ares_initialized = 0;

#ifdef HAVE_C_ARES

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <arpa/nameser.h>
#include <arpa/nameser_compat.h>

#include <errno.h>

#include <ares.h>
#include <ares_dns.h>

static const char *dns_servers = NULL;
static const char **dns_domains = NULL;
static int dns_domains_count = 0;

static ares_channel global_channel;

struct callback_arg {
  char *mallocd_addresses;
  int ai_family;
  int all_ips;
};

// Use a custom DNS server (specified by IP), instead of relying on the
// "nameserver" directive in /etc/resolv.conf.
int
pacparser_set_dns_servers(const char *ips)
{
  if (ares_initialized) {
    print_err(
        "pacparser_dns.c: pacparser_set_dns_servers: "
        "Cannot change DNS servers now; this function should be called "
        "before c-ares is initialized (typically by pacparser_init).\n");
    return 0;
  }
  dns_servers = (const char *) strdup(ips);
  return 1;
}

// Use a custom list of domains, instead of relying on, e.g., the
// "search" directive in /etc/resolv.conf.
int
pacparser_set_dns_domains(const char **domains)
{
  if (ares_initialized) {
    print_err(
        "pacparser_dns.c: pacparser_set_dns_domains: "
        "Cannot change DNS search domains now. This function should be called"
        "before c-ares is initialized (typically by pacparser_init).\n");
    return 0;
  }
  dns_domains = (const char **) measure_and_dup_string_list(domains,
                                                            &dns_domains_count);
  return 1;
}

static void
callback(void *arg, int status, int timeouts, struct hostent *host)
{
  (void) timeouts;  // unused

  // Sigh. But the callback must have a void* as first argument, so we
  // actually need this little abomination. Sorry.
  struct callback_arg *p = (struct callback_arg *) arg;

  if (status != ARES_SUCCESS || host->h_addrtype != p->ai_family)
    return;

  char **s;
  for (s = host->h_addr_list; *s; s++) {
    char addr_buf[INET6_ADDRSTRLEN];  // large enough for IPv4 and IPv6 alike
    ares_inet_ntop(p->ai_family, *s, addr_buf, sizeof(addr_buf));
    if (p->all_ips) {
      if (p->mallocd_addresses) {
        p->mallocd_addresses = concat_strings(p->mallocd_addresses, ";");
      }
      p->mallocd_addresses = concat_strings(p->mallocd_addresses, addr_buf);
    } else {
      p->mallocd_addresses = strdup(addr_buf);
      return;  // we only need and want one result
    }
  }
}

// Shamelessly copied from ares_process(3)
static int
ares_wait_for_all_queries(ares_channel channel)
{
   int nfds, count;
   fd_set readers, writers;
   struct timeval tv, *tvp;

   while (1) {
     FD_ZERO(&readers);
     FD_ZERO(&writers);
     nfds = ares_fds(channel, &readers, &writers);
     if (nfds == 0)
       break;
     tvp = ares_timeout(channel, NULL, &tv);
     count = select(nfds, &readers, &writers, NULL, tvp);
     if (count < 0 && errno != EINVAL) {
       perror("select"); // TODO(slattarini): proper print_error here
       return 0;
     }
     ares_process(channel, &readers, &writers);
   }
   return 1;
}

int
pacparser_ares_init(void)
{
  int optmask = ARES_OPT_FLAGS;
  struct ares_options options;
  options.flags = ARES_FLAG_NOCHECKRESP;

  if (ares_library_init(ARES_LIB_INIT_ALL) != ARES_SUCCESS) {
    print_err("Could not initialize the c-ares library");
    return 0;
  }

  if (dns_domains) {
    optmask |= ARES_OPT_DOMAINS;
    options.domains = dns_domains;
    options.ndomains = dns_domains_count;
  }

  if (ares_init_options(&global_channel, &options, optmask) != ARES_SUCCESS) {
    print_err("Could not initialize c-ares options");
    return 0;
  }

  if (dns_servers) {
    if (ares_set_servers_csv(global_channel, dns_servers) != ARES_SUCCESS) {
      print_err("Could not set c-ares DNS servers");
      return 0;
    }
  }

  ares_initialized = 1;
  return 1;
}

void
pacparser_ares_cleanup(void)
{
  if (ares_initialized) {
    // These functions return void, so no error checking is possible here.
    ares_destroy(global_channel);
    ares_library_cleanup();
  }
  ares_initialized = 0;
}

char *
pacparser_resolve_host_ares(const char *hostname, int all_ips)
{
  if (!hostname) {
    return strdup("");
  }

  struct callback_arg cba;
  cba.all_ips = all_ips;
  cba.mallocd_addresses = NULL;

  int i, ai_families[] = {AF_INET, AF_INET6};
  for (i = 0; i < 2; i++) {
    cba.ai_family = ai_families[i];
    ares_gethostbyname(global_channel, hostname, ai_families[i], callback,
                       (void *) &cba);
    if (!ares_wait_for_all_queries(global_channel)) {
      print_err("Some c-ares queries did not complete successfully");
      return NULL;
    }
    if (!all_ips && cba.mallocd_addresses)
      return cba.mallocd_addresses;
  }
  return cba.mallocd_addresses;
}

# else // !HAVE_C_ARES

// Dummy fallbacks for when c-ares is not available.
//
// Some of these functions shoud never be called when c-ares integration is
// disabled (and will complain if they are), some should just work as no-op
// (to make integration into the callers easier).

char *
resolve_host_ares(const char *hostname, int all_ips)
{
  pacparser_no_c_ares("resolve_host_ares");
  return NULL;
}

int
pacparser_set_dns_servers(const char *ips)
{
  pacparser_no_c_ares("pacparser_set_dns_servers");
  return 0;
}

int
pacparser_set_dns_domains(const char **domains)
{
  pacparser_no_c_ares("pacparser_set_dns_domains");
  return 0;
}

int
pacparser_ares_init(void)
{
  ares_initialized = 1;
  return 1;
}

void
pacparser_ares_cleanup(void)
{
  ares_initialized = 0;
}

#endif // !HAVE_C_ARES

//------------------------------------------------------------------------------
