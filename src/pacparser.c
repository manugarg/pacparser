// Copyright (C) 2007-2023 Manu Garg.
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
#include "quickjs.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#ifdef __FreeBSD__
#include <arpa/inet.h>
#endif

#ifndef _WIN32
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

#ifdef __GNUC__
#  define UNUSED(x) UNUSED_ ## x __attribute__((__unused__))
#else
#  define UNUSED(x) UNUSED_ ## x
#endif

// ---------------------------------------------------------------------------
// Thread-safety guard.
//
// pacparser keeps all of its JavaScript engine state in process-global
// variables (rt/ctx/global/proxy_result below) and creates no threads of its
// own. To keep multi-threaded embedders from racing -- most importantly, to
// stop one thread from freeing the engine in pacparser_cleanup() while another
// is still using it in pacparser_find_proxy() -- every public API entry point
// runs under this single global lock.
//
// The lock is recursive so that the composite entry points
// (pacparser_just_find_proxy, pacparser_parse_pac_file, ...) can call other
// public APIs without deadlocking. Note this only serializes calls; pacparser
// remains a single global interpreter and never runs PAC scripts concurrently.
#ifdef _WIN32
static CRITICAL_SECTION pacparser_lock;          // recursive by default
static INIT_ONCE pacparser_lock_once = INIT_ONCE_STATIC_INIT;
static BOOL CALLBACK init_lock(PINIT_ONCE o, PVOID p, PVOID *c) {
  (void)o; (void)p; (void)c;
  InitializeCriticalSection(&pacparser_lock);
  return TRUE;
}
static void lock_acquire(void) {
  InitOnceExecuteOnce(&pacparser_lock_once, init_lock, NULL, NULL);
  EnterCriticalSection(&pacparser_lock);
}
static void lock_release(void) { LeaveCriticalSection(&pacparser_lock); }
#else
#include <pthread.h>
static pthread_mutex_t pacparser_lock;
static pthread_once_t pacparser_lock_once = PTHREAD_ONCE_INIT;
static void init_lock(void) {
  pthread_mutexattr_t attr;
  pthread_mutexattr_init(&attr);
  pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
  pthread_mutex_init(&pacparser_lock, &attr);
  pthread_mutexattr_destroy(&attr);
}
static void lock_acquire(void) {
  pthread_once(&pacparser_lock_once, init_lock);
  pthread_mutex_lock(&pacparser_lock);
}
static void lock_release(void) { pthread_mutex_unlock(&pacparser_lock); }
#endif

static char my_ip_buf[INET6_ADDRSTRLEN+1];
static int my_ip_set = 0;

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
  lock_acquire();
  error_printer_func = func;
  lock_release();
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
  FILE *fptr = fopen(filename, "rb");
  if (fptr == NULL) return NULL;

  if (fseek(fptr, 0L, SEEK_END) != 0) goto error2;
  long file_size=ftell(fptr);
  if (file_size == -1) goto error2;
  if (fseek(fptr, 0L, SEEK_SET) != 0) goto error2;

  char *str = (char*) malloc(file_size+1);
  if (str == NULL) goto error2;

  // Read the file into the string
  size_t bytes_read = fread(str, 1, file_size, fptr);
  if (bytes_read != file_size) {
    free(str);
    goto error2;
  }

  // This check should not be needed but adding this satisfies
  // sonarlint static analysis, otherwise it complains about tainted
  // index.
  if (bytes_read < file_size+1) {
    str[bytes_read] = '\0';
  }
  fclose(fptr);
  return str;
error2:
  fclose(fptr);
  return NULL;
}

// Helper to dump QuickJS exceptions.
static void
dump_js_exception(JSContext *ctx)
{
  JSValue exception = JS_GetException(ctx);
  const char *msg = JS_ToCString(ctx, exception);
  if (msg) {
    print_error("JSERROR: %s\n", msg);
    JS_FreeCString(ctx, msg);
  }
  JS_FreeValue(ctx, exception);
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
  size_t offset = 0;
  for(struct addrinfo *ai = result; ai != NULL && i < max_results; ai = ai->ai_next, i++) {
    getnameinfo(ai->ai_addr, ai->ai_addrlen, ipaddr, sizeof(ipaddr), NULL, 0,
                NI_NUMERICHOST);
    if (offset > 0) {
      ipaddr_list[offset++] = ';';
    }
    size_t len = strlen(ipaddr);
    strcpy(ipaddr_list + offset, ipaddr);
    offset += len;
  }
  freeaddrinfo(result);
#ifdef _WIN32
  WSACleanup();
#endif
  return 0;
}

// dnsResolve in JS context; not available in core JavaScript.
// returns javascript null if not able to resolve.
static JSValue
dns_resolve(JSContext *ctx, JSValueConst UNUSED(this_val), int UNUSED(argc), JSValueConst *argv)
{
  const char *name = JS_ToCString(ctx, argv[0]);
  if (!name) return JS_EXCEPTION;
  char ipaddr[INET6_ADDRSTRLEN] = "";

  // Return null on failure.
  if(resolve_host(name, ipaddr, 1, AF_INET)) {
    JS_FreeCString(ctx, name);
    return JS_NULL;
  }

  JS_FreeCString(ctx, name);
  return JS_NewString(ctx, ipaddr);
}

// dnsResolveEx in JS context; not available in core JavaScript.
// returns empty string if not able to resolve.
static JSValue
dns_resolve_ex(JSContext *ctx, JSValueConst UNUSED(this_val), int UNUSED(argc), JSValueConst *argv)
{
  const char *name = JS_ToCString(ctx, argv[0]);
  if (!name) return JS_EXCEPTION;
  char ipaddr[INET6_ADDRSTRLEN * MAX_IP_RESULTS + MAX_IP_RESULTS] = "";

  // Return "" on failure.
  resolve_host(name, ipaddr, MAX_IP_RESULTS, AF_UNSPEC);

  JS_FreeCString(ctx, name);
  return JS_NewString(ctx, ipaddr);
}

// Prints space-separated args via the error printer (stderr by default).
static JSValue
js_log_print(JSContext *ctx, int argc, JSValueConst *argv, const char *prefix)
{
  print_error("%s:", prefix);
  for (int i = 0; i < argc; i++) {
    const char *s = JS_ToCString(ctx, argv[i]);
    if (!s) {
      print_error("\n");
      return JS_EXCEPTION;
    }
    print_error(" %s", s);
    JS_FreeCString(ctx, s);
  }
  print_error("\n");
  return JS_UNDEFINED;
}

static JSValue
pac_alert(JSContext *ctx, JSValueConst UNUSED(this_val), int argc, JSValueConst *argv)
{
  return js_log_print(ctx, argc, argv, "ALERT");
}

static JSValue
pac_console_log(JSContext *ctx, JSValueConst UNUSED(this_val), int argc, JSValueConst *argv)
{
  return js_log_print(ctx, argc, argv, "LOG");
}

// myIpAddress in JS context; not available in core JavaScript.
// returns 127.0.0.1 if not able to determine local ip.
static JSValue
my_ip(JSContext *ctx, JSValueConst UNUSED(this_val), int UNUSED(argc), JSValueConst *UNUSED(argv))
{
  char ipaddr[INET6_ADDRSTRLEN];

  if (my_ip_set)                  // If my (client's) IP address is already set.
    strcpy(ipaddr, my_ip_buf);
  else {
    char name[256];
    gethostname(name, sizeof(name));
    if (resolve_host(name, ipaddr, 1, AF_INET)) {
      strcpy(ipaddr, "127.0.0.1");
    }
  }

  return JS_NewString(ctx, ipaddr);
}

// myIpAddressEx in JS context; not available in core JavaScript.
// returns empty string if not able to determine local ip.
static JSValue
my_ip_ex(JSContext *ctx, JSValueConst UNUSED(this_val), int UNUSED(argc), JSValueConst *UNUSED(argv))
{
  char ipaddr[INET6_ADDRSTRLEN * MAX_IP_RESULTS + MAX_IP_RESULTS];

  if (my_ip_set)                  // If my (client's) IP address is already set.
    strcpy(ipaddr, my_ip_buf);
  else {
    char name[256];
    gethostname(name, sizeof(name));
    if (resolve_host(name, ipaddr, MAX_IP_RESULTS, AF_UNSPEC)) {
      strcpy(ipaddr, "");
    }
  }

  return JS_NewString(ctx, ipaddr);
}

// Define some JS context related variables.
static JSRuntime *rt = NULL;
static JSContext *ctx = NULL;
static JSValue global;
static const char *proxy_result = NULL;

// Set my (client's) IP address to a custom value.
static int
setmyip_impl(const char *ip)
{
  if (strlen(ip) > INET6_ADDRSTRLEN) {
    fprintf(stderr, "pacparser_setmyip: IP too long: %s\n", ip);
    return 0;
  }

  strcpy(my_ip_buf, ip);
  my_ip_set = 1;
  return 1;
}

int
pacparser_setmyip(const char *ip)
{
  lock_acquire();
  int ret = setmyip_impl(ip);
  lock_release();
  return ret;
}

// Deprecated: This function doesn't do anything.
//
// This function doesn't do anything. Microsoft extensions are now enabled by
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
// - Evaluates JavaScript code in pacUtils variable defined in pac_utils.h.
static int                              // 0 (=Failure) or 1 (=Success)
init_impl(void)
{
  char *error_prefix = "pacparser.c: pacparser_init:";
  // Initialize JS engine
  if (!(rt = JS_NewRuntime())) {
    print_error("%s %s\n", error_prefix, "Could not create JavaScript runtime.");
    return 0;
  }
  if (!(ctx = JS_NewContext(rt))) {
    print_error("%s %s\n", error_prefix, "Could not create JavaScript context.");
    return 0;
  }

  global = JS_GetGlobalObject(ctx);

  // Export our functions to Javascript engine
  JS_SetPropertyStr(ctx, global, "dnsResolve",
    JS_NewCFunction(ctx, dns_resolve, "dnsResolve", 1));
  JS_SetPropertyStr(ctx, global, "myIpAddress",
    JS_NewCFunction(ctx, my_ip, "myIpAddress", 0));
  JS_SetPropertyStr(ctx, global, "dnsResolveEx",
    JS_NewCFunction(ctx, dns_resolve_ex, "dnsResolveEx", 1));
  JS_SetPropertyStr(ctx, global, "myIpAddressEx",
    JS_NewCFunction(ctx, my_ip_ex, "myIpAddressEx", 0));

  JS_SetPropertyStr(ctx, global, "alert",
    JS_NewCFunction(ctx, pac_alert, "alert", 1));
  JSValue console = JS_NewObject(ctx);
  JS_SetPropertyStr(ctx, console, "log",
    JS_NewCFunction(ctx, pac_console_log, "log", 1));
  JS_SetPropertyStr(ctx, global, "console", console);

  // Evaluate pacUtils. Utility functions required to parse pac files.
  JSValue result = JS_Eval(ctx, pacUtils, strlen(pacUtils), "pac_utils",
                           JS_EVAL_TYPE_GLOBAL);
  if (JS_IsException(result)) {
    dump_js_exception(ctx);
    print_error("%s %s\n", error_prefix,
		  "Could not evaluate pacUtils defined in pac_utils.h.");
    JS_FreeValue(ctx, result);
    return 0;
  }
  JS_FreeValue(ctx, result);

  if (_debug()) print_error("DEBUG: Pacparser Initialized.\n");
  return 1;
}

int
pacparser_init(void)
{
  lock_acquire();
  int ret = init_impl();
  lock_release();
  return ret;
}

// Parses the given PAC script string.
//
// Evaluates the given PAC script string in the JavaScript context created
// by pacparser_init.
static int                              // 0 (=Failure) or 1 (=Success)
parse_pac_string_impl(const char *script)
{
  char *error_prefix = "pacparser.c: pacparser_parse_pac_string:";
  if (ctx == NULL) {
    print_error("%s %s\n", error_prefix, "Pac parser is not initialized.");
    return 0;
  }
  if (script == NULL) {
    print_error("%s %s\n", error_prefix, "PAC script is NULL.");
    return 0;
  }
  JSValue result = JS_Eval(ctx, script, strlen(script), "PAC script",
                           JS_EVAL_TYPE_GLOBAL);
  if (JS_IsException(result)) {
    dump_js_exception(ctx);
    print_error("%s %s\n", error_prefix, "Failed to evaluate the pac script.");
    if (_debug()) print_error("DEBUG: Failed to parse the PAC script:\n%s\n",
				script);
    JS_FreeValue(ctx, result);
    return 0;
  }
  JS_FreeValue(ctx, result);
  if (_debug()) print_error("DEBUG: Parsed the PAC script.\n");
  return 1;
}

int
pacparser_parse_pac_string(const char *script)
{
  lock_acquire();
  int ret = parse_pac_string_impl(script);
  lock_release();
  return ret;
}

// Parses the given PAC file.
//
// reads the given PAC file and evaluates it in the JavaScript context created
// by pacparser_init.
static int                              // 0 (=Failure) or 1 (=Success)
parse_pac_file_impl(const char *pacfile)
{
  char *script = NULL;

  if ((script = read_file_into_str(pacfile)) == NULL) {
    print_error("pacparser.c: pacparser_parse_pac: %s: %s: %s\n",
            "Could not read the pacfile: ", pacfile, strerror(errno));
    return 0;
  }

  int result = parse_pac_string_impl(script);
  if (script != NULL) free(script);

  if (_debug()) {
    if(result) print_error("DEBUG: Parsed the PAC file: %s\n", pacfile);
    else print_error("DEBUG: Could not parse the PAC file: %s\n", pacfile);
  }

  return result;
}

int
pacparser_parse_pac_file(const char *pacfile)
{
  lock_acquire();
  int ret = parse_pac_file_impl(pacfile);
  lock_release();
  return ret;
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
static char *                           // Proxy string or NULL if failed.
find_proxy_impl(const char *url, const char *host)
{
  char *error_prefix = "pacparser.c: pacparser_find_proxy:";
  if (_debug()) print_error("DEBUG: Finding proxy for URL: %s and Host:"
                        " %s\n", url, host);
  if (url == NULL || (strcmp(url, "") == 0)) {
    print_error("%s %s\n", error_prefix, "URL not defined");
    return NULL;
  }
  if (host == NULL || (strcmp(host,"") == 0)) {
    print_error("%s %s\n", error_prefix, "Host not defined");
    return NULL;
  }
  if (ctx == NULL) {
    print_error("%s %s\n", error_prefix, "Pac parser is not initialized.");
    return NULL;
  }

  // Free previous result if any
  if (proxy_result) {
    JS_FreeCString(ctx, proxy_result);
    proxy_result = NULL;
  }

  // Test if findProxyForURL is defined.
  const char *script = "typeof(findProxyForURL);";
  if (_debug()) print_error("DEBUG: Executing JavaScript: %s\n", script);
  JSValue check = JS_Eval(ctx, script, strlen(script), NULL, JS_EVAL_TYPE_GLOBAL);
  const char *type_str = JS_ToCString(ctx, check);
  int is_function = type_str && strcmp("function", type_str) == 0;
  JS_FreeCString(ctx, type_str);
  JS_FreeValue(ctx, check);
  if (!is_function) {
    print_error("%s %s\n", error_prefix,
		  "Javascript function findProxyForURL not defined.");
    return NULL;
  }

  // Get function and call it
  JSValue func = JS_GetPropertyStr(ctx, global, "findProxyForURL");
  JSValue args[2] = { JS_NewString(ctx, url), JS_NewString(ctx, host) };
  JSValue rval = JS_Call(ctx, func, global, 2, args);

  JS_FreeValue(ctx, args[0]);
  JS_FreeValue(ctx, args[1]);
  JS_FreeValue(ctx, func);

  if (JS_IsException(rval)) {
    dump_js_exception(ctx);
    print_error("%s %s\n", error_prefix, "Problem in executing findProxyForURL.");
    JS_FreeValue(ctx, rval);
    return NULL;
  }

  proxy_result = JS_ToCString(ctx, rval);
  JS_FreeValue(ctx, rval);
  return (char *)proxy_result;  // valid until next call or cleanup
}

char *
pacparser_find_proxy(const char *url, const char *host)
{
  lock_acquire();
  char *ret = find_proxy_impl(url, host);
  lock_release();
  return ret;
}

// Destroys JavaScript Engine.
static void
cleanup_impl(void)
{
  // Re-initialize config variables.
  my_ip_set = 0;

  if (proxy_result && ctx) {
    JS_FreeCString(ctx, proxy_result);
    proxy_result = NULL;
  }
  if (ctx) {
    JS_FreeValue(ctx, global);
    global = JS_UNDEFINED;
    JS_FreeContext(ctx);
    ctx = NULL;
  }
  if (rt) {
    JS_FreeRuntime(rt);
    rt = NULL;
  }
  if (_debug()) print_error("DEBUG: Pacparser destroyed.\n");
}

void
pacparser_cleanup(void)
{
  lock_acquire();
  cleanup_impl();
  lock_release();
}

// Finds proxy for the given PAC file, url and host.
//
// This function is a wrapper around functions pacparser_init,
// pacparser_parse_pac, pacparser_find_proxy and pacparser_cleanup. If you just
// want to find out proxy a given set of pac file, url and host, this is the
// function to call.
static char *                           // Proxy string or NULL if failed.
just_find_proxy_impl(const char *pacfile,
                     const char *url,
                     const char *host)
{
  char *proxy;
  char *out;
  int initialized_here = 0;
  char *error_prefix = "pacparser.c: pacparser_just_find_proxy:";
  if (!ctx) {
    if (!init_impl()) {
      print_error("%s %s\n", error_prefix, "Could not initialize pacparser");
      return NULL;
    }
    initialized_here = 1;
  }
  if (!parse_pac_file_impl(pacfile)) {
    print_error("%s %s %s\n", error_prefix, "Could not parse pacfile",
		  pacfile);
    if (initialized_here) cleanup_impl();
    return NULL;
  }
  if (!(out = find_proxy_impl(url, host))) {
    print_error("%s %s %s\n", error_prefix,
		  "Could not determine proxy for url", url);
    if (initialized_here) cleanup_impl();
    return NULL;
  }
  proxy = (char*) malloc(strlen(out) + 1);
  strcpy(proxy, out);
  if (initialized_here) cleanup_impl();
  return proxy;
}

char *
pacparser_just_find_proxy(const char *pacfile,
                         const char *url,
                         const char *host)
{
  lock_acquire();
  char *ret = just_find_proxy_impl(pacfile, url, host);
  lock_release();
  return ret;
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
