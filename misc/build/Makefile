.PHONY: all test clean superclean

CBUILD_WORKSPACE := misc/build
CBUILD_TOOLCHAIN := @CBUILD_TOOLCHAIN@

all:
	cd "$(CBUILD_WORKSPACE)" && cbuild build -c Release -c Debug -T "$(CBUILD_TOOLCHAIN)"

test:
	cd "$(CBUILD_WORKSPACE)" && cbuild test -c Release -c Debug -c DebugASAN -c DebugTSAN -T "$(CBUILD_TOOLCHAIN)"

clean:
	cd "$(CBUILD_WORKSPACE)" && cbuild clean -T "$(CBUILD_TOOLCHAIN)"

superclean:
	cd "$(CBUILD_WORKSPACE)" && cbuild clean -T "$(CBUILD_TOOLCHAIN)" && cbuild drop-files
