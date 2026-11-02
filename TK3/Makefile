# Bagian Adrian - build static/dynamic, clean, dan verifikasi linking (ls -lh/ldd).
.PHONY: all static dynamic clean

all: static dynamic

static:
	gcc -static *.c -o memory_static

dynamic:
	gcc *.c -o memory_dynamic

clean:
	rm -f memory_static memory_dynamic
