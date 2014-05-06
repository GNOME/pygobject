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

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include "pygobject-private.h"
#include "pygi-foreign.h"

#include <girepository.h>

typedef struct {
    const char *namespace;
    const char *name;
    PyGIArgOverrideToGIArgumentFunc to_func;
    PyGIArgOverrideFromGIArgumentFunc from_func;
    PyGIArgOverrideReleaseFunc release_func;
} PyGIForeignStruct;

static GPtrArray *foreign_structs = NULL;

static void
init_foreign_structs (void)
{
    foreign_structs = g_ptr_array_new ();
}

static PyGIForeignStruct *
do_lookup (const gchar *namespace, const gchar *name)
{
    gint i;
    for (i = 0; i < foreign_structs->len; i++) {
        PyGIForeignStruct *foreign_struct = \
                g_ptr_array_index (foreign_structs, i);

        if ( (strcmp (namespace, foreign_struct->namespace) == 0) &&
                (strcmp (name, foreign_struct->name) == 0)) {
            return foreign_struct;
        }
    }
    return NULL;
}

static PyObject *
pygi_struct_foreign_load_module (const char *namespace)
{
    gchar *module_name = g_strconcat ("gi._gi_", namespace, NULL);
    PyObject *module = PyImport_ImportModule (module_name);
    g_free (module_name);
    return module;
}

static PyGIForeignStruct *
pygi_struct_foreign_lookup_by_name (const char *namespace, const char *name)
{
    PyGIForeignStruct *result;

    result = do_lookup (namespace, name);

    if (result == NULL) {
        PyObject *module = pygi_struct_foreign_load_module (namespace);

        if (module == NULL)
            PyErr_Clear ();
        else {
            Py_DECREF (module);
            result = do_lookup (namespace, name);
        }
    }

    if (result == NULL) {
        PyErr_Format (PyExc_TypeError,
                      "Couldn't find foreign struct converter for '%s.%s'",
                      namespace,
                      name);
    }

    return result;
}

static PyGIForeignStruct *
pygi_struct_foreign_lookup (GIBaseInfo *base_info)
{
    const gchar *namespace = g_base_info_get_namespace (base_info);
    const gchar *name = g_base_info_get_name (base_info);

    return pygi_struct_foreign_lookup_by_name (namespace, name);
}

PyObject *
pygi_struct_foreign_convert_to_g_argument (PyObject        *value,
                                           GIInterfaceInfo *interface_info,
                                           GITransfer       transfer,
                                           GIArgument      *arg)
{
    PyObject *result;

    GIBaseInfo *base_info = (GIBaseInfo *) interface_info;
    PyGIForeignStruct *foreign_struct = pygi_struct_foreign_lookup (base_info);

    if (foreign_struct == NULL) {
        PyErr_Format(PyExc_KeyError, "could not find foreign type %s",
                     g_base_info_get_name (base_info));
        return FALSE;
    }

    result = foreign_struct->to_func (value, interface_info, transfer, arg);
    return result;
}

PyObject *
pygi_struct_foreign_convert_from_g_argument (GIInterfaceInfo *interface_info,
                                             GITransfer       transfer,
                                             GIArgument      *arg)
{
    GIBaseInfo *base_info = (GIBaseInfo *) interface_info;
    PyGIForeignStruct *foreign_struct = pygi_struct_foreign_lookup (base_info);

    if (foreign_struct == NULL)
        return NULL;

    return foreign_struct->from_func (interface_info, transfer, arg);
}

PyObject *
pygi_struct_foreign_release (GIBaseInfo *base_info,
                             gpointer    struct_)
{
    PyGIForeignStruct *foreign_struct = pygi_struct_foreign_lookup (base_info);

    if (foreign_struct == NULL)
        return NULL;

    if (!foreign_struct->release_func)
        Py_RETURN_NONE;

    return foreign_struct->release_func (base_info, struct_);
}

void
pygi_register_foreign_struct (const char* namespace_,
                              const char* name,
                              PyGIArgOverrideToGIArgumentFunc to_func,
                              PyGIArgOverrideFromGIArgumentFunc from_func,
                              PyGIArgOverrideReleaseFunc release_func)
{
    PyGIForeignStruct *new_struct = g_slice_new (PyGIForeignStruct);
    new_struct->namespace = namespace_;
    new_struct->name = name;
    new_struct->to_func = to_func;
    new_struct->from_func = from_func;
    new_struct->release_func = release_func;

    g_ptr_array_add (foreign_structs, new_struct);
}

PyObject *
pygi_require_foreign (PyObject *self, PyObject *args, PyObject *kwargs)
{
    static char *kwlist[] = { "namespace", "symbol", NULL };
    const char *namespace = NULL;
    const char *symbol = NULL;

    if (!PyArg_ParseTupleAndKeywords (args, kwargs,
                                      "s|z:require_foreign",
                                      kwlist, &namespace, &symbol)) {
        return NULL;
    }

    if (symbol) {
        PyGIForeignStruct *foreign;
        foreign = pygi_struct_foreign_lookup_by_name (namespace, symbol);
        if (foreign == NULL) {
            return NULL;
        }
    } else {
        PyObject *module = pygi_struct_foreign_load_module (namespace);
        if (module) {
            Py_DECREF (module);
        } else {
            return NULL;
        }
    }

    Py_RETURN_NONE;
}

void
pygi_foreign_init (void)
{
    if (foreign_structs == NULL) {
        init_foreign_structs ();
    }
}
