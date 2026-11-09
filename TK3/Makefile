# Bagian Adrian - build static/dynamic dan clean.
# TODO: Pastikan SOURCES/HEADERS mencakup variant8.c/variant8.h serta semua
# modul yang dipakai. Pertahankan target kompilasi statis (-static) dan dinamis.
CC = gcc

SOURCES = main.c banner.c malloc_module.c mmap_module.c \
	memory_monitor.c logging.c variant8.c
HEADERS = banner.h malloc_module.h mmap_module.h memory_monitor.h \
	logging.h variant8.h

.PHONY: all static dynamic clean

all: static dynamic

static: memory_static

dynamic: memory_dynamic

memory_static: $(SOURCES) $(HEADERS)
	$(CC) -static $(SOURCES) -o $@

memory_dynamic: $(SOURCES) $(HEADERS)
	$(CC) $(SOURCES) -o $@

clean:
	rm -f memory_static memory_dynamic
