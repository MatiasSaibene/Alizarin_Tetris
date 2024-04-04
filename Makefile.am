
bin_PROGRAMS = atris

atris_SOURCES =	ai.c \
		atris.c \
		button.c \
		display.c \
		event.c \
		fastrand.c \
		gamemenu.c \
		grid.c \
		highscore.c \
		identity.c \
		menu.c \
		network.c \
		piece.c \
		sound.c \
		xflame.c \
		ai.h \
		atris.h \
		button.h \
		display.h \
		fastrand.h \
		grid.h \
		highscore.h \
		identity.h \
		menu.h \
		options.h \
		piece.h \
		sound.h

INCLUDES = -I.protos

EXTRA_DIST = \
    AUTHORS \
    COPYING \
    ChangeLog \
    INSTALL \
    NEWS \
    README \
    atris.spec.in \
    graphics \
    styles \
    autogen.sh \
    icon.xpm \
    Atris.Players \
    Atris.Scores


SUBDIRS = .protos graphics styles Docs

# Special install rule for the game
install: install-am
	make install_gamedata target=@GAME_INSTALLDIR@

install_gamedata:
	sh mkinstalldirs $(target)/
	cp -rv graphics \
               styles \
	       Atris.Players \
	       Atris.Scores \
               $(target)/
	chmod 666 $(target)/Atris.Players
	chmod 666 $(target)/Atris.Scores

install_gamedocs:
	sh mkinstalldirs $(target)/
	cp -rv README* COPYING* Docs*   $(target)/

# Rule to build tar-gzipped distribution package
$(PACKAGE)-$(VERSION).tar.gz: dist

# Rule to build RPM distribution package

i386-rpm: $(PACKAGE)-$(VERSION).tar.gz
	cp $(PACKAGE)-$(VERSION).tar.gz /usr/src/RPM/SOURCES
	rpm -ba --target=i386 atris.spec

i686-rpm: $(PACKAGE)-$(VERSION).tar.gz
	cp $(PACKAGE)-$(VERSION).tar.gz /usr/src/RPM/SOURCES
	rpm -ba --target=i686 atris.spec

zip: distdir $(PACKAGE)
	mv $(PACKAGE) $(PACKAGE)-$(VERSION)/$(PACKAGE).exe
	mv $(PACKAGE)-$(VERSION) $(PACKAGE)
	cp ~/*.dll $(PACKAGE)
	rm $(PACKAGE)/config* $(PACKAGE)/*.[ch] $(PACKAGE)/Make* $(PACKAGE)/*.m4 $(PACKAGE)/{install-sh,missing,mkinstalldirs,autogen.sh} 
	rm $(PACKAGE)/*.in -r $(PACKAGE)/.protos
	zip -r9 $(PACKAGE)-$(VERSION)-win32.zip $(PACKAGE)
	rm -r $(PACKAGE)
