// This file implements a command called pacparse for downloading and
//  parsing proxy-auto-config files.  Intended to be used from scripts
//  including pacwget.
// Author: Dave Dykstra
//
// This file is based on pactester.c from the pacparser library
// (https://github.com/pacparser/pacparser/blob/master/src/pactester.c)
// Copyright (C) 2008 Manu Garg.
// Author: Manu Garg <manugarg@gmail.com>
//
// pacparse is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 3 of the License, or (at your option) any later version.
//
// pacparse is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA

#ifndef PACPARSE_VERSION
#define PACPARSE_VERSION "1.0"
#endif

#include <pacparser.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

int pacdebug=0;
char *progname="pacparse";

void usage()
{
  fprintf(stderr, "\nUsage: %s -p pacfile -u url [-h host] "
          "[-c clientip] [-U pacurl] [-46edv]", progname);
  fprintf(stderr, "\nOptions:\n");
  fprintf(stderr, "  -p pacfile   : PAC file to parse (specify '-' to read "
                  "from standard input)\n");
  fprintf(stderr, "  -u url       : URL parameter to the PAC file's "
  		  "FindProxyForURL function\n");
  fprintf(stderr, "  -h host      : Host part of the URL (default "
  		  "parsed from url)\n");
  fprintf(stderr, "  -c clientip : client IP address (as returned by "
                  "myIpAddress() function\n");
  fprintf(stderr, "                 in PAC files, defaults to IP address "
                  "of client hostname)\n");
  fprintf(stderr, "  -U pacurl    : URL that pacfile came from, to identify "
  		   "client IP address\n");
  fprintf(stderr, "                 (creates UDP socket to host, more reliable "
  		  "than default\n");
  fprintf(stderr, "                  on clients with multiple IP addresses)\n");
  fprintf(stderr, "  -4           : use only IPv4 addresses for -U\n");
  fprintf(stderr, "  -6           : use only IPv6 addresses for -U\n");
  fprintf(stderr, "  -e           : enable microsoft extensions "
                  "(Ex functions)\n");
  fprintf(stderr, "  -d           : enable debugging messages\n");
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
    fprintf(stderr, "%s: Not a proper URL\n", progname);
    return NULL;
  }
  p = p + 3;                            // Get past '://'
  // Host part starts from here.
  char *host = p;
  if (*p == '\0' || *p == '/' || *p == ':') {   // If host part is null.
    fprintf(stderr, "%s: Not a proper URL\n", progname);
    return NULL;
  }
  // Seek until next /, : or end of string.
  while (*p != '/' && *p != ':' && *p != '\0')
    p++;
  *p = '\0';
  return host;
}

void set_myip_from_host(const char *host, int ipversion)
{
  int err, fd;
  struct addrinfo addrinfo, *res, *r;
  struct sockaddr_in sinbuf;
  struct sockaddr_in6 sin6buf;
  socklen_t namelen;
  char errbuf[4096];
  char ipbuf[256];

  memset(&addrinfo, 0, sizeof(addrinfo));
  switch(ipversion)
  {
    case 4:
      addrinfo.ai_family = AF_INET;
      break;
    case 6:
      addrinfo.ai_family = AF_INET6;
      break;
    default:
      addrinfo.ai_family = AF_UNSPEC;
      break;
  }
  addrinfo.ai_socktype = SOCK_DGRAM;
  addrinfo.ai_protocol = IPPROTO_UDP;

  if ((err = getaddrinfo(host, 0, &addrinfo, &res)) != 0) {
    fprintf(stderr, "%s: error from getaddrinfo on %s: %s\n", 
    		progname, host, gai_strerror(err));
    exit(2);
  }
  errbuf[0] = '\0';
  ipbuf[0] = '\0';
  for (r = res; r; r = r->ai_next) {
    if (pacdebug && (errbuf[0] != '\0')) {
      fprintf(stderr, "DEBUG: trying next addr after %s\n", errbuf);
    }
    if ((fd = socket(r->ai_family, r->ai_socktype, r->ai_protocol)) < 0) {
      fprintf(stderr, "%s: error creating socket for %s: %s\n",
		  progname, host, strerror(errno));
      exit(2);
    }
    if (connect(fd, r->ai_addr, r->ai_addrlen) < 0) {
      snprintf(errbuf, sizeof(errbuf),
		  "error connecting UDP socket(s) to %s, last error: %s",
		      host, strerror(errno));
      close(fd);
      continue;
    }
    if (r->ai_family == AF_INET) {
      namelen = sizeof(sinbuf);
      if (getsockname(fd, (struct sockaddr *)&sinbuf, &namelen) < 0) {
	fprintf(stderr, "%s: error on getsockname from socket to %s: %s\n",
		  progname, host, strerror(errno));
	exit(2);
      }
      if (inet_ntop(AF_INET, &sinbuf.sin_addr, ipbuf, sizeof(ipbuf)) == 0) {
	fprintf(stderr, "%s: error on inet_ntop from getsockname to %s: %s\n",
		  progname, host, strerror(errno));
	exit(2);
      }
    }
    else if (r->ai_family == AF_INET6) {
      namelen = sizeof(sin6buf);
      if (getsockname(fd, (struct sockaddr *)&sin6buf, &namelen) < 0) {
	fprintf(stderr, "%s: error on getsockname from socket to %s: %s\n",
		  progname, host, strerror(errno));
	exit(2);
      }
      if (inet_ntop(AF_INET6, &sin6buf.sin6_addr, ipbuf, sizeof(ipbuf)) == 0) {
	fprintf(stderr, "%s: error on inet_ntop from getsockname to %s: %s\n",
		  progname, host, strerror(errno));
	exit(2);
      }
    }
    else {
      fprintf(stderr, "%s: unknown address family %d\n", 
	progname, r->ai_family);
      exit(2);
    }
    break;
  }
  freeaddrinfo(res);
  close(fd);
  if (ipbuf[0] == '\0') {
    if (errbuf[0] == '\0')
      fprintf(stderr, "%s: could not determine IP for %s, error unknown\n",
      	progname, host);
    else
      fprintf(stderr, "%s: %s\n", progname, errbuf);
    exit(2);
  }
  if (pacdebug) {
    fprintf(stderr, "DEBUG: Setting myip to %s\n", ipbuf);
  }
  pacparser_setmyip(ipbuf);
}

int main(int argc, char* argv[])
{
  char *pacfile=NULL, *url=NULL, *host=NULL, *clientip=NULL, *pacurl=NULL;
  int ipversion = 0;
  int enable_microsoft_extensions = 0;
  signed char c;
  while ((c = getopt(argc, argv, "edv46p:u:h:c:U:")) != -1)
    switch (c)
    {
      case 'v':
        printf("%s %s; pacparser library %s\n", progname, PACPARSE_VERSION,
				pacparser_version());
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
      case 'c':
        clientip = optarg;
        break;
      case 'U':
        pacurl = optarg;
        break;
      case '4':
	ipversion = 4;
	break;
      case '6':
        ipversion = 6;
	break;
      case 'e':
        enable_microsoft_extensions = 1;
        break;
      case 'd':
        pacdebug = 1;
	putenv("PACPARSER_DEBUG=1");
	putenv("DEBUG=1"); /* for older versions of the library */
        break;
      case '?':
        usage();
        return 1;
      default:
        abort ();
    }

  if (!pacfile) {
    fprintf(stderr, "%s: You didn't specify the PAC file\n", progname);
    usage();
    return 1;
  }

  if(enable_microsoft_extensions)
    pacparser_enable_microsoft_extensions();

  // initialize pacparser
  if (!pacparser_init()) {
      fprintf(stderr, "%s: Could not initialize pacparser\n", progname);
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
      fprintf(stderr,"%s: Failed to allocate memory for the script\n",
      	progname);
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
        fprintf(stderr, "%s: Failed to realloc %d bytes memory for the script %s\n",
	  progname, script_size);
        free(old);
        return 1;
      }
      strcat(script, buffer);
    }

    if (ferror(stdin)) {
      free(script);
      fprintf(stderr, "%s: Error reading from stdin\n", progname);
      return 1;
    }

    if(!pacparser_parse_pac_string(script)) {
      fprintf(stderr, "%s: Could not parse the pac script:\n%s\n",
              progname, script);
      free(script);
      pacparser_cleanup();
      return 1;
    }
    free(script);
  }
  else {
    if(!pacparser_parse_pac_file(pacfile)) {
      fprintf(stderr, "%s: Could not parse the pac file: %s\n",
              progname, pacfile);
      pacparser_cleanup();
      return 1;
    }
  }

  if(clientip)
    pacparser_setmyip(clientip);
  else if(pacurl) {
    char *pachost;
    pachost = get_host_from_url(pacurl);
    if (!pachost) {
      fprintf(stderr, "%s: Error finding hostname in %s\n", progname, pacurl);
      exit(2);
    }
    set_myip_from_host(pachost, ipversion);
  }

  char *proxy;

  if (url) {
    if (!host)
      host = get_host_from_url(url);
    if (host) {
      proxy = NULL;
      proxy = pacparser_find_proxy(url, host);
      if (proxy == NULL) {
        fprintf(stderr, "%s: Problem in finding proxy for %s\n", progname, url);
        pacparser_cleanup();
        return 1;
      }
      if (proxy) printf("%s\n", proxy);
    }
  }

  pacparser_cleanup();
  return 0;
}
