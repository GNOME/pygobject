/*
 * test-floating.h - Header for TestFloating
 * Copyright (C) 2010 Collabora Ltd.
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
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include <glib-object.h>

/* TestFloating */

#define TEST_TYPE_FLOATING (test_floating_get_type ())
G_DECLARE_FINAL_TYPE (TestFloating, test_floating, TEST, FLOATING,
                      GInitiallyUnowned);


/* TestOwnedByLibrary */

#define TEST_TYPE_OWNED_BY_LIBRARY (test_owned_by_library_get_type ())
G_DECLARE_FINAL_TYPE (TestOwnedByLibrary, test_owned_by_library, TEST,
                      OWNED_BY_LIBRARY, GObject);

void test_owned_by_library_release (TestOwnedByLibrary *self);
GSList *test_owned_by_library_get_instance_list (void);

/* TestFloatingAndSunk */

#define TEST_TYPE_FLOATING_AND_SUNK (test_floating_and_sunk_get_type ())
G_DECLARE_FINAL_TYPE (TestFloatingAndSunk, test_floating_and_sunk, TEST,
                      FLOATING_AND_SUNK, GInitiallyUnowned);

void test_floating_and_sunk_release (TestFloatingAndSunk *self);
GSList *test_floating_and_sunk_get_instance_list (void);
