gdata_contacts_headers = files([
  'gdata-contacts-contact.h',
  'gdata-contacts-group.h',
  'gdata-contacts-query.h',
  'gdata-contacts-service.h',
])

install_headers(
  gdata_contacts_headers,
  install_dir: gdata_includedir / 'gdata/services/contacts'
)

sources += files([
  'gdata-contacts-contact.c',
  'gdata-contacts-group.c',
  'gdata-contacts-query.c',
  'gdata-contacts-service.c',
])

headers += gdata_contacts_headers
