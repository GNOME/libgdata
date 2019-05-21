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
gdata_version_major = ver_arr[0]
gdata_version_minor = ver_arr[1]
gdata_version_micro = ver_arr[2]

# API version
gdata_api_version_major = 0
gdata_api_version_minor = 0

# Define the install directories
includedir = join_paths(get_option('includedir'), meson.project_name(), 'gdata')

# Define the arrays holding headers to be installed
gdata_include_headers = []
gdata_atom_include_headers = []
gdata_app_include_headers = []
gdata_gd_include_headers = []
gdata_gcontact_include_headers = []
gdata_media_include_headers = []
gdata_calendar_include_headers = []
gdata_contacts_include_headers = []
gdata_documents_include_headers = []
gdata_picasaweb_include_headers = []
gdata_youtube_include_headers = []
gdata_tasks_include_headers = []
gdata_freebase_include_headers = []

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
gdata_lt_version = '25:0:3'

########################
# Configuration begins #
########################

config_h = configuration_data()
gdata_conf = configuration_data()

config_h.set_quoted('VERSION',
  meson.project_version(),
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

config_h.set_quoted('PACKAGE_URL',
  'https://wiki.gnome.org/Projects/libgdata',
  description: 'Define to the home page for this package.')

config_h.set_quoted('PACKAGE_VERSION',
    meson.project_version(),
    description: 'Define to the version of this package.')

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
gio_unix_dep = dependency('gio-unix-2.0')
libxml_dep = dependency('libxml-2.0')

libsoup_dep = dependency('libsoup-2.4', version: ['>= 2.42.0'])
libjson_glib_dep = dependency('json-glib-1.0', version: '>= 0.15')
# liboauth_dep = dependency('liboauth', version: '>= 0.9.4')

# libsoup 2.47.3 is needed for the new SoupServer API; but it contained a bug in
# soup_server_set_ssl_cert_file() which was only fixed in 2.55.90.
have_libsoup_2_55_90 = dependency('libsoup-2.4', version: '>= 2.55.90')
config_h.set10('HAVE_LIBSOUP_2_55_90', have_libsoup_2_55_90.found(), description: 'Define if the new SoupServer API is available')

# Optional dependencies
have_gdk_pixbuf = dependency('gdk-pixbuf-2.0', version: '>= 2.14')
config_h.set10('HAVE_GDK_PIXBUF', have_gdk_pixbuf.found(), description: 'Defined if gdk-pixbuf is installed')

# Minimum and maximum requirements for gdk-pixbuf
#GDK_PIXBUF_CFLAGS="$GDK_PIXBUF_CFLAGS -DGDK_VERSION_MIN_REQUIRED=$GDK_PIXBUF_MIN_REQUIRED -DGDK_VERSION_MAX_ALLOWED=$GDK_PIXBUF_MAX_ALLOWED"

# *** Check for gtk ***
enable_gtk = get_option('gtk')
if enable_gtk
  if not gtk_dep.found()
    error('GTK+ support requested but gtk+-3.0 could not be found')
  endif
endif

# GNOME support, which pulls in gcr-base-3 to provide non-pageable memory
enable_gnome = get_option('gnome')

# *** Check for goa ***
# GNOME Online Accounts support -- only used if GNOME support is also enabled
# Pulls in gdata/gdata-goa-authorizer.[ch]

enable_goa = get_option('goa')
if enable_gnome

  # pkg-config package, version string (optional)
  gnome_packages = [
    ['gcr-base-3'],
    ['libxml-2.0'],
    ['goa-1.0', '>= 3.8']
  ]
  if enable_goa
    config_h.set10('HAVE_GOA,', true, description: 'Define if GOA support is enabled.')
    foreach pkg: gnome_packages
      if (pkg.length() == 1)
        dependency(pkg[0])
      elif (pkg.length() == 2)
        dependency(pkg[0], version: pkg[1])
      else
        error('''Messed up meson.build file. Please check the meson.build and
          restart.''')
      endif
    endforeach
  endif
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
  ['iconv', 'Define if you have the iconv() function and it works.']
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

# Check NLS
enable_nls = get_option('nls')
if cc.has_header('libintl.h')
  enable_nls = (true and enable_nls)
elif enable_nls
  error('NLS requested (default) but libintl.h not found.')
endif
config_h.set10('ENABLE_NLS', enable_nls, description: '''Define to 1 if translation of program messages to the user's native
  language is requested.''')

# Check dcgettext()
has_dcgettext = cc.has_function('dcgettext')
config_h.set10('HAVE_DCGETTEXT', has_dcgettext, description: 'Define if the GNU dcgettext() function is already present or preinstalled.')

# Check gettext()
has_gettext = cc.has_function('gettext')
config_h.set10('HAVE_GETTEXT', has_gettext, description: 'Define if the GNU gettext() function is already present or preinstalled.')

# Create the build sub-directories
#subdir('demos')
#subdir('docs')
subdir('gdata')
#subdir('po')

gdata_installable_headers = gdata_include_headers + \
  gdata_atom_include_headers + \
  gdata_app_include_headers + \
  gdata_gd_include_headers + \
  gdata_gcontact_include_headers + \
  gdata_media_include_headers + \
  gdata_calendar_include_headers + \
  gdata_contacts_include_headers + \
  gdata_documents_include_headers + \
  gdata_picasaweb_include_headers + \
  gdata_youtube_include_headers + \
  gdata_tasks_include_headers + \
  gdata_freebase_include_headers

# Install all the headers
foreach header: gdata_installable_headers
  # `header` here is the path from root directory to the header,
  # so we split it and take the first n-1 elements in the split array
  install_subdirectory = ''
  temp_arr = header.split('/')

  foreach elem: temp_arr
    if elem == temp_arr[-1]
      break
    else
      install_subdirectory = join_paths(install_subdirectory, elem)
    endif
  endforeach
  install_headers(header, subdir: join_paths(meson.project_name(), install_subdirectory))
endforeach

# Create the `config.h` file from the `config_h` data
configure_file(output : 'config.h',
  configuration : config_h)

