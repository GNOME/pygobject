/* gimarshallingtestsextra.c
 *
 * Copyright (C) 2016 Thibault Saunier <tsaunier@gnome.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, see <http://www.gnu.org/licenses/>.
 */

#include "gimarshallingtestsextra.h"
#include <string.h>

void
gi_marshalling_tests_compare_two_gerrors_in_gvalue (GValue *v, GValue *v1)
{
  GError *error, * error1;

  g_assert_cmpstr (g_type_name (G_VALUE_TYPE (v)), ==,
                   g_type_name (G_TYPE_ERROR));
  g_assert_cmpstr (g_type_name (G_VALUE_TYPE (v1)), ==,
                   g_type_name (G_TYPE_ERROR));

  error = (GError*) g_value_get_boxed (v);
  error1 = (GError*) g_value_get_boxed (v1);

  g_assert_cmpint (error->domain, ==, error1->domain);
  g_assert_cmpint (error->code, ==, error1->code);
  g_assert_cmpstr (error->message, ==, error1->message);
}

/**
 * gi_marshalling_tests_ghashtable_enum_none_in:
 * @hash_table: (element-type gint GIMarshallingTestsExtraEnum) (transfer none):
 */
void
gi_marshalling_tests_ghashtable_enum_none_in (GHashTable *hash_table)
{
  g_assert_cmpint (GPOINTER_TO_INT (g_hash_table_lookup (hash_table, GINT_TO_POINTER (1))), ==, GI_MARSHALLING_TESTS_EXTRA_ENUM_VALUE1);
  g_assert_cmpint (GPOINTER_TO_INT (g_hash_table_lookup (hash_table, GINT_TO_POINTER (2))), ==, GI_MARSHALLING_TESTS_EXTRA_ENUM_VALUE2);
  g_assert_cmpint (GPOINTER_TO_INT (g_hash_table_lookup (hash_table, GINT_TO_POINTER (3))), ==, GI_MARSHALLING_TESTS_EXTRA_ENUM_VALUE3);
}

/**
 * gi_marshalling_tests_ghashtable_enum_none_return:
 *
 * Returns: (element-type gint GIMarshallingTestsExtraEnum) (transfer none):
 */
GHashTable *
gi_marshalling_tests_ghashtable_enum_none_return (void)
{
  static GHashTable *hash_table = NULL;

  if (hash_table == NULL)
    {
      hash_table = g_hash_table_new (NULL, NULL);
      g_hash_table_insert (hash_table, GINT_TO_POINTER (1), GINT_TO_POINTER (GI_MARSHALLING_TESTS_EXTRA_ENUM_VALUE1));
      g_hash_table_insert (hash_table, GINT_TO_POINTER (2), GINT_TO_POINTER (GI_MARSHALLING_TESTS_EXTRA_ENUM_VALUE2));
      g_hash_table_insert (hash_table, GINT_TO_POINTER (3), GINT_TO_POINTER (GI_MARSHALLING_TESTS_EXTRA_ENUM_VALUE3));
    }

  return hash_table;
}

/**
 * gi_marshalling_tests_filename_copy:
 * @path_in: (type filename) (nullable)
 *
 * Returns: (type filename) (nullable)
 */
gchar *
gi_marshalling_tests_filename_copy (gchar *path_in)
{
  return g_strdup (path_in);
}

/**
 * gi_marshalling_tests_filename_to_glib_repr:
 * @path_in: (type filename) (nullable)
 *
 * Returns: (array length=len) (element-type guint8)
 */
gchar *
gi_marshalling_tests_filename_to_glib_repr (gchar *path_in, gsize *len)
{
  *len = strlen(path_in);
  return g_strdup (path_in);
}

/**
 * gi_marshalling_tests_filename_exists:
 * @path: (type filename)
 */
gboolean
gi_marshalling_tests_filename_exists (gchar *path)
{
  return g_file_test (path, G_FILE_TEST_EXISTS);
}


/**
 * gi_marshalling_tests_enum_array_return_type:
 * @n_members: (out): The number of members
 *
 * Returns: (array length=n_members) (transfer full): An array of enum values
 */
GIMarshallingTestsExtraEnum *
gi_marshalling_tests_enum_array_return_type (gsize *n_members)
{
  GIMarshallingTestsExtraEnum *res = g_new0(GIMarshallingTestsExtraEnum, 3);

  *n_members = 3;

  res[0] = GI_MARSHALLING_TESTS_EXTRA_ENUM_VALUE1;
  res[1] = GI_MARSHALLING_TESTS_EXTRA_ENUM_VALUE2;
  res[2] = GI_MARSHALLING_TESTS_EXTRA_ENUM_VALUE3;

  return res;
}

GType
gi_marshalling_tests_extra_flags_get_type (void)
{
  static GType type = 0;
  if (G_UNLIKELY (type == 0))
    {
      static const GFlagsValue values[] = {
        {GI_MARSHALLING_TESTS_EXTRA_FLAGS_VALUE1,
         "GI_MARSHALLING_TESTS_EXTRA_FLAGS_VALUE1", "value1"},
        {GI_MARSHALLING_TESTS_EXTRA_FLAGS_VALUE2,
         "GI_MARSHALLING_TESTS_EXTRA_FLAGS_VALUE2", "value2"},
        {0, NULL, NULL}
      };
      type = g_flags_register_static (
        g_intern_static_string ("GIMarshallingTestsExtraFlags"), values);
    }

  return type;
}

/**
 * gi_marshalling_tests_extra_flags_large_in:
 */
void
gi_marshalling_tests_extra_flags_large_in (GIMarshallingTestsExtraFlags value)
{
  g_assert_cmpint (value, ==, GI_MARSHALLING_TESTS_EXTRA_FLAGS_VALUE2);
}


/**
 * gi_marshalling_tests_extra_utf8_full_return_invalid:
 */
gchar *
gi_marshalling_tests_extra_utf8_full_return_invalid (void)
{
  return g_strdup ("invalid utf8 \xff\xfe");
}


/**
 * gi_marshalling_tests_extra_utf8_full_out_invalid:
 * @utf8: (out) (transfer full):
 */
void
gi_marshalling_tests_extra_utf8_full_out_invalid (gchar **utf8)
{
  *utf8 = g_strdup ("invalid utf8 \xff\xfe");
}
