// Copyright (C) 2008-2023 Manu Garg.
// Author: Manu Garg <manugarg@gmail.com>
//
// This file implements pactester using pacparser.
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

#include "pacparser.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define STREQ(s1, s2) (strcmp((s1), (s2)) == 0)

#define LINEMAX 4096  // Max length of any line read from text files (4 KiB)
#define PACMAX (1024 * 1024)  // Max size of the PAC script (1 MiB)

__attribute__((noreturn)) void usage(const char *progname)
{
  fprintf(stderr, "\nUsage:  %s <-p pacfile> <-u url> [-h host] "
          "[-c client_ip] [-e]", progname);
  fprintf(stderr, "\n        %s <-p pacfile> <-f urlslist> "
          "[-c client_ip] [-e]\n", progname);
  fprintf(stderr, "\nOptions:\n");
  fprintf(stderr, "  -p pacfile   : PAC file to test (specify '-' to read "
                  "from standard input)\n");
  fprintf(stderr, "  -u url       : URL to test for\n");
  fprintf(stderr, "  -h host      : Host part of the URL\n");
  fprintf(stderr, "  -c client_ip : client IP address (as returned by "
                  "myIpAddres() function\n");
  fprintf(stderr, "                 in PAC files), defaults to IP address "
                  "on which it is running.\n");
  fprintf(stderr, "  -e           : Deprecated: IPv6 extensions are enabled"
                  "by default now.\n");
  fprintf(stderr, "  -f urlslist  : a file containing list of URLs to be "
          "tested.\n");
  fprintf(stderr, "  -v           : print version and exit\n");
  exit(1);
}

char *get_host_from_url(const char *url)
{
  // copy  url to a  pointer that we'll use to seek through the string.
  char *p = strdup(url);
  char *q = p;
  // Move to :
  while (*p != ':' && *p != '\0')
    p++;
  if (p[0] == '\0'||                    // We reached end without hitting :
      p[1] != '/' || p[2] != '/'        // Next two characters are not //
      ) {
    free(q);
    fprintf(stderr, "pactester.c: Not a proper URL\n");
    return NULL;
  }
  p = p + 3;                            // Get past '://'
  // Host part starts from here.
  char *host = p;
  if (*p == '\0' || *p == '/' || *p == ':') {   // If host part is null.
    free(q);
    fprintf(stderr, "pactester.c: Not a proper URL\n");
    return NULL;
  }
  // Seek until next /, : or end of string.
  while (*p != '/' && *p != ':' && *p != '\0')
    p++;
  *p = '\0';
  return host;
}

int main(int argc, char* argv[])
{
  char *pacfile = NULL, *url = NULL, *host = NULL, *urlslist = NULL,
       *client_ip = NULL;

  if (argv[1] && (STREQ(argv[1], "--help") || STREQ(argv[1], "--helpshort"))) {
    usage(argv[0]);
  }

  signed char c;
  while ((c = getopt(argc, argv, "evp:u:h:f:c:")) != -1)
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
        urlslist = optarg;
        break;
      case 'c':
        client_ip = optarg;
        break;
      case 'e':
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
  if (!url && !urlslist) {
    fprintf(stderr, "pactester.c: You didn't specify the URL\n");
    usage(argv[0]);
  }

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

    script = (char *) malloc(sizeof(char) * LINEMAX);
    if (script == NULL) {
      perror("pactetser.c: Failed to allocate the memory for the script");
      return 1;
    }
    script[0] = '\0';                   // Null terminate to prepare for strcat

    while (fgets(buffer, LINEMAX, stdin)) {
      if (strlen(buffer) == 0)
        break;
      script_size += strlen(buffer);
      if (script_size > PACMAX) {
        fprintf(stderr, "Input file is too big. Maximum allowed size is: %d",
                PACMAX);
        exit(1);
      }
      script = realloc(script, script_size);
      if (script == NULL) {
        perror("pactester.c: Failed to allocate the memory for the script");
        exit(1);
      }
      strcat(script, buffer);
    }

    if (ferror(stdin)) {
      free(script);
      perror("pactester.c: Error reading from stdin");
      return 1;
    }

    if (!pacparser_parse_pac_string(script)) {
      fprintf(stderr, "pactester.c: Could not parse the pac script: %s\n",
              script);
      pacparser_cleanup();
      exit(1);
    }
    free(script);
  }
  else {
    if (!pacparser_parse_pac_file(pacfile)) {
      fprintf(stderr, "pactester.c: Could not parse the pac file: %s\n",
              pacfile);
      pacparser_cleanup();
      exit(1);
    }
  }

  if (client_ip && !pacparser_setmyip(client_ip)) {
    fprintf(stderr, "pactester.c: Error setting client IP\n");
    pacparser_cleanup();
    exit(1);
  }

  char *proxy;

  if (url) {
    // If the host was not explicitly given, get it from the URL.
    // If that fails, return with error (the get_host_from_url()
    // function will print a proper error message in that case).
    host = host ? host: get_host_from_url(url);
    if (!host) {
      exit(1);
    }
    proxy = pacparser_find_proxy(url, host);
    if (proxy == NULL) {
      fprintf(stderr, "pactester.c: %s %s.\n",
              "Problem in finding proxy for", url);
      pacparser_cleanup();
      exit(1);
    }
    printf("%s\n", proxy);
    exit(0);
  }

  if (urlslist) {
    char line[LINEMAX];
    FILE *fp;
    if (!(fp = fopen(urlslist, "r"))) {
      fprintf(stderr, "pactester.c: Could not open urlslist: %s", urlslist);
      pacparser_cleanup();
      exit(1);
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
      if (!(host = get_host_from_url(url)) )
        continue;
      proxy = NULL;
      proxy = pacparser_find_proxy(url, host);
      if (proxy == NULL) {
        fprintf(stderr, "pactester.c: %s %s.\n",
                "Problem in finding proxy for", url);
        pacparser_cleanup();
        exit(1);
      }
      if (proxy)
        printf("%s : %s\n", url, proxy);
    }
    fclose(fp);
    exit(0);
  }

  pacparser_cleanup();
  return 0;
}
