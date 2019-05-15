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
gdata_lt_version = '25:0:3'

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

# REVIEW: I'm currently having version 2.66.1. Meson throws an error and stops
# whereas Autotools just keeps running even after having the max version set in
# $SOUP_MAX_ALLOWED

libsoup_dep = dependency('libsoup-2.4', version: ['>= 2.42.0'])
libjson_glib_dep = dependency('json-glib-1.0', version: '>= 0.15')

# REVIEW: Can't find this library, even though pkg-config shows file is present
# in /usr/lib. So, commenting out for now.
#liboauth_dep = dependency('liboauth', version: '>= 0.9.4')

# libsoup 2.47.3 is needed for the new SoupServer API; but it contained a bug in
# soup_server_set_ssl_cert_file() which was only fixed in 2.55.90.
have_libsoup_2_55_90 = dependency('libsoup-2.4', version: '>= 2.55.90')

if have_libsoup_2_55_90.found()
  config_h.set10('HAVE_LIBSOUP_2_55_90', true, description: 'Define if the new SoupServer API is available')
endif

# Optional dependencies
have_gdk_pixbuf = dependency('gdk-pixbuf-2.0', version: '>= 2.14')
if have_gdk_pixbuf.found()
  config_h.set10('HAVE_GDK_PIXBUF,', true, description: 'Defined if gdk-pixbuf is installed')
endif


# TODO: How to handle the CFLAGS?

# Minimum and maximum requirements for gdk-pixbuf
#GDK_PIXBUF_CFLAGS="$GDK_PIXBUF_CFLAGS -DGDK_VERSION_MIN_REQUIRED=$GDK_PIXBUF_MIN_REQUIRED -DGDK_VERSION_MAX_ALLOWED=$GDK_PIXBUF_MAX_ALLOWED"

# *** Check for gtk ***
enable_gtk = get_option('gtk')
if enable_gtk
  if not gtk_dep.found()
    error('GTK+ support requested but gtk+-3.0 could not be found')
  endif
endif

# *** Check for GNOME ***
# GNOME support, which pulls in gcr-base-3 to provide non-pageable memory

# REVIEW: How to check if GNOME is actually installed?
#enable_gnome = get_option('gnome')
#if enable_gnome
  #gnome_dep = dependency('gnome', version: '>= 2.91.2')
  #if not gnome_dep.found()
    #error('GNOME support requested but GNOME could not be found')
    #endif
#endif

# *** Check for goa ***
# GNOME Online Accounts support -- only used if GNOME support is also enabled
# Pulls in gdata/gdata-goa-authorizer.[ch]
enable_goa = get_option('goa')

#if enable_gnome
  ## Each of the below is an array of arrays, which has the first member as the
  ## package and the second (optional) as the package version
  ## REVIEW: Useless?
  #gnome_packages_public = []
  #gnome_packages_private = [
    #['gcr-base-3'],
    #['libxml-2.0'],
  #]
  #if enable_goa
    #gnome_packages_public += []
    #gnome_packages_private += [['goa-1.0', '3.8']]
    #config_h.set10('HAVE_GOA,', true, description: 'Define if GOA support is enabled')
    #gnome_packages = gnome_packages_public + gnome_packages_private
    #foreach pkg: gnome_packages
      #if (pkg.length() == 1)
	#dependency(pkg[0])
      #elif (pkg.length() == 2)
	#dependency(pkg[0], version: pkg[1])
      #else
	#error('''Messed up meson.build file. Please check the meson.build and
	  #restart.''')
      #endif
    #endforeach
  #endif
#endif

# Enable always building tests (default: yes)
enable_always_build_tests = get_option('always_build_tests')

# Install test programs (default: no)
enable_installed_tests = get_option('installed_tests')
if enable_always_build_tests or enable_installed_tests
  dependency('libuhttpmock-0.0', version: '>= 0.5.0')
endif

# Various necessary functions
check_functions = [
  'memset',
  'setlocale',
  'strchr',
  'strstr',
]

foreach func: check_functions
  config_h.set('HAVE_' + func.to_upper(), cc.has_function(func))
endforeach

check_headers = [
  'sys/time.h',
  'arpa/inet.h',
  'locale.h',
  'netinet/in.h',
  'sys/socket.h',
]

foreach header: check_headers
  has_header = cc.has_header(header)
  config_h.set10('HAVE_' + header.underscorify().to_upper(), has_header)
endforeach

# Internationalisation support

#i18n = import('i18n')

#gettext_package = 'gdata'
#add_project_arguments('-DGETTEXT_PACKAGE=' + gettext_package, language: 'c')
#config_h.set_quoted('GETTEXT_PACKAGE', 'gdata', description: 'Define to the Gettext package name')
#subdir('po')

# TODO: Remove this **TO THE END**
# Create the `config.h` file from the `config_h` data
configure_file(output : 'config.h',
  configuration : config_h)


