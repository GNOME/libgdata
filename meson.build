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
gdata_includedir = get_option('includedir') / meson.project_name()
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
gdata_soversion = '@0@.@1@.@2@'.format(current - age, age, revision)

top_inc_dir = include_directories('.')

########################
# Configuration begins #
########################

config_h = configuration_data()

config_h.set_quoted('VERSION',
  gdata_version,
  description: 'Version number of package.')

# Globally define _GNU_SOURCE and therefore enable the GNU extensions
config_h.set('_GNU_SOURCE',
  true,
  description: 'Enable GNU extensions on systems that have them.')

gnome = import('gnome')
pkgconfig = import('pkgconfig')
python_prog = import('python').find_installation('python3')

# Internationalisation support
i18n = import('i18n')

cp_prog = find_program('cp')

# Absolute paths to glib-mkenums wrapper scripts
enum_header_generator_script = meson.current_source_dir() / './scripts/generate_enum_header.py'
enum_source_generator_script = meson.current_source_dir() / './scripts/generate_enum_source.py'

top_inc_dir = include_directories('.')

cc = meson.get_compiler('c')

symbol_map = meson.current_source_dir() / 'gdata/symbol.map'
ldflags = cc.get_supported_link_arguments('-Wl,--version-script,@0@'.format(symbol_map))

# Requirements
gobject_dep = dependency('gobject-2.0')
glib_dep = dependency('glib-2.0', version: '>= 2.44.0')
gio_dep = dependency('gio-2.0', version: '>= 2.44.0')
gio_unix_dep = dependency('gio-unix-2.0')
libxml_dep = dependency('libxml-2.0')
libsoup_dep = dependency('libsoup-2.4', version: ['>= 2.42.0'])
libjson_glib_dep = dependency('json-glib-1.0', version: '>= 0.15')
liboauth_dep = dependency('oauth', version: '>= 0.9.4')
libgthread_dep = dependency('gthread-2.0')

gdata_deps = [
  gobject_dep,
  glib_dep,
  gio_dep,
  gio_unix_dep,
  libxml_dep,
  libsoup_dep,
  libjson_glib_dep
]

# Private dependencies for libgdata (specifically used for pkgconfig)
gdata_private_deps = [
  liboauth_dep,
  libgthread_dep
]

# libsoup 2.47.3 is needed for the new SoupServer API; but it contained a bug in
# soup_server_set_ssl_cert_file() which was only fixed in 2.55.90.
config_h.set10('HAVE_LIBSOUP_2_55_90',
  libsoup_dep.version().version_compare('>= 2.55.90'),
  description: 'Define if the new SoupServer API is available')

# Optional dependencies
gdk_pixbuf_dep = dependency('gdk-pixbuf-2.0', version: '>= 2.14')
config_h.set10('HAVE_GDK_PIXBUF', gdk_pixbuf_dep.found(), description: 'Defined if gdk-pixbuf is installed')
if gdk_pixbuf_dep.found()
  gdata_deps += gdk_pixbuf_dep
endif

# Check for gtk
gtk_dep_req_version = '>= 2.91.2'
enable_gtk = get_option('gtk')
if enable_gtk.enabled()
  gtk_dep = dependency(
    'gtk+-3.0',
    version: gtk_dep_req_version,
    not_found_message: 'GTK+ support requested but gtk+-3.0 ' + gtk_dep_req_version + ' could not be found'
  )
  gdata_deps += gtk_dep
endif

# GNOME support, which pulls in gcr-base-3 to provide non-pageable memory
enable_gnome = get_option('gnome')

# Check for goa
# GNOME Online Accounts support -- only used if GNOME support is also enabled
# Pulls in gdata/gdata-goa-authorizer.[ch]
enable_goa = get_option('goa')
if enable_gnome.enabled()
  gdata_private_deps += dependency('gcr-base-3')
  goa_dep = dependency('goa-1.0', version: '>= 3.8', required: get_option('goa'))
  config_h.set10('HAVE_GOA', goa_dep.found(), description: 'Define if GOA support is enabled.')
  gdata_private_deps += goa_dep
endif

# Enable always building tests (default: yes) enable_always_build_tests = get_option('always_build_tests')
# Install test programs (default: no)
always_build_tests = get_option('always_build_tests')
install_tests = get_option('installed_tests')

if always_build_tests or install_tests
  libuhttpmock_dep = dependency('libuhttpmock-0.0', version: '>= 0.5.0')
  gdata_deps += libuhttpmock_dep
endif

check_functions = [
  'memset',
  'setlocale',
  'strchr',
  'strstr',
]

check_functions += [
  'CFLocaleCopyCurrent',
  'CFPreferencesCopyAppValue',
]

foreach func: check_functions
  config_h.set10('HAVE_' + func.to_upper(),
    cc.has_function(func))
endforeach

check_headers = [
  'arpa/inet.h',
  'dlfcn.h',
  'inttypes.h',
  'locale.h',
  'memory.h',
  'netinet/in.h',
  'stdint.h',
  'stdlib.h',
  'string.h',
  'strings.h',
  'sys/socket.h',
  'sys/stat.h',
  'sys/time.h',
  'sys/types.h',
  'unistd.h',
]

add_project_arguments('-DGETTEXT_PACKAGE="intltest"', language: 'c')

sources = []
headers = []

subdir('gdata')
subdir('demos')
subdir('po')
if get_option('gtk_doc')
  subdir('docs/reference')
endif

# Create the `config.h` file from the `config_h` data
configure_file(output : 'config.h', configuration : config_h)

