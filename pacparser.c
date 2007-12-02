// Copyright (C) 2007 Manu Garg.
// Author: Manu Garg <manugarg@gmail.com>
//
// pacparser is a C library that provides methods to parse proxy auto-config
// (PAC) files. Please read README file included with this package for more
// information about this library.
//
// pacparser is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 2.1 of the License, or (at your option) any later version.

// pacparser is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Lesser General Public License for more details.

// You should have received a copy of the GNU Lesser General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA

#include <jsapi.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#ifdef XP_UNIX
#include <unistd.h>
#include <sys/socket.h>                // for AF_INET
#include <arpa/inet.h>
#include <netdb.h>
#endif

#ifdef XP_WIN
#include <winsock2.h>
#include <ws2tcpip.h>
// wspiapi.h is needed because ws2tcpip.h that comes with mingw doesn't have
// complete definition of getnameinfo.
#define _inline __inline
#include <wspiapi.h>
#undef _inline
#endif

#include "pac_utils.h"

// inet_ntop is not defined on iindows. Define it as a wrapper to functions
// available on windows.
#ifdef _WIN32
const char *
inet_ntop(int af, const void *src, char *dst, socklen_t cnt) {
  if (af == AF_INET) {
    struct sockaddr_in in;
    memset(&in, 0, sizeof(in));
    in.sin_family = AF_INET;
    memcpy(&in.sin_addr, src, sizeof(struct in_addr));
    getnameinfo((struct sockaddr *)&in, sizeof(struct sockaddr_in), dst, cnt,
                NULL, 0, NI_NUMERICHOST);
    return dst;
  }
  else if (af == AF_INET6) {
    struct sockaddr_in6 in;
    memset(&in, 0, sizeof(in));
    in.sin6_family = AF_INET6;
    memcpy(&in.sin6_addr, src, sizeof(struct in_addr6));
    getnameinfo((struct sockaddr *)&in, sizeof(struct sockaddr_in6), dst, cnt,
                NULL, 0, NI_NUMERICHOST);
    return dst;
  }
  return NULL;
}
#endif

// Utility function to read a file into string.
char *                              // File content in string or NULL if failed.
read_file_into_str(const char *filename) {
  char *str;
  int file_size;
  FILE *fptr;
  if (!(fptr = fopen(filename, "r"))) goto error1;
  if ((fseek(fptr, 0L, SEEK_END) != 0)) goto error2;
  if (!(file_size=ftell(fptr))) goto error2;
  if ((fseek(fptr, 0L, SEEK_SET) != 0)) goto error2;
  if (!(str = (char*) malloc(file_size+1))) goto error2;
  if (fread(str, file_size, 1, fptr) != 1) {
    free(str);
    goto error2;
  }
  str[file_size] = '\0';
  fclose(fptr);
  return str;
error2:
  fclose(fptr);
error1:
  return NULL;
}

void
print_error(JSContext *cx, const char *message, JSErrorReport *report) {
    fprintf(stderr, "JSERROR: %s:%d:\n    %s\n",
            (report->filename ? report->filename : "NULL"), report->lineno,
            message);
}

// Define DNS Resolve function; not available in JavaScript.
JSBool                                  // JS_TRUE or JS_FALSE
dns_resolve(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval) {
  char* name = JS_GetStringBytes(JS_ValueToString(cx, argv[0]));
  char* out;
  struct hostent *hent;
  char ipaddr[INET_ADDRSTRLEN];
  if ((hent = gethostbyname(name)) == NULL) {
    *rval = JSVAL_NULL;
    return JS_TRUE;
  }
  inet_ntop(hent->h_addrtype, hent->h_addr_list[0], ipaddr, sizeof(ipaddr));
  out = JS_malloc(cx, strlen(ipaddr) + 1);
  strcpy(out, ipaddr);
  JSString *str = JS_NewString(cx, out, strlen(out));
  *rval = STRING_TO_JSVAL(str);
  return JS_TRUE;
}

// Define myIpAddress function; not available in JavaScript.
JSBool                                  // JS_TRUE or JS_FALSE
my_ip(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval) {
  char name[256];
  gethostname(name, sizeof(name));
  char* out;
  struct hostent *hent;
  char ipaddr[INET_ADDRSTRLEN];
  if ((hent = gethostbyname(name)) == NULL)
    strcpy(ipaddr, "127.0.0.1");
  else
    inet_ntop(hent->h_addrtype, hent->h_addr_list[0], ipaddr, sizeof(ipaddr));

  out = JS_malloc(cx, strlen(ipaddr) + 1);
  strcpy(out, ipaddr);
  JSString *str = JS_NewString(cx, out, strlen(out));
  *rval = STRING_TO_JSVAL(str);
  return JS_TRUE;
}

JSFunctionSpec dns_functions[] = {
  {"dnsResolve", dns_resolve, 1},
  {"myIpAddress", my_ip, 0}
};

JSRuntime *rt = NULL;
JSContext *cx = NULL;
JSObject *global = NULL;
JSClass global_class = {
    "global",0,
    JS_PropertyStub,JS_PropertyStub,JS_PropertyStub,JS_PropertyStub,
    JS_EnumerateStub,JS_ResolveStub,JS_ConvertStub,JS_FinalizeStub
};

int                                     // 0 (=Failure) or 1 (=Success)
init_pac_parser() {
  jsval rval;
  // Initialize JS engine
  if (!(rt = JS_NewRuntime(8L * 1024L * 1024L)) ||
      !(cx = JS_NewContext(rt, 8192)) ||
      !(global = JS_NewObject(cx, &global_class, NULL, NULL)) ||
      !JS_InitStandardClasses(cx, global))
    return 0;
  JS_SetErrorReporter(cx, print_error);
  // Export our functions to Javascript engine
  if (!JS_DefineFunctions(cx, global, dns_functions))
    return 0;
  // Evaluate pacUtils. Utility functions required to parse pac files.
  if (!JS_EvaluateScript(cx,           // JS engine context
                         global,       // global object
                         pacUtils,     // this is defined in pac_utils.h
                         strlen(pacUtils),
                         NULL,         // filename (NULL in this case)
                         1,            // line number, used for reporting.
                         &rval))
    return 0;
  return 1;
}

int                                     // 0 (=Failure) or 1 (=Success)
parse_pac(const char* pacfile) {
  jsval rval;
  char *script = NULL;
  if ((script = read_file_into_str(pacfile)) == NULL) {
    fprintf(stderr, "libpacparser.so: parse_pac: %s %s\n", "Could not read "
            "pacfile", pacfile);
    return 0;
  }
  if (cx == NULL || global == NULL) {
    fprintf(stderr, "libpacparser.so: parse_pac: %s\n", "Pac parser is "
            "not initialized.");
    return 0;
  }
  if (!JS_EvaluateScript(cx,
                         global,
                         script,       // Script read from pacfile
                         strlen(script),
                         pacfile,
                         1,
                         &rval)) {     // If script evaluation failed
    if (script != NULL) free(script);
    return 0;
  }
  if (script != NULL) free(script);
  return 1;
}

char *                                  // Proxy string or NULL if failed.
find_proxy_for_url(const char *url, const char *host) {
  jsval rval;
  char *script;
  if (url == NULL || (strcmp(url, "") == 0)) {
    fprintf(stderr, "libpacparser.so: find_proxy_for_url: %s\n", "URL not "
            "defined");
    return NULL;
  }
  if (host == NULL || (strcmp(host,"") == 0)) {
    fprintf(stderr, "libpacparser.so: find_proxy_for_url: %s\n", "Host not "
            "defined");
    return NULL;
  }
  if (cx == NULL || global == NULL) {
    fprintf(stderr, "libpacparser.so: find_proxy_for_url: %s\n",
            "Pac parser is not initialized.");
    return NULL;
  }
  // Test if FindProxyForURL is defined.
  script = "typeof(FindProxyForURL);";
  if ( !JS_EvaluateScript(cx, global, script, strlen(script), NULL, 1, &rval) )
    return NULL;
  if ( strcmp("function", JS_GetStringBytes(JS_ValueToString(cx, rval))) != 0 ) {
    fprintf(stderr, "libpacparser.so: find_proxy_for_url: %s\n", "Javascript"
            " function FindProxyForURL not defined.");
    return NULL;
  }
  script = (char*) malloc(32 + strlen(url) + strlen(host));
  script[0] = '\0';
  strcat(script, "FindProxyForURL('");
  strcat(script, url);
  strcat(script, "', '");
  strcat(script, host);
  strcat(script, "')");
  if (!JS_EvaluateScript(cx, global, script, strlen(script), NULL, 1, &rval))
    return NULL;
  return JS_GetStringBytes(JS_ValueToString(cx, rval));
}

void
destroy_pac_parser () {
  if (cx) {
    JS_DestroyContext(cx);
    cx = NULL;
  }
  if (rt) {
    JS_DestroyRuntime(rt);
    rt = NULL;
  }
  if (!cx && !rt) JS_ShutDown();
}

char *                                  // Proxy string or NULL if failed.
find_proxy (const char *pacfile, const char *url, const char *host) {
  char *proxy;
  char *out;
  int initialized_here = 0;
  if (!global) {
    if (!init_pac_parser()) {
      fprintf(stderr, "libpacparser.so: find_proxy: %s\n",
              "Could not initialize pac parser");
      return NULL;
    }
    initialized_here = 1;
  }
  if (!parse_pac(pacfile)) {
    fprintf(stderr, "libpacparser.so: find_proxy: %s %s\n",
            "Could not parse pacfile", pacfile);
    if (initialized_here) destroy_pac_parser();
    return NULL;
  }
  if (!(out = find_proxy_for_url(url, host))) {
    fprintf(stderr, "libpacparser.so: find_proxy: %s %s\n",
            "Could not determine proxy for url", url);
    if (initialized_here) destroy_pac_parser();
    return NULL;
  }
  proxy = (char*) malloc(strlen(out) + 1);
  strcpy(proxy, out);
  if (initialized_here) destroy_pac_parser();
  return proxy;
}
