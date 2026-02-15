INSTALL ?= install
PREFIX ?= /usr
CFLAGS := -O3 -std=c11 -Wall -pedantic -D_POSIX_C_SOURCE=200809L $(CFLAGS)

FUSE_FLAGS = -DFUSE_USE_VERSION=28 -D_FILE_OFFSET_BITS=64
FUSE_LIB = -lfuse
ifeq ($(shell uname), Darwin)
	FUSE_FLAGS += -I/usr/local/include/osxfuse
	FUSE_LIB = -losxfuse
else
	CFLAGS := -fPIC $(CFLAGS)
endif

CPPFLAGS := -Iinclude $(FUSE_FLAGS) $(CPPFLAGS)

.PHONY: all clean install uninstall

all: libidmap.a libfusemod_idmap.so

libidmap.a: lib/idmap.o
	$(AR) rcs $@ $^

libfusemod_idmap.so: lib/idmap.o src/idmapfuse.o
	$(CC) $(LDFLAGS) -shared -o $@ $^ $(FUSE_LIB) $(LDLIBS) -lpthread

clean:
	$(RM) lib/idmap.o libidmap.a src/idmapfuse.o libfusemod_idmap.so

install: libfusemod_idmap.so
	$(INSTALL) $< $(PREFIX)/lib/

uninstall:
	$(RM) $(PREFIX)/lib/libfusemod_idmap.so
