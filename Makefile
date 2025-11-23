CFLAGS=-Wall -Wextra -Wno-unused-parameter -g
PKG_CONFIG ?= pkg-config

WAYLAND_FLAGS = $(shell $(PKG_CONFIG) wayland-client --cflags --libs)
WAYLAND_PROTOCOLS_DIR = $(shell $(PKG_CONFIG) wayland-protocols --variable=pkgdatadir)
GTK4_FLAGS = $(shell $(PKG_CONFIG) gtk4 --cflags --libs)

SECURITY_CONTEXT_PROTOCOL=$(WAYLAND_PROTOCOLS_DIR)/staging/security-context/security-context-v1.xml

HEADERS=security-context-client-protocol.h wayland_security_context.h
LIBSRC=wayland_security_context.c security-context-protocol.c
LIBOBJ=wayland_security_context.o security-context-protocol.o

DEMO_SRC=gtk4_hello.c
DEMO_OBJ=gtk4_hello.o

libwayland_security_context.a: $(LIBOBJ)
	ar rcs $@ $(LIBOBJ)

security-context-client-protocol.h:
	wayland-scanner client-header $(SECURITY_CONTEXT_PROTOCOL) $@

security-context-protocol.c:
	wayland-scanner private-code $(SECURITY_CONTEXT_PROTOCOL) $@

wayland_security_context.o: wayland_security_context.c wayland_security_context.h security-context-client-protocol.h
	clang $(CFLAGS) -c wayland_security_context.c $(WAYLAND_FLAGS)

security-context-protocol.o: security-context-protocol.c security-context-client-protocol.h
	clang $(CFLAGS) -c security-context-protocol.c $(WAYLAND_FLAGS)

gtk4_hello.o: gtk4_hello.c wayland_security_context.h
	clang $(CFLAGS) -c gtk4_hello.c $(GTK4_FLAGS)

gtk4_hello: libwayland_security_context.a $(DEMO_OBJ)
	clang $(CFLAGS) -o $@ $(DEMO_OBJ) libwayland_security_context.a $(WAYLAND_FLAGS) $(GTK4_FLAGS)

.PHONY: clean
clean:
	rm -f gtk4_hello *.o libwayland_security_context.a security-context-protocol.c security-context-client-protocol.h