AC_INIT([Hitori],[3.22.0],[https://bugzilla.gnome.org/enter_bug.cgi?product=hitori],[hitori],[https://wiki.gnome.org/Apps/Hitori])
AX_IS_RELEASE([git-directory])

AM_INIT_AUTOMAKE([check-news dist-xz no-dist-gzip subdir-objects])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_MACRO_DIR([m4])
AM_MAINTAINER_MODE([enable])

# Options
AX_CHECK_ENABLE_DEBUG([yes],[DEBUG])

AC_PROG_CC
AM_PROG_CC_C_O
AC_PROG_CPP
LT_INIT

m4_ifdef([AM_SILENT_RULES],[AM_SILENT_RULES([yes])])

AX_COMPILER_FLAGS([WARN_CFLAGS],[WARN_LDFLAGS])

# Code coverage
AX_CODE_COVERAGE()

# GResources
AC_PATH_PROG([GLIB_COMPILE_RESOURCES],[glib-compile-resources])

# Internationalisation
GETTEXT_PACKAGE=hitori
AC_SUBST([GETTEXT_PACKAGE])
AC_DEFINE_UNQUOTED([GETTEXT_PACKAGE],["$GETTEXT_PACKAGE"],[gettext package name])
IT_PROG_INTLTOOL([0.50.2])

# GSettings
GLIB_GSETTINGS

# Documentation
YELP_HELP_INIT

# AppData
APPSTREAM_XML

# Dependencies
AC_CHECK_FUNCS([atexit])
PKG_CHECK_MODULES([GENERAL],[
  glib-2.0
  gio-2.0 >= 2.32
  gtk+-3.0 >= 3.15.0
  gmodule-2.0 cairo >= 1.4
])

# Output!
AC_CONFIG_FILES([
Makefile
po/Makefile.in
help/Makefile
])
AC_OUTPUT
