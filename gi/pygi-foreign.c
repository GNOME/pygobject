/* -*- mode: C; c-basic-offset: 4; indent-tabs-mode: nil; -*- */
/*
 * Copyright (c) 2010  litl, LLC
 * Copyright (c) 2010  Collabora Ltd. <http://www.collabora.co.uk/>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */

#include "pygi-foreign.h"

#include <config.h>
#include <girepository.h>

#include "pygi-foreign-cairo.h"

static struct {
    char *namespace;
    char *name;
    PyGIArgOverrideToGArgumentFunc to_func;
    PyGIArgOverrideFromGArgumentFunc from_func;
    PyGIArgOverrideReleaseGArgumentFunc release_func;
} foreign_structs[] = {
    {   "cairo", "Context", cairo_context_to_arg, cairo_context_from_arg,
        cairo_context_release_arg
    },
    {   "cairo", "Surface", cairo_surface_to_arg, cairo_surface_from_arg,
        cairo_surface_release_arg
    },
    { NULL }
};

static gint
pygi_struct_foreign_lookup (GITypeInfo *type_info)
{
    GIBaseInfo *base_info;

    base_info = g_type_info_get_interface (type_info);
    if (base_info) {
        gint i;
        const gchar *namespace = g_base_info_get_namespace (base_info);
        const gchar *name = g_base_info_get_name (base_info);

        for (i = 0; foreign_structs[i].namespace; ++i) {

            if ( (strcmp (namespace, foreign_structs[i].namespace) == 0) &&
                    (strcmp (name, foreign_structs[i].name) == 0)) {
                g_base_info_unref (base_info);
                return i;
            }
        }

        PyErr_Format (PyExc_TypeError, "Couldn't find type %s.%s", namespace,
                      name);

        g_base_info_unref (base_info);
    }
    return -1;
}

gboolean
pygi_struct_foreign_convert_to_g_argument (PyObject      *value,
                                           GITypeInfo     *type_info,
                                           GITransfer      transfer,
                                           GArgument      *arg)
{
    gint struct_index;

    struct_index = pygi_struct_foreign_lookup (type_info);
    if (struct_index < 0)
        return FALSE;

    if (!foreign_structs[struct_index].to_func (value, type_info, transfer, arg))
        return FALSE;

    return TRUE;
}

PyObject *
pygi_struct_foreign_convert_from_g_argument (GITypeInfo *type_info,
                                             GArgument  *arg)
{
    gint struct_index;

    struct_index = pygi_struct_foreign_lookup (type_info);
    if (struct_index < 0)
        return NULL;

    return foreign_structs[struct_index].from_func (type_info, arg);
}

gboolean
pygi_struct_foreign_release_g_argument (GITransfer  transfer,
                                        GITypeInfo *type_info,
                                        GArgument  *arg)
{
    gint struct_index;

    struct_index = pygi_struct_foreign_lookup (type_info);
    if (struct_index < 0)
        return FALSE;

    if (!foreign_structs[struct_index].release_func)
        return TRUE;

    if (!foreign_structs[struct_index].release_func (transfer, type_info, arg))
        return FALSE;

    return TRUE;
}
