subdir('icons')

desktop_file = i18n.merge_file(
  type: 'desktop',
  input: '@0@.desktop.in'.format(application_id),
  output: '@0@.desktop'.format(application_id),
  po_dir: join_paths(meson.source_root(), 'po'),
  install: true,
  install_dir: join_paths(get_option('datadir'), 'applications'),
)

desktop_file_validate = find_program('desktop-file-validate', required: false)
if desktop_file_validate.found()
  test(
    'validate-desktop',
    desktop_file_validate,
    args: [
      desktop_file.full_path(),
    ],
  )
endif

metainfo_file = i18n.merge_file(
  input: '@0@.metainfo.xml.in'.format(application_id),
  output: '@0@.metainfo.xml'.format(application_id),
  po_dir: join_paths(meson.source_root(), 'po'),
  install: true,
  install_dir: join_paths(get_option('datadir'), 'metainfo'),
)

appstreamcli = find_program('appstreamcli', required: false)
if appstreamcli.found()
  test(
    'validate-metainfo', appstreamcli,
    args: [
      'validate', '--no-net', '--explain', metainfo_file.full_path(),
    ],
  )
endif

install_data(
    'org.gnome.hitori.gschema.xml', 
    install_dir: join_paths(get_option('datadir'), 'glib-2.0', 'schemas'),
)

xmllint = find_program('xmllint', required: false)
if xmllint.found()
  gtk_prefix = gtk_dependency.get_pkgconfig_variable('prefix')
  test(
    'validate-ui', xmllint,
    args: [
      '--nonet', '--noblanks', '--noout',
      '--relaxng', join_paths(gtk_prefix, 'share', 'gtk-3.0', 'gtkbuilder.rng'),
      files('hitori.ui'),
    ],
  )
endif
