/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/*
 * GData Client
 * Copyright (C) 2014 Carlos Garnacho <carlosg@gnome.org>
 *
 * GData Client is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * GData Client is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with GData Client.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <gdata/gdata.h>
#include <locale.h>
#include <string.h>

#define MAX_RESULTS 10

static int
print_usage (char *argv[])
{
	g_printerr ("usage -- %s [search <term>|topic <ID> [<filter>]|query <mql>]\n\n"
		    "query examples (more info at https://developers.google.com/freebase/v1/mql-overview):\n"
		    " '{\"id\":\"/en/linux\",\"/computer/software/license\":[]}'\n"
		    " '[{\"name\":null,\"/geography/river/length\":null,\"type\":\"/geography/river\",\"/location/location/containedby\":{\"id\":\"/en/england\"}}]'\n"
		    " '[{\"type\":\"/location/citytown\",\"name\":null,\"/location/location/time_zones\":{\"id\":\"/en/central_european_time\"},\"limit\":200}]'\n"
		    "topic examples:\n"
		    " '/en/gnome'\n"
		    " '/m/0fpzzp'\n"
		    " '/computer/software'\n"
		    "search examples:\n"
		    " 'gnome'\n"
		    " 'linux'\n"
		    " 'operating system'\n",
		    argv[0]);
	return -1;
}

int
main (int argc, char *argv[])
{
	GDataFreebaseService *service;
	GError *error = NULL;
	gint retval = 0;

	if (argc < 3)
		return print_usage (argv);

	setlocale (LC_ALL, "");
	service = gdata_freebase_service_new (NULL, NULL);

	if (strcmp (argv[1], "query") == 0) {
		GDataFreebaseResult *result;
		GDataFreebaseQuery *query;

		query = gdata_freebase_query_new (argv[2]);
		result = gdata_freebase_service_query (service, query, NULL, &error);

		if (error) {
			g_critical ("Error querying Freebase: %s", error->message);
			g_error_free (error);
		} else {
			GVariant *variant;
			gchar *str;

			variant = gdata_freebase_result_dup_variant (result);
			g_object_unref (result);

			str = g_variant_print (variant, FALSE);
			g_print ("%s\n", str);
			g_variant_unref (variant);
			g_free (str);
		}

		g_object_unref (query);
	} else if (strcmp (argv[1], "search") == 0) {
		GDataFreebaseSearchResult *result;
		GDataFreebaseSearchQuery *query;

		query = gdata_freebase_search_query_new (argv[2]);
		result = gdata_freebase_service_search (service, query, NULL, &error);

		if (error) {
			g_critical ("Error querying Freebase: %s", error->message);
			g_error_free (error);
		} else {
			const GDataFreebaseSearchResultItem *item;
			guint count, i;

			count = gdata_freebase_search_result_get_num_items (result);

			g_print ("Showing %d of %d items:\n", count,
				 gdata_freebase_search_result_get_total_hits (result));

			for (i = 0; i < count; i++) {
				item = gdata_freebase_search_result_get_item (result, i);

				g_print ("%2d: %s (%s), score: %f\n", i,
					 gdata_freebase_search_result_item_get_name (item),
					 gdata_freebase_search_result_item_get_id (item),
					 gdata_freebase_search_result_item_get_score (item));

				if (gdata_freebase_search_result_item_get_notable_id (item)) {
					g_print ("    pertains to domain: %s(%s)",
						 gdata_freebase_search_result_item_get_notable_name (item),
						 gdata_freebase_search_result_item_get_notable_id (item));
				}

				g_print ("\n");
			}

			g_object_unref (result);
		}

		g_object_unref (query);
	} else if (strcmp (argv[1], "topic") == 0) {
		GDataFreebaseTopicResult *result;
		GDataFreebaseTopicQuery *query;

		query = gdata_freebase_topic_query_new (argv[2]);

		if (argc > 3) {
			const gchar *filter[] = { argv[3], NULL };
			gdata_freebase_topic_query_set_filter (query, filter);
		}

		result = gdata_freebase_service_get_topic (service, query, NULL, &error);

		if (error) {
			g_critical ("Error querying Freebase: %s", error->message);
			g_error_free (error);
		} else {
			GDataFreebaseTopicObject *object;
			GPtrArray *properties;
			guint i;

			object = gdata_freebase_topic_result_dup_object (result);
			g_object_unref (result);

			properties = gdata_freebase_topic_object_list_properties (object);

			for (i = 0; i < properties->len; i++) {
				const gchar *property;
				gint64 hits, count, j;

				property = g_ptr_array_index (properties, i);
				count = gdata_freebase_topic_object_get_property_count (object, property);
				hits = gdata_freebase_topic_object_get_property_hits (object, property);

				if (count == hits)
					g_print ("%s: (%" G_GINT64_FORMAT " values)\n", property, hits);
				else
					g_print ("%s: (%" G_GINT64_FORMAT " of %" G_GINT64_FORMAT " values)\n", property, count, hits);

				for (j = 0; j < count; j++) {
					GDataFreebaseTopicValue *value;
					GType gtype;

					value = gdata_freebase_topic_object_get_property_value (object, property, j);
					g_print ("  %s", gdata_freebase_topic_value_get_text (value));

					if (gdata_freebase_topic_value_is_image (value)) {
						GInputStream *stream;

						stream = gdata_freebase_service_get_image (service, value, NULL, 0, 0, NULL);
						g_print (" (URI: '%s')",
							 gdata_download_stream_get_download_uri (GDATA_DOWNLOAD_STREAM (stream)));
						g_object_unref (stream);
					}

					gtype = gdata_freebase_topic_value_get_value_type (value);

					if (gtype == GDATA_TYPE_FREEBASE_TOPIC_OBJECT) {
						const GDataFreebaseTopicObject *value_object;

						value_object = gdata_freebase_topic_value_get_object (value);
						g_print (" (ID: '%s')", gdata_freebase_topic_object_get_id (value_object));
					}

					g_print ("\n");
				}
			}

			gdata_freebase_topic_object_unref (object);
			g_ptr_array_unref (properties);
		}

		g_object_unref (query);
	} else {
		retval = print_usage (argv);
	}

	g_object_unref (service);

	return retval;
}
