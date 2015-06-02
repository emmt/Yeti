.SUFFIXES: .c .i .o .h .so .dll .dyn .a .exe .txt .text .ps .pdf .html
#------------------------------------------------------------------------------
# these values filled in by: yorick -batch make.i
Y_MAKEDIR=
Y_EXE=
Y_EXE_PKGS=
Y_EXE_HOME=
Y_EXE_SITE=

DOCDIR=$(Y_EXE_SITE)/doc

# command and rules to produce documentation
PREFIX = $(HOME)/sw
BINDIR = $(PREFIX)/bin
ASCIIDOC = $(BINDIR)/asciidoc
A2X = $(BINDIR)/a2x
DOCTYPE = article

default: doc

clean:
	rm -f *~ core

doc: make_doc.i
	$(Y_EXE) -batch ./make_doc.i

install: doc
	for file in *.doc; do \
	  cp -pf $$file $(DOCDIR)/$$file; \
	  chmod 644 $(DOCDIR)/$$file; \
	done
	cp -pf ../README.md $(DOCDIR)/README.yeti; \
	chmod 644 $(DOCDIR)/README.yeti

html: yeti.html README.md



../README: yeti.text
	rm -f $@
	cp -a $< $@
	chmod 444 $@

%.html: %.txt Makefile
	$(ASCIIDOC) -b xhtml11 -a icons -o $@ $<

%.text: %.txt Makefile
	$(A2X) -d $(DOCTYPE) -f text --icons --icons-dir=./pics $<


#------------------------------------------------------------------------------
