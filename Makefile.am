SUBDIRS = help po

ACLOCAL_AMFLAGS = --install -I m4 ${ACLOCAL_FLAGS}

# Executable
bin_PROGRAMS = src/hitori

src_hitori_SOURCES = \
	src/main.c \
	src/interface.c \
	src/main.h \
	src/interface.h \
	src/generator.c \
	src/generator.h \
	src/rules.c \
	src/rules.h \
	$(NULL)
nodist_src_hitori_SOURCES = \
	src/hitori-resources.c \
	$(NULL)

src_hitori_CPPFLAGS = \
	-DPACKAGE_LOCALE_DIR=\""$(prefix)/$(DATADIRNAME)/locale"\" \
	-DPACKAGE_SRC_DIR=\""$(srcdir)"\" \
	-DPACKAGE_DATA_DIR=\""$(datadir)"\" \
	$(DISABLE_DEPRECATED) \
	$(NULL)

src_hitori_LDADD = $(GENERAL_LIBS)
src_hitori_CFLAGS = \
	$(GENERAL_CFLAGS) \
	$(CODE_COVERAGE_CFLAGS) \
	$(WARN_CFLAGS) \
	$(NULL)
src_hitori_LDFLAGS = \
	$(WARN_LDFLAGS) \
	$(CODE_COVERAGE_LDFLAGS) \
	$(NULL)

# Code coverage
@CODE_COVERAGE_RULES@

# Icons
icon_dir = $(datadir)/icons/hicolor
gtk_update_icon_cache = gtk-update-icon-cache -f -t $(icon_dir)

install-data-hook: update-icon-cache
uninstall-hook: update-icon-cache
update-icon-cache:
	@-if test -z "$(DESTDIR)"; then \
		echo "Updating GTK+ icon cache."; \
		$(gtk_update_icon_cache); \
	else \
		echo "*** Icon cache not updated.  After (un)install, run this:"; \
		echo "***   $(gtk_update_icon_cache)"; \
	fi

icon16dir = $(icon_dir)/16x16/apps
icon16_DATA = data/icons/16x16/org.gnome.Hitori.png
icon22dir = $(icon_dir)/22x22/apps
icon22_DATA = data/icons/22x22/org.gnome.Hitori.png
icon32dir = $(icon_dir)/32x32/apps
icon32_DATA = data/icons/32x32/org.gnome.Hitori.png
icon48dir = $(icon_dir)/48x48/apps
icon48_DATA = data/icons/48x48/org.gnome.Hitori.png
iconsymbolicdir = $(icon_dir)/symbolic/apps
iconsymbolic_DATA = data/icons/symbolic/org.gnome.Hitori-symbolic.svg

EXTRA_DIST = \
	data/icons/16x16/org.gnome.Hitori.svg \
	$(icon16_DATA) \
	data/icons/22x22/org.gnome.Hitori.svg \
	$(icon22_DATA) \
	data/icons/32x32/org.gnome.Hitori.svg \
	$(icon32_DATA) \
	data/icons/48x48/org.gnome.Hitori.svg \
	$(icon48_DATA) \
	$(iconsymbolic_DATA) \
	$(NULL)

# GResources
src/hitori-resources.c: $(shell $(GLIB_COMPILE_RESOURCES) --generate-dependencies --sourcedir=$(top_srcdir) $(top_srcdir)/src/hitori.gresource.xml)
	$(AM_V_GEN) $(GLIB_COMPILE_RESOURCES) --target=$@ --sourcedir=$(top_srcdir) --generate-source $(top_srcdir)/src/hitori.gresource.xml

CLEANFILES = src/hitori-resources.c
EXTRA_DIST += src/hitori.gresource.xml

# UI & CSS files
dist_noinst_DATA = \
	data/hitori-menus.ui \
	data/hitori.ui \
	data/hitori.css

# GSettings
gsettings_SCHEMAS = data/org.gnome.hitori.gschema.xml
@GSETTINGS_RULES@

# Desktop file
# We can't use INTLTOOL_DESKTOP_RULE here due to lp#605826
%.desktop: %.desktop.in
	$(MKDIR_P) $(dir $@); LC_ALL=C $(INTLTOOL_MERGE) -d -u -c $(top_builddir)/po/.intltool-merge-cache $(top_srcdir)/po $< $@

desktop_in_files = data/org.gnome.Hitori.desktop.in
desktopdir = $(datadir)/applications
desktop_DATA = $(desktop_in_files:.desktop.in=.desktop)

EXTRA_DIST += \
	$(gsettings_SCHEMAS) \
	$(desktop_in_files) \
	$(NULL)
CLEANFILES += $(desktop_DATA)

# AppData
@APPSTREAM_XML_RULES@
@INTLTOOL_XML_RULE@

appstream_in_files = data/org.gnome.Hitori.appdata.xml.in
appstream_XML = $(appstream_in_files:.appdata.xml.in=.appdata.xml)

EXTRA_DIST += $(appstream_in_files)
CLEANFILES += $(appstream_XML)

# Gubbins
EXTRA_DIST += \
	autogen.sh \
	intltool-extract.in \
	intltool-merge.in \
	intltool-update.in \
	README \
	COPYING \
	COPYING-DOCS \
	AUTHORS \
	INSTALL \
	NEWS \
	ChangeLog.pre-0-2-2 \
	po/ChangeLog.pre-0-2-2 \
	MAINTAINERS \
	hitori.doap \
	$(NULL)

DISTCLEANFILES = \
	intltool-extract \
	intltool-merge \
	intltool-update \
	$(NULL)

MAINTAINERCLEANFILES = \
	$(GITIGNORE_MAINTAINERCLEANFILES_TOPLEVEL) \
	$(GITIGNORE_MAINTAINERCLEANFILES_MAKEFILE_IN) \
	ABOUT-NLS \
	INSTALL \
	compile \
	config.h.in \
	config.rpath \
	intltool-extract.in \
	intltool-merge.in \
	intltool-update.in \
	`find "m4" -type f -name "*.m4" -print` \
	ChangeLog \
	po/Makevars.template \
	po/Rules-quot \
	po/boldquot.sed \
	po/en@boldquot.header \
	po/en@quot.header \
	po/insert-header.sin \
	po/quot.sed \
	po/remove-potcdate.sin \
	$(NULL)

DISTCHECK_CONFIGURE_FLAGS = --disable-scrollkeeper --enable-appstream-util

# Copy all the spec files. Of course, only one is actually used.
dist-hook:
	for specfile in *.spec; do \
		if test -f $$specfile; then \
			cp -p $$specfile $(distdir); \
		fi \
	done

# ChangeLog generation
CHANGELOG_START = V_0_2_2^

ChangeLog: $(srcdir)/ChangeLog
$(srcdir)/ChangeLog:
	@echo Creating $@
	@if test -d "$(srcdir)/.git"; then \
	  (GIT_DIR=$(top_srcdir)/.git ./missing --run \
	   git log $(CHANGELOG_START).. --stat) | fmt --split-only > $@.tmp \
	  && mv -f $@.tmp $@ \
	  || ($(RM) $@.tmp; \
	      echo Failed to generate ChangeLog, your ChangeLog may be outdated >&2; \
	      (test -f $@ || echo git-log is required to generate this file >> $@)); \
	else \
	  test -f $@ || \
	  (echo A git checkout and git-log is required to generate ChangeLog >&2 && \
	  echo A git checkout and git-log is required to generate this file >> $@); \
	fi
.PHONY: $(srcdir)/ChangeLog

# xdg-app bundle
org.gnome.Hitori.bundle:
	$(MKDIR_P) $(abs_top_builddir)/xdg-app-build/hitori
	$(MKDIR_P) $(abs_top_builddir)/xdg-app-repos/hitori
	(cd $(top_srcdir) && \
	 xdg-app build-init $(abs_top_builddir)/xdg-app-build/hitori org.gnome.Hitori org.gnome.Sdk org.gnome.Platform 3.18 && \
	 xdg-app build $(abs_top_builddir)/xdg-app-build/hitori ./configure --prefix=/app && \
	 xdg-app build $(abs_top_builddir)/xdg-app-build/hitori make && \
	 xdg-app build $(abs_top_builddir)/xdg-app-build/hitori make install && \
	 xdg-app build-finish --command=hitori --socket=x11 --socket=session-bus --talk-name=ca.desrt.dconf --filesystem=home $(abs_top_builddir)/xdg-app-build/hitori)
	xdg-app build-export $(abs_top_builddir)/xdg-app-repos/hitori $(abs_top_builddir)/xdg-app-build/hitori
	xdg-app repo-update $(abs_top_builddir)/xdg-app-repos/hitori

.PHONY: org.gnome.Hitori.bundle

-include $(top_srcdir)/git.mk
