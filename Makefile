xdrape: xdrape.c
	cc -Wall -O -o xdrape -I/usr/X11R6/include -I/usr/local/include -L/usr/X11R6/lib -L/usr/local/lib -lX11 xdrape.c
install: xdrape
	cp -f xdrape ~/bin/
