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

LIB_VER=1
CFLAGS=-g -DXP_UNIX -Wall
LDFLAGS=-shared -soname=libpacparser.so.${LIB_VER}

NOSO=clean js install-js

ifeq (yes, $(shell [ -e /usr/lib/libjs.so -o -e /usr/local/lib/libjs.so ] && echo yes))
  SM_LIB= -ljs
else
  ifeq (yes, $(shell [ -e /usr/lib/libsmjs.so ] && echo yes))
    SM_LIB= -lsmjs
  else
    ifeq (yes, $(shell [ -e /usr/lib/libmozjs.so ] && echo yes))
      SM_LIB= -lmozjs
    endif
  endif
endif

ifeq (yes, $(shell [ -e /usr/include/js ] && echo yes))
  SM_INC= -I/usr/include/js
else
  ifeq (yes, $(shell [ -e /usr/include/smjs ] && echo yes))
    SM_INC= -I/usr/include/smjs
  else
    ifeq (yes, $(shell [ -e /usr/include/mozjs ] && echo yes))
      SM_INC= -I/usr/include/mozjs
    else
      ifeq (yes, $(shell [ -e spidermonkey/js/src ] && echo yes))
        SM_INC= -Ispidermonkey/js/src
      endif
    endif
  endif
endif

ifeq ($(NOSO), $(filter-out $(MAKECMDGOALS),$(NOSO)))
  ifndef SM_LIB
  $(error SpiderMonkey library not found. For unix based systems, to build \
          and install it, run 'make js' followed by 'sudo make install-js'.)
  else
    LDFLAGS+= ${SM_LIB}
  endif
  ifdef SM_INC
    CFLAGS+= ${SM_INC}
  else
    $(error SpiderMonkey api not found. It is required to build pacparser. Run \
	    'make js' now to get it)
  endif
endif


all: libpacparser.so

pacparser.o: pacparser.c pac_utils.h
	$(CC) $(CFLAGS) -c pacparser.c -o pacparser.o

libpacparser.so.${LIB_VER}: pacparser.o
	$(LD) -o libpacparser.so.${LIB_VER} pacparser.o $(LDFLAGS)

libpacparser.so: libpacparser.so.${LIB_VER}
	ln -sf libpacparser.so.${LIB_VER} libpacparser.so

install: all
	install -m 644 libpacparser.so.${LIB_VER} /usr/lib/libpacparser.so.${LIB_VER}
	ln -s libpacparser.so.${LIB_VER} /usr/lib/libpacparser.so

js:
	cd spidermonkey && $(MAKE)

install-js: js
	cd spidermonkey && $(MAKE) install

clean:
	rm -f libpacparser.so libpacparser.so.${LIB_VER} pacparser.o 
	cd spidermonkey && $(MAKE) clean
