# This currently builds a user space program and not a useful library

PREFIX = /usr

CFLAGS = -Wall -g -std=c99 -pedantic -D_POSIX_C_SOURCE=200809L
LDFLAGS = -L. -lmsr

LIB = libmsr.a
LIBSRCS = libmsr.c serialio.c msr206.c makstripe.c
LIBOBJS = $(LIBSRCS:.c=.o)

all: $(LIB)

$(LIB): $(LIBOBJS)
	ar rcs $(LIB) $(LIBOBJS)

.c.o:
	$(CC) $(CFLAGS) -o $@ -c $<

install: $(LIB)
	install -m644 -D $(LIB) $(PREFIX)/lib
	install -m644 -D libmsr.h $(PREFIX)/include

uninstall:
	rm -f $(PREFIX)/lib/$(LIB)
	rm -f $(PREFIX)/include/libmsr.h

clean:
	rm -rf *.o *~ $(LIB)
