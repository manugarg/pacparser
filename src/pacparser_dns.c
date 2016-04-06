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
#include "pacparser_utils.h"

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

struct dns_collector {
  char *mallocd_addresses;
  int all_ips;
  int ai_family;
};

enum collect_status {
  COLLECT_DONE = 0,
  COLLECT_MORE = 1
};

//------------------------------------------------------------------------------

static char *
concat_strings(char *mallocd_str, const char *appended_str)
{
  if (appended_str == NULL)
    return mallocd_str;
  if (mallocd_str == NULL)
    return strdup(appended_str);

  char *mallocd_result;
  int reallocd_size = strlen(mallocd_str) + strlen(appended_str) + 1;
  if ((mallocd_result = realloc(mallocd_str, reallocd_size)) == NULL)
    return NULL;
  return strcat(mallocd_result, appended_str);
}

static enum collect_status
collect_mallocd_address(struct dns_collector *dc, const char *addr_buf)
{
  if (dc->all_ips) {
    if (dc->mallocd_addresses == NULL) {
      dc->mallocd_addresses = strdup(addr_buf);
    } else {
      dc->mallocd_addresses = concat_strings(dc->mallocd_addresses, ";");
      dc->mallocd_addresses = concat_strings(dc->mallocd_addresses, addr_buf);
    }
    return COLLECT_MORE;  // it's ok to run again and get more results
  } else {
    dc->mallocd_addresses = strdup(addr_buf);
    return COLLECT_DONE;  // we only need and want one result
  }
}

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
  (void) all_ips;  // shut up linter
  return is_ip_address(hostname) ? strdup(hostname) : NULL;
}

//------------------------------------------------------------------------------

// DNS resolutions via the getaddrinfo(3) function.

static void
collect_getaddrinfo_results(struct dns_collector *dc, struct addrinfo *ai)
{
  for (; ai != NULL; ai = ai->ai_next) {
    char addr_buf[INET6_ADDRSTRLEN];  // large enough for IPv4 and IPv6 alike
    getnameinfo(ai->ai_addr, ai->ai_addrlen, addr_buf, sizeof(addr_buf),
                NULL, 0, NI_NUMERICHOST);
    if (collect_mallocd_address(dc, addr_buf) == COLLECT_DONE)
      return;
  }
}

char *
pacparser_resolve_host_getaddrinfo(const char *hostname, int all_ips)
{
  struct dns_collector dc;
  dc.all_ips = all_ips;
  dc.mallocd_addresses = NULL;

  struct addrinfo hints, *ai;
  memset(&hints, 0, sizeof(struct addrinfo));
  hints.ai_socktype = SOCK_STREAM;

#ifdef _WIN32
  // On windows, we need to initialize the winsock dll first.
  WSADATA WsaData;
  WSAStartup(MAKEWORD(2,0), &WsaData);
#endif

  int i, ai_families[] = {AF_INET, AF_INET6};
  for (i = 0; i < 2; i++) {
    dc.ai_family = hints.ai_family = ai_families[i];
    if (getaddrinfo(hostname, NULL, &hints, &ai) == 0)
      collect_getaddrinfo_results(&dc, ai);
    if (!all_ips && dc.mallocd_addresses)
      break;
  }

#ifdef _WIN32
  WSACleanup();
#endif
  return dc.mallocd_addresses;
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

static char *dns_servers = NULL;
static char *dns_domains = NULL;

static ares_channel global_channel;

// Use a custom DNS server (specified by IP), instead of relying on the
// "nameserver" directive in /etc/resolv.conf.
int
pacparser_set_dns_servers(const char *ips)
{
  if (ips == NULL)
    return 1;  // noop
  if (ares_initialized) {
    print_err(
        "pacparser_dns.c: pacparser_set_dns_servers: "
        "Cannot change DNS servers now; this function should be called "
        "before c-ares is initialized (typically by pacparser_init).\n");
    return 0;
  }
  if ((dns_servers = strdup(ips)) == NULL) {
    print_err(
        "pacparser_dns.c: pacparser_set_dns_servers: "
        "Could not allocate memory for the servers");
    return 0;
  }
  return 1;
}

// Use a custom list of domains, instead of relying on, e.g., the
// "search" directive in /etc/resolv.conf.
int
pacparser_set_dns_domains(const char *domains)
{
  if (domains == NULL)
    return 1;  // noop
  if (ares_initialized) {
    print_err(
        "pacparser_dns.c: pacparser_set_dns_domains: "
        "Cannot change DNS search domains now. This function should be called"
        "before c-ares is initialized (typically by pacparser_init).");
    return 0;
  }
  if ((dns_domains = strdup(domains)) == NULL) {
    print_err(
        "pacparser_dns.c: pacparser_set_dns_domains: "
        "Could not allocate memory for the domains");
    return 0;
  }
  return 1;
}

static void
callback_for_ares(void *arg, int status, int timeouts, struct hostent *host)
{
  (void) timeouts;  // unused

  // Sigh. But the callback must have a void* as first argument, so we
  // actually need this little abomination. Sorry.
  struct dns_collector *dc = (struct dns_collector *) arg;

  if (status != ARES_SUCCESS || host->h_addrtype != dc->ai_family)
    return;

  char **s;
  for (s = host->h_addr_list; *s; s++) {
    char addr_buf[INET6_ADDRSTRLEN];  // large enough for IPv4 and IPv6 alike
    ares_inet_ntop(dc->ai_family, *s, addr_buf, sizeof(addr_buf));
    if (collect_mallocd_address(dc, addr_buf) == COLLECT_DONE)
      return;
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
  char **domains_list = NULL;
  int optmask = ARES_OPT_FLAGS;
  struct ares_options options;
  options.flags = ARES_FLAG_NOCHECKRESP;

#define FREE_AND_RETURN(retval) \
  do { \
    free(domains_list); \
    return (retval); \
  } while(0)

  if (ares_library_init(ARES_LIB_INIT_ALL) != ARES_SUCCESS) {
    print_err("Could not initialize the c-ares library");
    FREE_AND_RETURN(0);
  }

  if (dns_domains) {
    int i = 0;
    char *p, *sp;
    p = strtok_r(dns_domains, ",", &sp);
    while (p != NULL) {
      domains_list = realloc(domains_list, (i + 2) * sizeof(char **));
      if (domains_list == NULL) {
        print_err("Could not allocate memory for domains list");
        FREE_AND_RETURN(0);
      }
      domains_list[i++] = p;
      p = strtok_r(NULL, ",", &sp);
    }
    if (domains_list)
      domains_list[i] = NULL;
    optmask |= ARES_OPT_DOMAINS;
    options.domains = domains_list;
    options.ndomains = i;
  }

  if (ares_init_options(&global_channel, &options, optmask) != ARES_SUCCESS) {
    print_err("Could not initialize c-ares options");
    FREE_AND_RETURN(0);
  }

  if (dns_servers) {
    if (ares_set_servers_csv(global_channel, dns_servers) != ARES_SUCCESS) {
      print_err("Could not set c-ares DNS servers");
      FREE_AND_RETURN(0);
    }
  }

  ares_initialized = 1;
  FREE_AND_RETURN(1);

#undef FREE_AND_RETURN
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
  free(dns_domains);
  free(dns_servers);
}

char *
pacparser_resolve_host_ares(const char *hostname, int all_ips)
{
  if (hostname == NULL) {
    return strdup("");
  }

  struct dns_collector dc;
  dc.all_ips = all_ips;
  dc.mallocd_addresses = NULL;

  int i, ai_families[] = {AF_INET, AF_INET6};
  for (i = 0; i < 2; i++) {
    dc.ai_family = ai_families[i];
    ares_gethostbyname(global_channel, hostname, ai_families[i],
                       callback_for_ares, (void *) &dc);
    if (!ares_wait_for_all_queries(global_channel)) {
      print_err("Some c-ares queries did not complete successfully");
      return NULL;
    }
    if (!all_ips && dc.mallocd_addresses)
      return dc.mallocd_addresses;
  }
  return dc.mallocd_addresses;
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
  if (ips == NULL)
    return 1;  // noop
  pacparser_no_c_ares("pacparser_set_dns_servers");
  return 0;
}

int
pacparser_set_dns_domains(const char *domains)
{
  if (domains == NULL)
    return 1;  // noop
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
