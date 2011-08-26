/*
 * test-floating.c - Source for TestFloating
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

#include "test-floating.h"

/* TestFloating */

G_DEFINE_TYPE(TestFloating, test_floating, G_TYPE_INITIALLY_UNOWNED)

static void
test_floating_finalize (GObject *gobject)
{
  TestFloating *object = TEST_FLOATING (gobject);

  if (g_object_is_floating (object))
    {
      g_warning ("A floating object was finalized. This means that someone\n"
		 "called g_object_unref() on an object that had only a floating\n"
		 "reference; the initial floating reference is not owned by anyone\n"
		 "and must be removed without g_object_ref_sink().");
    }

  G_OBJECT_CLASS (test_floating_parent_class)->finalize (gobject);
}

static void
test_floating_class_init (TestFloatingClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

  gobject_class->finalize = test_floating_finalize;
}

static void
test_floating_init (TestFloating *self)
{
}

/* TestOwnedByLibrary */

G_DEFINE_TYPE(TestOwnedByLibrary, test_owned_by_library, G_TYPE_OBJECT)

static GSList *obl_instance_list = NULL;

static void
test_owned_by_library_class_init (TestOwnedByLibraryClass *klass)
{
}

static void
test_owned_by_library_init (TestOwnedByLibrary *self)
{
    g_object_ref (self);
    obl_instance_list = g_slist_prepend (obl_instance_list, self);
}

void
test_owned_by_library_release (TestOwnedByLibrary *self)
{
    obl_instance_list = g_slist_remove (obl_instance_list, self);
    g_object_unref (self);
}

GSList *
test_owned_by_library_get_instance_list (void)
{
    return obl_instance_list;
}

/* TestFloatingAndSunk
 * This object is mimicking the GtkWindow behaviour, ie a GInitiallyUnowned subclass
 * whose floating reference has already been sunk when g_object_new() returns it.
 * The reference is already sunk because the instance is already owned by the instance
 * list.
 */

G_DEFINE_TYPE(TestFloatingAndSunk, test_floating_and_sunk, G_TYPE_INITIALLY_UNOWNED)

static GSList *fas_instance_list = NULL;

static void
test_floating_and_sunk_class_init (TestFloatingAndSunkClass *klass)
{
}

static void
test_floating_and_sunk_init (TestFloatingAndSunk *self)
{
    g_object_ref_sink (self);
    fas_instance_list = g_slist_prepend (fas_instance_list, self);
}

void
test_floating_and_sunk_release (TestFloatingAndSunk *self)
{
    fas_instance_list = g_slist_remove (fas_instance_list, self);
    g_object_unref (self);
}

GSList *
test_floating_and_sunk_get_instance_list (void)
{
    return fas_instance_list;
}
