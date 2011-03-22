/*
 * test-floating.h - Header for TestFloatingWithSinkFunc and TestFloatingWithoutSinkFunc
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

/* TestFloatingWithSinkFunc */

typedef struct {
  GInitiallyUnowned parent;
} TestFloatingWithSinkFunc;

typedef struct {
  GInitiallyUnownedClass parent_class;
} TestFloatingWithSinkFuncClass;

#define TEST_TYPE_FLOATING_WITH_SINK_FUNC (test_floating_with_sink_func_get_type())
#define TEST_FLOATING_WITH_SINK_FUNC(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), TEST_TYPE_FLOATING_WITH_SINK_FUNC, TestFloatingWithSinkFunc))
#define TEST_FLOATING_WITH_SINK_FUNC_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), TEST_TYPE_FLOATING_WITH_SINK_FUNC, TestFloatingWithSinkFuncClass))
#define TEST_IS_FLOATING_WITH_SINK_FUNC(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), TEST_TYPE_FLOATING_WITH_SINK_FUNC))
#define TEST_IS_FLOATING_WITH_SINK_FUNC_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((obj), TEST_TYPE_FLOATING_WITH_SINK_FUNC))
#define TEST_FLOATING_WITH_SINK_FUNC_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj), TEST_TYPE_FLOATING_WITH_SINK_FUNC, TestFloatingWithSinkFuncClass))

GType test_floating_with_sink_func_get_type (void);
void sink_test_floating_with_sink_func (GObject *object);

/* TestFloatingWithoutSinkFunc */

typedef struct {
  GInitiallyUnowned parent;
} TestFloatingWithoutSinkFunc;

typedef struct {
  GInitiallyUnownedClass parent_class;
} TestFloatingWithoutSinkFuncClass;

#define TEST_TYPE_FLOATING_WITHOUT_SINK_FUNC (test_floating_without_sink_func_get_type())
#define TEST_FLOATING_WITHOUT_SINK_FUNC(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), TEST_TYPE_FLOATING_WITHOUT_SINK_FUNC, TestFloatingWithoutSinkFunc))
#define TEST_FLOATING_WITHOUT_SINK_FUNC_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), TEST_TYPE_FLOATING_WITHOUT_SINK_FUNC, TestFloatingWithoutSinkFuncClass))
#define TEST_IS_FLOATING_WITHOUT_SINK_FUNC(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), TEST_TYPE_FLOATING_WITHOUT_SINK_FUNC))
#define TEST_IS_FLOATING_WITHOUT_SINK_FUNC_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((obj), TEST_TYPE_FLOATING_WITHOUT_SINK_FUNC))
#define TEST_FLOATING_WITHOUT_SINK_FUNC_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj), TEST_TYPE_FLOATING_WITHOUT_SINK_FUNC, TestFloatingWithoutSinkFuncClass))

GType test_floating_without_sink_func_get_type (void);

/* TestOwnedByLibrary */

typedef struct {
  GObject parent;
} TestOwnedByLibrary;

typedef struct {
  GObjectClass parent_class;
} TestOwnedByLibraryClass;

#define TEST_TYPE_OWNED_BY_LIBRARY            (test_owned_by_library_get_type())
#define TEST_OWNED_BY_LIBRARY(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), TEST_TYPE_OWNED_BY_LIBRARY, TestOwnedByLibrary))
#define TEST_OWNED_BY_LIBRARY_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), TEST_TYPE_OWNED_BY_LIBRARY, TestOwnedByLibraryClass))
#define TEST_IS_OWNED_BY_LIBRARY(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), TEST_TYPE_OWNED_BY_LIBRARY))
#define TEST_IS_OWNED_BY_LIBRARY_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((obj), TEST_TYPE_OWNED_BY_LIBRARY))
#define TEST_OWNED_BY_LIBRARY_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj), TEST_TYPE_OWNED_BY_LIBRARY, TestOwnedByLibraryClass))

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

#define TEST_TYPE_FLOATING_AND_SUNK            (test_floating_and_sunk_get_type())
#define TEST_FLOATING_AND_SUNK(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), TEST_TYPE_FLOATING_AND_SUNK, TestFloatingAndSunk))
#define TEST_FLOATING_AND_SUNK_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), TEST_TYPE_FLOATING_AND_SUNK, TestFloatingAndSunkClass))
#define TEST_IS_FLOATING_AND_SUNK(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), TEST_TYPE_FLOATING_AND_SUNK))
#define TEST_IS_FLOATING_AND_SUNK_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((obj), TEST_TYPE_FLOATING_AND_SUNK))
#define TEST_FLOATING_AND_SUNK_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj), TEST_TYPE_FLOATING_AND_SUNK, TestFloatingAndSunkClass))

GType test_floating_and_sunk_get_type (void);
void test_floating_and_sunk_release (TestFloatingAndSunk *self);
GSList *test_floating_and_sunk_get_instance_list (void);
