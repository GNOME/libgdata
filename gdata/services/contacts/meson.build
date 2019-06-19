headers = files(
  'gdata-contacts-contact.h',
  'gdata-contacts-group.h',
  'gdata-contacts-query.h',
  'gdata-contacts-service.h',
)

install_headers(
  headers,
  subdir: gdata_include_subdir / 'services/contacts',
)

gir_headers += headers

sources += files(
  'gdata-contacts-contact.c',
  'gdata-contacts-group.c',
  'gdata-contacts-query.c',
  'gdata-contacts-service.c',
)
