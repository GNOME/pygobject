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

typedef struct {
    GInitiallyUnowned parent;
} TestFloating;

typedef struct {
    GInitiallyUnownedClass parent_class;
} TestFloatingClass;

#define TEST_TYPE_FLOATING (test_floating_get_type ())
#define TEST_FLOATING(obj)                                                    \
    (G_TYPE_CHECK_INSTANCE_CAST ((obj), TEST_TYPE_FLOATING, TestFloating))
#define TEST_FLOATING_CLASS(klass)                                            \
    (G_TYPE_CHECK_CLASS_CAST ((klass), TEST_TYPE_FLOATING, TestFloatingClass))
#define TEST_IS_FLOATING(obj)                                                 \
    (G_TYPE_CHECK_INSTANCE_TYPE ((obj), TEST_TYPE_FLOATING))
#define TEST_IS_FLOATING_CLASS(klass)                                         \
    (G_TYPE_CHECK_CLASS_TYPE ((obj), TEST_TYPE_FLOATING))
#define TEST_FLOATING_GET_CLASS(obj)                                          \
    (G_TYPE_INSTANCE_GET_CLASS ((obj), TEST_TYPE_FLOATING, TestFloatingClass))

GType test_floating_get_type (void);

/* TestOwnedByLibrary */

typedef struct {
    GObject parent;
} TestOwnedByLibrary;

typedef struct {
    GObjectClass parent_class;
} TestOwnedByLibraryClass;

#define TEST_TYPE_OWNED_BY_LIBRARY (test_owned_by_library_get_type ())
#define TEST_OWNED_BY_LIBRARY(obj)                                            \
    (G_TYPE_CHECK_INSTANCE_CAST ((obj), TEST_TYPE_OWNED_BY_LIBRARY,           \
                                 TestOwnedByLibrary))
#define TEST_OWNED_BY_LIBRARY_CLASS(klass)                                    \
    (G_TYPE_CHECK_CLASS_CAST ((klass), TEST_TYPE_OWNED_BY_LIBRARY,            \
                              TestOwnedByLibraryClass))
#define TEST_IS_OWNED_BY_LIBRARY(obj)                                         \
    (G_TYPE_CHECK_INSTANCE_TYPE ((obj), TEST_TYPE_OWNED_BY_LIBRARY))
#define TEST_IS_OWNED_BY_LIBRARY_CLASS(klass)                                 \
    (G_TYPE_CHECK_CLASS_TYPE ((obj), TEST_TYPE_OWNED_BY_LIBRARY))
#define TEST_OWNED_BY_LIBRARY_GET_CLASS(obj)                                  \
    (G_TYPE_INSTANCE_GET_CLASS ((obj), TEST_TYPE_OWNED_BY_LIBRARY,            \
                                TestOwnedByLibraryClass))

GType test_owned_by_library_get_type (void);
void test_owned_by_library_release (TestOwnedByLibrary *self);
GSList *test_owned_by_library_get_instance_list (void);

/* TestFloatingAndSunk */

typedef struct {
    GInitiallyUnowned parent;
} TestFloatingAndSunk;

typedef struct {
    GInitiallyUnownedClass parent_class;
} TestFloatingAndSunkClass;

#define TEST_TYPE_FLOATING_AND_SUNK (test_floating_and_sunk_get_type ())
#define TEST_FLOATING_AND_SUNK(obj)                                           \
    (G_TYPE_CHECK_INSTANCE_CAST ((obj), TEST_TYPE_FLOATING_AND_SUNK,          \
                                 TestFloatingAndSunk))
#define TEST_FLOATING_AND_SUNK_CLASS(klass)                                   \
    (G_TYPE_CHECK_CLASS_CAST ((klass), TEST_TYPE_FLOATING_AND_SUNK,           \
                              TestFloatingAndSunkClass))
#define TEST_IS_FLOATING_AND_SUNK(obj)                                        \
    (G_TYPE_CHECK_INSTANCE_TYPE ((obj), TEST_TYPE_FLOATING_AND_SUNK))
#define TEST_IS_FLOATING_AND_SUNK_CLASS(klass)                                \
    (G_TYPE_CHECK_CLASS_TYPE ((obj), TEST_TYPE_FLOATING_AND_SUNK))
#define TEST_FLOATING_AND_SUNK_GET_CLASS(obj)                                 \
    (G_TYPE_INSTANCE_GET_CLASS ((obj), TEST_TYPE_FLOATING_AND_SUNK,           \
                                TestFloatingAndSunkClass))

GType test_floating_and_sunk_get_type (void);
void test_floating_and_sunk_release (TestFloatingAndSunk *self);
GSList *test_floating_and_sunk_get_instance_list (void);
