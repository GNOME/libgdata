project(
  'libgdata', 'c',
  version: '0.17.10',
  license: 'LGPL2+',
  default_options: 'buildtype=debugoptimized',
  meson_version: '>= 0.50.0',
)

gdata_name = meson.project_name()
gdata_version = meson.project_version()

ver_arr = gdata_version.split('.')
gdata_version_major = ver_arr[0].to_int()
gdata_version_minor = ver_arr[1].to_int()
gdata_version_micro = ver_arr[2].to_int()

# API version
gdata_api_version_major = 0
gdata_api_version_minor = 0

# Define the install directories
gdata_prefix = get_option('prefix')
gdata_bindir = get_option('bindir')
gdata_datadir = get_option('datadir')
gdata_includedir = join_paths(get_option('includedir'), meson.project_name())
gdata_localedir = get_option('localedir')
gdata_mandir = get_option('mandir')

# The following array contains private headers, and shouldn't be installed
gdata_private_headers = []

# Before making a release, the GDATA_LT_VERSION string should be modified. The string is of the form c:r:a. Follow these instructions sequentially:
#
#  1. If the library source code has changed at all since the last update, then increment revision (‘c:r:a’ becomes ‘c:r+1:a’).
#  2. If any interfaces have been added, removed, or changed since the last update, increment current, and set revision to 0.
#  3. If any interfaces have been added since the last public release, then increment age.
#  4. If any interfaces have been removed or changed since the last public release, then set age to 0.
#
# Note that versioning started at 2:0:0 to ensure no conflicts with e-d-s' libgdata library, whose maximum version was 1:0:0
current = 25
revision = 0
age = 3
gdata_libversion = '@0@.@1@.@2@'.format(current - age, age, revision)

########################
# Configuration begins #
########################

config_h = configuration_data()

config_h.set_quoted('VERSION',
  gdata_version,
  description: 'Version number of package.')

config_h.set_quoted('PACKAGE',
  meson.project_name(),
  description: 'Name of package')

config_h.set_quoted('GETTEXT_PACKAGE',
  'gdata',
  description: 'Define to the Gettext package name')

config_h.set_quoted('PACKAGE_BUGREPORT',
  'https://gitlab.gnome.org/GNOME/libgdata/issues/',
  description: 'Define to the address where bug reports for this package should be sent.')

config_h.set_quoted('PACKAGE_NAME',
  'libgdata',
  description: 'Define to the full name of this package.')

config_h.set_quoted('PACKAGE_STRING',
  'libgdata @0@'.format(meson.project_version()),
  description: 'Define to the full name and version of this package.')

config_h.set_quoted('PACKAGE_TARNAME',
  'libgdata',
  description: 'Define to the one symbol short name of this package.')

# Globally define _GNU_SOURCE and therefore enable the GNU extensions
config_h.set('_GNU_SOURCE',
  true,
  description: 'Enable GNU extensions on systems that have them.')
#gdata_debug = get_option('buildtype').contains('debug')

gnome = import('gnome')
pkg = import('pkgconfig')

po_dir = meson.current_source_dir() / 'po'

top_inc = include_directories('.')

cc = meson.get_compiler('c')

# Requirements
gobject_dep = dependency('gobject-2.0')
glib_dep = dependency('glib-2.0', version: '>= 2.44.0')
gio_dep = dependency('gio-2.0', version: '>= 2.44.0')
gio_unix_dep = dependency('gio-unix-2.0')
libxml_dep = dependency('libxml-2.0')

libsoup_dep = dependency('libsoup-2.4', version: ['>= 2.42.0'])
libjson_glib_dep = dependency('json-glib-1.0', version: '>= 0.15')
liboauth_dep = dependency('oauth', version: '>= 0.9.4')

# libsoup 2.47.3 is needed for the new SoupServer API; but it contained a bug in
# soup_server_set_ssl_cert_file() which was only fixed in 2.55.90.
config_h.set10('HAVE_LIBSOUP_2_55_90',
  libsoup_dep.version().version_compare('>= 2.55.90'),
  description: 'Define if the new SoupServer API is available')

# Optional dependencies
gdk_pixbuf_dep = dependency('gdk-pixbuf-2.0', version: '>= 2.14')
config_h.set10('HAVE_GDK_PIXBUF', gdk_pixbuf_dep.found(), description: 'Defined if gdk-pixbuf is installed')

# TODO: Handle the below CFLAGS
# Minimum and maximum requirements for gdk-pixbuf
#GDK_PIXBUF_CFLAGS="$GDK_PIXBUF_CFLAGS -DGDK_VERSION_MIN_REQUIRED=$GDK_PIXBUF_MIN_REQUIRED -DGDK_VERSION_MAX_ALLOWED=$GDK_PIXBUF_MAX_ALLOWED"

# *** Check for gtk ***
enable_gtk = get_option('gtk')
if enable_gtk.enabled()
  gtk_dep = dependency('gtk+-3.0',
    version: '>= 2.91.2',
    not_found_message: 'GTK+ support requested but gtk+-3.0 >= 2.91.2 could not be found')
endif

# GNOME support, which pulls in gcr-base-3 to provide non-pageable memory
enable_gnome = get_option('gnome')

# *** Check for goa ***
# GNOME Online Accounts support -- only used if GNOME support is also enabled
# Pulls in gdata/gdata-goa-authorizer.[ch]

enable_goa = get_option('goa')
if enable_gnome.enabled()
  dependency('gcr-base-3')
  dependency('libxml-2.0')
  goa_dep = dependency('goa-1.0', version: '>= 3.8')
  config_h.set10('HAVE_GOA,', enable_goa.enabled() and goa_dep.found(), description: 'Define if GOA support is enabled.')
endif

# Enable always building tests (default: yes)
enable_always_build_tests = get_option('always_build_tests')

# Install test programs (default: no)
enable_installed_tests = get_option('installed_tests')
if enable_always_build_tests or enable_installed_tests
  dependency('libuhttpmock-0.0', version: '>= 0.5.0')
endif

# Various necessary functions
# Array [ Array [ function_name, (optional) description ]
check_functions = [
  ['memset'],
  ['setlocale'],
  ['strchr'],
  ['strstr'],
]

check_functions += [
  ['CFLocaleCopyCurrent', '''Define to 1 if you have the Mac OS X function
    CFLocaleCopyCurrent in the CoreFoundation framework.'''],
  ['CFPreferencesCopyAppValue', '''Define to 1 if you have the Mac OS X function
    CFPreferencesCopyAppValue in the CoreFoundation framework.''']
]

foreach func: check_functions
  if (func.length() == 1)
    config_h.set10('HAVE_' + func[0].to_upper(),
      cc.has_function(func[0]),
      description: 'Define to 1 if you have the `@0@` function.'.format(func[0])
    )
  elif (func.length() == 2)
    config_h.set10('HAVE_' + func[0].to_upper(),
      cc.has_function(func[0]),
      description: func[1]
    )
  else
    error('''Messed up meson.build file. Please check the meson.build and
      restart.''')
  endif
endforeach

check_headers = [
  'sys/time.h',
  'arpa/inet.h',
  'locale.h',
  'netinet/in.h',
  'sys/socket.h',
  'sys/stat.h',
  'sys/types.h',
  'stdlib.h',
  'stdint.h',
  'string.h',
  'strings.h',
  'unistd.h',
  'memory.h',
  'dlfcn.h',
  'inttypes.h',
]

foreach header: check_headers
  has_header = cc.has_header(header)
  config_h.set10('HAVE_' + header.underscorify().to_upper(),
    has_header,
    description: 'Define to 1 if you have the <@0@> header file.'.format(header))
endforeach

# Internationalisation support
i18n = import('i18n')
add_project_arguments('-DGETTEXT_PACKAGE="intltest"', language: 'c')
i18n.gettext('gdata', args: '--directory=' + meson.source_root())

# Create the build sub-directories
#subdir('demos')
#subdir('docs')
subdir('gdata')
#subdir('po')

# Create the `config.h` file from the `config_h` data
configure_file(output : 'config.h',
  configuration : config_h)

