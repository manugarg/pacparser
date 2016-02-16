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
#include <arpa/inet.h>  // for inet_pton
#include <sys/socket.h>  // for AF_INET
#include <netdb.h>
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
get_my_ip_address(pacparser_resolve_host_func resolve_host_func, int all_ips)
{
  char *ipaddr;
  // According to the gethostname(2) manpage, SUSv2  guarantees that
  // "Host names are limited to 255 bytes".
  char name[256];
  gethostname(name, sizeof(name));
  if ((ipaddr = resolve_host_func(name, all_ips)) == NULL) {
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
resolve_host_literal_ips_only(const char *hostname, int all_ips)
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
resolve_host_getaddrinfo(const char *hostname, int all_ips)
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

#ifdef HAVE_C_ARES
#  error "TODO(slattarini): c-ares support TBD"
# else

// TODO(slattarini): refactor stuff to use print_error() here (will have to be
// ripped out from pacparser.c)
#define pacparser_no_c_ares(funcname) \
  fprintf(stderr, "function %s requires c-ares integration to be enabled " \
          "at compile time", funcname) \

// These functions shoud never be called when c-ares integration is disabled.

char *
resolve_host_c_ares(const char *hostname, int all_ips)
{
  pacparser_no_c_ares("resolve_host_c_ares");
  return NULL;
}

int
pacparser_set_dns_server(const char *ip)
{
  pacparser_no_c_ares("pacparser_set_dns_server");
  return 0;
}

int
pacparser_set_dns_domains(const char **domains)
{
  pacparser_no_c_ares("pacparser_set_dns_domains");
  return 0;
}

#endif // !HAVE_C_ARES

//------------------------------------------------------------------------------
