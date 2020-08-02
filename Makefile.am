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


EXTRA_DIST=external

dist_cfg_DATA=config/chiton.cfg.template web/inc/config.template.php
cfgdir=$(sysconfdir)/$(PACKAGE)

dist_apachecfg_DATA=config/apache-chiton.conf
apachecfgdir=@pkgapacheconfdir@

#man1_MANS = man/chiton.1 man/chiton-install.1
dist_man1_MANS = man/chiton.1 man/chiton-install.1

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
	echo "Cleaning, but not $(distdir) or " $$dc_install_base
	-rm -rf web/inc/external/*
	-rm -rf web/inc/tpl_{cache,compile}/*

#rules for external packages, we use their license files as build hooks
license_DATA=$(LICENSE_FILES)
licensedir=$(pkgdatadir)/license/


#
# Breeze has some rasterization work and we pick only some files
#
BREEZE_FILES=$(wildcard $(top_srcdir)/external/breeze-icons.git/icons/actions/symbolic/media-*.svg \
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
#all the web files
dist_web_DATA=web/camera.php  web/index.php  web/info.php  web/settings.php  web/stream.php
webdir=$(pkgdatadir)/web

dist_web_inc_DATA=web/inc/main.php web/inc/util.php web/inc/web_config.php
web_incdir=$(pkgdatadir)/web/inc

dist_web_inc_tpl_DATA=web/inc/tpl/camera.tpl web/inc/tpl/error.tpl web/inc/tpl/footer.tpl web/inc/tpl/header.tpl \
	web/inc/tpl/index.tpl web/inc/tpl/player.tpl web/inc/tpl/settings.tpl
web_inc_tpldir=$(pkgdatadir)/web/inc/tpl

dist_web_static_DATA=web/static/bottom-cursor.svg web/static/chiton.js web/static/default.css
web_static_DATA=web/static/2x-bottom-cursor.png web/static/4x-bottom-cursor.png
web_staticdir=$(pkgdatadir)/web/static

web_statichls_DATA=$(HLS_FILES)
web_statichlsdir=$(pkgdatadir)/web/static

web_static_breeze_DATA=$(BREEZE_2X_PNG) $(BREEZE_4X_PNG) $(BREEZE_SVG)
web_static_breezedir=$(pkgdatadir)/web/static/breeze

CLEANFILES=$(EXTERNAL_LICENSE) $(BREEZE_2X_PNG) $(BREEZE_4X_PNG) $(BREEZE_SVG) web/external/smarty $(HLS_BUILD_FILES)
