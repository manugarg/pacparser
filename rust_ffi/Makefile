

PACPARSER = libpacparser.a
JS = libjs.a
HEADER = pacparser.h
MFILE = Makefile

ifeq ($(OS),Windows_NT)
	PACPARSER = ./libs/pacparser.lib
	JS = ./libs/lib.lib
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


$(PACPARSER): d $(HEADER)
	make -C ../src  -f ../src/$(MFILE) staticlib
	cp ../src/$(PACPARSER) ./libs/$(JS)
	cp ../src/$(JS) ./libs/$(JS)

$(HEADER):
	cp ../src/$(HEADER) ./includes/$(HEADER)

build: $(PACPARSER) $(JS)
	cargo build

release:  $(PACPARSER) $(JS)
	cargo build --release

test:  $(PACPARSER) $(JS)
	cargo test -- --nocapture

lint:
	cargo clippy --all-targets --all-features -- -D warnings
