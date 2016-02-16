// Copyright (C) 2007 Manu Garg.
// Author: Manu Garg <manugarg@gmail.com>
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

#include <errno.h>
#include <jsapi.h>

#include "util.h"
#include "pac_utils.h"
#include "pacparser_dns.h"
#include "pacparser.h"

// To make some function calls more readable.
#define ONE_IP 0
#define ALL_IPS 1
#define DISABLED 0
#define ENABLED 1

static const char *myip = NULL;
static int enable_microsoft_extensions = ENABLED;

// Default error printer function.
// Returns the number of characters printed, and a negative value
// in case of output error.
static int
default_error_printer(const char *fmt, va_list argp)
{
  return vfprintf(stderr, fmt, argp);
}

// File level variable to hold error printer function pointer.
static pacparser_error_printer error_printer_func = &default_error_printer;

// Set error printer to a user defined function.
void
pacparser_set_error_printer(pacparser_error_printer func)
{
  error_printer_func = func;
}

static int
print_error(const char *fmt, ...)
{
  int ret;
  va_list args;
  va_start(args, fmt);
  ret = (*error_printer_func)(fmt, args);
  va_end(args);
  return ret;
}

#define print_debug(...) \
    do { \
      if (getenv("PACPARSER_DEBUG")) { \
        print_error("DEBUG: "); \
        print_error(__VA_ARGS__); \
      } \
    } while (0)

// Utility function to read a file into string.
// Returns a malloc'ed string containing the file content on success,
// NULL on error.
static char *
read_file_into_str(const char *filename)
{
  char *str = NULL;
  int file_size;
  FILE *fptr;
  int records_read;

  if (!(fptr = fopen(filename, "r")))
    return NULL;

  if (fseek(fptr, 0L, SEEK_END) != 0)
    goto close_and_return;
  if ((file_size = ftell(fptr)) < 0)
    goto close_and_return;
  if (fseek(fptr, 0L, SEEK_SET) != 0)
    goto close_and_return;
  if ((str = (char *) malloc(file_size + 1)) == NULL)
    goto close_and_return;

  // 'str' is no longer NULL if we are here.
  if (!(records_read = fread(str, 1, file_size, fptr))) {
    free(str);
    str = NULL;
    goto close_and_return;
  }
  str[records_read] = '\0';

close_and_return:
  fclose(fptr);
  return str;
}

static void
print_jserror(JSContext *cx, const char *message, JSErrorReport *report)
{
  print_error("JSERROR: %s:%d:\n    %s\n",
              (report->filename ? report->filename : "NULL"), report->lineno,
              message);
}

//------------------------------------------------------------------------------

// Functions related to DNS resolution.

static pacparser_resolve_host_func resolve_host_func = &resolve_host_getaddrinfo;

// Set my (client's) IP address to a custom value.
void
pacparser_setmyip(const char *ip)
{
  if (myip)
    free(myip);
  myip = strdup(ip);
}

int
pacparser_set_dns_resolver_type(dns_resolver_t type)
{
  switch (type) {
    case DNS_NONE:
      resolve_host_func = &resolve_host_literal_ips_only;
      return 1;
    case DNS_GETADDRINFO:
      resolve_host_func = &resolve_host_getaddrinfo;
      return 1;
    case DNS_C_ARES:
#ifdef HAVE_C_ARES
      resolve_host_func = &resolve_host_c_ares;
      return 1;
#else
      print_error("pacparser.c: cannot use c-ares as DNS resolver: was not "
                  "available at compile time.\n");
      return 0;
#endif
  }
  abort(); /* NOTREACHED */
}

// dnsResolve/dnsResolveEx in JS context; not available in core JavaScript.
// Return javascript null if not able to resolve.

static JSBool
dns_resolve_internals(JSContext *cx, JSObject *obj, uintN argc, jsval *argv,
                      jsval *rval, int all_ips)
{
  char *name = JS_GetStringBytes(JS_ValueToString(cx, argv[0]));
  char *ipaddr, *out;

  if ((ipaddr = resolve_host_func(name, all_ips)) == NULL) {
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
    ipaddr = get_my_ip_address(resolve_host_func, all_ips);
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

static void
pacparser_set_microsoft_extensions(int setting)
{
  if (cx) {
    print_error(
        "pacparser.c: pacparser_set_microsoft_extensions: cannot enable or "
        "disable microsoft extensions now. This function should be called "
        "before pacparser_init().\n");
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
//
// Return 0 on failure, 1 on success.
int
pacparser_init()
{
  jsval rval;
  char *error_prefix = "pacparser.c: pacparser_init";
  // Initialize JS engine.
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
    print_error("%s: %s\n", error_prefix,
                "Could not initialize JavaScript runtime.");
    return 0;
  }
  JS_SetErrorReporter(cx, print_jserror);
  // Export our functions to Javascript engine
  if (!JS_DefineFunction(cx, global, "dnsResolve", dns_resolve, 1, 0)) {
    print_error("%s: %s\n", error_prefix,
                "Could not define dnsResolve in JS context.");
    return 0;
  }
  if (!JS_DefineFunction(cx, global, "myIpAddress", my_ip, 0, 0)) {
    print_error("%s: %s\n", error_prefix,
                "Could not define myIpAddress in JS context.");
    return 0;
  }
  if (enable_microsoft_extensions) {
    if (!JS_DefineFunction(cx, global, "dnsResolveEx", dns_resolve_ex, 1, 0)) {
      print_error("%s: %s\n", error_prefix,
                  "Could not define dnsResolveEx in JS context.");
      return 0;
    }
    if (!JS_DefineFunction(cx, global, "myIpAddressEx", my_ip_ex, 0, 0)) {
      print_error("%s: %s\n", error_prefix,
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
    print_error("%s: %s\n", error_prefix,
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

  print_debug("Pacparser Initalized.\n");
  return 1;
}

// Parses the given PAC script string.
//
// Evaulates the given PAC script string in the JavaScript context created
// by pacparser_init.
//
// Return 0 on failure and 1 on succcess.
int
pacparser_parse_pac_string(const char *script)
{
  jsval rval;
  char *error_prefix = "pacparser.c: pacparser_parse_pac_string";
  if (cx == NULL || global == NULL) {
    print_error("%s: %s\n", error_prefix, "Pac parser is not initialized.");
    return 0;
  }
  if (!JS_EvaluateScript(cx,
                         global,
                         script,       // Script read from pacfile
                         strlen(script),
                         "PAC script",
                         1,
                         &rval)) {     // If script evaluation failed
    print_error("%s: %s\n", error_prefix, "Failed to evaluate the pac script.");
    print_debug("Failed to parse the PAC script:\n%s\n", script);
    return 0;
  }
  print_debug("Parsed the PAC script.\n");
  return 1;
}

// Parses the given PAC file.
//
// Rads the given PAC file and evaluates it in the JavaScript context
// created by pacparser_init.
//
// Return 0 on failure and 1 on succcess.
int
pacparser_parse_pac_file(const char *pacfile)
{
  char *script = NULL;

  if ((script = read_file_into_str(pacfile)) == NULL) {
    print_error("pacparser.c: pacparser_parse_pac: %s: %s: %s\n",
                "Could not read the pacfile: ", pacfile, strerror(errno));
    return 0;
  }

  int result = pacparser_parse_pac_string(script);
  free(script);

  if (result)
    print_debug("Parsed the PAC file: %s\n", pacfile);
  else
    print_debug("Could not parse the PAC file: %s\n", pacfile);

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
//
// Returns the proxy string on succcess, NULL on failure.
char *
pacparser_find_proxy(const char *url, const char *host)
{
  char *error_prefix = "pacparser.c: pacparser_find_proxy";
  char *script;
  jsval rval;

  print_debug("Finding proxy for URL '%s' and Host '%s'\n", url, host);

  if (url == NULL || STREQ(url, "")) {
    print_error("%s: %s\n", error_prefix, "URL not defined");
    return NULL;
  }
  if (host == NULL || STREQ(host,"")) {
    print_error("%s: %s\n", error_prefix, "Host not defined");
    return NULL;
  }
  if (cx == NULL || global == NULL) {
    print_error("%s: %s\n", error_prefix, "Pac parser is not initialized.");
    return NULL;
  }

  // Test if findProxyForURL is defined.
  script = "typeof(findProxyForURL);";
  print_debug("Executing JavaScript: %s\n", script);
  JS_EvaluateScript(cx, global, script, strlen(script), NULL, 1, &rval);
  if (!STREQ("function", JS_GetStringBytes(JS_ValueToString(cx, rval)))) {
    print_error("%s: %s\n", error_prefix,
                "Javascript function findProxyForURL not defined.");
    return NULL;
  }

  // URL-encode "'" as we use single quotes to stick the URL into a
  // temporary script.
  char *sanitized_url = str_replace(url, "'", "%27");
  // Hostname shouldn't have single quotes in them.
  if (strchr(host, '\'')) {
    print_error("%s: %s\n", error_prefix,
                "Invalid hostname: hostname can't have single quotes.");
    return NULL;
  }

  script = (char *) malloc(32 + strlen(url) + strlen(host));
  script[0] = '\0';
  strcat(script, "findProxyForURL('");
  strcat(script, sanitized_url);
  strcat(script, "', '");
  strcat(script, host);
  strcat(script, "')");
  print_debug("Executing JavaScript: %s\n", script);
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
  JS_ShutDown();
  global = NULL;
  print_debug("Pacparser destroyed.\n");
}

// Finds proxy for the given PAC file, url and host.
//
// This function is a wrapper around functions pacparser_init,
// pacparser_parse_pac, pacparser_find_proxy and pacparser_cleanup. If you just
// want to find out proxy a given set of pac file, url and host, this is the
// function to call.
//
// Returns the proxy string on succcess, NULL on failure.
char *
pacparser_just_find_proxy(const char *pacfile, const char *url,
                          const char *host)
{
  char *proxy;
  char *out;
  int initialized_here = 0;
  char *error_prefix = "pacparser.c: pacparser_just_find_proxy";
  if (!global) {
    if (!pacparser_init()) {
      print_error("%s: %s\n", error_prefix, "Could not initialize pacparser");
      return NULL;
    }
    initialized_here = 1;
  }
  if (!pacparser_parse_pac(pacfile)) {
    print_error("%s: %s %s\n", error_prefix,
                "Could not parse pacfile", pacfile);
    if (initialized_here)
      pacparser_cleanup();
    return NULL;
  }
  if (!(out = pacparser_find_proxy(url, host))) {
    print_error("%s: %s %s\n", error_prefix,
                "Could not determine proxy for url", url);
    if (initialized_here)
      pacparser_cleanup();
    return NULL;
  }
  proxy = strdup(out);
  if (initialized_here)
    pacparser_cleanup();
  return proxy;
}

#define QUOTEME_(x) #x
#define QUOTEME(x) QUOTEME_(x)

char *pacparser_version(void) {
#ifndef VERSION
  print_error("WARNING: VERSION not defined.");
  return "";
#endif
  return QUOTEME(VERSION);
}
