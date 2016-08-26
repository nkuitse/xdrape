include config.mk

SRC = ${PROG}.c
MAN = ${PROG}.1
OBJ = ${SRC:.c=.o}

all: options ${PROG}

options:
	@echo ${PROG} build options:
	@echo "CFLAGS   = ${CFLAGS}"
	@echo "LDFLAGS  = ${LDFLAGS}"
	@echo "CC       = ${CC}"

.c.o:
	@echo CC $<
	@${CC} -c ${CFLAGS} $<

${OBJ}: config.mk

${PROG}: ${OBJ}
	@echo CC -o $@
	@${CC} -o $@ ${OBJ} ${LDFLAGS}

install: ${PROG} ${BINDIR} ${MANDIR}
	@echo creating install dirs
	@install -d ${BINDIR} ${MANDIR}
	@echo installing executable to ${BINDIR}
	@install ${PROG} ${BINDIR}
	@echo installing man page to ${MANDIR}
	@install ${MAN} ${MANDIR}

clean:
	@echo cleaning
	@rm -f ${PROG} ${OBJ} ${PROG}-${VERSION}.tar.gz

dist: clean
	@echo creating dist tarball
	@mkdir -p ${PROG}-${VERSION}
	@cp -R LICENSE Makefile README* config.mk \
		${MAN} ${SRC} ${PROG}-${VERSION}/
	@tar -czf ${PROG}-${VERSION}.tar.gz ${PROG}-${VERSION}
	@rm -rf ${PROG}-${VERSION}

.PHONY: all options clean dist install
