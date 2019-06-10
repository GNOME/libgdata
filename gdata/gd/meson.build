gdata_gd_headers = files([
  'gdata-gd-email-address.h',
  'gdata-gd-im-address.h',
  'gdata-gd-name.h',
  'gdata-gd-organization.h',
  'gdata-gd-phone-number.h',
  'gdata-gd-postal-address.h',
  'gdata-gd-reminder.h',
  'gdata-gd-when.h',
  'gdata-gd-where.h',
  'gdata-gd-who.h',
])

install_headers(
  gdata_gd_headers,
  install_dir: join_paths(gdata_includedir, 'gdata/gd')
)

sources += files([
  'gdata-gd-email-address.c',
  'gdata-gd-feed-link.c',
  'gdata-gd-im-address.c',
  'gdata-gd-name.c',
  'gdata-gd-organization.c',
  'gdata-gd-phone-number.c',
  'gdata-gd-postal-address.c',
  'gdata-gd-reminder.c',
  'gdata-gd-when.c',
  'gdata-gd-where.c',
  'gdata-gd-who.c',
])

headers += gdata_gd_headers
