// Copyright (C) 2007 Manu Garg.
// Author: Manu Garg <manugarg@gmail.com>
//
// This file defines API for pacparser library.
//
// pacparser is a library that provides methods to parse proxy auto-config
// (PAC) files. Please read README file included with this package for more
// information about this library.
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

#ifndef PACPARSER_H_
#define PACPARSER_H_

#include <stdarg.h>

#ifdef __cplusplus
extern "C" {
#endif

/// @defgroup pacparser pacparser
/// @{
/// @brief API for pacparser library, a library to use proxy auto-config (PAC)
///        files. See project homepage: http://github.com/pacparser/pacparser
///        for more information.
/// @author Manu Garg <manugarg@gmail.com>

/// @brief Initializes pac parser.
/// @returns 0 on failure and 1 on success.
///
/// Initializes JavaScript engine and does few basic initializations specific
/// to pacparser.
int pacparser_init(void);

/// @brief Parses the given PAC file.
/// @param pacfile PAC file to parse.
/// @returns 0 on failure and 1 on success.
///
/// Reads the given PAC file and evaluates it in the JavaScript context created
/// by pacparser_init.
int pacparser_parse_pac_file(const char *pacfile);

/// @brief Parses the given PAC script string.
/// @param pacstring PAC string to parse.
/// @returns 0 on failure and 1 on success.
///
/// Evaulates the given PAC script string in the JavaScript context created
/// by pacparser_init.
int pacparser_parse_pac_string(const char *pacstring);

/// @brief Parses the gievn pac file.
/// \deprecated Use pacparser_parse_pac_file instead.
/// @param pacfile PAC file to parse.
/// @returns 0 on failure and 1 on success.
///
/// Same as pacparser_parse_pac_file. Included only for backward compatibility.
int pacparser_parse_pac(const char *pacfile);

/// @brief Finds proxy for the given URL and Host.
/// @param url URL to find proxy for.
/// @param host Host part of the URL.
/// @returns proxy string on sucess and NULL on error.
///
/// Finds proxy for the given URL and Host. This function should be called only
/// after pacparser engine has been initialized (using pacparser_init) and pac
/// script has been parsed (using pacparser_parse_pac_file or
/// pacparser_parse_pac_string).
char *pacparser_find_proxy(const char *url, const char *host);

/// @brief Finds proxy for the given PAC file, URL and Host.
/// @param pacfile PAC file to parse.
/// @param url URL to find proxy for.
/// @param host Host part of the URL.
/// @returns proxy string on success and NULL on error.
///
/// This function is a wrapper around functions pacparser_init,
/// pacparser_parse_pac_file, pacparser_find_proxy and pacparser_cleanup. If
/// you just want to find out proxy for a given set of pac file, url and host, this
/// is the function to call. This function takes care of all the initialization
/// and cleanup.
char *pacparser_just_find_proxy(const char *pacfile, const char *url,
                                const char *host);

/// @brief Destroys JavaSctipt context.
///
/// This function should be called once you're done with using pacparser engine.
void pacparser_cleanup(void);

/// @brief Sets my IP address.
/// @param ip Custom IP address.
///
/// Sets my IP address to a custom value. This is the IP address returned by
/// myIpAddress() javascript function.
void pacparser_setmyip(const char *ip);

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
/// @param domains The comma-separated list of domains.
/// @returns 0 on failure and 1 on success.
///
/// Use a custom list of domains, instead of relying on, e.g., the
/// "search" directive in /etc/resolv.conf.
/// It will always succeed if c-ares integration was active at compile time,
/// and always fail otherwise.
int pacparser_set_dns_domains(const char *domains);

/// @brief Set DNS resolver to use
/// @param dns_resolver_variant The DNS resolver variant to use.
/// @returns 0 on failure, non-zero otherwise.
///
/// Return value will be zero if the given DNS resolver variant is invalid.
/// This is also the case if the function is asked to use c-ares as the
/// DNS resolver, but c-ares was not available at compile time.
///
/// The valid variants are:
///   - `"none"` (also available as the `DNS_NONE` macro in the pacparser.h
///     file): only hostnames that are actually literal IPs are resovable (to
///     the same IP).
///   - `"getaddrinfo"` (also available as the `DNS_GETADDRINFO` macro in the
///     pacparser.h file): use the system's `getaddrinfo()` function to do DNS
///     resolution.
///   - `"c-ares"` (also available as the `DNS_C_ARES` macro in the pacparser.h
///     file): use the c-ares library to do DNS resolution.
///
int pacparser_set_dns_resolver_variant(const char *dns_resolver_variant);

// Valid DNS resolver types (see the documentation just above).
#define DNS_NONE "none"
#define DNS_GETADDRINFO "getaddrinfo"
#define DNS_C_ARES "c-ares"

/// @brief Type definition for pacparser_error_printer.
/// @param fmt printf format
/// @param argp Variadic arg list
///
/// Default printing function for pacparser_set_error_printer
typedef int (*pacparser_error_printer)(const char *fmt, va_list argp);

/// @brief Sets error printing function.
/// @param func Variadic-argument Printing function.
///
/// Sets error variadic-argument printing function.  If not set the messages
/// are printed to stderr.  If messages begin with DEBUG: or WARNING:,
/// they are not fatal error messages, otherwise they are.
/// May be called before pacparser_init().
void pacparser_set_error_printer(pacparser_error_printer func);

/// @brief (Deprecated) Enable Microsoft IPv6 PAC extensions.
///
/// Deprecated. IPv6 extension (*Ex functions) are enabled by default now.
void pacparser_enable_microsoft_extensions(void);

/// @brief Disable Microsoft IPv6 PAC extensions.
void pacparser_disable_microsoft_extensions(void);

/// @brief Returns pacparser version.
/// @returns version string if version defined, "" otherwise.
///
/// Version string is determined at the time of build. If built from a released
/// package, version corresponds to the latest release (git) tag. If built from
/// the repository, it corresponds to the head revision of the repo.
char* pacparser_version(void);

#ifdef __cplusplus
}
#endif

/// @}

#endif // PACPARSER_H_
