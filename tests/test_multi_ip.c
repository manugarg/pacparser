// Specific test for multiple IP resolution
// This uses a hostname that typically returns multiple IPs

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#ifndef _WIN32
#include <unistd.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#else
#include <winsock2.h>
#include <ws2tcpip.h>
#endif

#define MAX_IP_RESULTS 10

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
  return 0;
}

int main() {
  char ipaddr_list[INET6_ADDRSTRLEN * MAX_IP_RESULTS + MAX_IP_RESULTS];

  // Try multiple hostnames that might return multiple IPs
  const char *hosts[] = {
    "www.google.com",   // Usually returns multiple IPs
    "facebook.com",     // Often returns multiple IPs
    "amazon.com",       // Usually returns multiple IPs
    "cloudflare.com",   // May return multiple IPs
  };

  printf("Testing multiple IP resolution:\n\n");

  for (int h = 0; h < 4; h++) {
    printf("--- Testing: %s ---\n", hosts[h]);

    // Test with AF_UNSPEC which may return both IPv4 and IPv6
    int result = resolve_host(hosts[h], ipaddr_list, 10, AF_UNSPEC);

    if (result != 0) {
      printf("Failed to resolve: %s\n\n", gai_strerror(result));
      continue;
    }

    printf("Results: %s\n", ipaddr_list);

    // Count IPs
    int count = 1;
    for (char *p = ipaddr_list; *p; p++) {
      if (*p == ';') count++;
    }
    printf("Number of IPs: %d\n", count);

    if (count > 1) {
      printf("✓ Multiple IPs returned! Testing semicolon separation:\n");

      // Parse and display each IP
      char *copy = strdup(ipaddr_list);
      char *token = strtok(copy, ";");
      int idx = 1;
      while (token) {
        printf("  IP %d: %s\n", idx++, token);
        token = strtok(NULL, ";");
      }
      free(copy);

      // Verify format
      if (ipaddr_list[0] == ';') {
        printf("✗ ERROR: Starts with semicolon\n");
      } else {
        printf("✓ Doesn't start with semicolon\n");
      }

      size_t len = strlen(ipaddr_list);
      if (len > 0 && ipaddr_list[len-1] == ';') {
        printf("✗ ERROR: Ends with semicolon\n");
      } else {
        printf("✓ Doesn't end with semicolon\n");
      }

      if (strstr(ipaddr_list, ";;")) {
        printf("✗ ERROR: Contains double semicolons\n");
      } else {
        printf("✓ No double semicolons\n");
      }

      printf("\n** SUCCESSFULLY TESTED MULTIPLE IP CASE **\n");
      break;  // Found a multi-IP case, we're done
    }
    printf("\n");
  }

  // Test max_results limiting
  printf("\n--- Testing max_results limiting ---\n");
  for (int max = 1; max <= 5; max++) {
    int result = resolve_host("www.google.com", ipaddr_list, max, AF_UNSPEC);
    if (result == 0) {
      int count = 1;
      for (char *p = ipaddr_list; *p; p++) {
        if (*p == ';') count++;
      }
      printf("max_results=%d: got %d IPs\n", max, count);
      if (count > max) {
        printf("✗ ERROR: Returned more IPs than max_results!\n");
      } else {
        printf("✓ Correctly limited to %d\n", count);
      }
    }
  }

  return 0;
}
