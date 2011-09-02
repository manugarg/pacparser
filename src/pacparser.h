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


#ifdef __cplusplus
extern "C" {
#endif
// Returns pacparser version
// Version string is determined at the time of build. If built from a released
// package, version corresponds to the latest release (hg) tag. If built from the
// repository, it corresponds to the head revision of the repo.
// returns version string if version defined, "" otherwise.
char* pacparser_version(void);

// Initializes pac parser
// It initializes JavaScript engine and does few basic initializations specific
// to pacparser.
// returns 0 on failure and 1 on success.
int pacparser_init(void);

// Parses the given PAC file.
// Reads the given PAC file and evaluates it in the JavaScript context created
// by pacparser_init.
// returns 0 on failure and 1 on success.
int pacparser_parse_pac_file(const char *pacfile       // PAC file to parse
                             );

// Parses the given PAC script string.
// Evaulates the given PAC script string in the JavaScript context created
// by pacparser_init.
// returns 0 on failure and 1 on success.
int pacparser_parse_pac_string(const char *string      // PAC string to parse
                               );

// Parses pac file (deprecated, use pacparser_parse_pac_file instead)
// Same as pacparser_parse_pac_file. Included only for backward compatibility.
// returns 0 on failure and 1 on success.
int pacparser_parse_pac(const char *file               // PAC file to parse
                        );

// Finds proxy for the given URL and Host.
// Finds proxy for the given URL and Host. This function should be called only
// after pacparser engine has been initialized (using pacparser_init) and pac
// script has been parsed (using pacparser_parse_pac_file or
// pacparser_parse_pac_string).
// returns proxy string on sucess and NULL on error.
char *pacparser_find_proxy(const char *url,           // URL to find proxy for
                           const char *host           // Host part of the URL
                           );

// Finds proxy for the given PAC file, URL and Host.
// This function is a wrapper around functions pacparser_init,
// pacparser_parse_pac_file, pacparser_find_proxy and pacparser_cleanup. If
// you just want to find out proxy a given set of pac file, url and host, this
// is the function to call. This function takes care of all the initialization
// and cleanup.
// returns proxy string on success and NULL on error.
char *pacparser_just_find_proxy(const char *pacfile,  // PAC file
                           const char *url,           // URL to find proxy for
                           const char *host           // Host part of the URL
                           );

// Destroys JavaSctipt context.
// This function should be called once you're done with using pacparser engine.
void pacparser_cleanup(void);

// Sets my IP address.
// Sets my IP address to a custom value. This is the IP address returned by
// myIpAddress() javascript function.
void pacparser_setmyip(const char *ip                 // Custom IP address.
                       );

// Enable Microsoft PAC extensions.
// Enables a subset of Microsoft PAC extensions - dnsResolveEx, myIpAddressEx,
// isResolvableEx. These functions are used by Google Chrome and IE to work
// with IPv6. More info: http://code.google.com/p/pacparser/issues/detail?id=4
void pacparser_enable_microsoft_extensions(void);

#ifdef __cplusplus
}
#endif
