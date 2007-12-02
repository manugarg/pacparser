#include <stdio.h>

int init_pac_parser();
int parse_pac(char* pacfile);
char *find_proxy_for_url(char *url, char *host);
char *find_proxy(char * pacfile, char *url, char *host);
void destroy_pac_parser();

int main(int argc, char* argv[])
{
  char *proxy = NULL;
  init_pac_parser();
  parse_pac(argv[1]);
  proxy = find_proxy_for_url(argv[2], argv[3]);
  if(proxy) printf("%s\n", proxy);
  destroy_pac_parser();
}
