PROG    = xdrape
VERSION = 0.4.0

PREFIX = /usr/local
BINDIR = ${PREFIX}/bin
MANDIR = ${PREFIX}/share/man/man1

X11INC = /usr/X11R6/include
X11LIB = /usr/X11R6/lib

INCS = -I. -I/usr/include -I/usr/local/include -I${X11INC}
LIBS = -L/usr/lib -L/usr/local/lib -lc -L${X11LIB} -lX11

CFLAGS  = -std=c99 -pedantic -Wall -Os ${INCS} -DVERSION=${VERSION}
LDFLAGS = ${LIBS}

CC = cc
