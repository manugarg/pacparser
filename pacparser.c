// Copyright (C) 2007 Manu Garg.
// Authors: Manu Garg <manugarg@gmail.com> (main author)
//          Stefano Lattarini <slattarini@gmail.com> (c-ares integration)
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

#include "pac_builtins.h"
#include "pacparser.h"
#include "pacparser_utils.h"

#ifdef XP_UNIX
#  include <arpa/inet.h>  // for inet_pton
#  include <sys/socket.h>  // for AF_INET
#  include <netdb.h>
#endif

#ifdef HAVE_C_ARES
#  include <sys/socket.h>
#  include <netinet/in.h>
#  include <arpa/nameser.h>
//#  include <arpa/nameser_compat.h>
#  include <ares.h>
#  include <ares_dns.h>
#endif

#ifdef _WIN32
#ifdef __MINGW32__
// MinGW enables definition of getaddrinfo et al only if WINVER >= 0x0501.
#define WINVER 0x0501
#endif
#include <winsock2.h>
#include <ws2tcpip.h>
#endif

#define ERR_FMT_BUFSIZ 1024
static char err_fmt_buffer[ERR_FMT_BUFSIZ];

struct dns_collector {
  char *mallocd_addresses;
  int all_ips;
  int ai_family;
};

enum collect_status {
  COLLECT_DONE = 0,
  COLLECT_MORE = 1
};

// To make code more readable.
#define ONE_IP 0
#define ALL_IPS 1

static char *myip = NULL;
static int enable_microsoft_extensions = 1;

typedef char *(*pacparser_resolve_host_func)(const char *, int all_ips);

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

// This assumes that prefix1 and prefix2 cannot contain conversion
// specification, and that they are never NULL. Since we control all
// the callers, we can be sure that's the case.
static int
print_error(const char *prefix1, const char *prefix2, const char *fmt, ...)
{
  int ret, err_fmt_size;
  va_list args;
  char *err_fmt = NULL;

  err_fmt_size = strlen(prefix1) + strlen(prefix2) + strlen(fmt) + 6;
  // We still want to print errors even if they are too big for the buffer.
  // In that case, we just sacrifice the prefixes and the trailing newline.
  if (err_fmt_size <= ERR_FMT_BUFSIZ) {
    strcpy(err_fmt_buffer, prefix1);
    strcat(err_fmt_buffer, ": ");
    strcat(err_fmt_buffer, prefix2);
    strcat(err_fmt_buffer, ": ");
    strcat(err_fmt_buffer, fmt);
    strcat(err_fmt_buffer, "\n");
    err_fmt_buffer[err_fmt_size] = '\0';
    err_fmt = err_fmt_buffer;
  }
  va_start(args, fmt);
  ret = (*error_printer_func)(err_fmt ? err_fmt : fmt, args);
  va_end(args);
  return ret;
}

#define print_err(...) \
    do { \
      print_error(__FILE__, __func__, __VA_ARGS__); \
    } while (0)

#define print_debug(...) \
    do { \
      if (getenv("PACPARSER_DEBUG")) \
        print_error("DEBUG: " __FILE__, __func__, __VA_ARGS__); \
    } while (0)

static char *
concat_strings(char *mallocd_str, const char *appended_str)
{
  if (appended_str == NULL)
    return mallocd_str;

  char *mallocd_result;
  int reallocd_size = strlen(mallocd_str) + strlen(appended_str) + 1;
  if ((mallocd_result = realloc(mallocd_str, reallocd_size)) == NULL)
    return NULL;
  return strcat(mallocd_result, appended_str);
}

// You must free the result if result is non-NULL.
static char *
str_replace(const char *orig, char *rep, char *with)
{
  char *copy = strdup(orig);

  char *result;  // the returned string
  char *ins;     // the next insert point
  char *tmp;     // varies
  int count;     // number of replacements
  int len_front; // distance between rep and end of last rep
  int len_rep  = strlen(rep);
  int len_with = strlen(with);

  // Get the count of replacements
  ins = copy;
  for (count = 0; (tmp = strstr(ins, rep)); ++count) {
    ins = tmp + len_rep;
  }

  tmp = result = malloc(strlen(copy) + (len_with - len_rep) * count + 1);

  // First time through the loop, all the variable are set correctly
  // from here on,
  //    tmp points to the end of the result string
  //    ins points to the next occurrence of rep in copy
  //    copy points to the remainder of copy after "end of rep"
  while (count--) {
      ins = strstr(copy, rep);
      len_front = ins - copy;
      tmp = strncpy(tmp, copy, len_front) + len_front;
      tmp = strcpy(tmp, with) + len_with;
      copy += len_front + len_rep;  // move to next "end of rep"
  }
  strcpy(tmp, copy);
  free(copy);
  return result;
}

// Utility function to read a file into string.
// Returns a malloc'ed string containing the file content on success,
// NULL on error.
static char *
read_file_into_str(const char *filename)
{
  char *str = NULL;
  int file_size;
  FILE *fptr;

  if (!(fptr = fopen(filename, "r")))
    return NULL;

  if (fseek(fptr, 0L, SEEK_END) != 0)
    goto close_and_return;
  if ((file_size = ftell(fptr)) < 0)
    goto close_and_return;
  if (fseek(fptr, 0L, SEEK_SET) != 0)
    goto close_and_return;
  if ((str = calloc(file_size + 1, sizeof(char))) == NULL)
    goto close_and_return;

  // 'str' is no longer NULL if we are here.
  if (!fread(str, sizeof(char), file_size, fptr) && ferror(fptr)) {
    free(str);
    str = NULL;
  }

close_and_return:
  fclose(fptr);
  return str;
}

static void
print_jserror(JSContext *cx, const char *message, JSErrorReport *report)
{
  print_error("JSERROR", (report->filename ? report->filename : "NULL"),
              "%d:\n    %s", report->lineno, message);
}

//------------------------------------------------------------------------------

static enum collect_status
collect_mallocd_address(struct dns_collector *dc, const char *addr_buf)
{
  if (dc->all_ips) {
    if (dc->mallocd_addresses == NULL) {
      dc->mallocd_addresses = strdup(addr_buf);
    } else {
      dc->mallocd_addresses = concat_strings(dc->mallocd_addresses, ";");
      dc->mallocd_addresses = concat_strings(dc->mallocd_addresses, addr_buf);
    }
    return COLLECT_MORE;  // it's ok to run again and get more results
  } else {
    dc->mallocd_addresses = strdup(addr_buf);
    return COLLECT_DONE;  // we only need and want one result
  }
}

//------------------------------------------------------------------------------

// DNS "resolution" of literal IPs only.

static int
is_ip_address(const char *str)
{
  char ipaddr4[INET_ADDRSTRLEN], ipaddr6[INET6_ADDRSTRLEN];
  return (inet_pton(AF_INET, str, &ipaddr4) > 0 ||
          inet_pton(AF_INET6, str, &ipaddr6) > 0);
}

char *
pacparser_resolve_host_literal_ips(const char *hostname, int all_ips)
{
  (void) all_ips;  // shut up linter
  return is_ip_address(hostname) ? strdup(hostname) : NULL;
}

//------------------------------------------------------------------------------

// DNS resolutions via the getaddrinfo(3) function.

static void
collect_getaddrinfo_results(struct dns_collector *dc, struct addrinfo *ai)
{
  for (; ai != NULL; ai = ai->ai_next) {
    char addr_buf[INET6_ADDRSTRLEN];  // large enough for IPv4 and IPv6 alike
    getnameinfo(ai->ai_addr, ai->ai_addrlen, addr_buf, sizeof(addr_buf),
                NULL, 0, NI_NUMERICHOST);
    if (collect_mallocd_address(dc, addr_buf) == COLLECT_DONE)
      return;
  }
}

static char *
pacparser_resolve_host_getaddrinfo(const char *hostname, int all_ips)
{
  struct dns_collector dc;
  dc.all_ips = all_ips;
  dc.mallocd_addresses = NULL;

  struct addrinfo hints, *ai;
  memset(&hints, 0, sizeof(struct addrinfo));
  hints.ai_socktype = SOCK_STREAM;

#ifdef _WIN32
  // On windows, we need to initialize the winsock dll first.
  WSADATA WsaData;
  WSAStartup(MAKEWORD(2,0), &WsaData);
#endif

  int i, ai_families[] = {AF_INET, AF_INET6};
  for (i = 0; i < 2; i++) {
    dc.ai_family = hints.ai_family = ai_families[i];
    if (getaddrinfo(hostname, NULL, &hints, &ai) == 0)
      collect_getaddrinfo_results(&dc, ai);
    if (!all_ips && dc.mallocd_addresses)
      break;
  }

#ifdef _WIN32
  WSACleanup();
#endif
  return dc.mallocd_addresses;
}

//------------------------------------------------------------------------------

// DNS resolutions via the feature-rich c-ares third party library.

static int ares_initialized = 0;

#ifdef HAVE_C_ARES

static char *dns_servers = NULL;
static char *dns_domains = NULL;

static ares_channel global_channel;

// Use a custom DNS server (specified by IP), instead of relying on the
// "nameserver" directive in /etc/resolv.conf.
int
pacparser_set_dns_servers(const char *ips)
{
  if (ips == NULL)
    return 1;  // noop
  if (ares_initialized) {
    print_err("Cannot change DNS servers now; this function should be called "
              "before c-ares is initialized (typically by pacparser_init).");
    return 0;
  }
  if ((dns_servers = strdup(ips)) == NULL) {
    print_err("Could not allocate memory for the servers.");
    return 0;
  }
  return 1;
}

// Use a custom list of domains, instead of relying on, e.g., the
// "search" directive in /etc/resolv.conf.
int
pacparser_set_dns_domains(const char *domains)
{
  if (domains == NULL)
    return 1;  // noop
  if (ares_initialized) {
    print_err("Cannot change DNS search domains now; this function should "
              "be called before c-ares is initialized (typically by "
              "pacparser_init).");
    return 0;
  }
  if ((dns_domains = strdup(domains)) == NULL) {
    print_err("Could not allocate memory for the domains.");
    return 0;
  }
  return 1;
}

static void
callback_for_ares(void *arg, int status, int timeouts, struct hostent *host)
{
  (void) timeouts;  // unused

  // Sigh. But the callback must have a void* as first argument, so we
  // actually need this little abomination. Sorry.
  struct dns_collector *dc = (struct dns_collector *) arg;

  if (status != ARES_SUCCESS || host->h_addrtype != dc->ai_family)
    return;

  char **s;
  for (s = host->h_addr_list; *s; s++) {
    char addr_buf[INET6_ADDRSTRLEN];  // large enough for IPv4 and IPv6 alike
    ares_inet_ntop(dc->ai_family, *s, addr_buf, sizeof(addr_buf));
    if (collect_mallocd_address(dc, addr_buf) == COLLECT_DONE)
      return;
  }
}

// Shamelessly copied from ares_process(3)
static int
ares_wait_for_all_queries(ares_channel channel)
{
   int nfds, count;
   fd_set readers, writers;
   struct timeval tv, *tvp;

   while (1) {
     FD_ZERO(&readers);
     FD_ZERO(&writers);
     nfds = ares_fds(channel, &readers, &writers);
     if (nfds == 0)
       break;
     tvp = ares_timeout(channel, NULL, &tv);
     count = select(nfds, &readers, &writers, NULL, tvp);
     if (count < 0 && errno != EINVAL) {
       print_err("select() failed: %s", strerror(errno));
       return 0;
     }
     ares_process(channel, &readers, &writers);
   }
   return 1;
}

static int
pacparser_ares_init(void)
{
  char **domains_list = NULL;
  int optmask = ARES_OPT_FLAGS;
  struct ares_options options;
  options.flags = ARES_FLAG_NOCHECKRESP;

#define FREE_AND_RETURN(retval) \
  do { \
    free(domains_list); \
    return (retval); \
  } while(0)

  if (ares_library_init(ARES_LIB_INIT_ALL) != ARES_SUCCESS) {
    print_err("Could not initialize the c-ares library.");
    FREE_AND_RETURN(0);
  }

  if (dns_domains) {
    int i = 0;
    char *p, *sp;
    p = strtok_r(dns_domains, ",", &sp);
    while (p != NULL) {
      domains_list = realloc(domains_list, (i + 2) * sizeof(char **));
      if (domains_list == NULL) {
        print_err("Could not allocate memory for domains list.");
        FREE_AND_RETURN(0);
      }
      domains_list[i++] = p;
      p = strtok_r(NULL, ",", &sp);
    }
    if (domains_list)
      domains_list[i] = NULL;
    optmask |= ARES_OPT_DOMAINS;
    options.domains = domains_list;
    options.ndomains = i;
  }

  if (ares_init_options(&global_channel, &options, optmask) != ARES_SUCCESS) {
    print_err("Could not initialize c-ares options.");
    FREE_AND_RETURN(0);
  }

  if (dns_servers) {
    if (ares_set_servers_csv(global_channel, dns_servers) != ARES_SUCCESS) {
      print_err("Could not set c-ares DNS servers.");
      FREE_AND_RETURN(0);
    }
  }

  ares_initialized = 1;
  FREE_AND_RETURN(1);

#undef FREE_AND_RETURN
}

static void
pacparser_ares_cleanup(void)
{
  if (ares_initialized) {
    // These functions return void, so no error checking is possible here.
    ares_destroy(global_channel);
    ares_library_cleanup();
  }
  ares_initialized = 0;
  free(dns_domains);
  free(dns_servers);
}

static char *
pacparser_resolve_host_ares(const char *hostname, int all_ips)
{
  if (hostname == NULL) {
    return strdup("");
  }

  struct dns_collector dc;
  dc.all_ips = all_ips;
  dc.mallocd_addresses = NULL;

  int i, ai_families[] = {AF_INET, AF_INET6};
  for (i = 0; i < 2; i++) {
    dc.ai_family = ai_families[i];
    ares_gethostbyname(global_channel, hostname, ai_families[i],
                       callback_for_ares, (void *) &dc);
    if (!ares_wait_for_all_queries(global_channel)) {
      print_err("Some c-ares queries did not complete successfully.");
      return NULL;
    }
    if (!all_ips && dc.mallocd_addresses)
      return dc.mallocd_addresses;
  }
  return dc.mallocd_addresses;
}

# else // !HAVE_C_ARES

#define pacparser_no_c_ares() \
    print_err("requires c-ares integration to be compiled-in.")

// Dummy fallbacks for when c-ares is not available.
//
// Some of these functions shoud never be called when c-ares integration is
// disabled (and will complain if they are), some should just work as no-op
// (to make integration into the callers easier).

static char *
pacparser_resolve_host_ares(const char *hostname, int all_ips)
{
  pacparser_no_c_ares();
  return NULL;
}

int
pacparser_set_dns_servers(const char *ips)
{
  if (ips == NULL)
    return 1;  // noop
  pacparser_no_c_ares();
  return 0;
}

int
pacparser_set_dns_domains(const char *domains)
{
  if (domains == NULL)
    return 1;  // noop
  pacparser_no_c_ares();
  return 0;
}

static int
pacparser_ares_init(void)
{
  ares_initialized = 1;
  return 1;
}

static void
pacparser_ares_cleanup(void)
{
  ares_initialized = 0;
}

#endif // !HAVE_C_ARES

//------------------------------------------------------------------------------

// By default, we want to use getaddrinfo to do DNS resolution.
static pacparser_resolve_host_func dns_resolver = (
    &pacparser_resolve_host_getaddrinfo);

static char *
pacparser_get_my_ip_address(pacparser_resolve_host_func resolve_host_func,
                            int all_ips)
{
  char *ipaddr;
  // According to the gethostname(2) manpage, SUSv2  guarantees that
  // "Host names are limited to 255 bytes".
  char name[256];
  if (gethostname(name, sizeof(name)) < 0 ||
      (ipaddr = resolve_host_func(name, all_ips)) == NULL) {
    ipaddr = strdup("127.0.0.1");
  }
  return ipaddr;
}

void
pacparser_setmyip(const char *ip)
{
  free(myip);
  myip = strdup(ip);
}

int
pacparser_set_dns_resolver_variant(const char *dns_resolver_variant)
{
  if STREQ(dns_resolver_variant, DNS_NONE) {
    dns_resolver = &pacparser_resolve_host_literal_ips;
    return 1;
  } else if STREQ(dns_resolver_variant, DNS_GETADDRINFO) {
    dns_resolver = &pacparser_resolve_host_getaddrinfo;
    return 1;
  } else if STREQ(dns_resolver_variant, DNS_C_ARES) {
#ifdef HAVE_C_ARES
    dns_resolver = &pacparser_resolve_host_ares;
    return 1;
#else
    print_err("cannot use c-ares as DNS resolver: was not available "
              "at compile time.");
    return 0;
#endif
  } else {
    print_err("invalid DNS resolver variant \"%s\"", dns_resolver_variant);
    return 0;
  }
}

// dnsResolve/dnsResolveEx in JS context; not available in core JavaScript.
// Return javascript null if not able to resolve.

static JSBool
dns_resolve_js_internals(JSContext *cx, JSObject *obj, uintN argc, jsval *argv,
                         jsval *rval, int all_ips)
{
  char *name = JS_GetStringBytes(JS_ValueToString(cx, argv[0]));
  char *ipaddr, *out;

  if ((ipaddr = dns_resolver(name, all_ips)) == NULL) {
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
dns_resolve_js(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
  return dns_resolve_js_internals(cx, obj, argc, argv, rval, ONE_IP);
}

static JSBool
dns_resolve_ex_js(JSContext *cx, JSObject *obj, uintN argc, jsval *argv,
                  jsval *rval)
{
  return dns_resolve_js_internals(cx, obj, argc, argv, rval, ALL_IPS);
}

// myIpAddress/myIpAddressEx in JS context; not available in core JavaScript.
// Return 127.0.0.1 if not able to determine local ip.

static JSBool
my_ip_address_js_internals(JSContext *cx, JSObject *obj, uintN argc,
                           jsval *argv, jsval *rval, int all_ips)
{
  char *ipaddr, *out;

  if (myip) {
    // If "my" (client's) IP address is already set.
    ipaddr = strdup(myip);
  } else {
    ipaddr = pacparser_get_my_ip_address(dns_resolver, all_ips);
  }

  out = JS_strdup(cx, ipaddr);
  free(ipaddr);
  JSString *str = JS_NewString(cx, out, strlen(out));
  *rval = STRING_TO_JSVAL(str);
  return JS_TRUE;
}

static JSBool
my_ip_address_js(JSContext *cx, JSObject *obj, uintN argc, jsval *argv,
                 jsval *rval)
{
  return my_ip_address_js_internals(cx, obj, argc, argv, rval, ONE_IP);
}

static JSBool
my_ip_address_ex_js(JSContext *cx, JSObject *obj, uintN argc, jsval *argv,
                    jsval *rval)
{
  return my_ip_address_js_internals(cx, obj, argc, argv, rval, ALL_IPS);
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
    print_err("cannot enable or disable microsoft extensions now; "
              "can only be done before calling pacparser_init().");
    return;
  }
  enable_microsoft_extensions = setting;
}

void
pacparser_enable_microsoft_extensions(void)
{
  pacparser_set_microsoft_extensions(1);
}

void
pacparser_disable_microsoft_extensions(void)
{
  pacparser_set_microsoft_extensions(0);
}

// Initialize PAC parser.
//
// - Initialize the c-ares library (if needed).
// - Initializes JavaScript engine.
// - Exports dns_functions (defined above) to JavaScript context.
// - Sets error reporting function to print_jserror.
// - Evaluates JavaScript code in pacUtils variable defined in pac_builtins.h.
//
// Return 0 on failure, 1 on success.
int
pacparser_init()
{
  jsval rval;

  if (!pacparser_ares_init()) {
    print_err("Could not initialize c-ares DNS library.");
    return 0;
  }

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
    print_err("Could not initialize JavaScript runtime.");
    return 0;
  }
  JS_SetErrorReporter(cx, print_jserror);
  // Export our functions to Javascript engine
  if (!JS_DefineFunction(cx, global, "dnsResolve", dns_resolve_js,
                         1, 0)) {
    print_err("Could not define dnsResolve in JS context.");
    return 0;
  }
  if (!JS_DefineFunction(cx, global, "myIpAddress", my_ip_address_js,
                         0, 0)) {
    print_err("Could not define myIpAddress in JS context.");
    return 0;
  }
  if (enable_microsoft_extensions) {
    if (!JS_DefineFunction(cx, global, "dnsResolveEx", dns_resolve_ex_js,
                           1, 0)) {
      print_err("Could not define dnsResolveEx in JS context.");
      return 0;
    }
    if (!JS_DefineFunction(cx, global, "myIpAddressEx", my_ip_address_ex_js,
                           0, 0)) {
      print_err("Could not define myIpAddressEx in JS context.");
      return 0;
    }
  }

  // Evaluate pac_builtins -- code defining utility functions required to
  // parse PAC files.
  if (!JS_EvaluateScript(cx,           // JS engine context
                         global,       // global object
                         pac_builtins, // this is defined in pac_builtins.h
                         strlen(pac_builtins),
                         NULL,         // filename (NULL in this case)
                         1,            // line number, used for reporting
                         &rval)) {
    print_err("Could not evaluate pac_builtins defined in pac_builtins.h");
    return 0;
  }

  if (enable_microsoft_extensions) {
    // Evaluate pac_builtins_ex -- code defining extensions to the utility
    // functions defined above.
    if (!JS_EvaluateScript(cx,              // JS engine context
                           global,          // global object
                           pac_builtins_ex, // this is defined in pac_builtins.h
                           strlen(pac_builtins_ex),
                           NULL,            // filename (NULL in this case)
                           1,               // line number, used for reporting
                           &rval)) {
      print_err("Could not evaluate pac_builtins_ex defined in pac_builtins.h");
      return 0;
    }
  }

  print_debug("Pacparser Initalized.");
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
  if (cx == NULL || global == NULL) {
    print_err("Pac parser is not initialized.");
    return 0;
  }
  if (!JS_EvaluateScript(cx,
                         global,
                         script,       // Script read from pacfile
                         strlen(script),
                         "PAC script",
                         1,
                         &rval)) {     // If script evaluation failed
    print_err("Failed to evaluate the pac script.");
    print_debug("Failed to parse the PAC script:\n%s", script);
    return 0;
  }
  print_debug("Parsed the PAC script.");
  return 1;
}

// Parses the given PAC file.
//
// Reads the given PAC file and evaluates it in the JavaScript context
// created by pacparser_init.
//
// Return 0 on failure and 1 on succcess.
int
pacparser_parse_pac_file(const char *pacfile)
{
  char *script = NULL;

  if ((script = read_file_into_str(pacfile)) == NULL) {
    print_err("Could not read the pacfile %s: %s", pacfile, strerror(errno));
    return 0;
  }

  int result = pacparser_parse_pac_string(script);
  free(script);

  if (result)
    print_debug("Parsed the PAC file: %s", pacfile);
  else
    print_debug("Could not parse the PAC file: %s", pacfile);

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
  char *script, *retval;
  jsval rval;

  print_debug("Finding proxy for URL '%s' and Host '%s'.", url, host);

  if (url == NULL || STREQ(url, "")) {
    print_err("URL not defined");
    return NULL;
  }
  if (host == NULL || STREQ(host,"")) {
    print_err("Host not defined");
    return NULL;
  }
  if (cx == NULL || global == NULL) {
    print_err("Pac parser is not initialized.");
    return NULL;
  }

  // Test if findProxyForURL is defined.
  script = "typeof(findProxyForURL);";
  print_debug("Executing JavaScript: %s", script);
  JS_EvaluateScript(cx, global, script, strlen(script), NULL, 1, &rval);
  if (!STREQ("function", JS_GetStringBytes(JS_ValueToString(cx, rval)))) {
    print_err("Javascript function findProxyForURL not defined.");
    return NULL;
  }

  // URL-encode "'" as we use single quotes to stick the URL into a
  // temporary script.
  char *sanitized_url = str_replace(url, "'", "%27");
  // Hostname shouldn't have single quotes in them.
  if (strchr(host, '\'')) {
    print_err("Invalid hostname: hostname can't have single quotes.");
    return NULL;
  }

  script = malloc(32 + strlen(url) + strlen(host));
  script[0] = '\0';
  strcat(script, "findProxyForURL('");
  strcat(script, sanitized_url);
  strcat(script, "', '");
  strcat(script, host);
  strcat(script, "')");
  print_debug("Executing JavaScript: %s", script);
  if (!JS_EvaluateScript(cx, global, script, strlen(script), NULL, 1, &rval)) {
    print_err("Problem in executing findProxyForURL.");
    retval = NULL;
  } else {
    retval = JS_GetStringBytes(JS_ValueToString(cx, rval));
  }
  free(sanitized_url);
  free(script);
  return retval;
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
  pacparser_ares_cleanup();
  enable_microsoft_extensions = 1;
  print_debug("Pacparser destroyed.");
}

// Finds proxy for the given PAC file, url and host.
//
// This function is a wrapper around functions pacparser_init,
// pacparser_parse_pac, pacparser_find_proxy and pacparser_cleanup. If you just
// want to find out proxy a given set of pac file, url and host, this is the
// function to call.
//
// Returns the proxy string on success, NULL on failure.
char *
pacparser_just_find_proxy(const char *pacfile, const char *url,
                          const char *host)
{
  char *proxy;
  char *out;
  int initialized_here = 0;
  if (!global) {
    if (!pacparser_init()) {
      print_err("Could not initialize pacparser.");
      return NULL;
    }
    initialized_here = 1;
  }
  if (!pacparser_parse_pac(pacfile)) {
    print_err("Could not parse pacfile %s.", pacfile);
    if (initialized_here)
      pacparser_cleanup();
    return NULL;
  }
  if (!(out = pacparser_find_proxy(url, host))) {
    print_err("Could not determine proxy for url '%s'.", url);
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
  print_error("WARNING", __func__, "VERSION not defined.");
  return "";
#endif
  return QUOTEME(VERSION);
}
