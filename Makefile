
#CFLAGS=-g -W -Wall
CFLAGS=-O3 -W -Wall

prefix=/usr/local
bindir=$(DESTDIR)$(prefix)/bin
mandir=$(DESTDIR)$(prefix)/share/man/man1
datadir=$(DESTDIR)/etc

# See whether emacs is installed and try to guess its installation prefix.
emacs_prefix = $(patsubst %/bin/emacs,%,$(shell which emacs 2>/dev/null))
ifneq ($(strip $(emacs_prefix)),)
elispdir = $(emacs_prefix)/share/emacs/site-lisp
endif

# Check to see whether we have Jack installed. Needs pkg-config.
JACK := $(shell pkg-config --libs jack 2>/dev/null)

OBJ = readconfig.o midizap.o jackdriver.o

# Only try to install the manual page if it's actually there, to prevent
# errors if pandoc isn't installed.
INSTALL_TARGETS = midizap $(wildcard midizap.1)

.PHONY: all world install uninstall man pdf clean realclean

all: midizap midizap-mode.el

# This also creates the manual page (see below).
world: all man

install: all
	install -d $(bindir) $(datadir) $(mandir)
	install midizap $(bindir)
	install -m 0644 example.midizaprc $(datadir)/midizaprc
ifneq ($(elispdir),)
# If emacs was found, or elispdir was specified manually, install
# midizap-mode.el into the elispdir directory.
	install -d $(DESTDIR)$(elispdir)
	install -m 0644 midizap-mode.el $(DESTDIR)$(elispdir)
endif
# If present, the manual page will be installed along with the program.
ifneq ($(findstring midizap.1, $(INSTALL_TARGETS)),)
	install -m 0644 midizap.1 $(mandir)
else
	@echo "Manual page not found, create it with 'make man'."
endif

uninstall:
	rm -f $(bindir)/midizap $(mandir)/midizap.1 $(datadir)/midizaprc

midizap: $(OBJ)
	gcc $(CFLAGS) $(OBJ) -o midizap -L /usr/X11R6/lib -lX11 -lXtst $(JACK)

# This creates the manual page from the README. Requires pandoc
# (http://pandoc.org/).
man: midizap.1

# Manual page in pdf format. This also needs groff.
pdf: midizap.pdf

midizap.1: README.md
	pandoc -s -tman $< > $@

midizap.pdf: midizap.1
# This assumes that man does the right thing when given a file instead of a
# program name, and that it understands groff's -T option.
	man -Tpdf ./midizap.1 > $@

clean:
	rm -f midizap keys.h keys.el midizap-mode.el $(OBJ)

realclean:
	rm -f midizap midizap.1 midizap.pdf keys.h $(OBJ)

keys.h: keys.sed /usr/include/X11/keysymdef.h
	sed -f keys.sed < /usr/include/X11/keysymdef.h > keys.h

keys.el: keywords.sed /usr/include/X11/keysymdef.h
	sed -f keywords.sed < /usr/include/X11/keysymdef.h | tr '\n' ' ' > keys.el

midizap-mode.el: midizap-mode.el.in keys.el
	sed '/;; keysyms/r keys.el' < midizap-mode.el.in > midizap-mode.el

readconfig.o: midizap.h keys.h
midizap.o: midizap.h jackdriver.h
jackdriver.o: jackdriver.h
