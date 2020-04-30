


build_pacparser:
	make -C src

clean:
	rm -rf rust_ffi/libs
	rm -rf rust_ffi/includes
	rm -f rust_ffi/src/bindings.rs
	cd rust_ffi && cargo clean

	make clean -C src


build_rust: build_pacparser
	mkdir -p rust_ffi/libs
	mkdir -p rust_ffi/includes
	cp src/pacparser.h rust_ffi/includes/
	cp src/pac_utils.h rust_ffi/includes/
	cp src/pacparser.o rust_ffi/libs/
	cp src/libjs.a rust_ffi/libs/
	cp src/libpacparser.dylib rust_ffi/libs/

	cd rust_ffi && cargo build
