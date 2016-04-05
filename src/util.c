// Author: Stefano Lattarini <slattarini@google.com>
//
// Private utility functions used by the pacparser project.
//
// pacparser is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published
// by the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// pacparser is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with pacparser.  If not, see <http://www.gnu.org/licenses/>.

#include "util.h"

#define STRSIZE(s) (s ? strlen(s) + 1 : 0)

// You must free the result if result is non-NULL.
char *
str_replace(const char *orig, char *rep, char *with)
{
  char *copy = strdup(orig);

  char *result;  // the returned string
  char *ins;     // the next insert point
  char *tmp;     // varies
  int count;     // number of replacements
  int len_front; // distance between rep and end of last rep
  int len_rep  = strlen(rep);
  int len_with = strlen(with);

  // Get the count of replacements
  ins = copy;
  for (count = 0; (tmp = strstr(ins, rep)); ++count) {
    ins = tmp + len_rep;
  }

  tmp = result = malloc(strlen(copy) + (len_with - len_rep) * count + 1);

  // First time through the loop, all the variable are set correctly
  // from here on,
  //    tmp points to the end of the result string
  //    ins points to the next occurrence of rep in copy
  //    copy points to the remainder of copy after "end of rep"
  while (count--) {
      ins = strstr(copy, rep);
      len_front = ins - copy;
      tmp = strncpy(tmp, copy, len_front) + len_front;
      tmp = strcpy(tmp, with) + len_with;
      copy += len_front + len_rep;  // move to next "end of rep"
  }
  strcpy(tmp, copy);
  free(copy);
  return result;
}

char *
concat_strings(const char *mallocd_str1, const char *str2)
{
  int size1 = STRSIZE(mallocd_str1);
  int size2 = STRSIZE(str2);

  if (!size1)
    return strdup(str2);

  char *mallocd_result;
  if ((mallocd_result = realloc(mallocd_str1, size1 + size2)) == NULL)
    return NULL;
  if (size2)
    strcat(mallocd_result, str2);

  return mallocd_result;
}
