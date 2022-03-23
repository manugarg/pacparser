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

# Include config.mk to get the variable $(OBJDIR).
# We need to create $(OBJDIR) first to be able to build libjs.a alone.

DEPTH = js/src
include js/src/config.mk

jsapi: js-buildstamp

jslib: js-buildstamp
	cd js/src
	find . -name "libjs.a" -exec cp {} . \;

js-buildstamp:
	mkdir -p js/src/$(OBJDIR)
	CFLAGS="$(SMCFLAGS)" $(MAKE) -C js/src -f Makefile.ref libjs.a
	find js/src -name "jsautocfg.h" -exec cp {} js/src \;
	touch js-buildstamp

clean:
	rm -rf js/src/$(OBJDIR)
	rm -f libjs.a js-buildstamp
