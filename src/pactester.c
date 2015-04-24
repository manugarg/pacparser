// Copyright (C) 2008 Manu Garg.
// Author: Manu Garg <manugarg@gmail.com>
//
// This file implements pactester (http://code.google.com/p/pactester) using
// pacparser.
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

#include <pacparser.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

void usage(const char *progname)
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
}

char *get_host_from_url(const char *url)
{
  // copy  url to a  pointer that we'll use to seek through the string.
  char *p = strdup(url);
  // Move to :
  while (*p != ':' && *p != '\0')
    p++;
  if (p[0] == '\0'||                    // We reached end without hitting :
      p[1] != '/' || p[2] != '/'        // Next two characters are not //
      ) {
    fprintf(stderr, "pactester.c: Not a proper URL\n");
    return NULL;
  }
  p = p + 3;                            // Get past '://'
  // Host part starts from here.
  char *host = p;
  if (*p == '\0' || *p == '/' || *p == ':') {   // If host part is null.
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
  char *pacfile=NULL, *url=NULL, *host=NULL, *urlslist=NULL, *client_ip=NULL;
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
        if (optopt == 'p' || optopt == 'u' || optopt == 'h' ||
            optopt == 'f' || optopt == 'c')
          usage(argv[0]);
        else if (isprint (optopt))
          usage(argv[0]);
        else
          usage(argv[0]);
        return 1;
      default:
        abort ();
    }

  if (!pacfile) {
    fprintf(stderr, "pactester.c: You didn't specify the PAC file\n");
    usage(argv[0]);
    return 1;
  }
  if (!url && !urlslist) {
    fprintf(stderr, "pactester.c: You didn't specify the URL\n");
    usage(argv[0]);
    return 1;
  }

  // initialize pacparser
  if (!pacparser_init()) {
      fprintf(stderr, "pactester.c: Could not initialize pacparser\n");
      return 1;
  }

  // Read pacfile from stdin
  if (strcmp("-", pacfile) == 0) {
    char *script;
    int buffsize = 4096;
    int maxsize = 1024 * 1024;          // Limit the max script size to 1 MB
    size_t script_size = 1;             // For the null terminator
    char buffer[buffsize];

    script = (char*) malloc(sizeof(char) * buffsize);
    if (script == NULL) {
      perror("pactetser.c: Failed to allocate the memory for the script");
      return(1);
    }
    script[0] = '\0';                   // Null terminate to prepare for strcat

    while(fgets(buffer, buffsize, stdin)) {
      if (strlen(buffer) == 0) break;
      char *old = script;
      script_size += strlen(buffer);
      if (script_size > maxsize) {
        fprintf(stderr, "Input file is too big. Maximum allowed size is: %d",
                maxsize);
        free(script);
        return 1;
      }
      script = realloc(script, script_size);
      if (script == NULL) {
        perror("pactester.c: Failed to allocate the memory for the script");
        free(old);
        return 1;
      }
      strcat(script, buffer);
    }

    if (ferror(stdin)) {
      free(script);
      perror("pactester.c: Error reading from stdin");
      return 1;
    }

    if(!pacparser_parse_pac_string(script)) {
      fprintf(stderr, "pactester.c: Could not parse the pac script: %s\n",
              script);
      free(script);
      pacparser_cleanup();
      return 1;
    }
    free(script);
  }
  else {
    if(!pacparser_parse_pac_file(pacfile)) {
      fprintf(stderr, "pactester.c: Could not parse the pac file: %s\n",
              pacfile);
      pacparser_cleanup();
      return 1;
    }
  }

  if(client_ip)
    pacparser_setmyip(client_ip);

  char *proxy;

  if (url) {
    if (!host)
      host = get_host_from_url(url);
    if (host) {
      proxy = NULL;
      proxy = pacparser_find_proxy(url, host);
      if (proxy == NULL) {
        fprintf(stderr, "pactester.c: %s %s.\n",
                "Problem in finding proxy for", url);
        pacparser_cleanup();
        return 1;
      }
      if (proxy) printf("%s\n", proxy);
    }
  }

  else if (urlslist) {
    char line[1000];                    // this limits line length to 1000.
    FILE *fp;
    if (!(fp = fopen(urlslist, "r"))) {
      fprintf(stderr, "pactester.c: Could not open urlslist: %s", urlslist);
      pacparser_cleanup();
      return 1;
    }
    while (fgets(line, sizeof(line), fp)) {
      char *url = line;
      // remove spaces from the beginning.
      while (*url == ' ' || *url == '\t')
        url++;
      // skip comment lines
      if (*url == '#') {
        printf("%s", url);
        continue;
      }
      char *urlend = url;
      while (*urlend != '\r' && *urlend != '\n' && *urlend != '\0' &&
             *urlend != ' ' && *urlend != '\t')
        urlend++;                       // keeping moving till you hit space
                                        // or end of string.
      *urlend = '\0';
      if ( !(host = get_host_from_url(url)) )
        continue;
      proxy = NULL;
      proxy = pacparser_find_proxy(url, host);
      if (proxy == NULL) {
        fprintf(stderr, "pactester.c: %s %s.\n",
                "Problem in finding proxy for", url);
        pacparser_cleanup();
        return 1;
      }
      if(proxy) printf("%s : %s\n", url, proxy);
    }
    fclose(fp);
  }

  pacparser_cleanup();
  return 0;
}
