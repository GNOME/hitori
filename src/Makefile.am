INCLUDES = \
	-DPACKAGE_LOCALE_DIR=\""$(prefix)/$(DATADIRNAME)/locale"\"	\
	-DPACKAGE_SRC_DIR=\""$(srcdir)"\"				\
	-DPACKAGE_DATA_DIR=\""$(datadir)"\"				\
	$(GTK_CFLAGS)							\
	$(CAIRO_CFLAGS)

AM_CFLAGS = \
	-Wall		\
	-g

if DEBUG
	AM_CFLAGS += \
		-fprofile-arcs	\
		-ftest-coverage
endif

bin_PROGRAMS = hitori

hitori_SOURCES = \
	main.c		\
	interface.c	\
	main.h		\
	interface.h	\
	generator.c	\
	generator.h	\
	rules.c		\
	rules.h

hitori_LDFLAGS = 

hitori_LDADD = \
	$(GTK_LIBS)	\
	$(CAIRO_LIBS)
