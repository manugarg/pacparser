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
#include <winsock2.h>
#include <ws2tcpip.h>
#endif

#include "pac_utils.h"
#include "pacparser.h"

#define MAX_IP_RESULTS 10

static char *myip = NULL;

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

// DNS Resolve function; used by other routines.
// This function is used by dnsResolve, dnsResolveEx, myIpAddress,
// myIpAddressEx.
static int
resolve_host(const char *hostname, char *ipaddr_list, int max_results,
             int req_ai_family)
{
  struct addrinfo hints;
  struct addrinfo *result;
  struct addrinfo *ai;
  char ipaddr[INET6_ADDRSTRLEN];
  int error;

  // Truncate ipaddr_list to an empty string.
  ipaddr_list[0] = '\0';

#ifdef _WIN32
  // On windows, we need to initialize the winsock dll first.
  WSADATA WsaData;
  WSAStartup(MAKEWORD(2,0), &WsaData);
#endif

  memset(&hints, 0, sizeof(struct addrinfo));

  hints.ai_family = req_ai_family;
  hints.ai_socktype = SOCK_STREAM;

  error = getaddrinfo(hostname, NULL, &hints, &result);
  if (error) return error;
  int i = 0;
  for(ai = result; ai != NULL && i < max_results; ai = ai->ai_next, i++) {
    getnameinfo(ai->ai_addr, ai->ai_addrlen, ipaddr, sizeof(ipaddr), NULL, 0,
                NI_NUMERICHOST);
    if (ipaddr_list[0] == '\0') sprintf(ipaddr_list, "%s", ipaddr);
    else sprintf(ipaddr_list, "%s;%s", ipaddr_list, ipaddr);
  }
  freeaddrinfo(result);
#ifdef _WIN32
  WSACleanup();
#endif
  return 0;
}

// dnsResolve in JS context; not available in core JavaScript.
// returns javascript null if not able to resolve.
static JSBool                                  // JS_TRUE or JS_FALSE
dns_resolve(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
  char* name = JS_GetStringBytes(JS_ValueToString(cx, argv[0]));
  char* out;
  char ipaddr[INET6_ADDRSTRLEN] = "";

  // Return null on failure.
  if(resolve_host(name, ipaddr, 1, AF_INET)) {
    *rval = JSVAL_NULL;
    return JS_TRUE;
  }

  out = JS_malloc(cx, strlen(ipaddr) + 1);
  strcpy(out, ipaddr);
  JSString *str = JS_NewString(cx, out, strlen(out));
  *rval = STRING_TO_JSVAL(str);
  return JS_TRUE;
}

// dnsResolveEx in JS context; not available in core JavaScript.
// returns javascript null if not able to resolve.
static JSBool                                  // JS_TRUE or JS_FALSE
dns_resolve_ex(JSContext *cx, JSObject *obj, uintN argc, jsval *argv,
               jsval *rval)
{
  char* name = JS_GetStringBytes(JS_ValueToString(cx, argv[0]));
  char* out;
  char ipaddr[INET6_ADDRSTRLEN * MAX_IP_RESULTS + MAX_IP_RESULTS] = "";

  out = JS_malloc(cx, strlen(ipaddr) + 1);
  // Return "" on failure.
  if(resolve_host(name, ipaddr, MAX_IP_RESULTS, AF_UNSPEC)) {
    strcpy(out, "");
  }
  strcpy(out, ipaddr);
  JSString *str = JS_NewString(cx, out, strlen(out));
  *rval = STRING_TO_JSVAL(str);
  return JS_TRUE;
}

// myIpAddress in JS context; not available in core JavaScript.
// returns 127.0.0.1 if not able to determine local ip.
static JSBool                                  // JS_TRUE or JS_FALSE
my_ip(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
  char ipaddr[INET6_ADDRSTRLEN];
  char* out;

  if (myip)                  // If my (client's) IP address is already set.
    strcpy(ipaddr, myip);
  else {
    char name[256];
    gethostname(name, sizeof(name));
    if (resolve_host(name, ipaddr, 1, AF_INET)) {
      strcpy(ipaddr, "127.0.0.1");
    }
  }

  out = JS_malloc(cx, strlen(ipaddr) + 1);
  strcpy(out, ipaddr);
  JSString *str = JS_NewString(cx, out, strlen(out));
  *rval = STRING_TO_JSVAL(str);
  return JS_TRUE;
}

// myIpAddressEx in JS context; not available in core JavaScript.
// returns 127.0.0.1 if not able to determine local ip.
static JSBool                                  // JS_TRUE or JS_FALSE
my_ip_ex(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
  char ipaddr[INET6_ADDRSTRLEN * MAX_IP_RESULTS + MAX_IP_RESULTS];
  char* out;

  if (myip)                  // If my (client's) IP address is already set.
    strcpy(ipaddr, myip);
  else {
    char name[256];
    gethostname(name, sizeof(name));
    if (resolve_host(name, ipaddr, MAX_IP_RESULTS, AF_UNSPEC)) {
      strcpy(ipaddr, "");
    }
  }

  out = JS_malloc(cx, strlen(ipaddr) + 1);
  strcpy(out, ipaddr);
  JSString *str = JS_NewString(cx, out, strlen(out));
  *rval = STRING_TO_JSVAL(str);
  return JS_TRUE;
}

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
  myip = malloc(strlen(ip) +1);         // Allocate space just to be sure.
  strcpy(myip, ip);
}

// Decprecated: This function doesn't do anything.
//
// This function doesn't do anything. Microsoft exntensions are now enabled by
// default.
void
pacparser_enable_microsoft_extensions()
{
  return;
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
  if (!(rt = JS_NewRuntime(8L * 1024L * 1024L)) ||
      !(cx = JS_NewContext(rt, 8192)) ||
      !(global = JS_NewObject(cx, &global_class, NULL, NULL)) ||
      !JS_InitStandardClasses(cx, global)) {
    print_error("%s %s\n", error_prefix, "Could not initialize  JavaScript "
		  "runtime.");
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
  // Evaluate pacUtils. Utility functions required to parse pac files.
  if (!JS_EvaluateScript(cx,           // JS engine context
                         global,       // global object
                         pacUtils,     // this is defined in pac_utils.h
                         strlen(pacUtils),
                         NULL,         // filename (NULL in this case)
                         1,            // line number, used for reporting.
                         &rval)) {
    print_error("%s %s\n", error_prefix,
		  "Could not evaluate pacUtils defined in pac_utils.h.");
    return 0;
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

// Destroys JavaSctipt Engine.
void
pacparser_cleanup()
{
  // Reinitliaze config variables.
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
