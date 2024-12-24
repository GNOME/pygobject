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

#include <cairo-gobject.h>
#include <cairo.h>
#include <py3cairo.h>
#include <pythoncapi_compat.h>

/* Limit includes from PyGI to APIs which do not have link dependencies
 * (pygobject.h and pygi-foreign-api.h) since _gi_cairo is built as a separate
 * shared library that interacts with PyGI through a PyCapsule API at runtime.
 */
#include <pygi-foreign-api.h>

static int _gi_cairo_exec (PyObject *module);

/*
 * cairo_t marshaling
 */

static PyObject *
cairo_context_to_arg (PyObject *value, GIRegisteredTypeInfo *interface_info,
                      GITransfer transfer, GIArgument *arg)
{
    cairo_t *cr;

    if (!PyObject_TypeCheck (value, &PycairoContext_Type)) {
        PyErr_SetString (PyExc_TypeError, "Expected cairo.Context");
        return NULL;
    }

    cr = PycairoContext_GET (value);
    if (!cr) {
        return NULL;
    }

    if (transfer != GI_TRANSFER_NOTHING) cr = cairo_reference (cr);

    arg->v_pointer = cr;
    Py_RETURN_NONE;
}

static PyObject *
cairo_context_from_arg (GIRegisteredTypeInfo *interface_info,
                        GITransfer transfer, gpointer data)
{
    cairo_t *context = (cairo_t *)data;

    if (transfer == GI_TRANSFER_NOTHING) cairo_reference (context);

    return PycairoContext_FromContext (context, &PycairoContext_Type, NULL);
}


static PyObject *
cairo_context_release (GIBaseInfo *base_info, gpointer struct_)
{
    cairo_destroy ((cairo_t *)struct_);
    Py_RETURN_NONE;
}

static int
cairo_context_to_gvalue (GValue *value, PyObject *obj)
{
    cairo_t *cr;

    if (!PyObject_TypeCheck (obj, &PycairoContext_Type)) {
        PyErr_SetString (PyExc_TypeError, "Expected cairo.Context");
        return -1;
    }

    cr = PycairoContext_GET (obj);
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
        Py_RETURN_NONE;
    }

    return PycairoContext_FromContext (cr, &PycairoContext_Type, NULL);
}


/*
 * cairo_surface_t marshaling
 */

static PyObject *
cairo_surface_to_arg (PyObject *value, GIRegisteredTypeInfo *interface_info,
                      GITransfer transfer, GIArgument *arg)
{
    cairo_surface_t *surface;

    if (!PyObject_TypeCheck (value, &PycairoSurface_Type)) {
        PyErr_SetString (PyExc_TypeError, "Expected cairo.Surface");
        return NULL;
    }

    surface = ((PycairoSurface *)value)->surface;
    if (!surface) {
        PyErr_SetString (PyExc_ValueError,
                         "Surface instance wrapping a NULL surface");
        return NULL;
    }

    if (transfer != GI_TRANSFER_NOTHING)
        surface = cairo_surface_reference (surface);

    arg->v_pointer = surface;
    Py_RETURN_NONE;
}

static PyObject *
cairo_surface_from_arg (GIRegisteredTypeInfo *interface_info,
                        GITransfer transfer, gpointer data)
{
    cairo_surface_t *surface = (cairo_surface_t *)data;

    if (transfer == GI_TRANSFER_NOTHING) cairo_surface_reference (surface);

    return PycairoSurface_FromSurface (surface, NULL);
}

static PyObject *
cairo_surface_release (GIBaseInfo *base_info, gpointer struct_)
{
    cairo_surface_destroy ((cairo_surface_t *)struct_);
    Py_RETURN_NONE;
}

static int
cairo_surface_to_gvalue (GValue *value, PyObject *obj)
{
    cairo_surface_t *surface;

    if (!PyObject_TypeCheck (obj, &PycairoSurface_Type)) {
        PyErr_SetString (PyExc_TypeError, "Expected cairo.Surface");
        return -1;
    }

    surface = ((PycairoSurface *)obj)->surface;
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
        Py_RETURN_NONE;
    }

    return PycairoSurface_FromSurface (surface, NULL);
}


/*
 * cairo_path_t marshaling
 */

static cairo_path_t *
_cairo_path_copy (cairo_path_t *path)
{
    cairo_t *cr;
    cairo_surface_t *surface;
    cairo_path_t *copy;

    surface = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, 0, 0);
    cr = cairo_create (surface);
    cairo_append_path (cr, path);
    copy = cairo_copy_path (cr);
    cairo_destroy (cr);
    cairo_surface_destroy (surface);

    return copy;
}

static PyObject *
cairo_path_to_arg (PyObject *value, GIRegisteredTypeInfo *interface_info,
                   GITransfer transfer, GIArgument *arg)
{
    cairo_path_t *path;

    if (!PyObject_TypeCheck (value, &PycairoPath_Type)) {
        PyErr_SetString (PyExc_TypeError, "Expected cairo.Path");
        return NULL;
    }

    path = ((PycairoPath *)value)->path;
    if (!path) {
        PyErr_SetString (PyExc_ValueError,
                         "Path instance wrapping a NULL path");
        return NULL;
    }

    if (transfer != GI_TRANSFER_NOTHING) path = _cairo_path_copy (path);

    arg->v_pointer = path;
    Py_RETURN_NONE;
}

static PyObject *
cairo_path_from_arg (GIRegisteredTypeInfo *interface_info, GITransfer transfer,
                     gpointer data)
{
    cairo_path_t *path = (cairo_path_t *)data;

    if (transfer == GI_TRANSFER_NOTHING) {
        PyErr_SetString (
            PyExc_TypeError,
            "Unsupported annotation (transfer none) for cairo.Path return");
        return NULL;
    }

    return PycairoPath_FromPath (path);
}

static PyObject *
cairo_path_release (GIBaseInfo *base_info, gpointer struct_)
{
    cairo_path_destroy ((cairo_path_t *)struct_);
    Py_RETURN_NONE;
}


/*
 * cairo_font_face_t marshaling
 */

static PyObject *
cairo_font_face_to_arg (PyObject *value, GIRegisteredTypeInfo *interface_info,
                        GITransfer transfer, GIArgument *arg)
{
    cairo_font_face_t *font_face;

    if (!PyObject_TypeCheck (value, &PycairoFontFace_Type)) {
        PyErr_SetString (PyExc_TypeError, "Expected cairo.FontFace");
        return NULL;
    }

    font_face = ((PycairoFontFace *)value)->font_face;
    if (!font_face) {
        PyErr_SetString (PyExc_ValueError,
                         "FontFace instance wrapping a NULL font_face");
        return NULL;
    }

    if (transfer != GI_TRANSFER_NOTHING)
        font_face = cairo_font_face_reference (font_face);

    arg->v_pointer = font_face;
    Py_RETURN_NONE;
}

static PyObject *
cairo_font_face_from_arg (GIRegisteredTypeInfo *interface_info,
                          GITransfer transfer, gpointer data)
{
    cairo_font_face_t *font_face = (cairo_font_face_t *)data;

    if (transfer == GI_TRANSFER_NOTHING) cairo_font_face_reference (font_face);

    return PycairoFontFace_FromFontFace (font_face);
}

static PyObject *
cairo_font_face_release (GIBaseInfo *base_info, gpointer struct_)
{
    cairo_font_face_destroy ((cairo_font_face_t *)struct_);
    Py_RETURN_NONE;
}

static int
cairo_font_face_to_gvalue (GValue *value, PyObject *obj)
{
    cairo_font_face_t *font_face;

    if (!PyObject_TypeCheck (obj, &PycairoFontFace_Type)) {
        PyErr_SetString (PyExc_TypeError, "Expected cairo.FontFace");
        return -1;
    }

    font_face = ((PycairoFontFace *)obj)->font_face;
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
        Py_RETURN_NONE;
    }

    return PycairoFontFace_FromFontFace (font_face);
}


/*
 * cairo_font_options_t marshaling
 */

static PyObject *
cairo_font_options_to_arg (PyObject *value,
                           GIRegisteredTypeInfo *interface_info,
                           GITransfer transfer, GIArgument *arg)
{
    cairo_font_options_t *font_options;

    if (!PyObject_TypeCheck (value, &PycairoFontOptions_Type)) {
        PyErr_SetString (PyExc_TypeError, "Expected cairo.FontOptions");
        return NULL;
    }

    font_options = ((PycairoFontOptions *)value)->font_options;
    if (!font_options) {
        PyErr_SetString (PyExc_ValueError,
                         "FontOptions instance wrapping a NULL font_options");
        return NULL;
    }

    if (transfer != GI_TRANSFER_NOTHING)
        font_options = cairo_font_options_copy (font_options);

    arg->v_pointer = font_options;
    Py_RETURN_NONE;
}

static PyObject *
cairo_font_options_from_arg (GIRegisteredTypeInfo *interface_info,
                             GITransfer transfer, gpointer data)
{
    cairo_font_options_t *font_options = (cairo_font_options_t *)data;

    if (transfer == GI_TRANSFER_NOTHING)
        font_options = cairo_font_options_copy (font_options);

    return PycairoFontOptions_FromFontOptions (font_options);
}

static PyObject *
cairo_font_options_release (GIBaseInfo *base_info, gpointer struct_)
{
    cairo_font_options_destroy ((cairo_font_options_t *)struct_);
    Py_RETURN_NONE;
}


/*
 * scaled_font_t marshaling
 */

static PyObject *
cairo_scaled_font_to_arg (PyObject *value,
                          GIRegisteredTypeInfo *interface_info,
                          GITransfer transfer, GIArgument *arg)
{
    cairo_scaled_font_t *scaled_font;

    if (!PyObject_TypeCheck (value, &PycairoScaledFont_Type)) {
        PyErr_SetString (PyExc_TypeError, "Expected cairo.ScaledFont");
        return NULL;
    }

    scaled_font = ((PycairoScaledFont *)value)->scaled_font;
    if (!scaled_font) {
        PyErr_SetString (PyExc_ValueError,
                         "ScaledFont instance wrapping a NULL scaled_font");
        return NULL;
    }

    if (transfer != GI_TRANSFER_NOTHING)
        scaled_font = cairo_scaled_font_reference (scaled_font);

    arg->v_pointer = scaled_font;
    Py_RETURN_NONE;
}

static PyObject *
cairo_scaled_font_from_arg (GIRegisteredTypeInfo *interface_info,
                            GITransfer transfer, gpointer data)
{
    cairo_scaled_font_t *scaled_font = (cairo_scaled_font_t *)data;

    if (transfer == GI_TRANSFER_NOTHING)
        cairo_scaled_font_reference (scaled_font);

    return PycairoScaledFont_FromScaledFont (scaled_font);
}

static PyObject *
cairo_scaled_font_release (GIBaseInfo *base_info, gpointer struct_)
{
    cairo_scaled_font_destroy ((cairo_scaled_font_t *)struct_);
    Py_RETURN_NONE;
}

static int
cairo_scaled_font_to_gvalue (GValue *value, PyObject *obj)
{
    cairo_scaled_font_t *scaled_font;

    if (!PyObject_TypeCheck (obj, &PycairoScaledFont_Type)) {
        PyErr_SetString (PyExc_TypeError, "Expected cairo.ScaledFont");
        return -1;
    }

    scaled_font = ((PycairoScaledFont *)obj)->scaled_font;
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
        Py_RETURN_NONE;
    }

    return PycairoScaledFont_FromScaledFont (scaled_font);
}


/*
 * cairo_pattern_t marshaling
 */

static PyObject *
cairo_pattern_to_arg (PyObject *value, GIRegisteredTypeInfo *interface_info,
                      GITransfer transfer, GIArgument *arg)
{
    cairo_pattern_t *pattern;

    if (!PyObject_TypeCheck (value, &PycairoPattern_Type)) {
        PyErr_SetString (PyExc_TypeError, "Expected cairo.Pattern");
        return NULL;
    }

    pattern = ((PycairoPattern *)value)->pattern;
    if (!pattern) {
        PyErr_SetString (PyExc_ValueError,
                         "Pattern instance wrapping a NULL pattern");
        return NULL;
    }

    if (transfer != GI_TRANSFER_NOTHING)
        pattern = cairo_pattern_reference (pattern);

    arg->v_pointer = pattern;
    Py_RETURN_NONE;
}

static PyObject *
cairo_pattern_from_arg (GIRegisteredTypeInfo *interface_info,
                        GITransfer transfer, gpointer data)
{
    cairo_pattern_t *pattern = (cairo_pattern_t *)data;

    if (transfer == GI_TRANSFER_NOTHING)
        pattern = cairo_pattern_reference (pattern);

    return PycairoPattern_FromPattern (pattern, NULL);
}

static PyObject *
cairo_pattern_release (GIBaseInfo *base_info, gpointer struct_)
{
    cairo_pattern_destroy ((cairo_pattern_t *)struct_);
    Py_RETURN_NONE;
}

static int
cairo_pattern_to_gvalue (GValue *value, PyObject *obj)
{
    cairo_pattern_t *pattern;

    if (!PyObject_TypeCheck (obj, &PycairoPattern_Type)) {
        PyErr_SetString (PyExc_TypeError, "Expected cairo.Pattern");
        return -1;
    }

    pattern = ((PycairoPattern *)obj)->pattern;
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
        Py_RETURN_NONE;
    }

    return PycairoPattern_FromPattern (pattern, NULL);
}

static PyObject *
cairo_region_to_arg (PyObject *value, GIRegisteredTypeInfo *interface_info,
                     GITransfer transfer, GIArgument *arg)
{
    cairo_region_t *region;

    if (!PyObject_TypeCheck (value, &PycairoRegion_Type)) {
        PyErr_SetString (PyExc_TypeError, "Expected cairo.Region");
        return NULL;
    }

    region = ((PycairoRegion *)value)->region;
    if (!region) {
        PyErr_SetString (PyExc_ValueError,
                         "Region instance wrapping a NULL region");
        return NULL;
    }

    if (transfer != GI_TRANSFER_NOTHING) region = cairo_region_copy (region);

    arg->v_pointer = region;
    Py_RETURN_NONE;
}

static PyObject *
cairo_region_from_arg (GIRegisteredTypeInfo *interface_info,
                       GITransfer transfer, gpointer data)
{
    cairo_region_t *region = (cairo_region_t *)data;

    if (transfer == GI_TRANSFER_NOTHING) cairo_region_reference (region);

    return PycairoRegion_FromRegion (region);
}

static PyObject *
cairo_region_release (GIBaseInfo *base_info, gpointer struct_)
{
    cairo_region_destroy ((cairo_region_t *)struct_);
    Py_RETURN_NONE;
}

static PyObject *
cairo_matrix_from_arg (GIRegisteredTypeInfo *interface_info,
                       GITransfer transfer, gpointer data)
{
    cairo_matrix_t *matrix = (cairo_matrix_t *)data;

    if (transfer != GI_TRANSFER_NOTHING) {
        PyErr_SetString (
            PyExc_TypeError,
            "Unsupported annotation (transfer full) for cairo.Matrix");
        return NULL;
    }

    if (matrix == NULL) {
        /* NULL in case of caller-allocates */
        cairo_matrix_t temp = { 0 };
        return PycairoMatrix_FromMatrix (&temp);
    }

    return PycairoMatrix_FromMatrix (matrix);
}

static PyObject *
cairo_matrix_to_arg (PyObject *value, GIRegisteredTypeInfo *interface_info,
                     GITransfer transfer, GIArgument *arg)
{
    cairo_matrix_t *matrix;

    if (!PyObject_TypeCheck (value, &PycairoMatrix_Type)) {
        PyErr_SetString (PyExc_TypeError, "Expected cairo.Matrix");
        return NULL;
    }

    matrix = &(((PycairoMatrix *)value)->matrix);

    arg->v_pointer = matrix;
    Py_RETURN_NONE;
}

static PyObject *
cairo_matrix_release (GIBaseInfo *base_info, gpointer struct_)
{
    Py_RETURN_NONE;
}

static int
cairo_matrix_to_gvalue (GValue *value, PyObject *obj)
{
    cairo_matrix_t *matrix;

    if (!PyObject_TypeCheck (obj, &PycairoMatrix_Type)) {
        PyErr_SetString (PyExc_TypeError, "Expected cairo.Matrix");
        return -1;
    }

    matrix = &(((PycairoMatrix *)obj)->matrix);
    if (!matrix) {
        return -1;
    }

    g_value_set_boxed (value, matrix);
    return 0;
}

static PyObject *
cairo_matrix_from_gvalue (const GValue *value)
{
    cairo_matrix_t *matrix = g_value_get_boxed (value);
    if (!matrix) {
        Py_RETURN_NONE;
    }

    return PycairoMatrix_FromMatrix (matrix);
}

#ifdef __GNUC__
#define PYGI_MODINIT_FUNC                                                     \
    __attribute__ ((visibility ("default"))) PyMODINIT_FUNC
#else
#define PYGI_MODINIT_FUNC PyMODINIT_FUNC
#endif

static PyMethodDef _gi_cairo_functions[] = {
    {
        0,
    },
};

static PyModuleDef_Slot _gi_cairo_slots[] = {
    { Py_mod_exec, _gi_cairo_exec },
    { 0, NULL },
};

static struct PyModuleDef __gi_cairomodule = {
    PyModuleDef_HEAD_INIT, "_gi_cairo", NULL, 0,    _gi_cairo_functions,
    _gi_cairo_slots,       NULL,        NULL, NULL,
};

PYGI_MODINIT_FUNC PyInit__gi_cairo (void);

PYGI_MODINIT_FUNC
PyInit__gi_cairo (void) { return PyModuleDef_Init (&__gi_cairomodule); }


static int
_gi_cairo_exec (PyObject *module)
{
    PyObject *gobject_mod;

    import_cairo ();

    if (Pycairo_CAPI == NULL) return -1;

    gobject_mod = pygobject_init (3, 13, 2);
    if (gobject_mod == NULL) return -1;
    Py_DECREF (gobject_mod);

    pygi_register_foreign_struct ("cairo", "Matrix", cairo_matrix_to_arg,
                                  cairo_matrix_from_arg, cairo_matrix_release);

    pygi_register_foreign_struct ("cairo", "Context", cairo_context_to_arg,
                                  cairo_context_from_arg,
                                  cairo_context_release);

    pygi_register_foreign_struct ("cairo", "Surface", cairo_surface_to_arg,
                                  cairo_surface_from_arg,
                                  cairo_surface_release);

    pygi_register_foreign_struct ("cairo", "Path", cairo_path_to_arg,
                                  cairo_path_from_arg, cairo_path_release);

    pygi_register_foreign_struct ("cairo", "FontFace", cairo_font_face_to_arg,
                                  cairo_font_face_from_arg,
                                  cairo_font_face_release);

    pygi_register_foreign_struct (
        "cairo", "FontOptions", cairo_font_options_to_arg,
        cairo_font_options_from_arg, cairo_font_options_release);

    pygi_register_foreign_struct ("cairo", "Pattern", cairo_pattern_to_arg,
                                  cairo_pattern_from_arg,
                                  cairo_pattern_release);

    pygi_register_foreign_struct ("cairo", "Region", cairo_region_to_arg,
                                  cairo_region_from_arg, cairo_region_release);

    pygi_register_foreign_struct (
        "cairo", "ScaledFont", cairo_scaled_font_to_arg,
        cairo_scaled_font_from_arg, cairo_scaled_font_release);

    pyg_register_gtype_custom (CAIRO_GOBJECT_TYPE_MATRIX,
                               cairo_matrix_from_gvalue,
                               cairo_matrix_to_gvalue);

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

    return 0;
}
