// Buffer safety test for resolve_host
// Tests that we don't overflow the buffer even with many results

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>

#define MAX_IP_RESULTS 10
#define CANARY_PATTERN 0xDEADBEEF

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
  printf("=== Buffer Safety Test ===\n\n");

  // Create a buffer with canary values before and after
  size_t buffer_size = INET6_ADDRSTRLEN * MAX_IP_RESULTS + MAX_IP_RESULTS;
  unsigned int *canary_before = malloc(sizeof(unsigned int));
  char *ipaddr_list = malloc(buffer_size);
  unsigned int *canary_after = malloc(sizeof(unsigned int));

  *canary_before = CANARY_PATTERN;
  *canary_after = CANARY_PATTERN;

  printf("Buffer size: %zu bytes\n", buffer_size);
  printf("Canary before buffer: 0x%X at %p\n", *canary_before, (void*)canary_before);
  printf("Buffer address: %p\n", (void*)ipaddr_list);
  printf("Canary after buffer:  0x%X at %p\n\n", *canary_after, (void*)canary_after);

  // Test 1: Normal hostname
  printf("Test 1: Normal hostname (google.com)\n");
  int result = resolve_host("google.com", ipaddr_list, MAX_IP_RESULTS, AF_UNSPEC);
  if (result == 0) {
    printf("Result: %s\n", ipaddr_list);
    printf("Length: %zu bytes\n", strlen(ipaddr_list));
  }

  if (*canary_before != CANARY_PATTERN) {
    printf("✗ CANARY BEFORE CORRUPTED: 0x%X\n", *canary_before);
  } else {
    printf("✓ Canary before intact\n");
  }

  if (*canary_after != CANARY_PATTERN) {
    printf("✗ CANARY AFTER CORRUPTED: 0x%X\n", *canary_after);
  } else {
    printf("✓ Canary after intact\n");
  }

  // Test 2: Fill buffer to near-capacity
  printf("\n\nTest 2: Large max_results (stress test)\n");
  result = resolve_host("www.google.com", ipaddr_list, MAX_IP_RESULTS, AF_UNSPEC);
  if (result == 0) {
    size_t len = strlen(ipaddr_list);
    printf("Result length: %zu bytes (%.1f%% of buffer)\n",
           len, (len * 100.0) / buffer_size);
    printf("Result: %s\n", ipaddr_list);

    // Count IPs
    int count = 1;
    for (char *p = ipaddr_list; *p; p++) {
      if (*p == ';') count++;
    }
    printf("Number of IPs: %d\n", count);
  }

  if (*canary_before != CANARY_PATTERN) {
    printf("✗ CANARY BEFORE CORRUPTED: 0x%X\n", *canary_before);
  } else {
    printf("✓ Canary before intact\n");
  }

  if (*canary_after != CANARY_PATTERN) {
    printf("✗ CANARY AFTER CORRUPTED: 0x%X\n", *canary_after);
  } else {
    printf("✓ Canary after intact\n");
  }

  // Test 3: Edge case - max_results = 0
  printf("\n\nTest 3: Edge case - max_results = 0\n");
  result = resolve_host("localhost", ipaddr_list, 0, AF_INET);
  printf("Result: '%s'\n", ipaddr_list);
  printf("Length: %zu bytes (should be 0)\n", strlen(ipaddr_list));

  if (*canary_before != CANARY_PATTERN) {
    printf("✗ CANARY BEFORE CORRUPTED: 0x%X\n", *canary_before);
  } else {
    printf("✓ Canary before intact\n");
  }

  if (*canary_after != CANARY_PATTERN) {
    printf("✗ CANARY AFTER CORRUPTED: 0x%X\n", *canary_after);
  } else {
    printf("✓ Canary after intact\n");
  }

  // Test 4: Very long IPv6 addresses
  printf("\n\nTest 4: IPv6 addresses (longest format)\n");
  result = resolve_host("ipv6.google.com", ipaddr_list, MAX_IP_RESULTS, AF_INET6);
  if (result == 0) {
    printf("Result: %s\n", ipaddr_list);
    printf("Length: %zu bytes\n", strlen(ipaddr_list));
  } else {
    printf("Could not resolve ipv6.google.com (error: %s)\n", gai_strerror(result));
  }

  if (*canary_before != CANARY_PATTERN) {
    printf("✗ CANARY BEFORE CORRUPTED: 0x%X\n", *canary_before);
  } else {
    printf("✓ Canary before intact\n");
  }

  if (*canary_after != CANARY_PATTERN) {
    printf("✗ CANARY AFTER CORRUPTED: 0x%X\n", *canary_after);
  } else {
    printf("✓ Canary after intact\n");
  }

  free(canary_before);
  free(ipaddr_list);
  free(canary_after);

  printf("\n=== Buffer Safety Test Complete ===\n");
  printf("If all canaries are intact, the function is memory-safe.\n");

  return 0;
}
