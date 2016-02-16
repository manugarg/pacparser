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

typedef char *(*pacparser_resolve_host_func)(const char *, int all_ips);

char *get_my_ip_address(pacparser_resolve_host_func resolve_host_func,
                        int all_ips);

char *resolve_host_getaddrinfo(const char *hostname, int all_ips);
char *resolve_host_literal_ips_only(const char *hostname, int all_ips);
char *resolve_host_c_ares(const char *hostname, int all_ips);
