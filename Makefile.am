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
AM_CXXFLAGS = -std=gnu++11 -Wall
PRGM = chiton

bin_PROGRAMS = $(PRGM)
chiton_SOURCES = main.cpp config.cpp config_parser.cpp database.cpp mariadb.cpp \
	util.cpp mariadb_result.cpp camera.cpp stream_unwrap.cpp file_manager.cpp \
	chiton_ffmpeg.cpp stream_writer.cpp \
	camera.hpp chiton_ffmpeg.hpp database.hpp file_manager.hpp mariadb.hpp \
	stream_unwrap.hpp util.hpp chiton_config.hpp config_parser.hpp \
	database_result.hpp main.hpp mariadb_result.hpp stream_writer.hpp

chiton_CXXFLAGS = $(AM_CXXFLAGS)

EXTERNAL_LICENSE=LICENSE.breeze-icons  LICENSE.hls  LICENSE.smarty
LICENSE_FILES=COPYING $(EXTERNAL_LICENSE)

CLEANFILES=$(EXTERNAL_LICENSE)
EXTRA_DIST=web external config/chiton.cfg.template

cfg_DATA=config/chiton.cfg.template
cfgdir=$(sysconfdir)/$(PACKAGE)

dist-hook:
	-chmod u+w $(distdir)/external/*/.git
	-rm -rf $(distdir)/external/*/.git
	-rm $(distdir)/web/inc/tpl_cache/* $(distdir)/web/inc/tpl_compile/* $(distdir)/web/vids
	find $(distdir) -name .placeholder -delete

#install the web directory with some cp commands...
#too much to try and track all the files, at least right now
install-data-local:
	$(mkdir_p) $(DESTDIR)$(pkgdatadir)
	cp -r $(srcdir)/web $(DESTDIR)$(pkgdatadir)
	-chmod  u+w $(DESTDIR)$(pkgdatadir)/web/ $(DESTDIR)$(pkgdatadir)/web/inc
	-rm -rf $(DESTDIR)$(pkgdatadir)/web/vids
	rm -rf $(DESTDIR)$(pkgdatadir)/web/inc/tpl_cache $(DESTDIR)$(pkgdatadir)/web/inc/tpl_compile
	chmod -R ug-w $(DESTDIR)$(pkgdatadir)/web

uninstall-hook:
	if test -d $(DESTDIR)$(pkgdatadir); then chmod -R u+w $(DESTDIR)$(pkgdatadir) ; rm -rf $(DESTDIR)$(pkgdatadir); fi

#imagemagick command...should pull from ./configure
CONVERT=convert

clean-local:
	-rm -rf web/static/breeze web/static/hls.* web/static/*bottom-cursor.png
	-rm -rf web/inc/external/*
	-rm -rf web/inc/tpl_{cache,compile}/*

#rules for external packages, we use their license files as build hooks
dist_license_DATA=$(LICENSE_FILES)
licensedir=$(pkgdatadir)/license/


#
# Breeze has some rasterization work and we pick only some files
#
BREEZE_FILES=$(wildcard external/breeze-icons.git/icons/status/symbolic/media-*.svg \
	external/breeze-icons.git/icons/actions/symbolic/view-*.svg \
	external/breeze-icons.git/icons/actions/symbolic/zoom-*.svg \
	external/breeze-icons.git/icons/status/symbolic/audio-*.svg \
	external/breeze-icons.git/icons/actions/symbolic/open-menu*.svg)

#some makefile magic to generate the actual filenames we need
BREEZE_SVG=$(addprefix web/static/breeze/,$(subst media-,,$(subst -symbolic,,$(notdir $(BREEZE_FILES)))))
BREEZE_PNG=$(BREEZE_SVG:%.svg=%.png)
BREEZE_2X_PNG=$(addprefix web/static/breeze/2x-,$(notdir $(BREEZE_PNG)))
BREEZE_4X_PNG=$(addprefix web/static/breeze/4x-,$(notdir $(BREEZE_PNG)))

web/static/breeze:
	mkdir -p web/static/breeze

$(BREEZE_SVG): $(BREEZE_FILES) web/static/breeze
	cp $< $@

$(BREEZE_2X_PNG): $(BREEZE_SVG)
	$(CONVERT) -background none -density 144 $<  $@
$(BREEZE_4X_PNG): $(BREEZE_SVG)
	$(CONVERT) -background none -density 288 $<  $@

LICENSE.breeze-icons: $(BREEZE_2X_PNG) $(BREEZE_4X_PNG)
	cp external/breeze-icons.git/COPYING.LIB LICENSE.breeze-icons

LICENSE.hls:
	cp external/hls.js.git/dist/hls.* web/static/
	cp external/hls.js.git/LICENSE LICENSE.hls

LICENSE.smarty:
	cp -r external/smarty.git/libs web/inc/external/smarty
	sed -i "s/const SMARTY_VERSION = '[^']\+';/const SMARTY_VERSION = '$1';/" web/inc/external/smarty/Smarty.class.php
	cp external/smarty.git/LICENSE LICENSE.smarty


install-data-local: web $(LICENSE_FILES)


