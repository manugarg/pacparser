#ifndef PACPARSER_UTIL_H_
#define PACPARSER_UTIL_H_

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#ifdef XP_UNIX
#  include <unistd.h>
#endif

#define STREQ(s1, s2) (strcmp((s1), (s2)) == 0)
#define QUOTEME_(x) #x
#define QUOTEME(x) QUOTEME_(x)

int string_list_len(const char **list);
void deep_free_string_list(const char **list);
char **measure_and_dup_string_list(const char **original, int *len_ptr);
char **dup_string_list(const char **original);
char *join_string_list(const char **list, const char *separator);
char **concatenate_and_dup_string_lists(const char **head, const char **tail);
char **append_to_string_list(const char **list_ptr, const char *str);

#endif  // PACPARSER_UTIL_H_
