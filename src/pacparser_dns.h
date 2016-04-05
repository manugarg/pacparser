// Copyright (C) 2007 Manu Garg.
// Authors: Stefano Lattarini <slattarini@gmail.com>,
//          Manu Garg <manugarg@gmail.com>
//
// This file defines public and private APIs for DNS-related function to be
// used in the pacparser library.
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

#ifndef PACPARSER_DNS_H_
#define PACPARSER_DNS_H_

#ifdef __cplusplus
extern "C" {
#endif

//------------------------------------------------------------------------------

/// @defgroup pacparser_dns pacparser_dns
/// @{
/// @brief API for DNS base functionalities of the pacparser library, a library
///        to use proxy auto-config (PAC) files.
///        See project homepage: http://github.com/pacparser/pacparser
///        for more information.
/// @author Manu Garg <manugarg@gmail.com>
/// @author Stefano Lattarini <stefano.lattarini@gmail.com>

/// @brief Use a custom DNS server (specified by IP)
/// @param ips The comma-separated list of IPs of the DNS servers.
/// @returns 0 on failure and 1 on success.
///
/// Use custom DNS servers, instead of relying on the "nameserver" directive
/// in /etc/resolv.conf.
/// It will always succeed if c-ares integration was active at compile time,
/// and always fail otherwise.
int pacparser_set_dns_servers(const char *ips);

/// @brief Use a custom list of domains.
/// @param domains A NULL-terminated list of strings, one for each domain.
/// @returns 0 on failure and 1 on success.
///
/// Use a custom list of domains, instead of relying on, e.g., the
/// "search" directive in /etc/resolv.conf.
/// It will always succeed if c-ares integration was active at compile time,
/// and always fail otherwise.
int pacparser_set_dns_domains(const char **domains);

/// @}

//------------------------------------------------------------------------------

// Function not meant for external use. We reserve the right to change
// or remove these at any time.

typedef char *(*pacparser_resolve_host_func)(const char *, int all_ips);

char *pacparser_get_my_ip_address(pacparser_resolve_host_func resolve_host_func,
                                  int all_ips);

char *pacparser_resolve_host_literal_ips(const char *hostname, int all_ips);
char *pacparser_resolve_host_getaddrinfo(const char *hostname, int all_ips);
char *pacparser_resolve_host_ares(const char *hostname, int all_ips);

int pacparser_ares_init(void);
void pacparser_ares_cleanup(void);

//------------------------------------------------------------------------------

#ifdef __cplusplus
}
#endif

#endif // PACPARSER_DNS_H_