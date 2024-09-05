/* gimarshallingtestsextra.h
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

#ifndef EXTRA_TESTS
#define EXTRA_TESTS

#include <glib-object.h>
#include <gitestmacros.h>

#ifndef GI_TEST_EXTERN
#define GI_TEST_EXTERN extern
#endif

typedef enum
{
  GI_MARSHALLING_TESTS_EXTRA_ENUM_VALUE1,
  GI_MARSHALLING_TESTS_EXTRA_ENUM_VALUE2,
  GI_MARSHALLING_TESTS_EXTRA_ENUM_VALUE3 = 42
} GIMarshallingTestsExtraEnum;


typedef enum
{
  GI_MARSHALLING_TESTS_EXTRA_FLAGS_VALUE1 = 0,
  GI_MARSHALLING_TESTS_EXTRA_FLAGS_VALUE2 = (gint)(1 << 31),
} GIMarshallingTestsExtraFlags;


GI_TEST_EXTERN
GType gi_marshalling_tests_extra_flags_get_type (void) G_GNUC_CONST;
#define GI_MARSHALLING_TESTS_TYPE_EXTRA_FLAGS (gi_marshalling_tests_extra_flags_get_type ())

GI_TEST_EXTERN
void gi_marshalling_tests_compare_two_gerrors_in_gvalue (GValue *v, GValue *v1);
GI_TEST_EXTERN
gboolean gi_marshalling_tests_nullable_gerror(GError *error);

GI_TEST_EXTERN
void gi_marshalling_tests_ghashtable_enum_none_in (GHashTable *hash_table);
GI_TEST_EXTERN
GHashTable * gi_marshalling_tests_ghashtable_enum_none_return (void);

GI_TEST_EXTERN
gchar * gi_marshalling_tests_filename_copy (gchar *path_in);
GI_TEST_EXTERN
gboolean gi_marshalling_tests_filename_exists (gchar *path);
GI_TEST_EXTERN
gchar * gi_marshalling_tests_filename_to_glib_repr (gchar *path_in, gsize *len);

GI_TEST_EXTERN
GIMarshallingTestsExtraEnum * gi_marshalling_tests_enum_array_return_type (gsize *n_members);

GI_TEST_EXTERN
void gi_marshalling_tests_extra_flags_large_in (GIMarshallingTestsExtraFlags value);

GI_TEST_EXTERN
gchar *gi_marshalling_tests_extra_utf8_full_return_invalid (void);
GI_TEST_EXTERN
void gi_marshalling_tests_extra_utf8_full_out_invalid (gchar **utf8);

#endif /* EXTRA_TESTS */
