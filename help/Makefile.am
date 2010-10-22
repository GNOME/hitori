include $(top_srcdir)/gnome-doc-utils.make
dist-hook: doc-dist-hook

DOC_ID = hitori
DOC_LINGUAS = cs da de en_GB es sl sv zh_CN
DOC_FIGURES = figures/hitori_main_window.png
DOC_PAGES = \
	customization.page \
	game.page \
	index.page \
	license.page \
	playing.page \
	strategy.page

EXTRA_DIST = ChangeLog.pre-0-2-2

GITIGNOREFILES = \
	$(_DOC_LC_PAGES) \
	$(_DOC_MOFILES) \
	$(NULL)

-include $(top_srcdir)/git.mk
