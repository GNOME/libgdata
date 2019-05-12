project(
  'libgdata', 'c',
  version: '0.17.10',
  license: 'LGPL2+',
  default_options: 'buildtype=debugoptimized',
  meson_version: '>= 0.50.0',
)

gdata_name = meson.project_name()
gdata_version = meson.project_version()

# REVIEW: Is it useful, since we've already defined the version in project block?
ver_arr = gdata_version.split('.')
gdata_version_major = ver_arr[0]
gdata_version_minor = ver_arr[1]
gdata_version_micro = ver_arr[2]

# API version
# TODO: This currently isn't used in the library name; once we hit gdata_version_major=1, that should be changed
gdata_api_version_major = 0
gdata_api_version_minor = 0

# Before making a release, the GDATA_LT_VERSION string should be modified. The string is of the form c:r:a. Follow these instructions sequentially:
#
#  1. If the library source code has changed at all since the last update, then increment revision (‘c:r:a’ becomes ‘c:r+1:a’).
#  2. If any interfaces have been added, removed, or changed since the last update, increment current, and set revision to 0.
#  3. If any interfaces have been added since the last public release, then increment age.
#  4. If any interfaces have been removed or changed since the last public release, then set age to 0.
#
# Note that versioning started at 2:0:0 to ensure no conflicts with e-d-s' libgdata library, whose maximum version was 1:0:0
gdata_lt_version = 25:0:3

########################
# Configuration begins #
########################

gdata_conf = configuration_data()

gdata_conf.set('GDATA_VERSION_MAJOR', gdata_version_major)
gdata_conf.set('GDATA_VERSION_MINOR', gdata_version_minor)
gdata_conf.set('GDATA_VERSION_MICRO', gdata_version_micro)
gdata_conf.set('GDATA_API_VERSION', '@0@.@1@'.format(gdata_api_version_major, gdata_api_version_minor))
gdata_conf.set('GDATA_API_VERSION_MAJOR', gdata_api_version_major)
gdata_conf.set('GDATA_API_VERSION_MINOR', gdata_api_version_minor)

gdata_conf.set_quoted('GETTEXT_PACKAGE', 'gdata')
gdata_conf.set_quoted('PACKAGE_BUGREPORT', 'https://gitlab.gnome.org/GNOME/libgdata/issues/')
gdata_conf.set_quoted('PACKAGE_NAME', 'libgdata')
gdata_conf.set_quoted('PACKAGE_STRING', 'libgdata @0@'.format(meson.project_version()))
gdata_conf.set_quoted('PACKAGE_TARNAME', 'libgdata')
gdata_conf.set_quoted('PACKAGE_URL', '')
gdata_conf.set_quoted('PACKAGE_VERSION', meson.project_version())

# REVIEW: How to check if _GNU_SOURCE extensions are actually present?
# Globally define _GNU_SOURCE and therefore enable the GNU extensions
gdata_conf.set('_GNU_SOURCE', true)
#gdata_debug = get_option('buildtype').contains('debug')

gnome = import('gnome')
i18n = import('i18n')
pkg = import('pkgconfig')

po_dir = meson.current_source_dir() / 'po'

top_inc = include_directories('.')

#TODO: Create directory - gdata

cc = meson.get_compiler('c')

# defines
set_defines = [
  # package
  ['PACKAGE_STRING', '@0@ @1@'.format(gdata_name, gdata_version)],
  ['VERSION', gdata_version],
  # i18n
  ['GETTEXT_PACKAGE', gdata_name],
]

# Requirements
gobject_dep = dependency('gobject-2.0')
glib_dep = dependency('glib-2.0', version: '>= 2.44.0')
gio_dep = dependency('gio-2.0', version: '>= 2.44.0')
# REVIEW: Is it needed? How is it different from gio?
gio_unix_dep = dependency('gio-unix-2.0')
libxml_dep = dependency('libxml-2.0')
libsoup_dep = dependency('libsoup-2.4', version: ['>= 2.42.0', '<= 2.48.0'])
libjson_glib_dep- = dependency('json-glib-1.0', version: '>= 0.15')
liboauth_dep = dependency('liboauth', version: '>= 0.9.4')

# libsoup 2.47.3 is needed for the new SoupServer API; but it contained a bug in
# soup_server_set_ssl_cert_file() which was only fixed in 2.55.90.

have_libsoup_2_55_90 = dependency('libsoup-2.4', version: '>= 2.55.90')

