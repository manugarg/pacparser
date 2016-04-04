// Copyright (C) 2008 Manu Garg.
// Author: Manu Garg <manugarg@gmail.com>
//
// This file implements pactester using pacparser.
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

#include "util.h"
#include "pacparser.h"
#include "pacparser_dns.h"

#define LINEMAX 4096  // Max length of any line read from text files (4 KiB)
#define PACMAX (1024 * 1024)  // Max size of the PAC script (1 MiB)
#define DOMAINMAX 32  // Max number of domains passed via option '-d'

void
usage(const char *progname)
{
  const static char *common_args =
      "-p PAC-FILE [-e|-E] [-h HOST] [-c MY_IP] [-r DNS_RESOLVER_TYPE] "
      "[-s DNS_SERVER_IP] [-d DNS_DOMAIN_LIST]";

  fprintf(stderr, "\n"
"Usage:\n"
"  %s -u URL %s\n"
"or:\n"
"  %s -f URL-LIST %s\n"
"\n"
"Global flags:\n"
"  -p pacfile   : PAC file to test (specify '-' to read from standard input).\n"
"  -u url       : URL to test for.\n"
"  -h host      : Host part of the URL.\n"
"  -r dns_type  : the DNS resolver variant; valid values are:\n"
"                 * none: DNS resolution attempts will all return the\n"
"                     'null' JavaScript object (but literal ip addresses will\n"
"                     still be recognized and successfully \"resolved\");\n"
"                 * getaddrinfo: DNS resolution will use the getaddrinfo(3)\n"
"                     function from the system C library;\n"
"                 * c-ares: DNS resolution will use the c-ares library; if\n"
"                     that was not available at compile time, an error will\n"
"                     be returned at setup time.\n"
"  -d domains   : comma-separated list of domains to search instead of the\n"
"                 domains specified in resolv.conf or the domain derived\n"
"                 from the kernel hostname variable; this can be specified\n"
"                 only when the DNS resolver type is \"c-ares\".\n"
"  -s servers   : comma-separated IP addresses of the DNS servers to use for\n"
"                 DNS lookups, instead of the ones specified in resolv.conf;\n"
"                 this can be specified only when the DNS resolver type is\n"
"                 \"c-ares\".\n"
"  -c client_ip : client IP address (as returned by myIpAddres() function in\n"
"                 PAC files), defaults to IP address on which it is running\n"
"  -E           : disable microsoft extensions (*Ex functions)\n"
"  -e           : enable microsoft extensions (*Ex functions); notice that\n"
"                 those extensions are enabled by default\n"
"  -f urlsfile  : a file containing list of URLs to be tested\n"
"  -v           : print version and exit\n",
    progname, common_args, progname, common_args);
  exit(1);
}

char *
get_host_from_url(const char *url)
{
  // Copy url to a (modifiable) buffer that we'll use to seek through
  // the string.
  char *p = strdup(url);

  // Move to first ':'
  while (*p != ':' && *p != '\0')
    p++;
  if (p[0] == '\0')
    // We reached end without hitting ':'
    goto not_a_proper_url;
  if (p[1] != '/' || p[2] != '/')
    // Next two characters are not '//'
    goto not_a_proper_url;
  p += 3;  // get past '://'
  // Host part starts from here.
  char *host = p;
  if (*p == '\0' || *p == '/' || *p == ':')
    // If host part is null.
    goto not_a_proper_url;
  if (*p == '[') {
    // Expect a bracketed IPv6 address, such as in the URL http://[::1]
    while (*p != ']' && *p != '\0')
      p++;
    if (!*p)
      // Never saw the closing bracket.
      goto not_a_proper_url;
  }
  // Seek until next '/', ':' or end of string.
  while (*p != '/' && *p != ':' && *p != '\0')
    p++;
  *p = '\0';
  return host;

not_a_proper_url:
  fprintf(stderr, "pactester.c: Not a proper URL: %s\n", url);
  return NULL;
}

int
main(int argc, char* argv[])
{
  const char *pacfile = NULL, *host = NULL, *url = NULL, *urlsfile = NULL,
             *client_ip = NULL, *dns_servers = NULL, *dns_domains = NULL,
             *dns_resolver_variant = DNS_GETADDRINFO;

  int enable_microsoft_extensions = 1;

  if (argv[1] && (STREQ(argv[1], "--help") || STREQ(argv[1], "--helpshort"))) {
    usage(argv[0]);
  }

  signed char c;
  while ((c = getopt(argc, argv, "eEvp:u:h:f:c:s:d:r:")) != -1)
    switch (c)
    {
      case 'v':
        printf("%s\n", pacparser_version());
        return 0;
      case 'p':
        pacfile = optarg;
        break;
      case 'u':
        url = optarg;
        break;
      case 'h':
        host = optarg;
        break;
      case 'f':
        urlsfile = optarg;
        break;
      case 'c':
        client_ip = optarg;
        break;
      case 's':
        dns_servers = optarg;
        break;
      case 'd':
        dns_domains = optarg;
        break;
      case 'r':
        dns_resolver_variant = optarg;
        break;
     case 'e':
        enable_microsoft_extensions = 1;
        break;
     case 'E':
        enable_microsoft_extensions = 0;
        break;
      case '?':
        usage(argv[0]);
        /* fallthrough */
      default:
        abort ();
    }

  if (!pacfile) {
    fprintf(stderr, "pactester.c: You didn't specify the PAC file\n");
    usage(argv[0]);
  }
  if (!url && !urlsfile) {
    fprintf(stderr, "pactester.c: You didn't specify a URL or URL-FILE\n");
    usage(argv[0]);
  }

  if (!pacparser_set_dns_resolver_variant(dns_resolver_variant))
    usage(argv[0]);

  if (dns_servers) {
    if (!pacparser_set_dns_servers(dns_servers)) {
      fprintf(stderr, "pactester.c: pacparser_set_dns_servers() failed\n");
      return 1;
    }
  }

  if (dns_domains) {
    int i = 0;
    const char *dns_domains_list[DOMAINMAX + 1];
    char *p = strtok((char *) dns_domains, ",");
    while (p != NULL) {
      dns_domains_list[i++] = strdup(p);
      if (i > DOMAINMAX) {
        fprintf(stderr, "pactester.c: Too many domains specified. "
                "Maximum allowed number is: %d\n", DOMAINMAX);
        return 1;
      }
      p = strtok(NULL, ",");
    }
    dns_domains_list[i] = NULL;
    if (!pacparser_set_dns_domains(dns_domains_list)) {
      fprintf(stderr, "pactester.c: pacparser_set_dns_domains() failed\n");
      return 1;
    }
  }

  if (enable_microsoft_extensions)
    pacparser_enable_microsoft_extensions();
  else
    pacparser_disable_microsoft_extensions();

  // Initialize pacparser.
  if (!pacparser_init()) {
      fprintf(stderr, "pactester.c: Could not initialize pacparser\n");
      return 1;
  }

  // Read pacfile from stdin.
  if (STREQ("-", pacfile)) {
    char *script;
    size_t script_size = 1;  // for the null terminator
    char buffer[LINEMAX];

    script = (char *) calloc(1, sizeof(char));
    if (script == NULL) {
      perror("pactetser.c: Failed to allocate the memory for the script");
      return 1;
    }

    while (fgets(buffer, sizeof(buffer), stdin)) {
      if (strlen(buffer) == 0)
        break;
      char *old = script;
      script_size += strlen(buffer);
      if (script_size > PACMAX) {
        fprintf(stderr, "pactester.c: Input file is too big. "
                "Maximum allowed size in bytes is: %d\n", PACMAX);
        free(script);
        return 1;
      }
      script = realloc(script, script_size);
      if (script == NULL) {
        perror("pactester.c: Failed to allocate the memory for the script");
        free(old);
        return 1;
      }
      strncat(script, buffer, strlen(buffer));
    }

    if (ferror(stdin)) {
      free(script);
      perror("pactester.c: Error reading from stdin");
      return 1;
    }

    if (!pacparser_parse_pac_string(script)) {
      fprintf(stderr, "pactester.c: Could not parse the pac script: %s\n",
              script);
      free(script);
      pacparser_cleanup();
      return 1;
    }
    free(script);
  }
  else {
    if (!pacparser_parse_pac_file(pacfile)) {
      fprintf(stderr, "pactester.c: Could not parse the pac file: %s\n",
              pacfile);
      pacparser_cleanup();
      return 1;
    }
  }

  if (client_ip)
    pacparser_setmyip(client_ip);

  char *proxy;
  int rc = 0;

  if (url) {
    // If the host was not explicitly given, get it from the URL.
    // If that fails, return with error (the get_host_from_url()
    // function will print a proper error message in that case).
    host = host ? host : get_host_from_url(url);
    if (!host)
      return 1;
    proxy = pacparser_find_proxy(url, host);
    if (proxy == NULL) {
      fprintf(stderr, "pactester.c: %s %s\n",
              "Problem in finding proxy for", url);
      pacparser_cleanup();
      return 1;
    }
    printf("%s\n", proxy);
  }

  else if (urlsfile) {
    char line[LINEMAX];
    FILE *fp;
    if (!(fp = fopen(urlsfile, "r"))) {
      fprintf(stderr, "pactester.c: Could not open urlsfile: %s", urlsfile);
      pacparser_cleanup();
      return 1;
    }
    while (fgets(line, sizeof(line), fp)) {
      char *url = line;
      // Remove spaces from the beginning.
      while (*url == ' ' || *url == '\t')
        url++;
      // Skip comment lines.
      if (*url == '#') {
        printf("%s", url);
        continue;
      }
      char *urlend = url;
      while (*urlend != '\r' && *urlend != '\n' && *urlend != '\0' &&
             *urlend != ' ' && *urlend != '\t')
        urlend++;  // keep moving till you hit space or end of string
      *urlend = '\0';
      if (!(host = get_host_from_url(url))) {
        rc = 1;  // will exit with error.
        continue;
      }
      proxy = pacparser_find_proxy(url, host);
      if (proxy == NULL) {
        fprintf(stderr, "pactester.c: %s %s\n",
                "Problem in finding proxy for", url);
        pacparser_cleanup();
        return 1;
      }
      printf("%s : %s\n", url, proxy);
    }
    fclose(fp);
  }

  pacparser_cleanup();
  return rc;
}
