// Copyright (C) 2007 Manu Garg.
// Author: Manu Garg <manugarg@gmail.com>
//
// pac_utils.h defines some of the functions used by PAC files. This file is
// packaged with pacparser source code and is required for compiling pacparser.
// Please read README file included with this package for more information
// about pacparser.

// This file is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 3 of the License, or (at your option) any later version.

// pacparser is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Lesser General Public License for more details.

// You should have received a copy of the GNU Lesser General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301,
// USA

static const char *pacUtils =
// Note: The included file is derived from "nsProxyAutoConfig.js" file that
// comes with mozilla source code. Please check out the following for initial
// developer and contributors:
// http://lxr.mozilla.org/seamonkey/source/netwerk/base/src/nsProxyAutoConfig.js
#  include "pac_utils_js.h"
;

// You must free the result if result is non-NULL.
char *str_replace(const char *orig, char *rep, char *with) {
    char *tmporig = malloc(strlen(orig) + 1); // Copy of orig that we work with
    tmporig = strcpy(tmporig, orig);

    char *result;  // the return string
    char *ins;     // the next insert point
    char *tmp;     // varies
    int count;     // number of replacements
    int len_front; // distance between rep and end of last rep
    int len_rep  = strlen(rep);
    int len_with = strlen(with);

    // Get the count of replacements
    ins = tmporig;
    for (count = 0; (tmp = strstr(ins, rep)); ++count) {
        ins = tmp + len_rep;
    }

    result = malloc(strlen(tmporig) + (len_with - len_rep) * count + 1);

    // first time through the loop, all the variable are set correctly
    // from here on,
    //    tmp points to the end of the result string
    //    ins points to the next occurrence of rep in tmporig
    //    tmporig points to the remainder of tmporig after "end of rep"
    tmp = result;
    while (count--) {
        ins = strstr(tmporig, rep);
        len_front = ins - tmporig;
        tmp = strncpy(tmp, tmporig, len_front) + len_front;
        tmp = strcpy(tmp, with) + len_with;
        tmporig += len_front + len_rep; // move to next "end of rep"
    }
    strcpy(tmp, tmporig);
    return result;
}
