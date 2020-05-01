

LIBS = ./libs/libpacparser.a ./libs/libjs.a
HEADERS = ./includes/pacparser.h ./includes/pac_utils.h



clean:
	rm -rf ./libs ./includes ./src/bindings.rs
	cargo clean

	make clean -C ../src

d:
	mkdir -p ./libs ./includes


$(LIBS): d $(HEADERS)
	make -C ../src
	cp ../src/libpacparser.a ./libs/
	cp ../src/libjs.a ./libs/

$(HEADERS):
	cp ../src/pacparser.h ./includes/
	cp ../src/pac_utils.h ./includes/


build: $(LIBS)
	cargo build

release: $(LIBS)
	cargo build --release

test: $(LIBS)
	cargo test
