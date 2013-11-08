AC_INIT([Hitori],[0.4.2],[https://bugzilla.gnome.org/enter_bug.cgi?product=hitori],[hitori],[https://wiki.gnome.org/Apps/Hitori])

AM_INIT_AUTOMAKE([check-news dist-xz no-dist-gzip subdir-objects])
AC_CONFIG_HEADERS([config.h])
AM_MAINTAINER_MODE([enable])

AC_PROG_CC
AM_PROG_CC_C_O
AC_PROG_CPP
LT_INIT

m4_ifdef([AM_SILENT_RULES],[AM_SILENT_RULES([yes])])

GNOME_COMPILE_WARNINGS([maximum])
GNOME_MAINTAINER_MODE_DEFINES

# Options
AC_ARG_ENABLE([debug],AS_HELP_STRING([--enable-debug],[Enable debugging]),[debug=$enableval],[debug=no])
AM_CONDITIONAL([DEBUG],[test "$debug" = "yes"])

# Internationalisation
GETTEXT_PACKAGE=hitori
AC_SUBST([GETTEXT_PACKAGE])
AC_DEFINE_UNQUOTED([GETTEXT_PACKAGE],["$GETTEXT_PACKAGE"],[gettext package name])
IT_PROG_INTLTOOL([0.35.0])

# Documentation
YELP_HELP_INIT

# Dependencies
PKG_CHECK_MODULES([GENERAL],[glib-2.0 gtk+-3.0 gmodule-2.0 cairo >= 1.4])

# Output!
AC_CONFIG_FILES([
Makefile
po/Makefile.in
help/Makefile
])
AC_OUTPUT
