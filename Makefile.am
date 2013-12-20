SUBDIRS = help po

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

src_hitori_CPPFLAGS = \
	-DPACKAGE_LOCALE_DIR=\""$(prefix)/$(DATADIRNAME)/locale"\" \
	-DPACKAGE_SRC_DIR=\""$(srcdir)"\" \
	-DPACKAGE_DATA_DIR=\""$(datadir)"\" \
	$(DISABLE_DEPRECATED) \
	$(NULL)

src_hitori_LDADD = $(GENERAL_LIBS)
src_hitori_CFLAGS = \
	$(GENERAL_CFLAGS) \
	$(WARN_CFLAGS) \
	$(NULL)

if DEBUG
src_hitori_CFLAGS += \
	-fprofile-arcs \
	-ftest-coverage \
	$(NULL)
endif

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
icon16_DATA = data/icons/16x16/hitori.png
icon22dir = $(icon_dir)/22x22/apps
icon22_DATA = data/icons/22x22/hitori.png
icon32dir = $(icon_dir)/32x32/apps
icon32_DATA = data/icons/32x32/hitori.png
icon48dir = $(icon_dir)/48x48/apps
icon48_DATA = data/icons/48x48/hitori.png

EXTRA_DIST = \
	data/icons/16x16/hitori.svg \
	$(icon16_DATA) \
	data/icons/22x22/hitori.svg \
	$(icon22_DATA) \
	data/icons/32x32/hitori.svg \
	$(icon32_DATA) \
	data/icons/48x48/hitori.svg \
	$(icon48_DATA) \
	$(NULL)

# UI file
uidir = $(datadir)/hitori
ui_DATA = data/hitori.ui

# Desktop file
# We can't use INTLTOOL_DESKTOP_RULE here due to lp#605826
%.desktop: %.desktop.in
	$(MKDIR_P) $(dir $@); LC_ALL=C $(INTLTOOL_MERGE) -d -u -c $(top_builddir)/po/.intltool-merge-cache $(top_srcdir)/po $< $@

desktop_in_files = data/hitori.desktop.in
desktopdir = $(datadir)/applications
desktop_DATA = $(desktop_in_files:.desktop.in=.desktop)

EXTRA_DIST += \
	$(ui_DATA) \
	$(desktop_in_files) \
	$(NULL)
CLEANFILES = $(desktop_DATA)

# AppData
@APPDATA_XML_RULES@
@INTLTOOL_XML_RULE@

appdata_in_files = data/hitori.appdata.xml.in
appdata_XML = $(appdata_in_files:.appdata.xml.in=.appdata.xml)

EXTRA_DIST += $(appdata_in_files)
CLEANFILES += $(appdata_XML)

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
	ABOUT-NLS \
	INSTALL \
	aclocal.m4 \
	compile \
	config.guess \
	config.h.in \
	config.rpath \
	config.sub \
	depcomp \
	install-sh \
	intltool-extract.in \
	intltool-merge.in \
	intltool-update.in \
	ltmain.sh \
	`find "m4" -type f -name "*.m4" -print` \
	missing \
	mkinstalldirs \
	ChangeLog \
	`find "$(srcdir)" -type f -name Makefile.in -print` \
	po/Makevars.template \
	po/Rules-quot \
	po/boldquot.sed \
	po/en@boldquot.header \
	po/en@quot.header \
	po/insert-header.sin \
	po/quot.sed \
	po/remove-potcdate.sin \
	$(NULL)

GITIGNOREFILES = src/.dirstamp

DISTCHECK_CONFIGURE_FLAGS = --disable-scrollkeeper --enable-appdata-validate

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

-include $(top_srcdir)/git.mk
