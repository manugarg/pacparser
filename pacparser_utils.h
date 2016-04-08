// Author: Stefano Lattarini <slattarini@gmail.com>,
//
// Private utility functions to be used in the pacparser library.
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

#ifndef PACPARSER_UTILS_H_
#define PACPARSER_UTILS_H_

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#ifdef XP_UNIX
#  include <unistd.h>
#endif

#define STREQ(s1, s2) (strcmp((s1), (s2)) == 0)

#endif  // PACPARSER_UTILS_H_
