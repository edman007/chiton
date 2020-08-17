########################################################################
 #
 #     This file is part of Chiton.
 #
 #   Chiton is free software: you can redistribute it and/or modify
 #   it under the terms of the GNU General Public License as published by
 #   the Free Software Foundation, either version 3 of the License, or
 #   (at your option) any later version.
 #
 #   Chiton is distributed in the hope that it will be useful,
 #   but WITHOUT ANY WARRANTY; without even the implied warranty of
 #   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 #   GNU General Public License for more details.
 #
 #   You should have received a copy of the GNU General Public License
 #   along with Chiton.  If not, see <https://www.gnu.org/licenses/>.
 #
 #   Copyright 2020 Ed Martin <edman007@edman007.com>
 #
 ########################################################################

ACLOCAL_AMFLAGS = -I m4
AM_CXXFLAGS = -std=gnu++11 -Wall -DSYSCFGPATH='"$(sysconfdir)/$(PACKAGE)/chiton.cfg"'
PRGM = chiton
MAINTAINER=Ed Martin <edman007@edman007.com>

bin_PROGRAMS = $(PRGM)
chiton_SOURCES = main.cpp config.cpp config_parser.cpp database.cpp mariadb.cpp \
	util.cpp mariadb_result.cpp camera.cpp stream_unwrap.cpp file_manager.cpp \
	chiton_ffmpeg.cpp stream_writer.cpp \
	camera.hpp chiton_ffmpeg.hpp database.hpp file_manager.hpp mariadb.hpp \
	stream_unwrap.hpp util.hpp chiton_config.hpp config_parser.hpp \
	database_result.hpp main.hpp mariadb_result.hpp stream_writer.hpp

sbin_SCRIPTS = chiton-install

noinst_DATA=dist-version
chiton_CXXFLAGS = $(AM_CXXFLAGS)

EXTERNAL_LICENSE=LICENSE.breeze-icons  LICENSE.hls  LICENSE.smarty
LICENSE_FILES=COPYING $(EXTERNAL_LICENSE)

DEBIAN_PKG_FILES= packaging/debian/control packaging/debian/copyright \
	packaging/debian/changelog packaging/debian/rules packaging/debian/source-format \
	packaging/debian/compat
SLACKWARE_PACKAGE_FILES=packaging/scripts/rc.chiton.in \
	packaging/slackware/chiton.SlackBuild \
	packaging/slackware/slack-desc

PACKAGING_FILES=$(SLACKWARE_PACKAGE_FILES) \
	$(DEBIAN_PKG_FILES)

noinst_SCRIPTS=packaging/scripts/rc.chiton


EXTRA_DIST=external chiton-install.in $(PACKAGING_FILES) dist-version README.md \
	README NEWS AUTHORS ChangeLog

dist_cfg_DATA=config/chiton.cfg.template web/inc/config.template.php
cfgdir=$(sysconfdir)/$(PACKAGE)

dist_syslogcfg_DATA=config/syslog-chiton.conf
syslogcfgdir=$(SYSLOGCONFDIR)

dist_apachecfg_DATA=config/apache-chiton.conf
apachecfgdir=$(pkgdatadir)/apache/

#man1_MANS = man/chiton.1 man/chiton-install.1
dist_man1_MANS = man/chiton.1 man/chiton-install.1

do_subst = sed -e 's,[@]PKGNAME[@],$(PRGM),g' \
	-e 's,[@]pkgdatadir[@],$(pkgdatadir),g' \
	-e 's,[@]localstatedir[@],$(localstatedir),g' \
	-e 's,[@]runstatedir[@],$(localstatedir),g' \
	-e 's,[@]apachecfgdir[@],@apacheconfdir@,g' \
	-e 's,[@]bindir[@],$(bindir),g' \
	-e 's,[@]runuser[@],$(runuser),g' \
	-e 's,[@]cfgdir[@],$(cfgdir),g' \
	-e 's,[@]releaseversion[@],$(VERSION),g' \
	-e 's,[@]fullversion[@],$(VERSION)$(VER_GITVER),g' \
	-e 's,[@]homepage[@],$(PACKAGE_URL),g' \
	-e 's,[@]maintainer[@],$(MAINTAINER),g'


#the file for the debian packager source code (will be a symlink)
DEBSRC=$(PACKAGE_TARNAME)_$(VERSION).orig.tar.gz
#the tarball for the packaging metadata
DEBSRCPKG=$(PACKAGE_TARNAME)_$(VERSION)-1.debian.tar.gz


dist-hook:
	-chmod u+w $(distdir)/external/*/.git
	-rm -rf $(distdir)/external/*/.git
	-rm $(distdir)/web/inc/tpl_cache/* $(distdir)/web/inc/tpl_compile/* $(distdir)/web/vids
	find $(distdir) -name .placeholder -delete

#install the web directory with some cp commands...
#too much to try and track all the files, at least right now
install-data-local: web/inc/external/smarty
	$(mkdir_p) $(DESTDIR)$(pkgdatadir)/web/inc/external/
	cp -r web/inc/external/smarty $(DESTDIR)$(pkgdatadir)/web/inc/external/smarty

uninstall-hook:
	if test -d $(DESTDIR)$(pkgdatadir); then chmod -R u+w $(DESTDIR)$(pkgdatadir) ; rm -rf $(DESTDIR)$(pkgdatadir); fi

#imagemagick command...should pull from ./configure
CONVERT=@IMAGEMAGICK_CONVERT@

clean-local:
	-rm -rf web/inc/external/*
	-rm -rf web/inc/tpl_{cache,compile}/*

#rules for external packages, we use their license files as build hooks
license_DATA=$(LICENSE_FILES)
licensedir=$(pkgdatadir)/license/


#
# Breeze has some rasterization work and we pick only some files
#
BREEZE_FILES=$(wildcard $(top_srcdir)/external/breeze-icons.git/icons/actions/symbolic/media-*-symbolic.svg \
	$(top_srcdir)/external/breeze-icons.git/icons/actions/symbolic/view-*.svg \
	$(top_srcdir)/external/breeze-icons.git/icons/actions/symbolic/zoom-*.svg \
	$(top_srcdir)/external/breeze-icons.git/icons/status/symbolic/audio-*.svg \
	$(top_srcdir)/external/breeze-icons.git/icons/actions/symbolic/open-menu*.svg)

#some makefile magic to generate the actual filenames we need
BREEZE_SVG=$(addprefix web/static/breeze/,$(subst media-,,$(subst -symbolic,,$(notdir $(BREEZE_FILES)))))
BREEZE_PNG=$(BREEZE_SVG:%.svg=%.png)
BREEZE_2X_PNG=$(addprefix web/static/breeze/2x-,$(notdir $(BREEZE_PNG)))
BREEZE_4X_PNG=$(addprefix web/static/breeze/4x-,$(notdir $(BREEZE_PNG)))

#cludgy function to pull the index out of these file lists
_getpos = $(if $(findstring $1,$2),$(call _getpos,$1,$(wordlist 2,$(words $2),$2),x $3),$3)
getpos = $(words $(call _getpos,$1,$2))

HLS_FILES=external/hls.js.git/dist/hls.js external/hls.js.git/dist/hls.light.js \
	external/hls.js.git/dist/hls.light.min.js external/hls.js.git/dist/hls.min.js \
	external/hls.js.git/dist/hls.js.map  external/hls.js.git/dist/hls.light.js.map \
	external/hls.js.git/dist/hls.light.min.js.map  external/hls.js.git/dist/hls.min.js.map

HLS_BUILD_FILES=$(addprefix web/static/,$(notdir $(HLS_FILES)))

web/static/breeze:
	$(MKDIR_P) web/static/breeze

$(BREEZE_SVG):  $(BREEZE_FILES) | web/static/breeze
	cp $(word $(call getpos,$@,$(BREEZE_SVG)),$(BREEZE_FILES)) $@

$(BREEZE_2X_PNG): $(BREEZE_SVG)
	$(CONVERT) -background none -density 144 $(word $(call getpos,$@,$(BREEZE_2X_PNG)),$(BREEZE_SVG))  $@
$(BREEZE_4X_PNG): $(BREEZE_SVG)
	$(CONVERT) -background none -density 288 $(word $(call getpos,$@,$(BREEZE_4X_PNG)),$(BREEZE_SVG))  $@

LICENSE.breeze-icons: $(BREEZE_2X_PNG) $(BREEZE_4X_PNG)
	cp $(top_srcdir)/external/breeze-icons.git/COPYING.LIB LICENSE.breeze-icons

LICENSE.hls:
	cp $(top_srcdir)/external/hls.js.git/dist/hls.* web/static/
	cp $(top_srcdir)/external/hls.js.git/LICENSE LICENSE.hls

LICENSE.smarty web/inc/external/smarty:
	$(MKDIR_P) web/inc/external/
	cp -r $(top_srcdir)/external/smarty.git/libs web/inc/external/smarty
	chmod u+w -R web/inc/external/smarty
	cp $(top_srcdir)/external/smarty.git/LICENSE LICENSE.smarty

web/static/2x-bottom-cursor.png: $(top_srcdir)/web/static/bottom-cursor.svg
	$(CONVERT) -background none -density 144 $<  $@

web/static/4x-bottom-cursor.png: $(top_srcdir)/web/static/bottom-cursor.svg
	$(CONVERT) -background none -density 288 $<  $@

chiton-install: chiton-install.in Makefile
	$(do_subst) < $(srcdir)/chiton-install.in > chiton-install
	chmod +x chiton-install

packaging/scripts/rc.chiton: packaging/scripts/rc.chiton.in
	$(MKDIR_P) packaging/scripts
	$(do_subst) < $(srcdir)/packaging/scripts/rc.chiton.in > packaging/scripts/rc.chiton
	chmod +x packaging/scripts/rc.chiton

dist-version:
	echo $(shell echo $(shell git log --oneline --no-walk  || echo Unknown) | head -1 | cut -d ' ' -f 1) > dist-version

#all the web files
dist_web_DATA=web/camera.php  web/index.php  web/info.php  web/settings.php  web/stream.php
webdir=$(pkgdatadir)/web

dist_web_inc_DATA=web/inc/main.php web/inc/util.php web/inc/web_config.php
web_incdir=$(pkgdatadir)/web/inc

dist_web_inc_tpl_DATA=web/inc/tpl/camera.tpl web/inc/tpl/error.tpl web/inc/tpl/footer.tpl web/inc/tpl/header.tpl \
	web/inc/tpl/index.tpl web/inc/tpl/player.tpl web/inc/tpl/settings.tpl
web_inc_tpldir=$(pkgdatadir)/web/inc/tpl

dist_web_inc_cfg_DATA=web/inc/smarty_cfg/main.conf
web_inc_cfgdir=$(pkgdatadir)/web/inc/smarty_cfg


dist_web_static_DATA=web/static/bottom-cursor.svg web/static/chiton.js web/static/default.css
web_static_DATA=web/static/2x-bottom-cursor.png web/static/4x-bottom-cursor.png
web_staticdir=$(pkgdatadir)/web/static

web_statichls_DATA=$(HLS_FILES)
web_statichlsdir=$(pkgdatadir)/web/static

web_static_breeze_DATA=$(BREEZE_2X_PNG) $(BREEZE_4X_PNG) $(BREEZE_SVG)
web_static_breezedir=$(pkgdatadir)/web/static/breeze

CLEANFILES=$(EXTERNAL_LICENSE) $(BREEZE_2X_PNG) $(BREEZE_4X_PNG) $(BREEZE_SVG) web/external/smarty \
	$(HLS_BUILD_FILES) $(sbin_SCRIPTS) \
	web/static/2x-bottom-cursor.png web/static/4x-bottom-cursor.png \
	packaging/scripts/rc.chiton chiton-install

MAINTAINERCLEANFILES=dist-version chiton-$(VERSION).dsc $(DEBIAN_PKG_FILES)

# This is for actual release production...Creates the directory 'release' with all the files in our release
.PHONY: release
release: chiton-$(VERSION).dsc dist
	-rm -rf release
	$(MKDIR_P) release
	mv $(PACKAGE_TARNAME)-$(VERSION).tar.* $(DEBSRC) chiton-$(VERSION).dsc \
		$(DEBSRCPKG) \
		release

packaging/debian/control: packaging/debian/control.in Makefile
	$(do_subst) < $< > $@

packaging/debian/copyright: COPYING
	cp COPYING $@

#kinda crappy, but i need something that works
packaging/debian/changelog: Makefile
	echo $(PACKAGE_TARNAME) '('$(VERSION)'-1)' unstable\; urgency=low > $@
	git log -n 5 | sed 's/^/  /' >> $@
	echo -n ' -- $(MAINTAINER)  ' >> $@
	date -R >> $@

packaging/debian/rules: packaging/debian/rules.in Makefile
	$(do_subst) < $< > $@


$(DEBSRC): dist Makefile
	-rm -f $@
	ln -s $(PACKAGE_TARNAME)-$(VERSION).tar.gz $@

$(DEBSRCPKG): $(DEBIAN_PKG_FILES)
	tar --transform=s,packaging/,, --transform=s,-,/,g -czf $@  $(DEBIAN_PKG_FILES)

DEBSRC_FILESIZE=$(shell wc -c $(DEBSRC) | cut -d ' ' -f 1)
DEBSRCPKG_FILESIZE=$(shell wc -c $(DEBSRCPKG) | cut -d ' ' -f 1)
chiton-$(VERSION).dsc: packaging/debian/chiton.dsc.in $(DEBSRC) $(DEBSRCPKG) dist
	$(do_subst) < $(srcdir)/packaging/debian/chiton.dsc.in > chiton-$(VERSION).dsc
	echo -n -e 'Files:\n  ' >> chiton-$(VERSION).dsc
	md5sum $(DEBSRC) | sed -e 's/ / '$(DEBSRC_FILESIZE)'/' >> chiton-$(VERSION).dsc
	echo -n -e '  ' >> chiton-$(VERSION).dsc
	md5sum $(DEBSRCPKG) | sed -e 's/ / '$(DEBSRCPKG_FILESIZE)'/' >> chiton-$(VERSION).dsc
	echo -n -e 'Checksums-Sha1:\n  '  >> chiton-$(VERSION).dsc
	sha1sum $(DEBSRC) | sed -e 's/ / '$(DEBSRC_FILESIZE)'/' >> chiton-$(VERSION).dsc
	echo -n -e '  ' >> chiton-$(VERSION).dsc
	sha1sum $(DEBSRCPKG) | sed -e 's/ / '$(DEBSRCPKG_FILESIZE)'/' >> chiton-$(VERSION).dsc
	echo -n -e 'Checksums-Sha256:\n  '  >> chiton-$(VERSION).dsc
	sha256sum $(DEBSRC) | sed -e 's/ / '$(DEBSRC_FILESIZE)'/' >> chiton-$(VERSION).dsc
	echo -n -e '  ' >> chiton-$(VERSION).dsc
	sha256sum $(DEBSRCPKG) | sed -e 's/ / '$(DEBSRCPKG_FILESIZE)'/' >> chiton-$(VERSION).dsc

