project(
  'libgdata', 'c',
  version: '0.19.0',
  license: 'LGPL2.1+',
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
gdata_datadir = get_option('datadir')
gdata_libexecdir = get_option('libexecdir')
gdata_includedir = get_option('includedir')

gdata_include_subdir = gdata_name / 'gdata'

# Before making a release, the GDATA_LT_VERSION string should be modified. The string is of the form c:r:a. Follow these instructions sequentially:
#
#  1. If the library source code has changed at all since the last update, then increment revision (‘c:r:a’ becomes ‘c:r+1:a’).
#  2. If any interfaces have been added, removed, or changed since the last update, increment current, and set revision to 0.
#  3. If any interfaces have been added since the last public release, then increment age.
#  4. If any interfaces have been removed or changed since the last public release, then set age to 0.
#
# Note that versioning started at 2:0:0 to ensure no conflicts with e-d-s' libgdata library, whose maximum version was 1:0:0
current = 28
revision = 0
age = 6
gdata_soversion = '@0@.@1@.@2@'.format(current - age, age, revision)

top_inc_dir = include_directories('.')

########################
# Configuration begins #
########################

config_h = configuration_data()

config_h.set_quoted('VERSION', gdata_version)

# Globally define _GNU_SOURCE and therefore enable the GNU extensions
config_h.set('_GNU_SOURCE', true)

config_h.set_quoted('GETTEXT_PACKAGE', 'gdata')

gnome = import('gnome')
pkgconfig = import('pkgconfig')

# Internationalisation support
i18n = import('i18n')

build_root = meson.current_build_dir()

# FIXME: Work around the namespace being incorrectly detected
# by glib-mkenums. This needs to be fixed by changing the
# namespace in libgdata. See !6.
sed = find_program('sed')
enum_source_cmd = [sed, '-e', 's/-in//', '-e', 's/g_data/gdata/', build_root / '@INPUT@']
enum_header_cmd = [sed, '-e', 's/g_data/gdata/', '-e', 's/G_TYPE_DATA/GDATA_TYPE/', build_root / '@INPUT@']

top_inc_dir = include_directories('.')

cc = meson.get_compiler('c')

# Requirements
gdata_deps = [
  dependency('gobject-2.0'),
  dependency('glib-2.0', version: '>= 2.62.0'),
  dependency('gio-2.0', version: '>= 2.44.0'),
  dependency('json-glib-1.0', version: '>= 0.15'),
  dependency('libxml-2.0'),
]

common_c_args = [
  '-DGLIB_VERSION_MIN_REQUIRED=GLIB_VERSION_2_62',
  '-DGLIB_VERSION_MAX_ALLOWED=GLIB_VERSION_2_68',
]

# Private dependencies for libgdata (specifically used for pkgconfig)
gdata_private_deps = [
  dependency('gthread-2.0'),
]

libsoup_dep = dependency('libsoup-2.4', version: '>= 2.42.0')
gdata_deps += libsoup_dep
# libsoup 2.47.3 is needed for the new SoupServer API; but it contained a bug in
# soup_server_set_ssl_cert_file() which was only fixed in 2.55.90.
config_h.set('HAVE_LIBSOUP_2_55_90', libsoup_dep.version().version_compare('>= 2.55.90'))

# Check for gtk
gtk_dep_req_version = '>= 2.91.2'
gtk_dep = dependency(
  'gtk+-3.0',
  version: gtk_dep_req_version,
  required: get_option('gtk'),
  not_found_message: 'GTK+ support requested but gtk+-3.0 ' + gtk_dep_req_version + ' could not be found',
)

# GNOME support, which pulls in gcr-4 to provide non-pageable memory
gcr_dep = dependency('gcr-4', required: get_option('gnome'))
enable_gnome = gcr_dep.found()
if enable_gnome
  gdata_private_deps += dependency('gcr-4')
endif
config_h.set('HAVE_GNOME', enable_gnome)

# Check for goa
# GNOME Online Accounts support -- only used if GNOME support is also enabled
# Pulls in gdata/gdata-goa-authorizer.[ch]
goa_dep = dependency('goa-1.0', version: '>= 3.8', required: get_option('goa'))
enable_goa = goa_dep.found()
if enable_goa
  gdata_deps += goa_dep
endif

# Enable always building tests (default: yes) enable_always_build_tests = get_option('always_build_tests')
# Install test programs (default: no)
always_build_tests = get_option('always_build_tests')
install_tests = get_option('installed_tests')

if always_build_tests
  libuhttpmock_dep = dependency('libuhttpmock-0.0', version: '>= 0.5.0')

  gdk_pixbuf_dep = dependency('gdk-pixbuf-2.0', version: '>= 2.14')
  config_h.set('HAVE_GDK_PIXBUF', gdk_pixbuf_dep.found())
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
  cc.has_function(func)
endforeach

check_headers = [
  'arpa/inet.h',
  'locale.h',
  'memory.h',
  'netinet/in.h',
  'stdlib.h',
  'string.h',
  'strings.h',
  'sys/socket.h',
  'sys/time.h',
  'sys/types.h',
  'unistd.h',
]

foreach func: check_headers
  cc.has_header(func)
endforeach

subdir('gdata')
subdir('demos')
subdir('po')
if get_option('gtk_doc')
  subdir('docs/reference')
endif

# Create the `config.h` file from the `config_h` data
configure_file(output: 'config.h', configuration: config_h)
