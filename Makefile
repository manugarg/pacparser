# Copyright (C) 2007 Manu Garg.
# Author: Manu Garg <manugarg@gmail.com>
#
# Makefile for pacparser. Please read README file included with this package
# for more information about pacparser.
#
# pacparser is free software; you can redistribute it and/or
# modify it under the terms of the GNU Lesser General Public
# License as published by the Free Software Foundation; either
# version 3 of the License, or (at your option) any later version.

# pacparser is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# Lesser General Public License for more details.

# You should have received a copy of the GNU Lesser General Public
# License along with this library; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA

PREFIX ?= /usr
OS_ARCH := $(subst /,_,$(shell uname -s | sed /\ /s//_/))

LIBRARY_NAME = libpacparser
LIB_VER = 1

ifeq ($(OS_ARCH),Linux)
  SO_SUFFIX = so
  LIBRARY = $(LIBRARY_NAME).$(SO_SUFFIX).$(LIB_VER)
  MKSHLIB = $(CC) -shared
  LIB_OPTS = -Wl,-soname=$(LIBRARY)
endif
ifeq ($(OS_ARCH),Darwin)
  SO_SUFFIX = dylib
  LIBRARY = $(LIBRARY_NAME).$(LIB_VER).$(SO_SUFFIX)
  MKSHLIB = $(CC) -dynamiclib -framework System
  LIB_OPTS = -install_name $(PREFIX)/lib/$(notdir $@)
endif

CFLAGS = -g -DXP_UNIX -Wall
SHFLAGS = -fPIC

ifndef PYTHON
  PYTHON = python
endif

# Spidermonkey library.
CFLAGS += -Ispidermonkey/js/src
LDFLAGS += -lm

LIBRARY_LINK = $(LIBRARY_NAME).$(SO_SUFFIX)
PREFIX := $(DESTDIR)$(PREFIX)
LIB_PREFIX = $(PREFIX)/lib
INC_PREFIX = $(PREFIX)/include
BIN_PREFIX = $(PREFIX)/bin
MAN_PREFIX = $(PREFIX)/share/man

.PHONY: clean pymod install-pymod
all: pactester

jsapi: spidermonkey/js.tar.gz
	cd spidermonkey && $(MAKE) jsapi

libjs.a: spidermonkey/js.tar.gz
	cd spidermonkey && $(MAKE) jslib

pacparser.o: pacparser.c pac_utils.h jsapi
	$(CC) $(CFLAGS) $(SHFLAGS) -c pacparser.c -o pacparser.o
	touch pymod/pacparser_o_buildstamp

$(LIBRARY): pacparser.o libjs.a
	$(MKSHLIB) $(LIB_OPTS) -o $(LIBRARY) pacparser.o libjs.a $(LDFLAGS)

$(LIBRARY_LINK): $(LIBRARY)
	ln -sf $(LIBRARY) $(LIBRARY_LINK)

pactester: pactester.c pacparser.h $(LIBRARY_LINK)
	$(CC) pactester.c -o pactester -lpacparser -L. -I.

install: all
	install -d $(LIB_PREFIX) $(INC_PREFIX) $(BIN_PREFIX)
	install -m 644 $(LIBRARY) $(LIB_PREFIX)/$(LIBRARY)
	ln -sf $(LIBRARY) $(LIB_PREFIX)/$(LIBRARY_LINK)
	install -m 755 pactester $(BIN_PREFIX)/pactester
	install -m 644 pacparser.h $(INC_PREFIX)/pacparser.h
	# install pactester manpages
	install -d $(MAN_PREFIX)/man1/
	(test -d docs && install -m 644 docs/*.1 $(MAN_PREFIX)/man1/) || /bin/true
	# install pacparser manpages
	install -d $(MAN_PREFIX)/man3/
	(test -d docs && install -m 644 docs/*.3 $(MAN_PREFIX)/man3/) || /bin/true
	# install html docs
	install -d $(PREFIX)/share/doc/pacparser/html/
	(test -d docs/html && install -m 644 docs/html/* $(PREFIX)/share/doc/pacparser/html/) || /bin/true
	# install examples
	install -d $(PREFIX)/share/doc/pacparser/examples/
	(test -d examples && install -m 644 examples/* $(PREFIX)/share/doc//pacparser/examples/) || /bin/true

# Targets to build python module
pymod: pacparser.o pacparser.h
	cd pymod && ARCHFLAGS="" $(PYTHON) setup.py build

install-pymod: pymod
	cd pymod && ARCHFLAGS="" $(PYTHON) setup.py install --prefix="$(PREFIX)"

clean:
	rm -f $(LIBRARY_LINK) $(LIBRARY) libjs.a pacparser.o pactester pymod/pacparser_o_buildstamp
	cd pymod && python setup.py clean
	cd spidermonkey && $(MAKE) clean
