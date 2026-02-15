INSTALL ?= install
PREFIX ?= /usr
CFLAGS := -O3 -std=c11 -Wall -Werror -pedantic -D_POSIX_C_SOURCE=200809L $(CFLAGS)

FUSE_FLAGS = -DFUSE_USE_VERSION=28 -D_FILE_OFFSET_BITS=64
FUSE_LIB = -lfuse
OS=$(shell uname)
ifeq ($(OS), Darwin)
	FUSE_FLAGS += -I/usr/local/include/osxfuse
	FUSE_LIB = -losxfuse
else
	CFLAGS := -fPIC $(CFLAGS)
endif
ifneq ($(filter $(OS),FreeBSD DragonFly),)
	FUSE_FLAGS += -I/usr/local/include
	FUSE_LDFLAGS = -L/usr/local/lib
else ifeq ($(OS), NetBSD)
	FUSE_FLAGS += -I/usr/pkg/include
	FUSE_LDFLAGS = -L/usr/pkg/lib/ -Wl,-R/usr/pkg/lib
endif

CPPFLAGS := -Iinclude $(FUSE_FLAGS) $(CPPFLAGS)
LDFLAGS := $(FUSE_LDFLAGS) $(LDFLAGS)

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
