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
// version 2.1 of the License, or (at your option) any later version.

// pacparser is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Lesser General Public License for more details.

// You should have received a copy of the GNU Lesser General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA

#include <pacparser.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

void usage(const char *progname)
{
  fprintf(stderr, "\nUsage:  %s <-p pacfile> <-u url> [-h host] "
          "[-c client_ip]", progname);
  fprintf(stderr, "\n        %s <-p pacfile> <-f urlslist> "
          "[-c client_ip]\n", progname);
  fprintf(stderr, "\nOptions:\n");
  fprintf(stderr, "  -p pacfile   : PAC file to test\n");
  fprintf(stderr, "  -u url       : URL to test for\n");
  fprintf(stderr, "  -h host      : Host part of the URL\n");
  fprintf(stderr, "  -c client_ip : client IP address (as returned by "
                  "myIpAddres() function\n");
  fprintf(stderr, "                 in PAC files), defaults to IP address "
                  "on which it is running.\n");
  fprintf(stderr, "  -f urlslist  : a file containing list of URLs to be "
          "tested.\n");
}

char *get_host_from_url(const char *url)
{
  // copy  url to a  pointer that we'll use to seek through the string.
  char *p = strdup(url);
  char *proto = p;
  // Move to :
  while (*p != ':' && *p != '\0')
    p++;
  if (p[0] == '\0'||                    // We reached end without hitting :
      p[1] != '/' || p[2] != '/'        // Next to characters are not //
      ) {
    fprintf(stderr, "Not a proper URL\n");
    return NULL;
  }
  *p = '\0';                            // Terminate proto here.
  // Make sure protocol is either http, https or ftp.
  if ( strcmp(proto, "http") &&
       strcmp(proto, "https") &&
       strcmp(proto, "ftp") ) {
    fprintf(stderr, "Not a proper URL\n");
    return NULL;
  }
  p++; p++; p++;                      // Getting past '://'
  // Host part starts from here.
  char *host = p;
  if (*p == '\0' || *p == '/' || *p == ':') {   // If host part is null.
    fprintf(stderr, "Not a proper URL\n");
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
  char c;
  while ((c = getopt(argc, argv, "p:u:h:f:c:")) != -1)
    switch (c)
    {
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
    fprintf(stderr, "You didn't specify the PAC file\n");
    usage(argv[0]);
    return 1;
  }
  if (!url && !urlslist) {
    fprintf(stderr, "You didn't specify the URL\n");
    usage(argv[0]);
    return 1;
  }

  // initialize pacparser
  if (!pacparser_init()) {
      fprintf(stderr, "Could not initialize pacparser\n");
      return 1;
  }

  if(!pacparser_parse_pac(pacfile)) {
    fprintf(stderr, "Could not parse pac file: %s\n", pacfile);
    pacparser_cleanup();
    return 1;
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
      if(proxy) printf("%s\n", proxy);
    }
  }
  else if (urlslist) {
    char line[1000];                    // this limits line length to 1000.
    FILE *fp;
    if (!(fp = fopen(urlslist, "r"))) {
      fprintf(stderr, "Could not open urlslist: %s", urlslist);
      return 1;
    }
    while (fgets(line, sizeof(line), fp)) {
      char *url = strdup(line);
      // remove spaces from the beginning.
      while (*url == ' ' || *url == '\t')
        url++;
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
      if(proxy) printf("%s\n", proxy);
    }
  }
  pacparser_cleanup();
}
