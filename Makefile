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
LDFLAGS=-shared

ifndef PYTHON
  PYTHON=python
endif

# We need PIC code for shared libraries on x86_64 platform.
CPU_ARCH = $(shell uname -m)
ifeq ($(CPU_ARCH),x86_64)
  CFLAGS+= -fPIC
endif

NOSO=clean js install-js

ifndef SM_LIB
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
endif

ifndef SM_INC
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

.PHONY: clean js install-js pymod install-pymod

all: pactester

pacparser.o: pacparser.c pac_utils.h
	$(CC) $(CFLAGS) -c pacparser.c -o pacparser.o
	touch pymod/pacparser_o_buildstamp

libpacparser.so.${LIB_VER}: pacparser.o
	$(LD) -soname=libpacparser.so.${LIB_VER} -o libpacparser.so.${LIB_VER} pacparser.o $(LDFLAGS)

libpacparser.so: libpacparser.so.${LIB_VER}
	ln -sf libpacparser.so.${LIB_VER} libpacparser.so

pactester: pactester.c pacparser.h libpacparser.so
	$(CC) pactester.c -o pactester -lpacparser -L. -I.

install: all
	install -d $(DESTDIR)/usr/lib $(DESTDIR)/usr/include
	install -m 644 libpacparser.so.${LIB_VER} $(DESTDIR)/usr/lib/libpacparser.so.${LIB_VER}
	ln -sf libpacparser.so.${LIB_VER} $(DESTDIR)/usr/lib/libpacparser.so
	install -m 755 pactester $(DESTDIR)/usr/bin/pactester
	install -m 644 pacparser.h $(DESTDIR)/usr/include/pacparser.h
	# install manpages
	install -d $(DESTDIR)/usr/share/man/man3/
	(test -d docs && install -m 644 docs/*.3 $(DESTDIR)/usr/share/man/man3/) || /bin/true
	# install html docs
	install -d $(DESTDIR)/usr/share/doc/libpacparser/html/
	(test -d docs/html && install -m 644 docs/html/* $(DESTDIR)/usr/share/doc/libpacparser/html/) || /bin/true
	# install examples
	install -d $(DESTDIR)/usr/share/doc/libpacparser/examples/
	(test -d examples && install -m 644 examples/* $(DESTDIR)/usr/share/doc/libpacparser/examples/) || /bin/true

js:
	cd spidermonkey && $(MAKE)

install-js: js
	cd spidermonkey && $(MAKE) install

# Targets to build python module
pymod: pacparser.o pacparser.h
	cd pymod && LDFLAGS="$(LDFLAGS)" $(PYTHON) setup.py

install-pymod: pymod
	cd pymod && $(PYTHON) setup.py install

clean:
	rm -f libpacparser.so libpacparser.so.${LIB_VER} pacparser.o pactester pymod/pacparser_o_buildstamp
	cd pymod && python setup.py clean
	cd spidermonkey && $(MAKE) clean
