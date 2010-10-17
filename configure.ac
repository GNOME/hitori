AC_INIT([hitori],[0.2.4])

AM_INIT_AUTOMAKE(AC_PACKAGE_NAME, AC_PACKAGE_VERSION)
AC_CONFIG_HEADERS([config.h])
AM_MAINTAINER_MODE

AC_PROG_CC
AC_PROG_CPP
LT_INIT

dnl ***************************************************************************
dnl Options
dnl ***************************************************************************

AC_ARG_ENABLE([debug],
	AS_HELP_STRING([--enable-debug],[turn on debugging]),
	[case "${enableval}" in
		yes) debug=true ;;
		no)  debug=false ;;
		*) AC_MSG_ERROR([bad value ${enableval} for --enable-debug]) ;;
	esac],[debug=false])
	AM_CONDITIONAL([DEBUG], [test x$debug = xtrue])

dnl ***************************************************************************
dnl Internationalisation
dnl ***************************************************************************

GETTEXT_PACKAGE=hitori
AC_SUBST(GETTEXT_PACKAGE)
AC_DEFINE_UNQUOTED(GETTEXT_PACKAGE,"$GETTEXT_PACKAGE", [GETTEXT package name])
AM_GNU_GETTEXT([external])
AM_GNU_GETTEXT_VERSION([0.17])
IT_PROG_INTLTOOL([0.35.0])

GNOME_DOC_INIT

dnl ***************************************************************************
dnl Dependencies
dnl ***************************************************************************

AC_CHECK_FUNCS([floor])

PKG_CHECK_MODULES(GENERAL, glib-2.0 gtk+-2.0 >= 2.13 gmodule-2.0 cairo >= 1.4)
AC_SUBST(GENERAL_CFLAGS)
AC_SUBST(GENERAL_LIBS)

AC_CONFIG_FILES([
Makefile
po/Makefile.in
help/Makefile
])
AC_OUTPUT
