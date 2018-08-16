
#CFLAGS=-g -W -Wall
CFLAGS=-O3 -W -Wall

prefix=/usr/local
bindir=$(DESTDIR)$(prefix)/bin
mandir=$(DESTDIR)$(prefix)/share/man/man1
datadir=$(DESTDIR)/etc

# We still keep this alias around for backward compatibility:
INSTALL_DIR=$(bindir)

# Check to see whether we have Jack installed. Needs pkg-config.
JACK := $(shell pkg-config --libs jack 2>/dev/null)

OBJ = readconfig.o midizap.o jackdriver.o

all: midizap midizap.1

install: all
	install -d $(bindir) $(datadir) $(mandir)
	install midizap $(bindir)
	install -m 0644 example.midizaprc $(datadir)/midizaprc
	install -m 0644 midizap.1 $(mandir)

uninstall:
	rm -f $(bindir)/midizap $(mandir)/midizap.1 $(datadir)/midizaprc

midizap: $(OBJ)
	gcc $(CFLAGS) $(OBJ) -o midizap -L /usr/X11R6/lib -lX11 -lXtst $(JACK)

midizap.1: README.md
	pandoc -s -tman $< > $@

clean:
	rm -f midizap keys.h $(OBJ)

realclean:
	rm -f midizap midizap.1 keys.h $(OBJ)

keys.h: keys.sed /usr/include/X11/keysymdef.h
	sed -f keys.sed < /usr/include/X11/keysymdef.h > keys.h

readconfig.o: midizap.h keys.h
midizap.o: midizap.h jackdriver.h
jackdriver.o: jackdriver.h
