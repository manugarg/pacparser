// libFuzzer harness for the PAC parsing path. QuickJS is already fuzzed
// upstream, so the target here is pacparser's C layer on top of it. See
// fuzz/README.md for build/run instructions and the input format.

#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "pacparser.h"

// Discard pacparser's error/alert output; otherwise it floods the fuzzer log.
static int silent_printer(const char *fmt, va_list ap) {
  (void)fmt;
  (void)ap;
  return 0;
}

int LLVMFuzzerInitialize(int *argc, char ***argv) {
  (void)argc;
  (void)argv;
  pacparser_set_error_printer(silent_printer);
  return 0;
}

// Returns the next NUL-delimited field as a heap string the caller must free.
static char *take_field(const uint8_t *data, size_t size, size_t *pos) {
  size_t start = *pos;
  size_t i = start;
  while (i < size && data[i] != '\0') i++;
  size_t len = i - start;
  char *out = (char *)malloc(len + 1);
  if (!out) return NULL;
  memcpy(out, data + start, len);
  out[len] = '\0';
  *pos = (i < size) ? i + 1 : size;
  return out;
}

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
  if (size == 0) return 0;

  size_t pos = 0;
  char *script = take_field(data, size, &pos);
  char *url = take_field(data, size, &pos);
  char *host = take_field(data, size, &pos);
  if (!script || !url || !host) {
    free(script);
    free(url);
    free(host);
    return 0;
  }
  if (url[0] == '\0')  { free(url);  url  = strdup("http://example.com/"); }
  if (host[0] == '\0') { free(host); host = strdup("example.com"); }

  // A fresh engine per input keeps iterations independent.
  if (pacparser_init()) {
    if (pacparser_parse_pac_string(script)) {
      pacparser_find_proxy(url, host);  // result is owned and freed by pacparser
    }
    pacparser_cleanup();
  }

  free(script);
  free(url);
  free(host);
  return 0;
}
