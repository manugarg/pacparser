// Comprehensive edge case testing
// Tests unusual scenarios and boundary conditions

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>

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

void test_case(const char *name, const char *hostname, int max_results, int family) {
  char ipaddr_list[INET6_ADDRSTRLEN * MAX_IP_RESULTS + MAX_IP_RESULTS];
  memset(ipaddr_list, 0xFF, sizeof(ipaddr_list));  // Fill with 0xFF

  printf("\n--- %s ---\n", name);
  printf("Hostname: '%s', max_results: %d\n", hostname, max_results);

  int result = resolve_host(hostname, ipaddr_list, max_results, family);

  if (result != 0) {
    printf("Failed: %s\n", gai_strerror(result));
    // Verify buffer was initialized to empty string
    if (ipaddr_list[0] != '\0') {
      printf("✗ ERROR: Buffer not initialized to empty string on failure\n");
    } else {
      printf("✓ Buffer properly initialized to empty string on failure\n");
    }
    return;
  }

  size_t len = strlen(ipaddr_list);
  printf("Success: '%s' (length: %zu)\n", ipaddr_list, len);

  // Count IPs
  int ip_count = (len > 0) ? 1 : 0;
  int semicolon_count = 0;
  for (size_t i = 0; i < len; i++) {
    if (ipaddr_list[i] == ';') {
      semicolon_count++;
      ip_count++;
    }
  }
  printf("IPs: %d, Semicolons: %d\n", ip_count, semicolon_count);

  // Validation checks
  int errors = 0;

  // Check null termination
  if (ipaddr_list[len] != '\0') {
    printf("✗ ERROR: String not null-terminated\n");
    errors++;
  }

  // Check no writes beyond null terminator
  int found_non_ff = 0;
  for (size_t i = len + 1; i < sizeof(ipaddr_list); i++) {
    if ((unsigned char)ipaddr_list[i] != 0xFF) {
      found_non_ff = 1;
      break;
    }
  }
  if (found_non_ff) {
    printf("✗ ERROR: Wrote beyond null terminator\n");
    errors++;
  }

  // Check semicolon placement
  if (len > 0) {
    if (ipaddr_list[0] == ';') {
      printf("✗ ERROR: Starts with semicolon\n");
      errors++;
    }
  }
  if (len > 0 && ipaddr_list[len-1] == ';') {
    printf("✗ ERROR: Ends with semicolon\n");
    errors++;
  }

  // Check for double semicolons
  if (len > 1) {
    for (size_t i = 0; i < len - 1; i++) {
      if (ipaddr_list[i] == ';' && ipaddr_list[i+1] == ';') {
        printf("✗ ERROR: Contains ';;'\n");
        errors++;
        break;
      }
    }
  }

  // Check IP count matches semicolons
  if (ip_count != semicolon_count + 1 && len > 0) {
    printf("✗ ERROR: IP count mismatch (expected %d, got %d)\n",
           semicolon_count + 1, ip_count);
    errors++;
  }

  if (errors == 0) {
    printf("✓ All validations passed\n");
  }
}

int main() {
  printf("=== Comprehensive Edge Case Testing ===\n");

  // Edge case: max_results boundaries
  test_case("Max results = 0", "localhost", 0, AF_INET);
  test_case("Max results = 1", "localhost", 1, AF_INET);
  test_case("Max results = MAX_IP_RESULTS", "google.com", MAX_IP_RESULTS, AF_UNSPEC);

  // Edge case: Different IP types
  test_case("IPv4 only", "google.com", 10, AF_INET);
  test_case("IPv6 only", "google.com", 10, AF_INET6);
  test_case("Both IPv4 and IPv6", "google.com", 10, AF_UNSPEC);

  // Edge case: Numeric IPs (should not fail)
  test_case("Numeric IPv4", "8.8.8.8", 10, AF_INET);
  test_case("Numeric IPv6 full", "2001:4860:4860::8888", 10, AF_INET6);
  test_case("Numeric IPv6 compressed", "::1", 10, AF_INET6);
  test_case("Numeric IPv4 mapped IPv6", "::ffff:8.8.8.8", 10, AF_INET6);

  // Edge case: Special hostnames
  test_case("Localhost", "localhost", 10, AF_INET);
  test_case("Localhost IPv6", "localhost", 10, AF_INET6);

  // Edge case: Non-existent hostnames
  test_case("Invalid hostname", "this-definitely-does-not-exist.invalid", 10, AF_INET);

  printf("\n=== All Edge Case Tests Complete ===\n");

  return 0;
}
