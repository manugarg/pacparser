// Test program for resolve_host function
// Compile: gcc -g -Wall -o test_resolve_host test_resolve_host.c -DTEST_RESOLVE_HOST
// Run: ./test_resolve_host

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

// Copy the resolve_host function from pacparser.c
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

// Test harness
typedef struct {
  const char *name;
  const char *hostname;
  int max_results;
  int ai_family;
  int expect_success;
  const char *description;
} test_case_t;

void run_test(test_case_t *tc) {
  char ipaddr_list[INET6_ADDRSTRLEN * MAX_IP_RESULTS + MAX_IP_RESULTS];

  // Fill with sentinel pattern to detect buffer issues
  memset(ipaddr_list, 0xAA, sizeof(ipaddr_list));

  printf("\n--- Test: %s ---\n", tc->name);
  printf("Description: %s\n", tc->description);
  printf("Hostname: %s, max_results: %d, family: %s\n",
         tc->hostname, tc->max_results,
         tc->ai_family == AF_INET ? "AF_INET" :
         tc->ai_family == AF_INET6 ? "AF_INET6" : "AF_UNSPEC");

  int result = resolve_host(tc->hostname, ipaddr_list, tc->max_results, tc->ai_family);

  if (result != 0) {
    printf("Result: FAILED (error code: %d, %s)\n", result, gai_strerror(result));
    if (!tc->expect_success) {
      printf("✓ Expected failure\n");
    } else {
      printf("✗ Unexpected failure\n");
    }
  } else {
    printf("Result: SUCCESS\n");
    printf("IP addresses: '%s'\n", ipaddr_list);

    // Verify results
    if (!tc->expect_success) {
      printf("✗ Expected failure but got success\n");
    } else {
      printf("✓ Expected success\n");

      // Check for buffer overruns (sentinel bytes after the null terminator)
      size_t len = strlen(ipaddr_list);
      int overrun = 0;
      for (size_t i = len + 1; i < sizeof(ipaddr_list); i++) {
        if (ipaddr_list[i] != (char)0xAA) {
          overrun = 1;
          break;
        }
      }
      if (overrun) {
        printf("✗ Buffer overrun detected!\n");
      } else {
        printf("✓ No buffer overrun\n");
      }

      // Count semicolons
      int semicolons = 0;
      for (size_t i = 0; i < len; i++) {
        if (ipaddr_list[i] == ';') semicolons++;
      }
      printf("  Number of IPs: %d (semicolons: %d)\n", semicolons + 1, semicolons);

      // Verify no double semicolons
      if (strstr(ipaddr_list, ";;")) {
        printf("✗ Double semicolons found!\n");
      } else {
        printf("✓ No double semicolons\n");
      }

      // Verify doesn't start or end with semicolon
      if (len > 0 && (ipaddr_list[0] == ';' || ipaddr_list[len-1] == ';')) {
        printf("✗ Starts or ends with semicolon!\n");
      } else {
        printf("✓ Doesn't start/end with semicolon\n");
      }
    }
  }
}

int main() {
  printf("=== resolve_host() Test Suite ===\n");

  test_case_t tests[] = {
    // Basic tests
    {
      .name = "IPv4 localhost",
      .hostname = "localhost",
      .max_results = 10,
      .ai_family = AF_INET,
      .expect_success = 1,
      .description = "Resolve localhost to IPv4 (should get 127.0.0.1)"
    },
    {
      .name = "IPv6 localhost",
      .hostname = "localhost",
      .max_results = 10,
      .ai_family = AF_INET6,
      .expect_success = 1,
      .description = "Resolve localhost to IPv6 (should get ::1)"
    },
    {
      .name = "Any family localhost",
      .hostname = "localhost",
      .max_results = 10,
      .ai_family = AF_UNSPEC,
      .expect_success = 1,
      .description = "Resolve localhost to any family (may get both IPv4 and IPv6)"
    },

    // Edge cases
    {
      .name = "Max results = 1",
      .hostname = "localhost",
      .max_results = 1,
      .ai_family = AF_UNSPEC,
      .expect_success = 1,
      .description = "Limit to 1 result (should not have semicolons)"
    },
    {
      .name = "Max results = 2",
      .hostname = "localhost",
      .max_results = 2,
      .ai_family = AF_UNSPEC,
      .expect_success = 1,
      .description = "Limit to 2 results"
    },
    {
      .name = "Invalid hostname",
      .hostname = "this.hostname.definitely.does.not.exist.invalid",
      .max_results = 10,
      .ai_family = AF_INET,
      .expect_success = 0,
      .description = "Should fail with invalid hostname"
    },
    {
      .name = "Empty hostname",
      .hostname = "",
      .max_results = 10,
      .ai_family = AF_INET,
      .expect_success = 0,
      .description = "Should fail with empty hostname"
    },

    // Real-world tests (may fail if no network)
    {
      .name = "google.com IPv4",
      .hostname = "google.com",
      .max_results = 10,
      .ai_family = AF_INET,
      .expect_success = 1,
      .description = "Resolve google.com to IPv4 (may return multiple IPs)"
    },
    {
      .name = "google.com IPv6",
      .hostname = "google.com",
      .max_results = 10,
      .ai_family = AF_INET6,
      .expect_success = 1,
      .description = "Resolve google.com to IPv6 (may fail if no IPv6)"
    },

    // Numeric IP tests
    {
      .name = "Numeric IPv4",
      .hostname = "127.0.0.1",
      .max_results = 10,
      .ai_family = AF_INET,
      .expect_success = 1,
      .description = "Numeric IPv4 address (should return itself)"
    },
    {
      .name = "Numeric IPv6",
      .hostname = "::1",
      .max_results = 10,
      .ai_family = AF_INET6,
      .expect_success = 1,
      .description = "Numeric IPv6 address (should return itself)"
    },
  };

  int num_tests = sizeof(tests) / sizeof(tests[0]);

  for (int i = 0; i < num_tests; i++) {
    run_test(&tests[i]);
  }

  printf("\n=== Test Suite Complete ===\n");
  printf("Total tests run: %d\n", num_tests);
  printf("\nNote: Some tests (google.com, IPv6) may fail depending on network availability.\n");
  printf("This is expected and not a bug in resolve_host().\n");

  return 0;
}
