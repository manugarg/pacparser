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
  fprintf(stderr, "  -u url       : URL to test\n");
  fprintf(stderr, "  -h host      : Host part of the URL\n");
  fprintf(stderr, "  -c client_ip : client IP address (defaults to IP "
          "address of the machine \n");
  fprintf(stderr, "                 on which script is running)\n");
  fprintf(stderr, "  -f urlslist  : a file containing list of URLs to be "
          "tested.\n");
}

char *get_host_from_url(const char *url)
{
  // copy  url to a  pointer that we'll use to seek through the string.
  char *p = strdup(url);
  char *proto = p;
  // Move to first /
  while (*p != '/' && *p != '\0')
    p++;
  char *protoend = p - 1;
  *protoend = '\0';
  // Make sure protocol is either http, https or ftp.
  if ( strcmp(proto, "http") &&
       strcmp(proto, "https") &&
       strcmp(proto, "ftp") ) {
    fprintf(stderr, "Not a proper URL\n");
    return NULL;
  }
  if ( p[0] != '/' || p[1] != '/') {
    fprintf(stderr, "Not a proper URL\n");
    return NULL;
  }
  p++; p++;                      // Getting past '//'
  char *host = p;
  char *hostend = host;          // We'll move hostend to the end of host part.
  if (*host == '\0' || *host == '/') {
    fprintf(stderr, "Not a proper URL\n");
    return NULL;
  }
  while (*hostend != '/' && *hostend != '\0')
    hostend++;
  *hostend = '\0';
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
    char line[1000];
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
