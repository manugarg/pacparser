

PACPARSER = libpacparser.a
JS = libjs.a
HEADER = pacparser.h
MFILE = Makefile

ifeq ($(OS),Windows_NT)
	PACPARSER = pacparser.lib
	JS = js.lib
	MFILE = Makefile.win32
endif


clean:
	@pwsh -Command "if(Test-Path ./includes){Remove-Item -Recurse -Force ./includes}"
	@pwsh -Command "if(Test-Path ./src/bindings.rs){Remove-Item -Recurse -Force ./src/bindings.rs}"
	@pwsh -Command "if(Test-Path ./libs){Remove-Item -Recurse -Force ./libs}"
	cargo clean
	make -C ../src -f ../src/$(MFILE) clean

d:
	@pwsh -Command "if (!(Test-Path ./libs)){New-Item -ItemType Directory -Path ./libs}"
	@pwsh -Command "if (!(Test-Path ./includes)){New-Item -ItemType Directory -Path ./includes}"


$(PACPARSER): d ./includes/$(HEADER)
	make -C ../src  -f ../src/$(MFILE) staticlib
	@pwsh -Command "Copy-Item ../src/$(PACPARSER) ./libs/$(PACPARSER)"
	@pwsh -Command "Copy-Item ../src/$(JS) ./libs/$(PACPARJSSER)"

./includes/$(HEADER):
	@pwsh -Command "Copy-Item ../src/$(HEADER) ./includes/$(HEADER)"


pacparser: $(PACPARSER)


build:
	cargo build

release:  $(PACPARSER)
	cargo build --release

test:  $(PACPARSER)
	cargo test -- --nocapture

lint:
	cargo clippy --all-targets --all-features -- -D warnings
