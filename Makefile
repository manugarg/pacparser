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

ifeq ($(OS_ARCH),Linux)
  SO_SUFFIX = so
  MKSHLIB = $(CC) -shared
  LIB_OPTS = -Wl,-soname=$(LIBRARY),-rpath=.
  BIN_OPTS = -Wl,-rpath=$(LIB_PREFIX)
endif
ifeq ($(OS_ARCH),Darwin)
  SO_SUFFIX = dylib
  MKSHLIB = $(CC) -dynamiclib -framework System
  LIB_OPTS = -install_name $(PREFIX)/lib/pacparser/$(notdir $@)
endif

LIB_VER = 1
CFLAGS = -g -DXP_UNIX -Wall
SHFLAGS = -fPIC

ifndef PYTHON
  PYTHON = python
endif

# Spidermonkey library.
CFLAGS += -Ispidermonkey/js/src
LDFLAGS += -ljs -L.

LIBRARY = libpacparser.$(SO_SUFFIX).$(LIB_VER)
JS_LIBRARY = libjs.$(SO_SUFFIX)
LIB_PREFIX = $(DESTDIR)$(PREFIX)/lib/pacparser
PYLIB_PREFIX = $(DESTDIR)$(PREFIX)/lib
INC_PREFIX = $(DESTDIR)$(PREFIX)/include
BIN_PREFIX = $(DESTDIR)$(PREFIX)/bin
MAN_PREFIX = $(DESTDIR)$(PREFIX)/share/man

.PHONY: clean pymod install-pymod
all: pactester

jsapi: spidermonkey/js.tar.gz
	cd spidermonkey && $(MAKE) jsapi

$(JS_LIBRARY): spidermonkey/js.tar.gz
	cd spidermonkey && SO_SUFFIX=$(SO_SUFFIX) $(MAKE) jslib
ifeq ($(OS_ARCH),Darwin)
	install_name_tool -id "@loader_path/libjs.dylib" $(JS_LIBRARY)
endif

pacparser.o: pacparser.c pac_utils.h jsapi
	$(CC) $(CFLAGS) $(SHFLAGS) -c pacparser.c -o pacparser.o
	touch pymod/pacparser_o_buildstamp

$(LIBRARY): pacparser.o $(JS_LIBRARY)
	$(MKSHLIB) $(LIB_OPTS) -o $(LIBRARY) pacparser.o $(LDFLAGS)

libpacparser.$(SO_SUFFIX): $(LIBRARY)
	ln -sf $(LIBRARY) libpacparser.$(SO_SUFFIX)

pactester: pactester.c pacparser.h libpacparser.$(SO_SUFFIX)
	$(CC) pactester.c -o pactester -lpacparser -L. -I. $(BIN_OPTS)

install: all
	install -d $(LIB_PREFIX) $(INC_PREFIX) $(BIN_PREFIX)
	install -m 644 $(LIBRARY) $(LIB_PREFIX)/$(LIBRARY)
	install -m 644 $(JS_LIBRARY) $(LIB_PREFIX)/$(JS_LIBRARY)
	ln -sf $(LIBRARY) $(LIB_PREFIX)/libpacparser.$(SO_SUFFIX)
	ln -sf pacparser/$(LIBRARY) $(LIB_PREFIX)/../libpacparser.$(SO_SUFFIX)
	install -m 755 pactester $(BIN_PREFIX)/pactester
	install -m 644 pacparser.h $(INC_PREFIX)/pacparser.h
	# install pactester manpages
	install -d $(MAN_PREFIX)/man1/
	(test -d docs && install -m 644 docs/*.1 $(MAN_PREFIX)/man1/) || /bin/true
	# install pacparser manpages
	install -d $(MAN_PREFIX)/man3/
	(test -d docs && install -m 644 docs/*.3 $(MAN_PREFIX)/man3/) || /bin/true
	# install html docs
	install -d $(DESTDIR)$(PREFIX)/share/doc/pacparser/html/
	(test -d docs/html && install -m 644 docs/html/* $(DESTDIR)$(PREFIX)/share/doc/pacparser/html/) || /bin/true
	# install examples
	install -d $(DESTDIR)$(PREFIX)/share/doc/pacparser/examples/
	(test -d examples && install -m 644 examples/* $(DESTDIR)$(PREFIX)/share/doc//pacparser/examples/) || /bin/true

# Targets to build python module
pymod: pacparser.o pacparser.h
	cd pymod && $(PYTHON) setup.py

install-pymod: pymod
	cd pymod && LIB_PREFIX="$(PYLIB_PREFIX)" $(PYTHON) setup.py install

clean:
	rm -f libpacparser.$(SO_SUFFIX) $(LIBRARY) $(JS_LIBRARY) pacparser.o pactester pymod/pacparser_o_buildstamp
	cd pymod && python setup.py clean
	cd spidermonkey && $(MAKE) clean
