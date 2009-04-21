SUBDIRS = help src po data

EXTRA_DIST =			\
	autogen.sh		\
	intltool-extract.in	\
	intltool-merge.in	\
	intltool-update.in	\
	gnome-doc-utils.make	\
	README			\
	COPYING			\
	COPYING-DOCS		\
	AUTHORS			\
	INSTALL			\
	NEWS			\
	ChangeLog.pre-0-2-2	\
	po/ChangeLog.pre-0-2-2	\
	MAINTAINERS		\
	hitori.doap

DISTCLEANFILES = \
	intltool-extract	\
	intltool-merge		\
	intltool-update

MAINTAINERCLEANFILES = \
	INSTALL			\
	aclocal.m4		\
	compile			\
	config.guess		\
	config.h.in		\
	config.sub		\
	depcomp			\
	install-sh		\
	ltmain.sh		\
	missing			\
	mkinstalldirs		\
	gnome-doc-utils.make	\
	ChangeLog \
	`find "$(srcdir)" -type f -name Makefile.in -print`

DISTCHECK_CONFIGURE_FLAGS = --disable-scrollkeeper

# Copy all the spec files. Of cource, only one is actually used.
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
