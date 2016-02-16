// Author: Stefano Lattarini <slattarini@gmail.com>,
//
// This file defines private utility functions to be used in the pacparser
// library.
//
// Pacparser is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published
// by the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// Pacparser is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with Pacparser.  If not, see <http://www.gnu.org/licenses/>.

#ifndef PACPARSER_UTIL_H_
#define PACPARSER_UTIL_H_

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#ifdef XP_UNIX
#  include <unistd.h>
#endif

#define STREQ(s1, s2) (strcmp((s1), (s2)) == 0)
#define FREE(x) free((void *)(x))  // silence annoying compiler warnings

char *str_replace(const char *orig, char *rep, char *with);
int string_list_len(const char **list);
void deep_free_string_list(const char **list);
char **measure_and_dup_string_list(const char **original, int *len_ptr);
char **dup_string_list(const char **original);
char *join_string_list(const char **list, const char *separator);
char **concatenate_and_dup_string_lists(const char **head, const char **tail);
char **append_to_string_list(const char **list_ptr, const char *str);

#endif  // PACPARSER_UTIL_H_
