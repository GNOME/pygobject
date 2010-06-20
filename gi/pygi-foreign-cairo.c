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
#include <pycairo.h>
extern Pycairo_CAPI_t *Pycairo_CAPI;

#include "pygi-foreign.h"
#include "pygi-foreign-cairo.h"

gboolean
cairo_context_to_arg (PyObject       *value,
                      GITypeInfo     *type_info,
                      GITransfer      transfer,
                      GArgument      *arg)
{
    cairo_t *cr;

    g_assert (transfer == GI_TRANSFER_NOTHING);

    cr = PycairoContext_GET (value);
    if (!cr)
        return FALSE;

    arg->v_pointer = cr;
    return TRUE;
}

PyObject *
cairo_context_from_arg (GITypeInfo *type_info, GArgument  *arg)
{
    cairo_t *context = (cairo_t*) arg;

    cairo_reference (context);

    return PycairoContext_FromContext (context, &PycairoContext_Type, NULL);
}

gboolean
cairo_context_release_arg (GITransfer  transfer, GITypeInfo *type_info,
                           GArgument  *arg)
{
    cairo_destroy ( (cairo_t*) arg->v_pointer);
    return TRUE;
}


gboolean
cairo_surface_to_arg (PyObject       *value,
                      GITypeInfo     *type_info,
                      GITransfer      transfer,
                      GArgument      *arg)
{
    cairo_surface_t *surface;

    g_assert (transfer == GI_TRANSFER_NOTHING);

    surface = ( (PycairoSurface*) value)->surface;
    if (!surface)
        return FALSE;

    arg->v_pointer = surface;
    return TRUE;
}

PyObject *
cairo_surface_from_arg (GITypeInfo *type_info, GArgument  *arg)
{
    cairo_surface_t *surface = (cairo_surface_t*) arg;

    cairo_surface_reference (surface);

    return PycairoSurface_FromSurface (surface, NULL);
}

gboolean
cairo_surface_release_arg (GITransfer  transfer, GITypeInfo *type_info,
                           GArgument  *arg)
{
    cairo_surface_destroy ( (cairo_surface_t*) arg->v_pointer);
    return TRUE;
}

