// Copyright (C) 2007 Manu Garg.
// Authors: Stefano Lattarini <slattarini@gmail.com>,
//          Manu Garg <manugarg@gmail.com>
//
// This file defines private API for DNS-related function to be used in
// the pacparser library.
//
// pacparser is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 3 of the License, or (at your option) any later version.

// pacparser is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Lesser General Public License for more details.

// You should have received a copy of the GNU Lesser General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA

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
/// @param ip The IP of the DNS server as a string.
/// @returns 0 on failure and 1 on success.
///
/// Use a custom DNS server, instead of relying on the "nameserver" directive
/// in /etc/resolv.conf.
/// It will always succeed if c-ares integration was active at compile time,
/// and always fail otherwise.
int pacparser_set_dns_server(const char *ip);

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

char *get_my_ip_address(pacparser_resolve_host_func resolve_host_func,
                        int all_ips);

char *resolve_host_getaddrinfo(const char *hostname, int all_ips);
char *resolve_host_literal_ips_only(const char *hostname, int all_ips);
char *resolve_host_c_ares(const char *hostname, int all_ips);

//------------------------------------------------------------------------------

#ifdef __cplusplus
}
#endif
