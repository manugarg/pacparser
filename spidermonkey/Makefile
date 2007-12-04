js: js-buildstamp

js-buildstamp:
	tar xzvf js*.tar.gz
	cd js/src; $(MAKE) -f Makefile.ref
	find js/src -name "jsautocfg.h" -exec cp {} js/src \;
	touch js-buildstamp

install: js
	cd js/src
	find . -name "libjs.so" -exec install -m 644 {} /usr/lib/ \;
	mkdir -p /usr/include/js
	install -m 644 js/src/*.h /usr/include/js/

clean:
	rm -rf js js-buildstamp
