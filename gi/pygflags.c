/* -*- Mode: C; c-basic-offset: 4 -*-
 * pygtk- Python bindings for the GTK toolkit.
 * Copyright (C) 1998-2003  James Henstridge
 * Copyright (C) 2004       Johan Dahlin
 *
 *   pygflags.c: GFlags wrapper
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
 * License along with this library; if not, see <http://www.gnu.org/licenses/>.
 */

#include <config.h>

#include <pythoncapi_compat.h>

#include "pygi-type.h"

#include "pygflags.h"
#include "pygi-util.h"

GQuark pygflags_class_key;

PyTypeObject *PyGFlags_Type;
static PyObject *IntFlag_Type;

PyObject *
pyg_flags_val_new (PyObject *pyclass, guint value)
{
    PyObject *retval, *intvalue;
    PyObject *args[2] = { NULL };

    intvalue = PyLong_FromUnsignedLong (value);
    if (!intvalue) return NULL;

    args[1] = intvalue;
    retval = PyObject_Vectorcall (pyclass, &args[1],
                                  1 + PY_VECTORCALL_ARGUMENTS_OFFSET, NULL);
    if (!retval && PyErr_ExceptionMatches (PyExc_ValueError)) {
        PyErr_Clear ();
        return intvalue;
    }
    Py_DECREF (intvalue);

    return retval;
}

PyObject *
pyg_flags_from_gtype (GType gtype, guint value)
{
    PyObject *pyclass;

    g_return_val_if_fail (gtype != G_TYPE_INVALID, NULL);

    /* Get a wrapper class by:
     * 1. check for one attached to the gtype
     * 2. lookup one in a typelib
     * 3. creating a new one
     */
    pyclass = (PyObject *)g_type_get_qdata (gtype, pygflags_class_key);
    if (!pyclass) pyclass = pygi_type_import_by_g_type (gtype);
    if (!pyclass)
        pyclass = pyg_flags_add (NULL, g_type_name (gtype), NULL, gtype);
    if (!pyclass) return PyLong_FromUnsignedLong (value);

    return pyg_flags_val_new (pyclass, value);
}

static void
add_value (PyObject *dict, const char *value_nick, unsigned int value)
{
    char *upper = g_ascii_strup (value_nick, -1);
    char *c;
    PyObject *v;

    for (c = upper; *c != '\0'; c++) {
        if (*c == '-') *c = '_';
    }

    /* skip if the name already exists in the dictionary */
    if (PyMapping_HasKeyString (dict, upper)) {
        g_free (upper);
        return;
    }

    v = PyLong_FromUnsignedLong (value);
    PyMapping_SetItemString (dict, upper, v);
    Py_DECREF (v);
    g_free (upper);
}

PyObject *
pyg_flags_add_full (PyObject *module, const char *typename, GType gtype,
                    GIFlagsInfo *info)
{
    PyObject *stub;
    PyObject *base_class, *method_name, *flags_name, *bases, *values;
    PyObject *args[4] = { NULL };

    if (gtype == G_TYPE_NONE && info == NULL) {
        PyErr_SetString (PyExc_ValueError,
                         "cannot create enum without a GType or EnumInfo");
        return NULL;
    }
    if (gtype != G_TYPE_NONE && !g_type_is_a (gtype, G_TYPE_FLAGS)) {
        PyErr_Format (PyExc_TypeError,
                      "Trying to register gtype '%s' as flags when in fact it "
                      "is of type '%s'",
                      g_type_name (gtype),
                      g_type_name (G_TYPE_FUNDAMENTAL (gtype)));
        return NULL;
    }
    if (info != NULL) {
        GType info_gtype = gi_registered_type_info_get_g_type (
            GI_REGISTERED_TYPE_INFO (info));

        if (info_gtype != gtype) {
            PyErr_Format (PyExc_ValueError,
                          "gtype '%s' does not match FlagsInfo '%s'",
                          g_type_name (gtype),
                          gi_base_info_get_name (GI_BASE_INFO (info)));
            return NULL;
        }
    }

    if (gtype == G_TYPE_NONE)
        base_class = IntFlag_Type;
    else
        base_class = (PyObject *)PyGFlags_Type;
    flags_name = PyUnicode_FromString (typename);
    bases = PyTuple_New (1);
    PyTuple_SET_ITEM (bases, 0, Py_NewRef (base_class));

    args[0] = (PyObject *)Py_TYPE (base_class);
    args[1] = flags_name;
    args[2] = bases;

    method_name = PyUnicode_FromString ("__prepare__");
    values = PyObject_VectorcallMethod (
        method_name, args, 3 + PY_VECTORCALL_ARGUMENTS_OFFSET, NULL);
    Py_DECREF (method_name);
    if (!values) {
        Py_DECREF (flags_name);
        Py_DECREF (bases);
        return NULL;
    }

    /* Collect values from registered GType */
    if (gtype != G_TYPE_NONE) {
        GFlagsClass *fclass;
        unsigned int i;

        fclass = G_FLAGS_CLASS (g_type_class_ref (gtype));
        for (i = 0; i < fclass->n_values; i++) {
            add_value (values, fclass->values[i].value_nick,
                       fclass->values[i].value);
        }
        g_type_class_unref (fclass);
    }

    /* Collect values from GIFlagsInfo (which may include additional aliases) */
    if (info != NULL) {
        unsigned int i, length;

        length = gi_enum_info_get_n_values (GI_ENUM_INFO (info));
        for (i = 0; i < length; i++) {
            GIValueInfo *v = gi_enum_info_get_value (GI_ENUM_INFO (info), i);

            add_value (values, gi_base_info_get_name (GI_BASE_INFO (v)),
                       gi_value_info_get_value (v));
        }
    }

    if (module && !Py_IsNone (module)) {
        PyObject *module_name = PyModule_GetNameObject (module);

        PyMapping_SetItemString (values, "__module__", module_name);
        Py_DECREF (module_name);
    }
    if (gtype != G_TYPE_NONE) {
        PyObject *o = pyg_type_wrapper_new (gtype);

        PyMapping_SetItemString (values, "__gtype__", o);
        Py_DECREF (o);
    }

    args[3] = values;

    stub = PyObject_Vectorcall ((PyObject *)Py_TYPE (base_class), &args[1],
                                3 + PY_VECTORCALL_ARGUMENTS_OFFSET, NULL);
    Py_DECREF (values);
    Py_DECREF (bases);
    Py_DECREF (flags_name);

    if (!stub) return NULL;

    ((PyTypeObject *)stub)->tp_flags &= ~Py_TPFLAGS_BASETYPE;
    if (gtype != G_TYPE_NONE) {
        g_type_set_qdata (gtype, pygflags_class_key, stub);
    }

    return stub;
}


/*
 * pyg_flags_add
 * Dynamically create a class derived from PyGFlags based on the given GType.
 */
PyObject *
pyg_flags_add (PyObject *module, const char *typename,
               const char *strip_prefix, GType gtype)
{
    PyGILState_STATE state;
    PyObject *stub;

    g_return_val_if_fail (typename != NULL, NULL);
    if (!g_type_is_a (gtype, G_TYPE_FLAGS)) {
        g_warning (
            "Trying to register gtype '%s' as flags when in fact it is of "
            "type '%s'",
            g_type_name (gtype), g_type_name (G_TYPE_FUNDAMENTAL (gtype)));
        return NULL;
    }

    state = PyGILState_Ensure ();

    stub = pyg_flags_add_full (module, typename, gtype, NULL);
    if (!stub) {
        PyGILState_Release (state);
        return NULL;
    }

    if (module) {
        GFlagsClass *fclass;
        guint i;

        /* Add it to the module name space */
        PyModule_AddObject (module, (char *)typename, stub);
        Py_INCREF (stub);

        /* Register flags values */
        fclass = G_FLAGS_CLASS (g_type_class_ref (gtype));
        for (i = 0; i < fclass->n_values; i++) {
            PyObject *item, *intval;
            PyObject *args[2] = { NULL };
            char *prefix;

            intval = PyLong_FromUnsignedLong (fclass->values[i].value);
            args[1] = intval;
            item = PyObject_Vectorcall (
                stub, &args[1], 1 + PY_VECTORCALL_ARGUMENTS_OFFSET, NULL);
            Py_DECREF (intval);

            prefix = g_strdup (pyg_constant_strip_prefix (
                fclass->values[i].value_name, strip_prefix));
            PyModule_AddObject (module, prefix, item);
            g_free (prefix);
        }
        g_type_class_unref (fclass);
    }

    PyGILState_Release (state);
    return stub;
}

gboolean
pyg_flags_register (PyTypeObject *flags_class, char *type_name)
{
    gboolean result = FALSE;
    GType gtype = G_TYPE_INVALID;
    GFlagsValue *flags_values = NULL;
    PyObject *values = NULL, *pygtype = NULL;
    Py_ssize_t length, i;

    values = PySequence_List ((PyObject *)flags_class);
    if (!values) goto out;

    length = PyList_Size (values);
    flags_values = g_new0 (GFlagsValue, length + 1);
    for (i = 0; i < length; i++) {
        PyObject *value = PyList_GetItem (values, i);
        PyObject *name = PyObject_GetAttrString (value, "name");
        char *c;

        if (!name) {
            goto out;
        }
        if (!PyUnicode_Check (name)) {
            PyErr_SetString (PyExc_TypeError,
                             "flags value names should be strings");
            Py_DECREF (name);
            goto out;
        }

        flags_values[i].value = PyLong_AsUnsignedLongMask (value);
        flags_values[i].value_name =
            g_strdup (PyUnicode_AsUTF8AndSize (name, NULL));
        c = g_ascii_strdown (flags_values[i].value_name, -1);
        flags_values[i].value_nick = c;
        while (*c != '\0') {
            if (*c == '_') *c = '-';
            c++;
        }
        Py_DECREF (name);
    }

    gtype = g_flags_register_static (type_name, flags_values);
    if (gtype == G_TYPE_INVALID) {
        PyErr_Format (PyExc_RuntimeError, "Unable to register flags '%s'",
                      type_name);
        goto out;
    }
    g_type_set_qdata (gtype, pygflags_class_key, flags_class);

    pygtype = pyg_type_wrapper_new (gtype);
    if (!pygtype) goto out;
    PyObject_SetAttrString ((PyObject *)flags_class, "__gtype__", pygtype);

    result = TRUE;

out:
    Py_XDECREF (values);
    Py_XDECREF (pygtype);
    /* If type registration succeeded, this data should not be freed */
    if (gtype == G_TYPE_INVALID) {
        g_free (type_name);
        if (flags_values != NULL) {
            GFlagsValue *v;
            for (v = flags_values; v->value_name != NULL; v++) {
                g_free ((char *)v->value_name);
                g_free ((char *)v->value_nick);
            }
            g_free (flags_values);
        }
    }
    return result;
}

static GType
get_flags_gtype (PyTypeObject *type)
{
    PyObject *pytc;
    GType gtype;

    pytc = PyObject_GetAttrString ((PyObject *)type, "__gtype__");
    if (!pytc) return G_TYPE_INVALID;

    if (!PyObject_TypeCheck (pytc, &PyGTypeWrapper_Type)) {
        Py_DECREF (pytc);
        PyErr_SetString (PyExc_TypeError,
                         "__gtype__ attribute not a typecode");
        return G_TYPE_INVALID;
    }

    gtype = pyg_type_from_object (pytc);
    Py_DECREF (pytc);

    if (!G_TYPE_IS_FLAGS (gtype)) {
        PyErr_SetString (PyExc_TypeError,
                         "__gtype__ attribute not a flags typecode");
        return G_TYPE_INVALID;
    }

    return gtype;
}

static PyObject *
pyg_flags_get_first_value_name (PyObject *self, void *closure)
{
    GType gtype;
    GFlagsClass *flags_class;
    GFlagsValue *flags_value;
    PyObject *retval;

    gtype = get_flags_gtype (Py_TYPE (self));
    if (gtype == G_TYPE_INVALID) return NULL;
    flags_class = g_type_class_ref (gtype);
    g_assert (G_IS_FLAGS_CLASS (flags_class));
    flags_value = g_flags_get_first_value (
        flags_class, (guint)PyLong_AsUnsignedLongMask ((PyObject *)self));
    if (flags_value)
        retval = PyUnicode_FromString (flags_value->value_name);
    else {
        retval = Py_NewRef (Py_None);
    }
    g_type_class_unref (flags_class);

    return retval;
}

static PyObject *
pyg_flags_get_first_value_nick (PyObject *self, void *closure)
{
    GType gtype;
    GFlagsClass *flags_class;
    GFlagsValue *flags_value;
    PyObject *retval;

    gtype = get_flags_gtype (Py_TYPE (self));
    if (gtype == G_TYPE_INVALID) return NULL;
    flags_class = g_type_class_ref (gtype);
    g_assert (G_IS_FLAGS_CLASS (flags_class));

    flags_value = g_flags_get_first_value (
        flags_class, (guint)PyLong_AsUnsignedLongMask ((PyObject *)self));
    if (flags_value)
        retval = PyUnicode_FromString (flags_value->value_nick);
    else {
        retval = Py_NewRef (Py_None);
    }
    g_type_class_unref (flags_class);

    return retval;
}

static PyObject *
pyg_flags_get_value_names (PyObject *self, void *closure)
{
    GType gtype;
    GFlagsClass *flags_class;
    PyObject *retval;
    guint i;

    gtype = get_flags_gtype (Py_TYPE (self));
    if (gtype == G_TYPE_INVALID) return NULL;
    flags_class = g_type_class_ref (gtype);
    g_assert (G_IS_FLAGS_CLASS (flags_class));

    retval = PyList_New (0);
    for (i = 0; i < flags_class->n_values; i++) {
        PyObject *value_name;

        if ((PyLong_AsUnsignedLongMask ((PyObject *)self)
             & flags_class->values[i].value)
            == flags_class->values[i].value) {
            value_name =
                PyUnicode_FromString (flags_class->values[i].value_name);
            PyList_Append (retval, value_name);
            Py_DECREF (value_name);
        }
    }

    g_type_class_unref (flags_class);

    return retval;
}

static PyObject *
pyg_flags_get_value_nicks (PyObject *self, void *closure)
{
    GType gtype;
    GFlagsClass *flags_class;
    PyObject *retval;
    guint i;

    gtype = get_flags_gtype (Py_TYPE (self));
    if (gtype == G_TYPE_INVALID) return NULL;
    flags_class = g_type_class_ref (gtype);
    g_assert (G_IS_FLAGS_CLASS (flags_class));

    retval = PyList_New (0);
    for (i = 0; i < flags_class->n_values; i++)
        if ((PyLong_AsUnsignedLongMask ((PyObject *)self)
             & flags_class->values[i].value)
            == flags_class->values[i].value) {
            PyObject *py_nick =
                PyUnicode_FromString (flags_class->values[i].value_nick);
            PyList_Append (retval, py_nick);
            Py_DECREF (py_nick);
        }

    g_type_class_unref (flags_class);

    return retval;
}

static PyGetSetDef pyg_flags_getsets[] = {
    { "first_value_name", (getter)pyg_flags_get_first_value_name, (setter)0 },
    { "first_value_nick", (getter)pyg_flags_get_first_value_nick, (setter)0 },
    { "value_names", (getter)pyg_flags_get_value_names, (setter)0 },
    { "value_nicks", (getter)pyg_flags_get_value_nicks, (setter)0 },
    { NULL, 0, 0 },
};

/**
 * Returns 0 on success, or -1 and sets an exception.
 */
int
pygi_flags_register_types (PyObject *mod)
{
    PyObject *enum_module, *item;
    int i;

    pygflags_class_key = g_quark_from_static_string ("PyGFlags::class");

    enum_module = PyImport_ImportModule ("enum");
    if (!enum_module) return -1;

    IntFlag_Type = PyObject_GetAttrString (enum_module, "IntFlag");
    Py_DECREF (enum_module);
    if (!IntFlag_Type) return -1;

    enum_module = PyImport_ImportModule ("gi._enum");
    if (!enum_module) return -1;

    PyGFlags_Type =
        (PyTypeObject *)PyObject_GetAttrString (enum_module, "GFlags");
    Py_DECREF (enum_module);
    if (!PyGFlags_Type) return -1;

    item = pyg_type_wrapper_new (G_TYPE_FLAGS);
    PyObject_SetAttrString ((PyObject *)PyGFlags_Type, "__gtype__", item);
    Py_DECREF (item);

    for (i = 0; pyg_flags_getsets[i].name != NULL; i++) {
        item = PyDescr_NewGetSet (PyGFlags_Type, &pyg_flags_getsets[i]);
        PyObject_SetAttrString ((PyObject *)PyGFlags_Type,
                                pyg_flags_getsets[i].name, item);
        Py_DECREF (item);
    }

    PyModule_AddObject (mod, "GFlags", Py_NewRef ((PyObject *)PyGFlags_Type));

    return 0;
}
