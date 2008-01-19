# Copyright (C) 2007 Manu Garg.
# Author: Manu Garg <manugarg@gmail.com>
#
# Makefile for pacparser. Please read README file included with this package
# for more information about pacparser.
#
# pacparser is free software; you can redistribute it and/or
# modify it under the terms of the GNU Lesser General Public
# License as published by the Free Software Foundation; either
# version 2.1 of the License, or (at your option) any later version.

# pacparser is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# Lesser General Public License for more details.

# You should have received a copy of the GNU Lesser General Public
# License along with this library; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA

CFLAGS=-g -DXP_UNIX -Wall -I..

ifndef PY_VER
  PY_VER= 2.5
endif

ifndef PY_INC
ifeq (yes, $(shell [ -e /usr/include/python$(PY_VER)/Python.h ] && echo yes))
  PY_INC= -I/usr/include/python$(PY_VER)
else
  $(error Python headers not found in /usr/include/python$(PY_VER). Please \
	  install python headers. Set PY_INC variable to the location of \
	  python headers, if headers are not installed in the standard \
	  location. e.g. PY_INC=/usr/local/include/python$(PY_VER) make)
endif
endif

CFLAGS+=$(PY_INC)

# We need PIC code for shared libraries on x86_64 platform.
CPU_ARCH = $(shell uname -m)
ifeq ($(CPU_ARCH),x86_64)
  CFLAGS+= -fPIC
endif

all: _pacparser.so

pacparser_py.o: pacparser_py.c
	$(CC) $(CFLAGS) -c pacparser_py.c -o pacparser_py.o

_pacparser.so: pacparser_py.o pacparser_o_buildstamp
	$(LD) -soname=pacparser.so -o _pacparser.so ../pacparser.o pacparser_py.o $(LDFLAGS)

install: all
	install -d $(DESTDIR)/usr/lib/python$(PY_VER)/site-packages/pacparser
	install -m 644 *.{py,so} $(DESTDIR)/usr/lib/python$(PY_VER)/site-packages/pacparser/

clean:
	rm -f pacparser_py.o _pacparser.so
