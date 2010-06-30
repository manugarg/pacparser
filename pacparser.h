// Copyright (C) 2007 Manu Garg.
// Author: Manu Garg <manugarg@gmail.com>
//
// This file defines API for pacparser library.
//
// pacparser is a library that provides methods to parse proxy auto-config
// (PAC) files. Please read README file included with this package for more
// information about this library.
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

// Initializes pac parser
// It initializes JavaScript engine and does few basic initializations specific
// to pacparser.
// returns 0 on failure and 1 on success.
int pacparser_init();

// Parse pac file
// parses and evaulates PAC file in JavaScript context created by
// pacparser_init.
// returns 0 on failure and 1 on success.
int pacparser_parse_pac(const char *pacfile           // PAC file to parse
                        );

// Finds proxy for a given URL and Host.
// This function should be called only after pacparser engine has been
// initialized (using pacparser_init) and pac file has been parsed (using
// pacparser_parse_pac). It determines "right" proxy (based on pac file) for
// url and host.
// returns proxy string on sucess and NULL on error.
char *pacparser_find_proxy(const char *url,           // URL to find proxy for
                           const char *host           // Host part of the URL
                           );

// Finds proxy for a given PAC file, url and host.
// This function is a wrapper around functions pacparser_init,
// pacparser_parse_pac, pacparser_find_proxy and pacparser_cleanup. If you just
// want to find out proxy a given set of pac file, url and host, this is the
// function to call. This function takes care of all the initialization and
// cleanup.
// returns proxy string on success and NULL on error.
char *pacparser_just_find_proxy(const char *pacfile,  // PAC file
                           const char *url,           // URL to find proxy for
                           const char *host           // Host part of the URL
                           );

// Destroys JavaSctipt context.
// This function should be called once you're done with using pacparser engine.
void pacparser_cleanup();

// Sets my IP address.
// Sets my IP address to a custom value. This is the IP address returned by
// myIpAddress() javascript function.
void pacparser_setmyip(const char *ip                 // Custom IP address.
                       );

// Enable Microsoft PAC extensions.
// Enables a subset of Microsoft PAC extensions - dnsResolveEx, myIpAddressEx,
// isResolvableEx. These functions are used by Google Chrome and IE to work
// with IPv6. More info: http://code.google.com/p/pacparser/issues/detail?id=4
void pacparser_enable_microsoft_extensions(
    int enable_extensions       // Whether to enable extensions (0: No, 1: Yes)
    );
