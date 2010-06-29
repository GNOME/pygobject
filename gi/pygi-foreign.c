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

typedef struct {
    const char *namespace;
    const char *name;
    PyGIArgOverrideToGArgumentFunc to_func;
    PyGIArgOverrideFromGArgumentFunc from_func;
    PyGIArgOverrideReleaseGArgumentFunc release_func;
} PyGIForeignStruct;

static GPtrArray *foreign_structs = NULL;

static PyGIForeignStruct *
pygi_struct_foreign_lookup (GITypeInfo *type_info)
{
    gint i;
    PyObject *module;
    gchar *module_name;
    GIBaseInfo *base_info;
    const gchar *namespace;
    const gchar *name;

    base_info = g_type_info_get_interface (type_info);
    if (base_info == NULL) {
        PyErr_Format (PyExc_ValueError, "Couldn't resolve the type of this foreign struct");
        return NULL;
    }

    namespace = g_base_info_get_namespace (base_info);
    name = g_base_info_get_name (base_info);

    module_name = g_strconcat ("gi._gi_", g_base_info_get_namespace (base_info), NULL);
    module = PyImport_ImportModule (module_name);
    g_free (module_name);

    if (foreign_structs != NULL) {
        for (i = 0; i < foreign_structs->len; i++) {
            PyGIForeignStruct *foreign_struct = \
                    g_ptr_array_index (foreign_structs, i);

            if ( (strcmp (namespace, foreign_struct->namespace) == 0) &&
                    (strcmp (name, foreign_struct->name) == 0)) {
                g_base_info_unref (base_info);
                return foreign_struct;
            }
        }
    }

    g_base_info_unref (base_info);

    PyErr_Format (PyExc_TypeError, "Couldn't find conversion for foreign struct '%s.%s'", namespace, name);
    return NULL;
}

PyObject *
pygi_struct_foreign_convert_to_g_argument (PyObject      *value,
                                           GITypeInfo     *type_info,
                                           GITransfer      transfer,
                                           GArgument      *arg)
{
    PyGIForeignStruct *foreign_struct = pygi_struct_foreign_lookup (type_info);

    if (foreign_struct == NULL)
        return NULL;

    if (!foreign_struct->to_func (value, type_info, transfer, arg))
        return NULL;

    Py_RETURN_NONE;
}

PyObject *
pygi_struct_foreign_convert_from_g_argument (GITypeInfo *type_info,
                                             GArgument  *arg)
{
    PyGIForeignStruct *foreign_struct = pygi_struct_foreign_lookup (type_info);

    if (foreign_struct == NULL)
        return NULL;

    return foreign_struct->from_func (type_info, arg);
}

PyObject *
pygi_struct_foreign_release_g_argument (GITransfer  transfer,
                                        GITypeInfo *type_info,
                                        GArgument  *arg)
{
    PyGIForeignStruct *foreign_struct = pygi_struct_foreign_lookup (type_info);

    if (foreign_struct == NULL)
        return NULL;

    if (!foreign_struct->release_func)
        Py_RETURN_NONE;

    if (!foreign_struct->release_func (transfer, type_info, arg))
        return NULL;

    Py_RETURN_NONE;
}

void
pygi_register_foreign_struct_real (const char* namespace_,
                                   const char* name,
                                   PyGIArgOverrideToGArgumentFunc to_func,
                                   PyGIArgOverrideFromGArgumentFunc from_func,
                                   PyGIArgOverrideReleaseGArgumentFunc release_func)
{
    PyGIForeignStruct *new_struct = g_slice_new0 (PyGIForeignStruct);
    new_struct->namespace = namespace_;
    new_struct->name = name;
    new_struct->to_func = to_func;
    new_struct->from_func = from_func;
    new_struct->release_func = release_func;

    if (foreign_structs == NULL)
        foreign_structs = g_ptr_array_new ();

    g_ptr_array_add (foreign_structs, new_struct);
}
