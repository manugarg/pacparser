// pac_utils_dump.c - Dumps pac_utils.h JavaScript as a pac_utils.js file
// for use by the web-based PAC file tester.
//
// Build:  cc -o pac_utils_dump pac_utils_dump.c
// Usage:  ./pac_utils_dump > ../web/pac_utils.js
//         (or via: make -C src pac_utils_js)

#include <stdio.h>
#include "pac_utils.h"

int main() {
    printf("// Auto-generated from src/pac_utils.h — do not edit manually.\n");
    printf("// Regenerate with: make -C src pac_utils_js\n");
    printf("//\n");
    printf("// Exported as a source string (PAC_UTILS_JS) rather than executed\n");
    printf("// directly, so that the eval'd PAC sandbox can define its own\n");
    printf("// dnsResolve() / myIpAddress() in the same scope as the utility\n");
    printf("// functions — keeping DNS mocking correct via lexical scoping.\n");
    // String.raw preserves backslashes as-is, preventing the JS template
    // literal parser from mangling regex patterns like /\*/g → /*/g (invalid).
    printf("const PAC_UTILS_JS = String.raw`\n");
    printf("%s", pacUtils);
    printf("`;\n");
    return 0;
}
