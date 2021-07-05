Hitori
======

Hitori is a GTK+ application to generate and let you play games of Hitori, a logic game similar to Sudoku created by Nikoli.

News
====

See NEWS file.

Dependencies
============

[GNOME 3.0 development platform](http://www.gnome.org)

Controls
========

 * Left-click on a cell: Paint/Unpaint cell
 * Ctrl+Left-click on a cell: Tag cell
 * Shift+Left-click on a cell: Tag cell
 * Ctrl+N: New game
 * Ctrl+H: Display a hint
 * Ctrl+Z: Undo last move
 * Ctrl+Shift+Z: Redo last undone move
 * Ctrl+Q: Quit

Copyright
=========

 * Philip Withnall <philip@tecnocode.co.uk>
 * Algorithm assistance from Ben Windsor <benjw_823@hotmail.com>
 * Icon by Jakub Szypulka <cube@szypulka.com>

BUGS
====

Bugs should be entered in [GNOME GitLab](https://gitlab.gnome.org/GNOME/hitori/issues).

To get a better debug output, run:
```
hitori --debug
```

Contact
=======

Philip Withnall <philip@tecnocode.co.uk>

https://wiki.gnome.org/Apps/Hitori

## Default branch renamed to `main`

The default development branch of Hitori has been renamed to `main`. To update
your local checkout, use:
```sh
git checkout master
git branch -m master main
git fetch
git branch --unset-upstream
git branch -u origin/main
git symbolic-ref refs/remotes/origin/HEAD refs/remotes/origin/main
```
