/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/*
 * GData Client
 * Copyright (C) Philip Withnall 2008-2009 <philip@tecnocode.co.uk>
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

#include <glib.h>
#include <locale.h>

#include "gdata.h"

static void
test_entry_get_xml (void)
{
	/*GTimeVal updated, published, updated2, published2;*/
	GDataEntry *entry, *entry2;
	GDataCategory *category;
	GDataLink *link;
	GDataAuthor *author;
	gchar *xml;
	GError *error = NULL;

	entry = gdata_entry_new (NULL);
	gdata_entry_set_title (entry, "Testing title & escaping");
	gdata_entry_set_content (entry, "This is some sample content testing, amongst other things, <markup> & odd characters‽");

	/*g_time_val_from_iso8601 ("2009-01-25T14:07:37.880860Z", &updated);
	gdata_entry_set_updated (entry, &updated);

	g_time_val_from_iso8601 ("2009-01-23T14:06:37.880860Z", &published);
	gdata_entry_set_published (entry, &published);*/

	/* Categories */
	category = gdata_category_new ("test", NULL, NULL);
	gdata_entry_add_category (entry, category);
	g_object_unref (category);
	category = gdata_category_new ("example", NULL, "Example stuff");
	gdata_entry_add_category (entry, category);
	g_object_unref (category);
	category = gdata_category_new ("Film", "http://gdata.youtube.com/schemas/2007/categories.cat", "Film & Animation");
	gdata_entry_add_category (entry, category);
	g_object_unref (category);

	/* Links */
	link = gdata_link_new ("http://test.com/", "self");
	gdata_link_set_content_type (link, "application/atom+xml");
	gdata_entry_add_link (entry, link);
	g_object_unref (link);
	link = gdata_link_new ("http://example.com/", NULL);
	gdata_entry_add_link (entry, link);
	g_object_unref (link);
	link = gdata_link_new ("http://test.mn/", "related");
	gdata_link_set_content_type (link, "text/html");
	gdata_link_set_language (link, "mn");
	gdata_link_set_title (link, "A treatise on Mongolian test websites & other stuff.");
	gdata_link_set_length (link, 5010);
	gdata_entry_add_link (entry, link);
	g_object_unref (link);

	/* Authors */
	author = gdata_author_new ("Joe Bloggs", "http://example.com/", "joe@example.com");
	gdata_entry_add_author (entry, author);
	g_object_unref (author);
	author = gdata_author_new ("John Smith", NULL, "smith.john@example.com");
	gdata_entry_add_author (entry, author);
	g_object_unref (author);
	author = gdata_author_new ("F. Barr؟", NULL, NULL);
	gdata_entry_add_author (entry, author);
	g_object_unref (author);

	/* Check the generated XML's OK */
	xml = gdata_parsable_get_xml (GDATA_PARSABLE (entry));
	g_assert_cmpstr (xml, ==,
			 "<entry xmlns='http://www.w3.org/2005/Atom' xmlns:gd='http://schemas.google.com/g/2005'>"
				 "<title type='text'>Testing title &amp; escaping</title>"
				 /*"<updated>2009-01-25T14:07:37.880860Z</updated>"
				 "<published>2009-01-23T14:06:37.880860Z</published>"*/
				 "<content type='text'>This is some sample content testing, amongst other things, &lt;markup&gt; &amp; odd characters\342\200\275</content>"
				 "<category term='Film' scheme='http://gdata.youtube.com/schemas/2007/categories.cat' label='Film &amp; Animation'/>"
				 "<category term='example' label='Example stuff'/>"
				 "<category term='test'/>"
				 "<link href='http://test.mn/' title='A treatise on Mongolian test websites &amp; other stuff.' rel='http://www.iana.org/assignments/relation/related' type='text/html' hreflang='mn' length='5010'/>"
				 "<link href='http://example.com/' rel='http://www.iana.org/assignments/relation/alternate'/>"
				 "<link href='http://test.com/' rel='http://www.iana.org/assignments/relation/self' type='application/atom+xml'/>"
				 "<author><name>F. Barr\330\237</name></author>"
				 "<author><name>John Smith</name><email>smith.john@example.com</email></author>"
				 "<author><name>Joe Bloggs</name><uri>http://example.com/</uri><email>joe@example.com</email></author>"
			 "</entry>");

	/* Check again by re-parsing the XML to a GDataEntry */
	entry2 = GDATA_ENTRY (gdata_parsable_new_from_xml (GDATA_TYPE_ENTRY, xml, -1, &error));
	g_assert_no_error (error);
	g_assert (GDATA_IS_ENTRY (entry2));
	g_clear_error (&error);
	g_free (xml);

	g_assert_cmpstr (gdata_entry_get_title (entry), ==, gdata_entry_get_title (entry2));
	g_assert_cmpstr (gdata_entry_get_id (entry), ==, gdata_entry_get_id (entry2)); /* should both be NULL */
	g_assert_cmpstr (gdata_entry_get_content (entry), ==, gdata_entry_get_content (entry2));

	/*gdata_entry_get_updated (entry, &updated);
	gdata_entry_get_updated (entry2, &updated2);
	g_assert_cmpuint (updated.tv_sec, ==, updated2.tv_sec);
	g_assert_cmpuint (updated.tv_usec, ==, updated2.tv_usec);

	gdata_entry_get_published (entry, &published);
	gdata_entry_get_published (entry2, &published2);
	g_assert_cmpuint (published.tv_sec, ==, published2.tv_sec);
	g_assert_cmpuint (published.tv_usec, ==, published2.tv_usec);*/

	/* TODO: Check categories, links and authors */

	g_object_unref (entry);
	g_object_unref (entry2);
}

static void
test_entry_parse_xml (void)
{
	GDataEntry *entry;
	gchar *xml;
	GError *error = NULL;

	/* Create an entry from XML with unhandled elements */
	entry = GDATA_ENTRY (gdata_parsable_new_from_xml (GDATA_TYPE_ENTRY,
		"<entry xmlns='http://www.w3.org/2005/Atom' xmlns:ns='http://example.com/'>"
			"<title type='text'>Testing unhandled XML</title>"
			"<updated>2009-01-25T14:07:37.880860Z</updated>"
			"<published>2009-01-23T14:06:37.880860Z</published>"
			"<content type='text'>Here we test unhandled XML elements.</content>"
			"<foobar>Test!</foobar>"
			"<barfoo shizzle='zing'/>"
			"<ns:barfoo shizzle='zing' fo='shizzle'>How about some characters‽</ns:barfoo>"
		 "</entry>", -1, &error));
	g_assert_no_error (error);
	g_assert (GDATA_IS_ENTRY (entry));
	g_clear_error (&error);

	/* Now check the outputted XML from the entry still has the unhandled elements */
	xml = gdata_parsable_get_xml (GDATA_PARSABLE (entry));
	g_assert_cmpstr (xml, ==,
			 "<entry xmlns='http://www.w3.org/2005/Atom' xmlns:gd='http://schemas.google.com/g/2005' xmlns:ns='http://example.com/'>"
				"<title type='text'>Testing unhandled XML</title>"
				"<updated>2009-01-25T14:07:37.880860Z</updated>"
				"<published>2009-01-23T14:06:37.880860Z</published>"
				"<content type='text'>Here we test unhandled XML elements.</content>"
				"<foobar>Test!</foobar>"
				"<barfoo shizzle=\"zing\"/>"
				"<ns:barfoo shizzle=\"zing\" fo=\"shizzle\">How about some characters‽</ns:barfoo>"
			 "</entry>");
	g_free (xml);
}

static void
test_query_categories (void)
{
	GDataQuery *query;
	gchar *query_uri;

	query = gdata_query_new ("foobar");

	/* AND */
	gdata_query_set_categories (query, "Fritz/Laurie");
	query_uri = gdata_query_get_query_uri (query, "http://example.com");

	g_assert_cmpstr (query_uri, ==, "http://example.com/-/Fritz/Laurie?q=foobar");
	g_free (query_uri);

	/* OR */
	gdata_query_set_categories (query, "Fritz|Laurie");
	query_uri = gdata_query_get_query_uri (query, "http://example.com");

	g_assert_cmpstr (query_uri, ==, "http://example.com/-/Fritz%7CLaurie?q=foobar");
	g_free (query_uri);

	/* Combination */
	gdata_query_set_categories (query, "A|-{urn:google.com}B/-C");
	query_uri = gdata_query_get_query_uri (query, "http://example.com/gdata_test");

	g_assert_cmpstr (query_uri, ==, "http://example.com/gdata_test/-/A%7C-%7Burn%3Agoogle.com%7DB/-C?q=foobar");
	g_free (query_uri);

	/* Same combination without q param */
	gdata_query_set_q (query, NULL);
	query_uri = gdata_query_get_query_uri (query, "http://example.com");

	g_assert_cmpstr (query_uri, ==, "http://example.com/-/A%7C-%7Burn%3Agoogle.com%7DB/-C");
	g_free (query_uri);

	g_object_unref (query);
}

static void
test_color_parsing (void)
{
	GDataColor color;

	/* With hash */
	g_assert (gdata_color_from_hexadecimal ("#F99Ff0", &color) == TRUE);
	g_assert_cmpuint (color.red, ==, 249);
	g_assert_cmpuint (color.green, ==, 159);
	g_assert_cmpuint (color.blue, ==, 240);

	/* Without hash */
	g_assert (gdata_color_from_hexadecimal ("F99Ff0", &color) == TRUE);
	g_assert_cmpuint (color.red, ==, 249);
	g_assert_cmpuint (color.green, ==, 159);
	g_assert_cmpuint (color.blue, ==, 240);

	/* Invalid, but correct length */
	g_assert (gdata_color_from_hexadecimal ("foobar", &color) == FALSE);

	/* Wildly invalid */
	g_assert (gdata_color_from_hexadecimal ("this is not a real colour!", &color) == FALSE);
}

static void
test_color_output (void)
{
	GDataColor color;
	gchar *color_string;

	/* General test */
	g_assert (gdata_color_from_hexadecimal ("#F99Ff0", &color) == TRUE);
	color_string = gdata_color_to_hexadecimal (&color);
	g_assert_cmpstr (color_string, ==, "#f99ff0");
	g_free (color_string);

	/* Boundary tests */
	g_assert (gdata_color_from_hexadecimal ("#ffffff", &color) == TRUE);
	color_string = gdata_color_to_hexadecimal (&color);
	g_assert_cmpstr (color_string, ==, "#ffffff");
	g_free (color_string);

	g_assert (gdata_color_from_hexadecimal ("#000000", &color) == TRUE);
	color_string = gdata_color_to_hexadecimal (&color);
	g_assert_cmpstr (color_string, ==, "#000000");
	g_free (color_string);
}

/*static void
test_media_thumbnail_parse_time (const gchar *locale)
{
	g_test_bug ("584737");

	g_test_message ("Testing gdata_media_thumbnail_parse_time in the \"%s\" locale...", locale);
	g_assert_cmpstr (setlocale (LC_ALL, locale), ==, locale);

	g_assert_cmpint (gdata_media_thumbnail_parse_time ("00:01:42.500"), ==, 102500);
	g_assert_cmpint (gdata_media_thumbnail_parse_time ("00:02:45"), ==, 165000);
	g_assert_cmpint (gdata_media_thumbnail_parse_time ("12:00:15.000"), ==, 43215000);
	g_assert_cmpint (gdata_media_thumbnail_parse_time ("00:00:00"), ==, 0);
	g_assert_cmpint (gdata_media_thumbnail_parse_time ("foobar"), ==, -1);

	setlocale (LC_ALL, "");
}*/

static void
test_atom_author (void)
{
	GDataAuthor *author, *author2;
	gchar *xml;
	GError *error = NULL;

	author = GDATA_AUTHOR (gdata_parsable_new_from_xml (GDATA_TYPE_AUTHOR,
		"<author>"
			"<name>John Smöth</name>"
			"<uri>http://example.com/</uri>"
			"<email>john@example.com</email>"
		"</author>", -1, &error));
	g_assert_no_error (error);
	g_assert (GDATA_IS_AUTHOR (author));
	g_clear_error (&error);

	/* Check the properties */
	g_assert_cmpstr (gdata_author_get_name (author), ==, "John Smöth");
	g_assert_cmpstr (gdata_author_get_uri (author), ==, "http://example.com/");
	g_assert_cmpstr (gdata_author_get_email_address (author), ==, "john@example.com");

	/* Compare it against another identical author */
	author2 = gdata_author_new ("John Smöth", "http://example.com/", "john@example.com");
	g_assert_cmpint (gdata_author_compare (author, author2), ==, 0);
	g_object_unref (author2);

	/* …and a different author */
	author2 = gdata_author_new ("Brian Blessed", NULL, NULL);
	g_assert_cmpint (gdata_author_compare (author, author2), !=, 0);
	g_object_unref (author2);

	/* Check the outputted XML is the same */
	xml = gdata_parsable_get_xml (GDATA_PARSABLE (author));
	g_assert_cmpstr (xml, ==,
			 "<author xmlns='http://www.w3.org/2005/Atom'>"
				"<name>John Smöth</name>"
				"<uri>http://example.com/</uri>"
				"<email>john@example.com</email>"
			 "</author>");
	g_free (xml);
	g_object_unref (author);

	/* Now parse an author with little information available */
	author = GDATA_AUTHOR (gdata_parsable_new_from_xml (GDATA_TYPE_AUTHOR,
		"<author>"
			"<name>James Johnson</name>"
		"</author>", -1, &error));
	g_assert_no_error (error);
	g_assert (GDATA_IS_AUTHOR (author));
	g_clear_error (&error);

	/* Check the properties */
	g_assert_cmpstr (gdata_author_get_name (author), ==, "James Johnson");
	g_assert (gdata_author_get_uri (author) == NULL);
	g_assert (gdata_author_get_email_address (author) == NULL);
	g_object_unref (author);
}

static void
test_atom_category (void)
{
	GDataCategory *category, *category2;
	gchar *xml;
	GError *error = NULL;

	category = GDATA_CATEGORY (gdata_parsable_new_from_xml (GDATA_TYPE_CATEGORY,
		"<category term='jokes' scheme='http://foobar.com#categories' label='Jokes &amp; Trivia'/>", -1, &error));
	g_assert_no_error (error);
	g_assert (GDATA_IS_CATEGORY (category));
	g_clear_error (&error);

	/* Check the properties */
	g_assert_cmpstr (gdata_category_get_term (category), ==, "jokes");
	g_assert_cmpstr (gdata_category_get_scheme (category), ==, "http://foobar.com#categories");
	g_assert_cmpstr (gdata_category_get_label (category), ==, "Jokes & Trivia");

	/* Compare it against another identical category */
	category2 = gdata_category_new ("jokes", "http://foobar.com#categories", "Jokes & Trivia");
	g_assert_cmpint (gdata_category_compare (category, category2), ==, 0);
	g_object_unref (category2);

	/* …and a different category */
	category2 = gdata_category_new ("sports", "http://foobar.com#categories", NULL);
	g_assert_cmpint (gdata_category_compare (category, category2), !=, 0);
	g_object_unref (category2);

	/* Check the outputted XML is the same */
	xml = gdata_parsable_get_xml (GDATA_PARSABLE (category));
	g_assert_cmpstr (xml, ==,
			 "<category xmlns='http://www.w3.org/2005/Atom' "
			 	"term='jokes' scheme='http://foobar.com#categories' label='Jokes &amp; Trivia'/>");
	g_free (xml);
	g_object_unref (category);

	/* Now parse a category with less information available */
	category = GDATA_CATEGORY (gdata_parsable_new_from_xml (GDATA_TYPE_CATEGORY,
		"<category term='sports'/>", -1, &error));
	g_assert_no_error (error);
	g_assert (GDATA_IS_CATEGORY (category));
	g_clear_error (&error);

	/* Check the properties */
	g_assert_cmpstr (gdata_category_get_term (category), ==, "sports");
	g_assert (gdata_category_get_scheme (category) == NULL);
	g_assert (gdata_category_get_label (category) == NULL);
	g_object_unref (category);

	/* Try a category with custom content */
	category = GDATA_CATEGORY (gdata_parsable_new_from_xml (GDATA_TYPE_CATEGORY,
		"<category term='documentary'>"
			"<foobar/>"
			"<shizzle/>"
		"</category>", -1, &error));
	g_assert_no_error (error);
	g_assert (GDATA_IS_CATEGORY (category));
	g_clear_error (&error);

	/* Check the outputted XML contains the unknown XML */
	xml = gdata_parsable_get_xml (GDATA_PARSABLE (category));
	g_assert_cmpstr (xml, ==,
			 "<category xmlns='http://www.w3.org/2005/Atom' term='documentary'>"
			 	"<foobar/>"
			 	"<shizzle/>"
			 "</category>");
	g_free (xml);
	g_object_unref (category);
}

static void
test_atom_generator (void)
{
	GDataGenerator *generator;
	GError *error = NULL;

	generator = GDATA_GENERATOR (gdata_parsable_new_from_xml (GDATA_TYPE_GENERATOR,
		"<generator uri='http://example.com/' version='15'>Bach &amp; Son's Generator</generator>", -1, &error));
	g_assert_no_error (error);
	g_assert (GDATA_IS_GENERATOR (generator));
	g_clear_error (&error);

	/* Check the properties */
	g_assert_cmpstr (gdata_generator_get_name (generator), ==, "Bach & Son's Generator");
	g_assert_cmpstr (gdata_generator_get_uri (generator), ==, "http://example.com/");
	g_assert_cmpstr (gdata_generator_get_version (generator), ==, "15");
	g_object_unref (generator);

	/* Now parse a generator with less information available */
	generator = GDATA_GENERATOR (gdata_parsable_new_from_xml (GDATA_TYPE_GENERATOR,
		"<generator/>", -1, &error));
	g_assert_no_error (error);
	g_assert (GDATA_IS_GENERATOR (generator));
	g_clear_error (&error);

	/* Check the properties */
	g_assert (gdata_generator_get_name (generator) == NULL);
	g_assert (gdata_generator_get_uri (generator) == NULL);
	g_assert (gdata_generator_get_version (generator) == NULL);
	g_object_unref (generator);
}

static void
test_atom_link (void)
{
	GDataLink *link, *link2;
	gchar *xml;
	GError *error = NULL;

	link = GDATA_LINK (gdata_parsable_new_from_xml (GDATA_TYPE_LINK,
		"<link href='http://example.com/' rel='http://test.com#link-type' type='text/plain' hreflang='de' "
			"title='All About Angle Brackets: &lt;, &gt;' length='2000'/>", -1, &error));
	g_assert_no_error (error);
	g_assert (GDATA_IS_LINK (link));
	g_clear_error (&error);

	/* Check the properties */
	g_assert_cmpstr (gdata_link_get_uri (link), ==, "http://example.com/");
	g_assert_cmpstr (gdata_link_get_relation_type (link), ==, "http://test.com#link-type");
	g_assert_cmpstr (gdata_link_get_content_type (link), ==, "text/plain");
	g_assert_cmpstr (gdata_link_get_language (link), ==, "de");
	g_assert_cmpstr (gdata_link_get_title (link), ==, "All About Angle Brackets: <, >");
	g_assert_cmpint (gdata_link_get_length (link), ==, 2000);

	/* Compare it against another identical link */
	link2 = gdata_link_new ("http://example.com/", "http://test.com#link-type");
	g_assert_cmpint (gdata_link_compare (link, link2), ==, 0);
	gdata_link_set_content_type (link2, "text/plain");
	gdata_link_set_language (link2, "de");
	gdata_link_set_title (link2, "All About Angle Brackets: <, >");
	gdata_link_set_length (link2, 2000);
	g_assert_cmpint (gdata_link_compare (link, link2), ==, 0);

	/* Try with a dissimilar link */
	gdata_link_set_uri (link2, "http://gnome.org/");
	g_assert_cmpint (gdata_link_compare (link, link2), !=, 0);
	g_object_unref (link2);

	/* Check the outputted XML is the same */
	xml = gdata_parsable_get_xml (GDATA_PARSABLE (link));
	g_assert_cmpstr (xml, ==,
			 "<link xmlns='http://www.w3.org/2005/Atom' href='http://example.com/' title='All About Angle Brackets: &lt;, &gt;' "
			 	"rel='http://test.com#link-type' type='text/plain' hreflang='de' length='2000'/>");
	g_free (xml);
	g_object_unref (link);

	/* Now parse a link with less information available */
	link = GDATA_LINK (gdata_parsable_new_from_xml (GDATA_TYPE_LINK,
		"<link href='http://shizzle.com'>Test Content<foobar/></link>", -1, &error));
	g_assert_no_error (error);
	g_assert (GDATA_IS_LINK (link));
	g_clear_error (&error);

	/* Check the properties */
	g_assert_cmpstr (gdata_link_get_uri (link), ==, "http://shizzle.com");
	g_assert_cmpstr (gdata_link_get_relation_type (link), ==, "http://www.iana.org/assignments/relation/alternate");
	g_assert (gdata_link_get_content_type (link) == NULL);
	g_assert (gdata_link_get_language (link) == NULL);
	g_assert (gdata_link_get_title (link) == NULL);
	g_assert (gdata_link_get_length (link) == -1);

	/* Check the outputted XML contains the unknown XML */
	xml = gdata_parsable_get_xml (GDATA_PARSABLE (link));
	g_assert_cmpstr (xml, ==,
			 "<link xmlns='http://www.w3.org/2005/Atom' href='http://shizzle.com' rel='http://www.iana.org/assignments/relation/alternate'>"
			 	"Test Content<foobar/></link>");
	g_free (xml);
	g_object_unref (link);
}

int
main (int argc, char *argv[])
{
	g_type_init ();
	g_test_init (&argc, &argv, NULL);
	g_test_bug_base ("http://bugzilla.gnome.org/show_bug.cgi?id=");

	g_test_add_func ("/entry/get_xml", test_entry_get_xml);
	g_test_add_func ("/entry/parse_xml", test_entry_parse_xml);
	g_test_add_func ("/query/categories", test_query_categories);
	g_test_add_func ("/color/parsing", test_color_parsing);
	g_test_add_func ("/color/output", test_color_output);
	g_test_add_func ("/atom/author", test_atom_author);
	g_test_add_func ("/atom/category", test_atom_category);
	g_test_add_func ("/atom/generator", test_atom_generator);
	g_test_add_func ("/atom/link", test_atom_link);
	/*g_test_add_data_func ("/media/thumbnail/parse_time", "", test_media_thumbnail_parse_time);
	g_test_add_data_func ("/media/thumbnail/parse_time", "de_DE", test_media_thumbnail_parse_time);*/

	return g_test_run ();
}
