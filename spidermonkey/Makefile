jsapi: js-buildstamp

jslib: js-buildstamp
	cd js/src
	find . -name "libjs.a" -exec cp {} .. \;

js-buildstamp:
	tar xzvf js.tar.gz
	cd js/src; $(MAKE) -f Makefile.ref
	find js/src -name "jsautocfg.h" -exec cp {} js/src \;
	touch js-buildstamp

clean:
	rm -rf js js-buildstamp
