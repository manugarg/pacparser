jsapi: js-buildstamp

jslib: js-buildstamp
	cd js/src
	find . -name "libjs.a" -exec cp {} .. \;

js-buildstamp:
	tar xzvf js.tar.gz
	# Create $(OBJDIR), which is defined in config.mk
	# This file is required to build libjs.a alone. 
	echo 'include config.mk\nall:\n\tmkdir -p $$(OBJDIR)' > js/src/temp.mk
	DEPTH=. make -C js/src -f temp.mk
	rm js/src/temp.mk
	CFLAGS="$(SMCFLAGS)" $(MAKE) -C js/src -f Makefile.ref libjs.a
	find js/src -name "jsautocfg.h" -exec cp {} js/src \;
	touch js-buildstamp

clean:
	rm -rf js js-buildstamp
