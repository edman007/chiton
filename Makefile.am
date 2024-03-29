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
 #   Copyright 2020-2023 Ed Martin <edman007@edman007.com>
 #
 ########################################################################

ACLOCAL_AMFLAGS = -I m4
AM_CXXFLAGS = -std=gnu++11 -Wall -DSYSCFGPATH='"$(sysconfdir)/$(PACKAGE)/chiton.cfg"' -DDEFAULT_SOCKET_PATH='"$(defaultsocketpath)"' \
	-D_FILE_OFFSET_BITS=64
PRGM = chiton
MAINTAINER=Ed Martin <edman007@edman007.com>
DEBVERSION=$(shell echo -n $(VERSION) | sed s/pre/~pre/)
#make things go a little faster when testing packaging
export XZ_OPT = -e -T0

bin_PROGRAMS = $(PRGM)
chiton_SOURCES = main.cpp config.cpp config_parser.cpp database.cpp mariadb.cpp \
	util.cpp mariadb_result.cpp camera.cpp stream_unwrap.cpp file_manager.cpp \
	chiton_ffmpeg.cpp stream_writer.cpp \
	camera.hpp chiton_ffmpeg.hpp database.hpp file_manager.hpp mariadb.hpp \
	stream_unwrap.hpp util.hpp chiton_config.hpp config_parser.hpp \
	database_result.hpp main.hpp mariadb_result.hpp stream_writer.hpp \
	database_manager.hpp database_manager.cpp setting.hpp remote.hpp remote.cpp \
	export.hpp export.cpp image_util.hpp image_util.cpp filter.hpp filter.cpp \
	io/io_wrapper.hpp io/cfmp4.cpp io/cfmp4.hpp \
	motion_controller.hpp motion_controller.cpp motion_algo.hpp motion_algo.cpp \
	motion_opencv.hpp motion_opencv.cpp motion_cvbackground.hpp motion_cvbackground.cpp \
	motion_cvmask.hpp motion_cvmask.cpp motion_cvdebugshow.hpp motion_cvdebugshow.cpp \
	motion_cvresize.hpp motion_cvresize.cpp system_controller.hpp system_controller.cpp \
	motion_cvdetect.cpp motion_cvdetect.hpp cv_target_rect.cpp cv_target_rect.hpp \
	event.hpp event.cpp event_controller.hpp event_controller.cpp system_controller.fwd.hpp \
	event_notification.hpp event_notification.cpp module.hpp module_controller.hpp \
	event_console.hpp event_console.cpp event_db.hpp event_db.cpp json_encoder.cpp \
	json_encoder.hpp camera_status.hpp camera_status.cpp

#these are actually to make it possible to serve the website out of the source for development
EXTRA_chiton_DEPENDENCIES=hls pinch-zoom
sbin_SCRIPTS = chiton-install

noinst_DATA=dist-version
chiton_CXXFLAGS = $(AM_CXXFLAGS)

#this should be derived from external/npm/package-lock.json
NPM_LICENSE_LIST=external/npm/LICENSE.pointer-tracker \
	external/npm/LICENSE.hls.js external/npm/LICENSE.pinch-zoom-element
NPM_LICENSE=external/npm/license-meta.csv $(NPM_LICENSE_LIST)
EXTERNAL_NO_NPM_LICENSE=LICENSE.breeze-icons LICENSE.smarty
EXTERNAL_LICENSE=$(EXTERNAL_NO_NPM_LICENSE) $(NPM_LICENSE)
LICENSE_FILES=COPYING $(EXTERNAL_LICENSE)

DEBIAN_PKG_FILES=packaging/debian/source-format packaging/debian/compat \
	packaging/debian/chiton.templates packaging/debian/control.in \
	packaging/debian/copyright.in packaging/debian/rules.in \
	external/npm/debian.license.in
DEBIAN_PKG_FILES_EARLY=packaging/debian/control packaging/debian/copyright \
	packaging/debian/changelog packaging/debian/rules
DEBIAN_PKG_FILES_BUILT=$(DEBIAN_PKG_FILES_EARLY) \
	packaging/debian/preinst  packaging/debian/postinst \
	packaging/debian/prerm packaging/debian/postrm \
	packaging/debian/chiton.config 
SLACKWARE_PACKAGE_FILES=packaging/slackware/slack-desc
SLACKWARE_PACKAGE_FILES_BUILT=packaging/slackware/doinst.sh \
	 packaging/slackware/douninst.sh packaging/slackware/chiton.SlackBuild

#packaging/debian/changelog is required here because it's needed
PACKAGING_FILES=packaging/scripts/rc.chiton.in packaging/scripts/chiton.service.in \
	packaging/scripts/postinst.sh.in packaging/scripts/preinst.sh.in \
	packaging/scripts/prerm.sh.in packaging/scripts/postrm.sh.in \
	packaging/scripts/config.sh.in packaging/slackware/chiton.SlackBuild.in \
	$(DEBIAN_PKG_FILES_EARLY) \
	$(DEBIAN_PKG_FILES) $(SLACKWARE_PACKAGE_FILES)

noinst_SCRIPTS=packaging/scripts/rc.chiton

EXTRA_DIST=external chiton-install.in $(PACKAGING_FILES) dist-version README.md man/transform-settings.awk \
	README NEWS AUTHORS ChangeLog web/inc/configdb.php.in external/npm/debian.license.in $(NPM_LICENSE) \
	man/chiton.cfg.5.in

dist_cfg_DATA=config/chiton.cfg.template web/inc/config.template.php
cfgdir=$(sysconfdir)/$(PACKAGE)

dist_apachecfg_DATA=config/apache-chiton.conf
apachecfgdir=$(pkgdatadir)/apache/

#man1_MANS = man/chiton.1 man/chiton-install.1
dist_man1_MANS = man/chiton.1 man/chiton-install.1
man5_MANS = man/chiton.cfg.5

defaultsocketpath = $(localstatedir)/run/$(PRGM)/chiton.sock

if ENABLESYSV
initfile=$(initdir)/rc.chiton
inittype=sysv
else
initfile=$(initdir)/chiton.service
inittype=systemd
endif


do_subst = sed -e 's,[@]PKGNAME[@],$(PRGM),g' \
	-e 's,[@]pkgdatadir[@],$(pkgdatadir),g' \
	-e 's,[@]localstatedir[@],$(localstatedir)/lib,g' \
	-e 's,[@]runstatedir[@],$(localstatedir)/run/$(PRGM),g' \
	-e 's,[@]apachecfgdir[@],@apacheconfdir@,g' \
	-e 's,[@]bindir[@],$(bindir),g' \
	-e 's,[@]runuser[@],$(runuser),g' \
	-e 's,[@]cfgdir[@],$(cfgdir),g' \
	-e 's,[@]releaseversion[@],$(VERSION),g' \
	-e 's,[@]debianversion[@],$(DEBVERSION),g' \
	-e 's,[@]fullversion[@],$(VERSION)$(VER_GITVER),g' \
	-e 's,[@]homepage[@],$(PACKAGE_URL),g' \
	-e 's,[@]maintainer[@],$(MAINTAINER),g' \
	-e 's,[@]webuser[@],$(webuser),g' \
	-e 's,[@]apachesymlinkdir[@],$(apachesymlinkdir),g' \
	-e 's,[@]apacheappendconf[@],$(apacheappendconf),g' \
	-e 's,[@]initscript[@],$(initfile),g' \
	-e 's,[@]inittype[@],$(inittype),g' \
	-e 's,[@]defaultsocketpath[@],$(defaultsocketpath),g'


#the file for the debian packager source code (will be a symlink)
DEBSRC=$(PACKAGE_TARNAME)_$(DEBVERSION).orig.tar.gz
#the tarball for the packaging metadata
DEBSRCPKG=$(PACKAGE_TARNAME)_$(DEBVERSION)-1.debian.tar.gz
SLACKSRCPKG=$(PACKAGE_TARNAME)-$(VERSION).slackbuild.tar.xz

dist-hook:
	-chmod u+w $(distdir)/external/*/.git
	-rm -rf $(distdir)/external/*/.git
	-rm $(distdir)/web/inc/tpl_cache/* $(distdir)/web/inc/tpl_compile/* $(distdir)/web/vids
	-cp $(NPM_LICENSE_LIST) $(distdir)/external/npm/
	find $(distdir) -name .placeholder -delete

#install the web directory with some cp commands...
#too much to try and track all the files, at least right now
#this option will not work on non-linux...
SMARTYPATH_SYMLINK:=$(shell realpath -m --relative-to $(pkgdatadir)/web/inc/external/ $(SMARTYDIR))
install-data-local: web/inc/external/smarty
	$(mkdir_p) $(DESTDIR)$(pkgdatadir)/web/inc/external/
if INSTALLSMARTY
	cp -r web/inc/external/smarty $(DESTDIR)$(pkgdatadir)/web/inc/external/smarty
else
	ln -s $(SMARTYPATH_SYMLINK) $(DESTDIR)$(pkgdatadir)/web/inc/external/smarty
endif

uninstall-hook:
	if test -d $(DESTDIR)$(pkgdatadir); then chmod -R u+w $(DESTDIR)$(pkgdatadir) ; rm -rf $(DESTDIR)$(pkgdatadir); fi

#imagemagick command...should pull from ./configure
CONVERT=@RSVG_CONVERT@

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

HLS_FILES= external/npm/node_modules/hls.js/dist/hls.light.js external/npm/node_modules/hls.js/dist/hls.light.min.js \
	external/npm/node_modules/hls.js/dist/hls.light.js.map external/npm/node_modules/hls.js//dist/hls.light.min.js.map

HLS_BUILD_FILES=$(addprefix web/static/,$(notdir $(HLS_FILES)))

web/static/breeze:
	$(MKDIR_P) web/static/breeze

$(BREEZE_SVG):  $(BREEZE_FILES) | web/static/breeze
	cp $(word $(call getpos,$@,$(BREEZE_SVG)),$(BREEZE_FILES)) $@

$(BREEZE_2X_PNG): $(BREEZE_SVG)
	$(CONVERT) -x 2 -y 2 -o $@ $(word $(call getpos,$@,$(BREEZE_2X_PNG)),$(BREEZE_SVG))
$(BREEZE_4X_PNG): $(BREEZE_SVG)
	$(CONVERT) -x 4 -y 4 -o $@ $(word $(call getpos,$@,$(BREEZE_4X_PNG)),$(BREEZE_SVG))


external/npm/node_modules $(HLS_FILES) external/npm/node_modules/pinch-zoom-element/dist/pinch-zoom-min.js &:
	$(MKDIR_P) $(abs_builddir)/external/npm/
	(cd external/npm/ && npm install --only-prod hls.js 'github:edman007/pinch-zoom#494b4a2a26ab6307e9f70f2f9a3d9a82844f2709' )

$(NPM_LICENSE) external/npm/debian.license.in  &: external/npm/create-license.sh | external/npm/node_modules
	($(MKDIR_P) $(abs_builddir)/external/npm/ && \
	cd $(abs_srcdir)/external/npm/ && \
	license-checker --relativeLicensePath --csv --customPath license-format.json --out $(abs_builddir)/external/npm/license-meta.csv && \
	cd $(abs_builddir)/external/npm/ && \
	$(abs_srcdir)/external/npm/create-license.sh $(abs_srcdir) )


LICENSE.breeze-icons: $(BREEZE_2X_PNG) $(BREEZE_4X_PNG)
	cp $(top_srcdir)/external/breeze-icons.git/COPYING.LIB LICENSE.breeze-icons


LICENSE.smarty web/inc/external/smarty:
	$(MKDIR_P) web/inc/external/
	cp -r $(top_srcdir)/external/smarty.git/libs web/inc/external/smarty
	chmod u+w -R web/inc/external/smarty
	cp $(top_srcdir)/external/smarty.git/LICENSE LICENSE.smarty

#These are developer install rules
.PHONY: hls
hls: | external/npm/node_modules $(abs_builddir)/web/static
	-rm -f web/static/hls*.js web/static/hls.light.*
	cp $(top_srcdir)/external/npm/node_modules/hls.js/dist/hls.light.* web/static/
.PHONY: pinch-zoom
pinch-zoom: | external/npm/node_modules
	-rm -f web/static/pinch-zoom-min.js
	cp $(top_srcdir)/external/npm/node_modules/pinch-zoom-element/dist/pinch-zoom-min.js  web/static/pinch-zoom-min.js

web/static/2x-bottom-cursor.png: $(top_srcdir)/web/static/bottom-cursor.svg | $(abs_builddir)/web/static
	$(CONVERT) -x 2 -y 2 --dpi-x 144 --dpi-y 144 -o $@ $<

web/static/4x-bottom-cursor.png: $(top_srcdir)/web/static/bottom-cursor.svg | $(abs_builddir)/web/static
	$(CONVERT) -x 2 -y 2 -o $@ $<
$(abs_builddir)/web/static:
	$(MKDIR_P) web/static
chiton-install: chiton-install.in Makefile
	$(do_subst) < $(srcdir)/chiton-install.in > chiton-install
	chmod +x chiton-install

$(abs_builddir)/packaging/scripts/:
	$(MKDIR_P) $(builddir)/packaging/scripts
$(abs_builddir)/packaging/slackware:
	$(MKDIR_P) $(builddir)/packaging/slackware
packaging/scripts/rc.chiton: packaging/scripts/rc.chiton.in | $(abs_builddir)/packaging/scripts/
	$(do_subst) < $(srcdir)/packaging/scripts/rc.chiton.in > packaging/scripts/rc.chiton
	chmod +x packaging/scripts/rc.chiton

packaging/scripts/chiton.service: packaging/scripts/chiton.service.in | $(abs_builddir)/packaging/scripts/
	$(do_subst) < $(srcdir)/packaging/scripts/chiton.service.in > packaging/scripts/chiton.service

packaging/debian/preinst: packaging/scripts/preinst.sh.in
	$(do_subst) < $< > $@
packaging/debian/postinst:  packaging/scripts/postinst.sh.in
	$(do_subst) < $< > $@
packaging/debian/prerm: packaging/scripts/prerm.sh.in
	$(do_subst) < $< > $@
packaging/debian/postrm: packaging/scripts/postrm.sh.in
	$(do_subst) < $< > $@
packaging/slackware/douninst.sh: packaging/scripts/prerm.sh.in packaging/scripts/postrm.sh.in | $(abs_builddir)/packaging/slackware
	cat $^ | $(do_subst) > $@
packaging/debian/chiton.config:  packaging/scripts/config.sh.in
	$(do_subst) < $< > $@
packaging/slackware/doinst.sh: packaging/scripts/preinst.sh.in packaging/scripts/postinst.sh.in packaging/scripts/config.sh.in | $(abs_builddir)/packaging/slackware
	cat $^ | $(do_subst) > $@
packaging/slackware/chiton.SlackBuild: packaging/slackware/chiton.SlackBuild.in | $(abs_builddir)/packaging/slackware
	$(do_subst) < $< > $@
	chmod +x $@
dist-version:
	echo "$(shell git log --oneline --no-walk  || echo Unknown)" | head -1 | cut -d ' ' -f 1 > dist-version

#all the web files
dist_web_DATA=web/camera.php  web/index.php  web/info.php  web/settings.php  web/stream.php web/license.php web/events.php \
	web/stats.php
webdir=$(pkgdatadir)/web

dist_web_inc_DATA=web/inc/main.php web/inc/util.php web/inc/webconfig.php web/inc/remote.php web/inc/events.php
web_inc_DATA=web/inc/configdb.php
web_incdir=$(pkgdatadir)/web/inc

dist_web_inc_tpl_DATA=web/inc/tpl/camera.tpl web/inc/tpl/error.tpl web/inc/tpl/footer.tpl web/inc/tpl/header.tpl \
	web/inc/tpl/index.tpl web/inc/tpl/player.tpl web/inc/tpl/settings.tpl web/inc/tpl/license.tpl web/inc/tpl/events.tpl \
	web/inc/tpl/stats.tpl
web_inc_tpldir=$(pkgdatadir)/web/inc/tpl

dist_web_inc_cfg_DATA=web/inc/smarty_cfg/main.conf
web_inc_cfgdir=$(pkgdatadir)/web/inc/smarty_cfg


dist_web_static_DATA=web/static/bottom-cursor.svg web/static/chiton.js web/static/default.css
web_static_DATA=web/static/2x-bottom-cursor.png web/static/4x-bottom-cursor.png
web_staticdir=$(pkgdatadir)/web/static

web_statichls_DATA=$(HLS_FILES)
web_statichlsdir=$(pkgdatadir)/web/static

web_staticpinchzoom_DATA=external/npm/node_modules/pinch-zoom-element/dist/pinch-zoom-min.js
web_staticpinchzoomdir=$(pkgdatadir)/web/static

web_static_breeze_DATA=$(BREEZE_2X_PNG) $(BREEZE_4X_PNG) $(BREEZE_SVG)
web_static_breezedir=$(pkgdatadir)/web/static/breeze

if ENABLESYSV
initscript_DATA=packaging/scripts/rc.chiton
else
initscript_DATA=packaging/scripts/chiton.service
endif
initscriptdir=$(initdir)

CLEANFILES=$(EXTERNAL_NO_NPM_LICENSE) $(BREEZE_2X_PNG) $(BREEZE_4X_PNG) $(BREEZE_SVG) web/inc/external/smarty \
	$(HLS_BUILD_FILES) $(sbin_SCRIPTS) 	$(SLACKWARE_PACKAGE_FILES_BUILT) $(DEBIAN_PKG_FILES_BUILT) \
	web/static/2x-bottom-cursor.png web/static/4x-bottom-cursor.png \
	packaging/scripts/rc.chiton chiton-install packaging/scripts/chiton.service \
	web/inc/configdb.php web/static/pinch-zoom-min.js

MAINTAINERCLEANFILES=dist-version chiton-$(DEBVERSION).dsc external/npm/license-meta.csv external/npm/debian.license.in

maintainer-clean-local:
	-rm -rf external/npm/node_modules $(NPM_LICENSE_LIST)

# This is for actual release production...Creates the directory 'release' with all the files in our release
.PHONY: release
release: chiton-$(DEBVERSION).dsc dist $(SLACKSRCPKG)
	-rm -rf release
	$(MKDIR_P) release
	mv $(PACKAGE_TARNAME)-$(VERSION).tar.* $(DEBSRC) chiton-$(DEBVERSION).dsc \
		$(DEBSRCPKG) $(SLACKSRCPKG) \
		release

packaging/debian/control: packaging/debian/control.in Makefile | $(abs_builddir)/packaging/debian/
	$(do_subst) < $< > $@

#these next two rules have special syntax to deal with make running two instances during make dist and having a race against itself
packaging/debian/copyright: packaging/debian/copyright.in external/npm/debian.license.in $(LICENSE_FILES) | external/npm/node_modules $(abs_builddir)/packaging/debian/
	$(shell tmp=$$(mktemp $@.XXXXXX ) && \
	$(do_subst) < $< > $$tmp && \
	cat $(srcdir)/external/npm/debian.license.in >> $$tmp ; \
	mv $$tmp $@ )

#kinda crappy, but i need something that works
packaging/debian/changelog: Makefile.am dist-version | $(abs_builddir)/packaging/debian/
	$(shell tmp=$$(mktemp $@.XXXXXX ) && \
	echo $(PACKAGE_TARNAME) '('$(DEBVERSION)'-1)' unstable\; urgency=low > $$tmp && \
	git log -n 5 | sed -e 's/.\{70\}/&\n       /g' -e 's/^/  /' >> $$tmp && \
	echo -n ' -- $(MAINTAINER)  ' >> $$tmp && \
	date -R >> $$tmp && \
	mv $$tmp $@ )

packaging/debian/rules: packaging/debian/rules.in Makefile | $(abs_builddir)/packaging/debian/
	$(do_subst) < $< > $@
$(abs_builddir)/packaging/debian/:
	$(MKDIR_P) $(abs_builddir)/packaging/debian/

$(DEBSRC): dist Makefile
	-rm -f $@
	ln -s $(PACKAGE_TARNAME)-$(VERSION).tar.gz $@

$(DEBSRCPKG): $(DEBIAN_PKG_FILES) $(DEBIAN_PKG_FILES_BUILT)
	tar --transform=s,packaging/,, --transform=s,-,/,g -czf $@  $(DEBIAN_PKG_FILES) $(DEBIAN_PKG_FILES_BUILT)
$(SLACKSRCPKG): $(SLACKWARE_PACKAGE_FILES) $(SLACKWARE_PACKAGE_FILES_BUILT)
	tar --transform=s,packaging/,, --transform=s,slackware,$(PACKAGE_TARNAME),g -czf $@  $(SLACKWARE_PACKAGE_FILES) $(SLACKWARE_PACKAGE_FILES_BUILT)


DEBSRC_FILESIZE=$(shell wc -c $(DEBSRC) | cut -d ' ' -f 1)
DEBSRCPKG_FILESIZE=$(shell wc -c $(DEBSRCPKG) | cut -d ' ' -f 1)
chiton-$(DEBVERSION).dsc: packaging/debian/chiton.dsc.in $(DEBSRC) $(DEBSRCPKG) dist
	$(do_subst) < $(srcdir)/packaging/debian/chiton.dsc.in > chiton-$(DEBVERSION).dsc
	echo -n -e 'Files:\n  ' >> chiton-$(DEBVERSION).dsc
	md5sum $(DEBSRC) | sed -e 's/ / '$(DEBSRC_FILESIZE)'/' >> chiton-$(DEBVERSION).dsc
	echo -n -e '  ' >> chiton-$(DEBVERSION).dsc
	md5sum $(DEBSRCPKG) | sed -e 's/ / '$(DEBSRCPKG_FILESIZE)'/' >> chiton-$(DEBVERSION).dsc
	echo -n -e 'Checksums-Sha1:\n  '  >> chiton-$(DEBVERSION).dsc
	sha1sum $(DEBSRC) | sed -e 's/ / '$(DEBSRC_FILESIZE)'/' >> chiton-$(DEBVERSION).dsc
	echo -n -e '  ' >> chiton-$(DEBVERSION).dsc
	sha1sum $(DEBSRCPKG) | sed -e 's/ / '$(DEBSRCPKG_FILESIZE)'/' >> chiton-$(DEBVERSION).dsc
	echo -n -e 'Checksums-Sha256:\n  '  >> chiton-$(DEBVERSION).dsc
	sha256sum $(DEBSRC) | sed -e 's/ / '$(DEBSRC_FILESIZE)'/' >> chiton-$(DEBVERSION).dsc
	echo -n -e '  ' >> chiton-$(DEBVERSION).dsc
	sha256sum $(DEBSRCPKG) | sed -e 's/ / '$(DEBSRCPKG_FILESIZE)'/' >> chiton-$(DEBVERSION).dsc

web/inc/configdb.php: web/inc/configdb.php.in setting.hpp dist-version
	-$(MKDIR_P) $(abs_builddir)/web/inc
	$(do_subst) < $(abs_srcdir)/web/inc/configdb.php.in > $@
	echo '/* DO NO EDIT!!! autogenerated from setting.hpp, edit setting.hpp */' >> $@
	grep -A 1000 '//PHP BELOW' $(abs_srcdir)/setting.hpp | sed -E -e 's/^\s*\{"([^"]*?)",/"\1" \=\> array(/'  -e 's/\}/)/g' -e 's/^\s+\"/.\"/' >> $@

man/chiton.cfg.5: man/chiton.cfg.5.in setting.hpp dist-version man/transform-settings.awk
	-$(MKDIR_P) $(abs_builddir)/man
	grep -B 1000 '.\\" START SETTINGS' $(abs_srcdir)/man/chiton.cfg.5.in > $@
	grep -A 1000 '//PHP BELOW' $(abs_srcdir)/setting.hpp | sed -e 's/^ {//' -e 's/},$$/\n/' -e 's/\/\/PHP BELOW//' -e 's/\*\/$$/\n\n/' | $(abs_srcdir)/man/transform-settings.awk >> $@
	grep -A 1000 '.\\" END SETTINGS' $(abs_srcdir)/man/chiton.cfg.5.in >> $@
