project('hitori', 'c',
    version: '3.22.4')


i18n = import('i18n')
gnome = import('gnome')
application_id = 'org.gnome.Hitori'

# Dependencies
glib_dependency = dependency('glib-2.0')
gio_dependency = dependency('gio-2.0', version: '>= 2.32')
gtk_dependency = dependency('gtk+-3.0', version: '>= 3.15.0')
gmodule_dependency = dependency('gmodule-2.0')
cairo_dependency = dependency('cairo', version: '>= 1.4')

config_h = configuration_data()
config_h.set_quoted('PACKAGE', meson.project_name())
config_h.set_quoted('PACKAGE_NAME', 'Hitori')
config_h.set_quoted('PACKAGE_TARNAME', meson.project_name())
config_h.set_quoted('PACKAGE_STRING', 'Hitori @0@'.format(meson.project_version()))
config_h.set_quoted('PACKAGE_VERSION', meson.project_version())
config_h.set_quoted('PACKAGE_BUGREPORT', 'https://gitlab.gnome.org/GNOME/hitori/issues')
config_h.set_quoted('PACKAGE_URL', 'https://wiki.gnome.org/Apps/Hitori')
config_h.set_quoted('GETTEXT_PACKAGE', meson.project_name())
config_h.set_quoted('PACKAGE_LOCALE_DIR', join_paths(get_option('prefix'), get_option('localedir')))
config_h.set_quoted('VERSION', meson.project_version())


subdir('data')
subdir('help')
subdir('po')
subdir('src')

meson.add_install_script('build-aux/meson_post_install.py')
