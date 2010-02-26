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
  LDFLAGS += -Wl,-soname=$(LIBRARY)
endif
ifeq ($(OS_ARCH),Darwin)
  SO_SUFFIX = dylib
  MKSHLIB = $(CC) -dynamiclib -framework System -install_name $(PREFIX)/lib/$(notdir $@)
endif

LIB_VER = 1
CFLAGS = -g -DXP_UNIX -Wall
SHFLAGS = -fPIC

ifndef PYTHON
  PYTHON = python
endif

NOSO = clean

ifndef SM_LIB
ifeq (yes, $(shell [ -e /usr/lib/libjs.$(SO_SUFFIX) -o -e /usr/local/lib/libjs.$(SO_SUFFIX) ] && echo yes))
  SM_LIB = -ljs
else
  ifeq (yes, $(shell [ -e /opt/local/lib/libjs.$(SO_SUFFIX) ] && echo yes))
    SM_LIB = -ljs
    LDFLAGS += -L/opt/local/lib
  else
    ifeq (yes, $(shell [ -e /usr/lib/libsmjs.$(SO_SUFFIX) ] && echo yes))
      SM_LIB = -lsmjs
    else
      ifeq (yes, $(shell [ -e /usr/lib/libmozjs.$(SO_SUFFIX) ] && echo yes))
        SM_LIB = -lmozjs
      endif
    endif
  endif
endif
endif

ifndef SM_INC
ifeq (yes, $(shell [ -e /usr/local/include/js ] && echo yes))
  SM_INC = -I/usr/local/include/js
else
  ifeq (yes, $(shell [ -e /opt/local/include/js ] && echo yes))
    SM_INC = -I/opt/local/include/js
  else
    ifeq (yes, $(shell [ -e /usr/include/smjs ] && echo yes))
      SM_INC = -I/usr/include/smjs
    else
      ifeq (yes, $(shell [ -e /usr/include/mozjs ] && echo yes))
        SM_INC = -I/usr/include/mozjs
      endif
    endif
  endif
endif
endif

ifeq ($(NOSO), $(filter-out $(MAKECMDGOALS),$(NOSO)))
  ifndef SM_LIB
  $(error SpiderMonkey library not found. See 'README_SM' file.)
  else
    LDFLAGS += ${SM_LIB}
  endif
  ifdef SM_INC
    CFLAGS += ${SM_INC}
  else
    $(error SpiderMonkey api not found. See 'README_SM' file.)
  endif
endif

LIBRARY = libpacparser.$(SO_SUFFIX).$(LIB_VER)
LIB_PREFIX = $(DESTDIR)$(PREFIX)/lib
INC_PREFIX = $(DESTDIR)$(PREFIX)/include
BIN_PREFIX = $(DESTDIR)$(PREFIX)/bin
MAN_PREFIX = $(DESTDIR)$(PREFIX)/share/man

.PHONY: clean pymod install-pymod
all: pactester

pacparser.o: pacparser.c pac_utils.h
	$(CC) $(CFLAGS) $(SHFLAGS) -c pacparser.c -o pacparser.o
	touch pymod/pacparser_o_buildstamp

$(LIBRARY): pacparser.o
	$(MKSHLIB) -o $(LIBRARY) pacparser.o $(LDFLAGS)

libpacparser.$(SO_SUFFIX): $(LIBRARY)
	ln -sf $(LIBRARY) libpacparser.$(SO_SUFFIX)

pactester: pactester.c pacparser.h libpacparser.$(SO_SUFFIX)
	$(CC) pactester.c -o pactester -lpacparser -L. -I.

install: all
	install -d $(LIB_PREFIX) $(INC_PREFIX) $(BIN_PREFIX)
	install -m 644 $(LIBRARY) $(LIB_PREFIX)/$(LIBRARY)
	ln -sf $(LIBRARY) $(LIB_PREFIX)/libpacparser.$(SO_SUFFIX)
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
	cd pymod && LDFLAGS="$(LDFLAGS)" SHFLAGS="$(SHFLAGS)" MKSHLIB="$(MKSHLIB)" $(PYTHON) setup.py

install-pymod: pymod
	cd pymod && LIB_PREFIX="$(LIB_PREFIX)" $(PYTHON) setup.py install

clean:
	rm -f libpacparser.$(SO_SUFFIX) $(LIBRARY) pacparser.o pactester pymod/pacparser_o_buildstamp
	cd pymod && python setup.py clean
