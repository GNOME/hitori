sources = files(
  'generator.c',
  'interface.c',
  'main.c',
  'rules.c',
)

if not cc.has_function('atexit')
  error('atexit() needed for generated GResource files')
endif

resources = gnome.compile_resources(
  'hitori-resources',
  '@0@.gresource.xml'.format(meson.project_name()),
  source_dir: meson.source_root(),
)

config_header = configure_file(
  output: 'config.h',
  configuration: config_h,
)

executable(
  meson.project_name(),
  sources + resources,
  config_header,
  dependencies: [
    glib_dependency,
    gio_dependency,
    gtk_dependency,
    gmodule_dependency,
    cairo_dependency
  ],
  install: true,
  c_args: [
    '-DHAVE_CONFIG_H',
    '-DGETTEXT_PACKAGE="@0@"'.format (meson.project_name())
  ],
  install_dir: get_option('bindir'),
)
