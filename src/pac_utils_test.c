#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "pac_utils.h"

#define STREQ(s1, s2) (strcmp((s1), (s2)) == 0)

// Unit tests
int main() {
    // Test case 1: Single replacement
    assert(STREQ("Hello, universe!", str_replace("Hello, world!", "world", "universe")));

    // Test case 2: Multiple replacements
    assert(STREQ("one cat, two cat, red cat, blue cat",
           str_replace("one fish, two fish, red fish, blue fish", "fish", "cat")));
    // Expected output: "one cat, two cat, red cat, blue cat"

    // Test case 3: No replacements
    assert(STREQ("AI is amazing", str_replace("AI is amazing", "robot", "AI")));
    
    // Test case 4: Empty original string
    assert(STREQ("", str_replace("", "hello", "world")));
    
    // Test case 5: Empty replacement string
    assert(STREQ("Hello, world!", str_replace("Hello, world!", "", "universe")));

    // Test case 6: Empty "with" string
    assert(STREQ("Hello, !", str_replace("Hello, world!", "world", "")));

    // Test case 7: Complex replacements
    assert(STREQ("abcdeXYZcba", str_replace("abcdeedcba", "ed", "XYZ")));

    return 0;
}