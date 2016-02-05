/* gimarshallingtestsextra.c
 *
 * Copyright (C) 2016 Thibault Saunier <tsaunier@gnome.org>
 *
 * This file is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation; either version 3 of the
 * License, or (at your option) any later version.
 *
 * This file is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "gimarshallingtestsextra.h"

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
