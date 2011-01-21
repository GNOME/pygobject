/* -*- mode: C; c-basic-offset: 4; indent-tabs-mode: nil; -*- */
/*
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

#include "pygobject.h"

#include "pygi-private.h"
#include "pygi-foreign-gvariant.h"

PyObject *
g_variant_to_arg (PyObject       *value,
                  GITypeInfo     *type_info,
                  GITransfer      transfer,
                  GIArgument      *arg)
{
    g_assert (transfer == GI_TRANSFER_NOTHING);

    /* TODO check that value is a PyGPointer */

    arg->v_pointer = (GVariant *) ( (PyGPointer *) value)->pointer;
    Py_RETURN_NONE;
}

PyObject *
g_variant_from_arg (GITypeInfo *type_info,
                    gpointer    data)
{
    GVariant *variant = (GVariant *) data;
    GITypeInfo *interface_info = g_type_info_get_interface (type_info);
    PyObject *type = _pygi_type_import_by_gi_info (interface_info);

    g_variant_ref_sink (variant);

    return _pygi_struct_new ( (PyTypeObject *) type, variant, FALSE);
}

PyObject *
g_variant_release_foreign (GIBaseInfo *base_info,
                           gpointer struct_)
{
    g_variant_unref ( (GVariant *) struct_);
    Py_RETURN_NONE;
}

