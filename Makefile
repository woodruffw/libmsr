# This currently builds a user space program and not a useful library

PREFIX = /usr

CFLAGS = -Wall -g -fPIC -std=c99 -pedantic -D_POSIX_C_SOURCE=200809L
LDFLAGS = -L. -lmsr

LIB = libmsr.a
LIBSRCS = libmsr.c serialio.c msr206.c
LIBOBJS = $(LIBSRCS:.c=.o)

all: $(LIB)

debug: CFLAGS += -DDEBUG -g
debug: all

$(LIB): $(LIBOBJS)
	ar rcs $(LIB) $(LIBOBJS)

.c.o:
	$(CC) $(CFLAGS) -o $@ -c $<

doc:
	VERS=$(shell git describe --tags --dirty --always 2>/dev/null \
			|| git rev-parse --short HEAD) \
	doxygen Doxyfile

install: $(LIB)
	install -m644 -D $(LIB) $(PREFIX)/lib
	install -m644 -D libmsr.h $(PREFIX)/include

uninstall:
	rm -f $(PREFIX)/lib/$(LIB)
	rm -f $(PREFIX)/include/libmsr.h

clean:
	rm -rf *.o *~ $(LIB)
	rm -rf html/
	rm -rf man/
