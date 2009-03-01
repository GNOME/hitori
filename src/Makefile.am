INCLUDES = \
	-DPACKAGE_LOCALE_DIR=\""$(prefix)/$(DATADIRNAME)/locale"\" \
	-DPACKAGE_SRC_DIR=\""$(srcdir)"\" \
	-DPACKAGE_DATA_DIR=\""$(datadir)"\" \
	$(GTK_CFLAGS)

AM_CFLAGS =\
	 -Wall\
	 -g

bin_PROGRAMS = hitori

hitori_SOURCES = \
	main.c  \
	interface.c  \
	main.h  \
	interface.h

hitori_LDFLAGS = 

hitori_LDADD = $(GTK_LIBS)
