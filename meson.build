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

config_h = configuration_data()
gdata_conf = configuration_data()

# REVIEW: Not sure which configuration file this should go to (AC_SUBST shouldn't be going anywhere)
config_h.set('GDATA_VERSION_MAJOR', gdata_version_major)
config_h.set('GDATA_VERSION_MINOR', gdata_version_minor)
config_h.set('GDATA_VERSION_MICRO', gdata_version_micro)
config_h.set('GDATA_API_VERSION', '@0@.@1@'.format(gdata_api_version_major, gdata_api_version_minor))
config_h.set('GDATA_API_VERSION_MAJOR', gdata_api_version_major)
config_h.set('GDATA_API_VERSION_MINOR', gdata_api_version_minor)

config_h.set_quoted('GETTEXT_PACKAGE', 'gdata')
config_h.set_quoted('PACKAGE_BUGREPORT', 'https://gitlab.gnome.org/GNOME/libgdata/issues/')
config_h.set_quoted('PACKAGE_NAME', 'libgdata')
config_h.set_quoted('PACKAGE_STRING', 'libgdata @0@'.format(meson.project_version()))
config_h.set_quoted('PACKAGE_TARNAME', 'libgdata')
config_h.set_quoted('PACKAGE_URL', '')
config_h.set_quoted('PACKAGE_VERSION', meson.project_version())

# REVIEW: How to check if _GNU_SOURCE extensions are actually present?
# Globally define _GNU_SOURCE and therefore enable the GNU extensions
config_h.set('_GNU_SOURCE', true)
#gdata_debug = get_option('buildtype').contains('debug')

gnome = import('gnome')
i18n = import('i18n')
pkg = import('pkgconfig')

po_dir = meson.current_source_dir() / 'po'

top_inc = include_directories('.')

#TODO: Create directory - gdata

cc = meson.get_compiler('c')

# defines
# set_defines = [
#   # package
#   ['PACKAGE_STRING', '@0@ @1@'.format(gdata_name, gdata_version)],
#   ['VERSION', gdata_version],
#   # i18n
#   ['GETTEXT_PACKAGE', gdata_name],
# ]

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

if have_libsoup_2_55_90
  config_h.set10('HAVE_LIBSOUP_2_55_90', true, description: 'Define if the new SoupServer API is available')

# Optional dependencies
have_gdk_pixbuf = dependency('gdk-pixbuf-2.0', version: '>= 2.14')
if have_gdk_pixbuf 
  config_h.set10('HAVE_GDK_PIXBUF,', true, description: 'Defined if gdk-pixbuf is installed')

# Minimum and maximum requirements for gdk-pixbuf
GDK_PIXBUF_CFLAGS="$GDK_PIXBUF_CFLAGS -DGDK_VERSION_MIN_REQUIRED=$GDK_PIXBUF_MIN_REQUIRED -DGDK_VERSION_MAX_ALLOWED=$GDK_PIXBUF_MAX_ALLOWED"

# Create the `config.h` file from the `config_h` data
configure_file(output : 'config.h',
  configuration : config_h)
