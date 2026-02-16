INSTALL ?= install
PREFIX ?= /usr
CFLAGS := -O3 -std=c11 -Wall -Werror -pedantic -D_POSIX_C_SOURCE=200809L $(CFLAGS)

FUSE_FLAGS = -D_FILE_OFFSET_BITS=64
OS=$(shell uname)
ifeq ($(OS), Darwin)
	FUSE_LDFLAGS = -L/usr/local/lib
	ifeq ($(shell [ -e /usr/local/lib/libosxfuse.dylib ] && echo 1), 1)
		FUSE_FLAGS += -I/usr/local/include/osxfuse
		FUSE_LIB = -losxfuse
	else ifeq ($(shell [ -e /usr/local/lib/libfuse.dylib -o -e /usr/local/lib/libfuse3.dylib ] && echo 1), 1)
		FUSE_FLAGS += -I/usr/local/include
		FUSE_FLAGS += -Wno-language-extension-token -D_DARWIN_C_SOURCE
	else ifeq ($(shell [ -e /usr/local/lib/libfuse-t.dylib ] && echo 1), 1)
		FUSE_FLAGS += -I/usr/local/include/fuse
		FUSE_LIB = -lfuse-t
	endif
else
	CFLAGS := -D_GNU_SOURCE -fPIC $(CFLAGS)
endif
ifneq ($(filter $(OS),FreeBSD DragonFly),)
	FUSE_FLAGS += -I/usr/local/include
	FUSE_LDFLAGS = -L/usr/local/lib
else ifeq ($(OS), NetBSD)
	FUSE_FLAGS += -I/usr/pkg/include
	FUSE_LDFLAGS = -L/usr/pkg/lib/ -Wl,-R/usr/pkg/lib
endif

ifeq ($(shell echo | $(CC) $(FUSE_FLAGS) -DFUSE_USE_VERSION=30 -include fuse3/fuse.h -xc -fsyntax-only - > /dev/null 2> /dev/null; echo $$?), 0)
	FUSE_FLAGS += -DFUSE_USE_VERSION=35
	FUSE_LIB ?= -lfuse3
else
	FUSE_FLAGS += -DFUSE_USE_VERSION=29
	FUSE_LIB ?= -lfuse
endif
ifeq ($(shell echo 'struct statx _;' | $(CC) $(CFLAGS) -include sys/stat.h -xc -fsyntax-only - > /dev/null 2> /dev/null; echo $$?), 0)
	FUSE_FLAGS += -DHAVE_STATX
endif

CPPFLAGS := -Iinclude $(FUSE_FLAGS) $(CPPFLAGS)
LDFLAGS := $(FUSE_LDFLAGS) $(LDFLAGS)

.PHONY: all clean install uninstall

all: libidmap.a libfusemod_idmap.so

libidmap.a: lib/idmap.o
	$(AR) rcs $@ $^

libfusemod_idmap.so: lib/idmap.o src/idmapfuse.o
	$(CC) $(LDFLAGS) -shared -o $@ $^ $(FUSE_LIB) $(LDLIBS)

clean:
	$(RM) lib/idmap.o libidmap.a src/idmapfuse.o libfusemod_idmap.so

install: libfusemod_idmap.so
	$(INSTALL) $< $(PREFIX)/lib/

uninstall:
	$(RM) $(PREFIX)/lib/libfusemod_idmap.so
