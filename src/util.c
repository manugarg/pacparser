// Author: Stefano Lattarini <slattarini@gmail.com>,
//
// Private utility functions used by the pacparser project.
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

#include "util.h"
#include <assert.h>

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

int
string_list_len(const char **list)
{
  // Calculate the length of the list, *not* including trailing NULL.
  int len = 0;
  if (list)
    while (list[len])
      len++;
  return len;
}

void
deep_free_string_list(const char **list)
{
  // Nothing to do if the list was the NULL pointer.
  if (!list)
    return;
  // Free the individual strings.
  int i;
  for (i = 0; list[i]; i++)
    free(list[i]);
  // Free the pointers to the already-freed strings.
  free(list);
}

char **
measure_and_dup_string_list(const char **original, int *len_ptr)
{
  int len = string_list_len(original);
  // Allocate space for the copied list. The '+1' is to account for the
  // trailing NULL pointer.
  char **copy = (char **) calloc(len + 1, sizeof(char **));
  // Copy all the strings from the original list.
  int i;
  for (i = 0; i < len; i++) {
    copy[i] = strdup(original[i]);
  }
  copy[len] = NULL;
  // Register the length of the list, if so asked.
  if (len_ptr)
    *len_ptr = len;
  // Return pointer to the copied list.
  return copy;
}

char **
dup_string_list(const char **original)
{
  return measure_and_dup_string_list(original, NULL);
}

char **
concatenate_and_dup_string_lists(const char **head, const char **tail)
{
  int head_len, tail_len, len, i;
  // Strdup all the strings of the original lists (so that we can copy around
  // pointers to them from now on) and calculate their length.
  head = (const char **) measure_and_dup_string_list(head, &head_len);
  tail = (const char **) measure_and_dup_string_list(tail, &tail_len);
  // The length of the concatenated list we are going to build.
  len = head_len + tail_len;
  // Allocate space for the copied list. The '+1' is to account for the
  // trailing NULL pointer.
  const char **concat = (const char **) calloc(len + 1, sizeof(char **));
  // Concatenate the strduped lists.
  for (i = 0; i < head_len; i++) {
    concat[i] = head[i];
  }
  for (i = 0; i < tail_len; i++) {
    concat[head_len + i] = tail[i];
  }
  concat[len] = NULL;
  // Do *not* use deep_free_string_list() here, as the strings these lists point
  // to are also pointed to by the concatenated list.
  free(head);
  free(tail);
   // Return pointer to the concatenated list.
  return (char **) concat;
}

char *
join_string_list(const char **list, const char *separator)
{
  assert(separator);
  int len = string_list_len(list);
  if (!len)
    return strdup("");
  // Calculate the number of bytes we need to store the result.
  int i, size = 0;
  for (i = 0; list[i]; i++) {
    size += strlen(list[i]);
  }
  // Account for size of separators between elements of the lists.
  size += (len - 1) * strlen(separator);
  // Account for the trailing '\0' char.
  size++;
  // Allocate memory to hold the result.
  char *result = (char *) calloc(size, sizeof(char *));
  // Build and return the result.
  for (i = 0; i < len; i++) {
    strcat(result, list[i]);
    if (i < len - 1)
      strcat(result, separator);
  }
  return result;
}

char **
append_to_string_list(const char **list, const char *str)
{
  int len = string_list_len(list);
  // Make sure we have space to append the string and the new terminating NULL.
  const char **extended_list = realloc(list, (len + 2) * sizeof(char **));
  // Add the string to be appended where the terminating NULL used to be, and
  // append a new NULL after that.
  extended_list[len] = str;
  extended_list[len + 1] = NULL;
  return extended_list;
}
