CC ?= cc
AR ?= ar
RANLIB ?= ranlib
INSTALL ?= install
PREFIX ?= /usr
CFLAGS := -O3 -std=gnu11 $(CFLAGS)

FUSE_FLAGS = -DFUSE_USE_VERSION=28 -D_FILE_OFFSET_BITS=64
FUSE_LIB = -lfuse
ifeq ($(shell uname), Darwin)
	FUSE_FLAGS += -I/usr/local/include/osxfuse
	FUSE_LIB = -losxfuse
else
	CFLAGS := -fPIC $(CFLAGS)
endif

.PHONY: all clean install uninstall

all: libidmap.a libfusemod_idmap.so

%.o: %.c
	$(CC) $(CFLAGS) -Iinclude/ $(FUSE_FLAGS) -c -o $*.o $^

libidmap.a: lib/idmap.o
	$(AR) rcs $@ $^

libfusemod_idmap.so: lib/idmap.o src/idmapfuse.o
	$(CC) $(CFLAGS) -shared -o $@ $^ $(FUSE_LIB) -lpthread

clean:
	rm -f lib/idmap.o libidmap.a src/idmapfuse.o libfusemod_idmap.so

install: libfusemod_idmap.so
	install $< $(PREFIX)/lib/

uninstall:
	rm -f $(PREFIX)/lib/libfusemod_idmap.so
