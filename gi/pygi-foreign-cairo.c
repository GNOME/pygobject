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

#include <cairo.h>
#include <Python.h>

#if PY_VERSION_HEX < 0x03000000
#include <pycairo.h>
#else
#include <pycairo/py3cairo.h>
#endif

Pycairo_CAPI_t *Pycairo_CAPI;

#include "pygi-foreign.h"

#include <pyglib-python-compat.h>

PyObject *
cairo_context_to_arg (PyObject       *value,
                      GITypeInfo     *type_info,
                      GITransfer      transfer,
                      GIArgument      *arg)
{
    cairo_t *cr;

    g_assert (transfer == GI_TRANSFER_NOTHING);

    cr = PycairoContext_GET (value);
    if (!cr) {
        return NULL;
    }

    arg->v_pointer = cr;
    Py_RETURN_NONE;
}

PyObject *
cairo_context_from_arg (GITypeInfo *type_info, gpointer data)
{
    cairo_t *context = (cairo_t*) data;

    cairo_reference (context);

    return PycairoContext_FromContext (context, &PycairoContext_Type, NULL);
}

PyObject *
cairo_context_release (GIBaseInfo *base_info,
                       gpointer    struct_)
{
    cairo_destroy ( (cairo_t*) struct_);
    Py_RETURN_NONE;
}


PyObject *
cairo_surface_to_arg (PyObject       *value,
                      GITypeInfo     *type_info,
                      GITransfer      transfer,
                      GIArgument      *arg)
{
    cairo_surface_t *surface;

    g_assert (transfer == GI_TRANSFER_NOTHING);

    surface = ( (PycairoSurface*) value)->surface;
    if (!surface) {
        PyErr_SetString (PyExc_ValueError, "Surface instance wrapping a NULL surface");
        return NULL;
    }

    arg->v_pointer = surface;
    Py_RETURN_NONE;
}

PyObject *
cairo_surface_from_arg (GITypeInfo *type_info, gpointer data)
{
    cairo_surface_t *surface = (cairo_surface_t*) data;

    cairo_surface_reference (surface);

    return PycairoSurface_FromSurface (surface, NULL);
}

PyObject *
cairo_surface_release (GIBaseInfo *base_info,
                       gpointer    struct_)
{
    cairo_surface_destroy ( (cairo_surface_t*) struct_);
    Py_RETURN_NONE;
}

static PyMethodDef _gi_cairo_functions[] = {};
PYGLIB_MODULE_START(_gi_cairo, "_gi_cairo")
{
    Pycairo_IMPORT;
    if (Pycairo_CAPI == NULL)
        return PYGLIB_MODULE_ERROR_RETURN;

    pygi_register_foreign_struct ("cairo",
                                  "Context",
                                  cairo_context_to_arg,
                                  cairo_context_from_arg,
                                  cairo_context_release);

    pygi_register_foreign_struct ("cairo",
                                  "Surface",
                                  cairo_surface_to_arg,
                                  cairo_surface_from_arg,
                                  cairo_surface_release);
}
PYGLIB_MODULE_END;
