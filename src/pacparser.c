// Copyright (C) 2007 Manu Garg.
// Author: Manu Garg <manugarg@gmail.com>
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

#include <errno.h>
#include <jsapi.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#ifdef XP_UNIX
#include <unistd.h>
#include <sys/socket.h>                // for AF_INET
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

#include "util.h"
#include "pac_utils.h"
#include "pacparser.h"

// To make some function calls more readable.
#define ONE_IP 0
#define ALL_IPS 1
#define DISABLED 0
#define ENABLED 1

// TODO(slattarini): this should disappear.
#define MAX_IP_RESULTS 10

static char *myip = NULL;
static int enable_microsoft_extensions = ENABLED;

// Default error printer function.
static int		// Number of characters printed, negative value in case of output error.
_default_error_printer(const char *fmt, va_list argp)
{
  return vfprintf(stderr, fmt, argp);
}

// File level variable to hold error printer function pointer.
static pacparser_error_printer error_printer_func = &_default_error_printer;

// Set error printer to a user defined function.
void
pacparser_set_error_printer(pacparser_error_printer func)
{
  error_printer_func = func;
}

static int print_error(const char *fmt, ...)
{
  int ret;
  va_list args;
  va_start(args, fmt);
  ret = (*error_printer_func)(fmt, args);
  va_end(args);
  return ret;
}

static int
_debug(void) {
  if(getenv("PACPARSER_DEBUG")) return 1;
  return 0;
}

// Utility function to read a file into string.
static char *                      // File content in string or NULL if failed.
read_file_into_str(const char *filename)
{
  char *str;
  int file_size;
  FILE *fptr;
  int records_read;
  if (!(fptr = fopen(filename, "r"))) goto error1;
  if ((fseek(fptr, 0L, SEEK_END) != 0)) goto error2;
  if (!(file_size=ftell(fptr))) goto error2;
  if ((fseek(fptr, 0L, SEEK_SET) != 0)) goto error2;
  if (!(str = (char*) malloc(file_size+1))) goto error2;
  if (!(records_read=fread(str, 1, file_size, fptr))) {
    free(str);
    goto error2;
  }
  str[records_read] = '\0';
  fclose(fptr);
  return str;
error2:
  fclose(fptr);
error1:
  return NULL;
}

static void
print_jserror(JSContext *cx, const char *message, JSErrorReport *report)
{
  print_error("JSERROR: %s:%d:\n    %s\n",
              (report->filename ? report->filename : "NULL"), report->lineno,
              message);
}

//------------------------------------------------------------------------------

// DNS Resolve functions; used by other routines which implement the PAC
// builtins dnsResolve(), dnsResolveEx(), myIpAddress(), myIpAddressEx().

int
addrinfo_len(const struct addrinfo *ai)
{
  int i = 0;
  while (ai != NULL) {
    i++;
    ai = ai->ai_next;
  }
  return i;
}

static char *
resolve_host(const char *hostname, int all_ips)
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
  return retval;
}

//------------------------------------------------------------------------------

// dnsResolve/dnsResolveEx in JS context; not available in core JavaScript.
// Return javascript null if not able to resolve.
static JSBool
dns_resolve_internals(JSContext *cx, JSObject *obj, uintN argc, jsval *argv,
                      jsval *rval, int all_ips)
{
  char *name = JS_GetStringBytes(JS_ValueToString(cx, argv[0]));
  char *ipaddr, *out;

  if ((ipaddr = resolve_host(name, all_ips)) == NULL) {
    *rval = JSVAL_NULL;
    return JS_TRUE;
  }

  out = JS_strdup(cx, ipaddr);
  free(ipaddr);
  JSString *str = JS_NewString(cx, out, strlen(out));
  *rval = STRING_TO_JSVAL(str);
  return JS_TRUE;
}

static JSBool
dns_resolve(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
  return dns_resolve_internals(cx, obj, argc, argv, rval, ONE_IP);
}

static JSBool
dns_resolve_ex(JSContext *cx, JSObject *obj, uintN argc, jsval *argv,
               jsval *rval)
{
  return dns_resolve_internals(cx, obj, argc, argv, rval, ALL_IPS);
}

//------------------------------------------------------------------------------

// myIpAddress/myIpAddressEx in JS context; not available in core JavaScript.
// Return 127.0.0.1 if not able to determine local ip.
static JSBool
my_ip_internals(JSContext *cx, JSObject *obj, uintN argc, jsval *argv,
                jsval *rval, int all_ips)
{
  char *ipaddr, *out;

  if (myip) {
    // If "my" (client's) IP address is already set.
    ipaddr = strdup(myip);
  } else {
    // According to the gethostname(2) manpage, SUSv2  guarantees that
    // "Host names are limited to 255 bytes".
    char name[256];
    gethostname(name, sizeof(name));
    if ((ipaddr = resolve_host(name, all_ips)) == NULL) {
      ipaddr = strdup("127.0.0.1");
    }
  }

  out = JS_strdup(cx, ipaddr);
  free(ipaddr);
  JSString *str = JS_NewString(cx, out, strlen(out));
  *rval = STRING_TO_JSVAL(str);
  return JS_TRUE;
}

static JSBool
my_ip(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
  return my_ip_internals(cx, obj, argc, argv, rval, ONE_IP);
}

static JSBool
my_ip_ex(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
  return my_ip_internals(cx, obj, argc, argv, rval, ALL_IPS);
}

//------------------------------------------------------------------------------

// Define some JS context related variables.
static JSRuntime *rt = NULL;
static JSContext *cx = NULL;
static JSObject *global = NULL;
static JSClass global_class = {
    "global",0,
    JS_PropertyStub,JS_PropertyStub,JS_PropertyStub,JS_PropertyStub,
    JS_EnumerateStub,JS_ResolveStub,JS_ConvertStub,JS_FinalizeStub
};

// Set my (client's) IP address to a custom value.
void
pacparser_setmyip(const char *ip)
{
  if (myip)
    free(myip);
  myip = (const char *) strdup(ip);
}

static void
pacparser_set_microsoft_extensions(int setting)
{
  if (cx) {
    print_error(
        "pacparser.c: pacparser_set_microsoft_extensions: cannot enable or "
        "disable microsoft extensions now. This function should be called "
        "before pacparser_init.\n");
    return;
  }
  enable_microsoft_extensions = setting;
}

void
pacparser_enable_microsoft_extensions(void)
{
  pacparser_set_microsoft_extensions(ENABLED);
}

void
pacparser_disable_microsoft_extensions(void)
{
  pacparser_set_microsoft_extensions(DISABLED);
}

// Initialize PAC parser.
//
// - Initializes JavaScript engine,
// - Exports dns_functions (defined above) to JavaScript context.
// - Sets error reporting function to print_jserror,
// - Evaluates JavaScript code in pacUtils variable defined in pac_utils.h.
int                                     // 0 (=Failure) or 1 (=Success)
pacparser_init()
{
  jsval rval;
  char *error_prefix = "pacparser.c: pacparser_init:";
  // Initialize JS engine
  if (
    // https://developer.mozilla.org/en-US/docs/Mozilla/Projects/SpiderMonkey/JSAPI_reference/JS_NewRuntime
    // First argument is "the maximum number of allocated bytes after
    // which garbage collection is run".
    !(rt = JS_NewRuntime(8L * 1024L * 1024L)) ||
    // https://developer.mozilla.org/en-US/docs/Mozilla/Projects/SpiderMonkey/JSAPI_reference/JS_NewContext
    // The second argument is "a memory management tuning parameter which
    // most users should not adjust; 8192 is a good default value".
    !(cx = JS_NewContext(rt, 8192)) ||
    !(global = JS_NewObject(cx, &global_class, NULL, NULL)) ||
    !JS_InitStandardClasses(cx, global)
  ) {
    print_error("%s %s\n", error_prefix,
                "Could not initialize JavaScript runtime.");
    return 0;
  }
  JS_SetErrorReporter(cx, print_jserror);
  // Export our functions to Javascript engine
  if (!JS_DefineFunction(cx, global, "dnsResolve", dns_resolve, 1, 0)) {
    print_error("%s %s\n", error_prefix,
                "Could not define dnsResolve in JS context.");
    return 0;
  }
  if (!JS_DefineFunction(cx, global, "myIpAddress", my_ip, 0, 0)) {
    print_error("%s %s\n", error_prefix,
                "Could not define myIpAddress in JS context.");
    return 0;
  }
  if (enable_microsoft_extensions) {
    if (!JS_DefineFunction(cx, global, "dnsResolveEx", dns_resolve_ex, 1, 0)) {
      print_error("%s %s\n", error_prefix,
                  "Could not define dnsResolveEx in JS context.");
      return 0;
    }
    if (!JS_DefineFunction(cx, global, "myIpAddressEx", my_ip_ex, 0, 0)) {
      print_error("%s %s\n", error_prefix,
                  "Could not define myIpAddressEx in JS context.");
      return 0;
    }
  }

  // Evaluate pac_builtins -- code defining utility functions required to
  // parse PAC files.
  if (!JS_EvaluateScript(cx,           // JS engine context
                         global,       // global object
                         pac_builtins, // this is defined in pac_utils.h
                         strlen(pac_builtins),
                         NULL,         // filename (NULL in this case)
                         1,            // line number, used for reporting
                         &rval)) {
    print_error("%s %s\n", error_prefix,
                "Could not evaluate pac_builtins defined in pac_utils.h");
    return 0;
  }

  if (enable_microsoft_extensions) {
    // Evaluate pac_builtins_ex -- code defining extensions to the utility
    // functions defined above.
    if (!JS_EvaluateScript(cx,              // JS engine context
                           global,          // global object
                           pac_builtins_ex, // this is defined in pac_utils.h
                           strlen(pac_builtins_ex),
                           NULL,            // filename (NULL in this case)
                           1,               // line number, used for reporting
                           &rval)) {
      print_error("%s: %s\n", error_prefix,
                  "Could not evaluate pac_builtins_ex defined in pac_utils.h");
      return 0;
    }
  }

  if (_debug()) print_error("DEBUG: Pacparser Initalized.\n");
  return 1;
}

// Parses the given PAC script string.
//
// Evaulates the given PAC script string in the JavaScript context created
// by pacparser_init.
int                                     // 0 (=Failure) or 1 (=Success)
pacparser_parse_pac_string(const char *script)
{
  jsval rval;
  char *error_prefix = "pacparser.c: pacparser_parse_pac_string:";
  if (cx == NULL || global == NULL) {
    print_error("%s %s\n", error_prefix, "Pac parser is not initialized.");
    return 0;
  }
  if (!JS_EvaluateScript(cx,
                         global,
                         script,       // Script read from pacfile
                         strlen(script),
                         "PAC script",
                         1,
                         &rval)) {     // If script evaluation failed
    print_error("%s %s\n", error_prefix, "Failed to evaluate the pac script.");
    if (_debug()) print_error("DEBUG: Failed to parse the PAC script:\n%s\n",
				script);
    return 0;
  }
  if (_debug()) print_error("DEBUG: Parsed the PAC script.\n");
  return 1;
}

// Parses the given PAC file.
//
// reads the given PAC file and evaluates it in the JavaScript context created
// by pacparser_init.
int                                     // 0 (=Failure) or 1 (=Success)
pacparser_parse_pac_file(const char *pacfile)
{
  char *script = NULL;

  if ((script = read_file_into_str(pacfile)) == NULL) {
    print_error("pacparser.c: pacparser_parse_pac: %s: %s: %s\n",
            "Could not read the pacfile: ", pacfile, strerror(errno));
    return 0;
  }

  int result = pacparser_parse_pac_string(script);
  if (script != NULL) free(script);

  if (_debug()) {
    if(result) print_error("DEBUG: Parsed the PAC file: %s\n", pacfile);
    else print_error("DEBUG: Could not parse the PAC file: %s\n", pacfile);
  }

  return result;
}

// Parses PAC file (same as pacparser_parse_pac_file)
//
// (Deprecated) Use pacparser_parse_pac_file instead.
int                                     // 0 (=Failure) or 1 (=Success)
pacparser_parse_pac(const char *pacfile)
{
  return pacparser_parse_pac_file(pacfile);
}

// Finds proxy for the given URL and Host.
//
// If JavaScript engine is intialized and findProxyForURL function is defined,
// it evaluates code findProxyForURL(url,host) in JavaScript context and
// returns the result.
char *                                  // Proxy string or NULL if failed.
pacparser_find_proxy(const char *url, const char *host)
{
  char *error_prefix = "pacparser.c: pacparser_find_proxy:";
  if (_debug()) print_error("DEBUG: Finding proxy for URL: %s and Host:"
                        " %s\n", url, host);
  jsval rval;
  char *script;
  if (url == NULL || (strcmp(url, "") == 0)) {
    print_error("%s %s\n", error_prefix, "URL not defined");
    return NULL;
  }
  if (host == NULL || (strcmp(host,"") == 0)) {
    print_error("%s %s\n", error_prefix, "Host not defined");
    return NULL;
  }
  if (cx == NULL || global == NULL) {
    print_error("%s %s\n", error_prefix, "Pac parser is not initialized.");
    return NULL;
  }
  // Test if findProxyForURL is defined.
  script = "typeof(findProxyForURL);";
  if (_debug()) print_error("DEBUG: Executing JavaScript: %s\n", script);
  JS_EvaluateScript(cx, global, script, strlen(script), NULL, 1, &rval);
  if (strcmp("function", JS_GetStringBytes(JS_ValueToString(cx, rval))) != 0) {
    print_error("%s %s\n", error_prefix,
		  "Javascript function findProxyForURL not defined.");
    return NULL;
  }

  // URL-encode "'" as we use single quotes to stick the URL into a temporary script.
  char *sanitized_url = str_replace(url, "'", "%27");
  // Hostname shouldn't have single quotes in them
  if (strchr(host, '\'')) {
    print_error("%s %s\n", error_prefix,
		"Invalid hostname: hostname can't have single quotes.");
    return NULL;
  }

  script = (char*) malloc(32 + strlen(url) + strlen(host));
  script[0] = '\0';
  strcat(script, "findProxyForURL('");
  strcat(script, sanitized_url);
  strcat(script, "', '");
  strcat(script, host);
  strcat(script, "')");
  if (_debug()) print_error("DEBUG: Executing JavaScript: %s\n", script);
  if (!JS_EvaluateScript(cx, global, script, strlen(script), NULL, 1, &rval)) {
    print_error("%s %s\n", error_prefix, "Problem in executing findProxyForURL.");
    free(sanitized_url);
    free(script);
    return NULL;
  }
  free(sanitized_url);
  free(script);
  return JS_GetStringBytes(JS_ValueToString(cx, rval));
}

// Destroys JavaScript Engine.
void
pacparser_cleanup()
{
  // Reinitialize config variables.
  myip = NULL;
  if (cx) {
    JS_DestroyContext(cx);
    cx = NULL;
  }
  if (rt) {
    JS_DestroyRuntime(rt);
    rt = NULL;
  }
  if (!cx && !rt) JS_ShutDown();
  global = NULL;
  if (_debug()) print_error("DEBUG: Pacparser destroyed.\n");
}

// Finds proxy for the given PAC file, url and host.
//
// This function is a wrapper around functions pacparser_init,
// pacparser_parse_pac, pacparser_find_proxy and pacparser_cleanup. If you just
// want to find out proxy a given set of pac file, url and host, this is the
// function to call.
char *                                  // Proxy string or NULL if failed.
pacparser_just_find_proxy(const char *pacfile,
                         const char *url,
                         const char *host)
{
  char *proxy;
  char *out;
  int initialized_here = 0;
  char *error_prefix = "pacparser.c: pacparser_just_find_proxy:";
  if (!global) {
    if (!pacparser_init()) {
      print_error("%s %s\n", error_prefix, "Could not initialize pacparser");
      return NULL;
    }
    initialized_here = 1;
  }
  if (!pacparser_parse_pac(pacfile)) {
    print_error("%s %s %s\n", error_prefix, "Could not parse pacfile",
		  pacfile);
    if (initialized_here) pacparser_cleanup();
    return NULL;
  }
  if (!(out = pacparser_find_proxy(url, host))) {
    print_error("%s %s %s\n", error_prefix,
		  "Could not determine proxy for url", url);
    if (initialized_here) pacparser_cleanup();
    return NULL;
  }
  proxy = (char*) malloc(strlen(out) + 1);
  strcpy(proxy, out);
  if (initialized_here) pacparser_cleanup();
  return proxy;
}

#define QUOTEME_(x) #x
#define QUOTEME(x) QUOTEME_(x)

char* pacparser_version(void) {
#ifndef VERSION
  print_error("WARNING: VERSION not defined.");
  return "";
#endif
  return QUOTEME(VERSION);
}
