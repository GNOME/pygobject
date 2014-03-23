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
static Pycairo_CAPI_t *Pycairo_CAPI;
#else
#include <pycairo/py3cairo.h>
#endif

#include <cairo-gobject.h>

/* Limit includes from PyGI to APIs which do not have link dependencies
 * (pygobject.h and pygi-foreign-api.h) since _gi_cairo is built as a separate
 * shared library that interacts with PyGI through a PyCapsule API at runtime.
 */
#include <pygi-foreign-api.h>
#include <pyglib-python-compat.h>

/*
 * cairo_t marshaling
 */

static PyObject *
cairo_context_to_arg (PyObject        *value,
                      GIInterfaceInfo *interface_info,
                      GITransfer       transfer,
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

static PyObject *
cairo_context_from_arg (GIInterfaceInfo *interface_info,
                        GITransfer       transfer,
                        gpointer         data)
{
    cairo_t *context = (cairo_t*) data;

    if (transfer == GI_TRANSFER_NOTHING)
        cairo_reference (context);

    return PycairoContext_FromContext (context, &PycairoContext_Type, NULL);
}


static PyObject *
cairo_context_release (GIBaseInfo *base_info,
                       gpointer    struct_)
{
    cairo_destroy ( (cairo_t*) struct_);
    Py_RETURN_NONE;
}

static int
cairo_context_to_gvalue (GValue *value, PyObject *obj)
{
    cairo_t *cr = PycairoContext_GET (obj);
    if (!cr) {
        return -1;
    }

    /* PycairoContext_GET returns a borrowed reference, use set_boxed
     * to add new ref to the context which will be managed by the GValue. */
    g_value_set_boxed (value, cr);
    return 0;
}

static PyObject *
cairo_context_from_gvalue (const GValue *value)
{
    /* PycairoContext_FromContext steals a ref, so we dup it out of the GValue. */
    cairo_t *cr = g_value_dup_boxed (value);
    if (!cr) {
        return NULL;
    }

    return PycairoContext_FromContext (cr, &PycairoContext_Type, NULL);
}


/*
 * cairo_surface_t marshaling
 */

static PyObject *
cairo_surface_to_arg (PyObject        *value,
                      GIInterfaceInfo *interface_info,
                      GITransfer       transfer,
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

static PyObject *
cairo_surface_from_arg (GIInterfaceInfo *interface_info,
                        GITransfer       transfer,
                        gpointer         data)
{
    cairo_surface_t *surface = (cairo_surface_t*) data;

    if (transfer == GI_TRANSFER_NOTHING)
        cairo_surface_reference (surface);

    return PycairoSurface_FromSurface (surface, NULL);
}

static PyObject *
cairo_surface_release (GIBaseInfo *base_info,
                       gpointer    struct_)
{
    cairo_surface_destroy ( (cairo_surface_t*) struct_);
    Py_RETURN_NONE;
}

static int
cairo_surface_to_gvalue (GValue *value, PyObject *obj)
{
    cairo_surface_t *surface = ((PycairoSurface*) obj)->surface;
    if (!surface) {
        return -1;
    }

    /* surface is a borrowed reference, use set_boxed
     * to add new ref to the context which will be managed by the GValue. */
    g_value_set_boxed (value, surface);
    return 0;
}

static PyObject *
cairo_surface_from_gvalue (const GValue *value)
{
    /* PycairoSurface_FromSurface steals a ref, so we dup it out of the GValue. */
    cairo_surface_t *surface = g_value_dup_boxed (value);
    if (!surface) {
        return NULL;
    }

    return PycairoSurface_FromSurface (surface, NULL);
}


/*
 * cairo_path_t marshaling
 */

static PyObject *
cairo_path_to_arg (PyObject        *value,
                   GIInterfaceInfo *interface_info,
                   GITransfer       transfer,
                   GIArgument      *arg)
{
    cairo_path_t *path;

    g_assert (transfer == GI_TRANSFER_NOTHING);

    path = ( (PycairoPath*) value)->path;
    if (!path) {
        PyErr_SetString (PyExc_ValueError, "Path instance wrapping a NULL path");
        return NULL;
    }

    arg->v_pointer = path;
    Py_RETURN_NONE;
}

static PyObject *
cairo_path_from_arg (GIInterfaceInfo *interface_info,
                     GITransfer       transfer,
                     gpointer         data)
{
    cairo_path_t *path = (cairo_path_t*) data;

    if (transfer == GI_TRANSFER_NOTHING) {
        PyErr_SetString(PyExc_TypeError, "Unsupported annotation (transfer none) for cairo.Path return");
        return NULL;
    }

    return PycairoPath_FromPath (path);
}

static PyObject *
cairo_path_release (GIBaseInfo *base_info,
                    gpointer    struct_)
{
    cairo_path_destroy ( (cairo_path_t*) struct_);
    Py_RETURN_NONE;
}


/*
 * cairo_font_face_t marshaling
 */

static int
cairo_font_face_to_gvalue (GValue *value, PyObject *obj)
{
    cairo_font_face_t *font_face = ((PycairoFontFace*) obj)->font_face;
    if (!font_face) {
        return -1;
    }

    g_value_set_boxed (value, font_face);
    return 0;
}

static PyObject *
cairo_font_face_from_gvalue (const GValue *value)
{
    cairo_font_face_t *font_face = g_value_dup_boxed (value);
    if (!font_face) {
        return NULL;
    }

    return PycairoFontFace_FromFontFace (font_face);
}


/*
 * cairo_font_options_t marshaling
 */

static PyObject *
cairo_font_options_to_arg (PyObject        *value,
                           GIInterfaceInfo *interface_info,
                           GITransfer       transfer,
                           GIArgument      *arg)
{
    cairo_font_options_t *font_options;

    g_assert (transfer == GI_TRANSFER_NOTHING);

    font_options = ( (PycairoFontOptions*) value)->font_options;
    if (!font_options) {
        PyErr_SetString (PyExc_ValueError, "FontOptions instance wrapping a NULL font_options");
        return NULL;
    }

    arg->v_pointer = font_options;
    Py_RETURN_NONE;
}

static PyObject *
cairo_font_options_from_arg (GIInterfaceInfo *interface_info,
                             GITransfer       transfer,
                             gpointer         data)
{
    cairo_font_options_t *font_options = (cairo_font_options_t*) data;

    if (transfer == GI_TRANSFER_NOTHING)
        font_options = cairo_font_options_copy (font_options);

    return PycairoFontOptions_FromFontOptions (font_options);
}

static PyObject *
cairo_font_options_release (GIBaseInfo *base_info,
                            gpointer    struct_)
{
    cairo_font_options_destroy ( (cairo_font_options_t*) struct_);
    Py_RETURN_NONE;
}


/*
 * scaled_font_t marshaling
 */

static int
cairo_scaled_font_to_gvalue (GValue *value, PyObject *obj)
{
    cairo_scaled_font_t *scaled_font = ((PycairoScaledFont*) obj)->scaled_font;
    if (!scaled_font) {
        return -1;
    }

    /* scaled_font is a borrowed reference, use set_boxed
     * to add new ref to the context which will be managed by the GValue. */
    g_value_set_boxed (value, scaled_font);
    return 0;
}

static PyObject *
cairo_scaled_font_from_gvalue (const GValue *value)
{
    /* PycairoScaledFont_FromScaledFont steals a ref, so we dup it out of the GValue. */
    cairo_scaled_font_t *scaled_font = g_value_dup_boxed (value);
    if (!scaled_font) {
        return NULL;
    }

    return PycairoScaledFont_FromScaledFont (scaled_font);
}


/*
 * cairo_pattern_t marshaling
 */

static int
cairo_pattern_to_gvalue (GValue *value, PyObject *obj)
{
    cairo_pattern_t *pattern = ((PycairoPattern*) obj)->pattern;
    if (!pattern) {
        return -1;
    }

    /* pattern is a borrowed reference, use set_boxed
     * to add new ref to the context which will be managed by the GValue. */
    g_value_set_boxed (value, pattern);
    return 0;
}

static PyObject *
cairo_pattern_from_gvalue (const GValue *value)
{
    /* PycairoPattern_FromPattern steals a ref, so we dup it out of the GValue. */
    cairo_pattern_t *pattern = g_value_dup_boxed (value);
    if (!pattern) {
        return NULL;
    }

    return PycairoPattern_FromPattern (pattern, NULL);
}


static PyMethodDef _gi_cairo_functions[] = { {0,} };
PYGLIB_MODULE_START(_gi_cairo, "_gi_cairo")
{
#if PY_VERSION_HEX < 0x03000000
    Pycairo_IMPORT;
#else
    import_cairo();
#endif

    if (Pycairo_CAPI == NULL)
        return PYGLIB_MODULE_ERROR_RETURN;

    pygobject_init (3, 13, 2);

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

    pygi_register_foreign_struct ("cairo",
                                  "Path",
                                  cairo_path_to_arg,
                                  cairo_path_from_arg,
                                  cairo_path_release);

    pygi_register_foreign_struct ("cairo",
                                  "FontOptions",
                                  cairo_font_options_to_arg,
                                  cairo_font_options_from_arg,
                                  cairo_font_options_release);

    pyg_register_gtype_custom (CAIRO_GOBJECT_TYPE_CONTEXT,
                               cairo_context_from_gvalue,
                               cairo_context_to_gvalue);

    pyg_register_gtype_custom (CAIRO_GOBJECT_TYPE_SURFACE,
                               cairo_surface_from_gvalue,
                               cairo_surface_to_gvalue);

    pyg_register_gtype_custom (CAIRO_GOBJECT_TYPE_FONT_FACE,
                               cairo_font_face_from_gvalue,
                               cairo_font_face_to_gvalue);

    pyg_register_gtype_custom (CAIRO_GOBJECT_TYPE_SCALED_FONT,
                               cairo_scaled_font_from_gvalue,
                               cairo_scaled_font_to_gvalue);

    pyg_register_gtype_custom (CAIRO_GOBJECT_TYPE_PATTERN,
                               cairo_pattern_from_gvalue,
                               cairo_pattern_to_gvalue);

}
PYGLIB_MODULE_END;
