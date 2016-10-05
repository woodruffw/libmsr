# This currently builds a user space program and not a useful library

CFLAGS=	-Wall -g -std=c99 -pedantic -D_POSIX_C_SOURCE=200809L
LDFLAGS= -L. -lmsr

LIB=	libmsr.a
LIBSRCS=	libmsr.c serialio.c msr206.c makstripe.c
LIBOBJS=	$(LIBSRCS:.c=.o)

DAB=	dab
DABSRCS=	dab.c
DABOBJS=	$(DABSRCS:.c=.o)

DMSB=	dmsb
DMSBSRCS=	dmsb.c
DMSBOBJS=	$(DMSBSRCS:.c=.o)

all:	$(LIB)

$(LIB): $(LIBOBJS)
	ar rcs $(LIB) $(LIBOBJS)

.c.o:
	$(CC) $(CFLAGS) -o $@ -c $<

install: $(LIB)
	install -m644 -D $(LIB) $(DESTDIR)/usr/lib/$(LIB)
	install -m644 -D libmsr.h $(DESTDIR)/usr/include

AUDIOLDFLAGS=-lsndfile

$(DAB): $(DABOBJS)
	$(CC) -o $(DAB) $(DABOBJS) $(AUDIOLDFLAGS)
$(DMSB): $(DMSBOBJS)
	$(CC) -o $(DMSB) $(DMSBOBJS) $(AUDIOLDFLAGS)

audio: $(DAB) $(DMSB)

clean:
	rm -rf *.o *~ $(LIB) $(DAB) $(DMSB)
