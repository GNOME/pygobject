/* -*- Mode: C; c-basic-offset: 4 -*-
 * vim: tabstop=4 shiftwidth=4 expandtab
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

#include <pythoncapi_compat.h>

#include "pygi-value.h"

#include "pygboxed.h"
#include "pygenum.h"
#include "pygflags.h"
#include "pygi-basictype.h"
#include "pygi-fundamental.h"
#include "pygi-repository.h"
#include "pygi-struct.h"
#include "pygi-type.h"
#include "pygobject-object.h"
#include "pygpointer.h"


/* glib 2.62 has started to print warnings for these which can't be disabled selectively, so just copy them here */
#define PYGI_TYPE_VALUE_ARRAY (g_value_array_get_type ())
#define PYGI_IS_PARAM_SPEC_VALUE_ARRAY(pspec)                                 \
    (G_TYPE_CHECK_INSTANCE_TYPE ((pspec), PYGI_TYPE_VALUE_ARRAY))
#define PYGI_PARAM_SPEC_VALUE_ARRAY(pspec)                                    \
    (G_TYPE_CHECK_INSTANCE_CAST ((pspec), g_param_spec_types[18],             \
                                 GParamSpecValueArray))

GIArgument
_pygi_argument_from_g_value (const GValue *value, GITypeInfo *type_info)
{
    GIArgument arg = {
        0,
    };

    GITypeTag type_tag = gi_type_info_get_tag (type_info);

    /* For the long handling: long can be equivalent to
       int32 or int64, depending on the architecture, but
       gi doesn't tell us (and same for ulong)
    */
    switch (type_tag) {
    case GI_TYPE_TAG_BOOLEAN:
        arg.v_boolean = g_value_get_boolean (value);
        break;
    case GI_TYPE_TAG_INT8:
        arg.v_int8 = g_value_get_schar (value);
        break;
    case GI_TYPE_TAG_INT16:
    case GI_TYPE_TAG_INT32:
        if (g_type_is_a (G_VALUE_TYPE (value), G_TYPE_LONG))
            arg.v_int32 = (gint32)g_value_get_long (value);
        else
            arg.v_int32 = (gint32)g_value_get_int (value);
        break;
    case GI_TYPE_TAG_INT64:
        if (g_type_is_a (G_VALUE_TYPE (value), G_TYPE_LONG))
            arg.v_int64 = g_value_get_long (value);
        else
            arg.v_int64 = g_value_get_int64 (value);
        break;
    case GI_TYPE_TAG_UINT8:
        arg.v_uint8 = g_value_get_uchar (value);
        break;
    case GI_TYPE_TAG_UINT16:
    case GI_TYPE_TAG_UINT32:
        if (g_type_is_a (G_VALUE_TYPE (value), G_TYPE_ULONG))
            arg.v_uint32 = (guint32)g_value_get_ulong (value);
        else
            arg.v_uint32 = (guint32)g_value_get_uint (value);
        break;
    case GI_TYPE_TAG_UINT64:
        if (g_type_is_a (G_VALUE_TYPE (value), G_TYPE_ULONG))
            arg.v_uint64 = g_value_get_ulong (value);
        else
            arg.v_uint64 = g_value_get_uint64 (value);
        break;
    case GI_TYPE_TAG_UNICHAR:
        arg.v_uint32 = g_value_get_schar (value);
        break;
    case GI_TYPE_TAG_FLOAT:
        arg.v_float = g_value_get_float (value);
        break;
    case GI_TYPE_TAG_DOUBLE:
        arg.v_double = g_value_get_double (value);
        break;
    case GI_TYPE_TAG_GTYPE:
        arg.v_size = g_value_get_gtype (value);
        break;
    case GI_TYPE_TAG_UTF8:
    case GI_TYPE_TAG_FILENAME:
        /* Callers are responsible for ensuring the GValue stays alive
             * long enough for the string to be copied. */
        arg.v_string = (char *)g_value_get_string (value);
        break;
    case GI_TYPE_TAG_GLIST:
    case GI_TYPE_TAG_GSLIST:
    case GI_TYPE_TAG_ARRAY:
    case GI_TYPE_TAG_GHASH:
        if (G_VALUE_HOLDS_BOXED (value))
            arg.v_pointer = g_value_get_boxed (value);
        else
            /* e. g. GSettings::change-event */
            arg.v_pointer = g_value_get_pointer (value);
        break;
    case GI_TYPE_TAG_INTERFACE: {
        GIBaseInfo *info;

        info = gi_type_info_get_interface (type_info);

        if (GI_IS_FLAGS_INFO (info)) {
            /* Check flags before enums: flags are a subtype of enum. */
            arg.v_uint = g_value_get_flags (value);
        } else if (GI_IS_ENUM_INFO (info)) {
            arg.v_int = g_value_get_enum (value);
        } else if (GI_IS_INTERFACE_INFO (info) || GI_IS_OBJECT_INFO (info)) {
            if (G_VALUE_HOLDS_PARAM (value))
                arg.v_pointer = g_value_get_param (value);
            else if (G_VALUE_HOLDS_OBJECT (value))
                arg.v_pointer = g_value_get_object (value);
            else
                arg.v_pointer = pygi_fundamental_from_value (value);
        } else if (GI_IS_STRUCT_INFO (info) || GI_IS_UNION_INFO (info)) {
            if (G_VALUE_HOLDS (value, G_TYPE_BOXED)) {
                arg.v_pointer = g_value_get_boxed (value);
            } else if (G_VALUE_HOLDS (value, G_TYPE_VARIANT)) {
                arg.v_pointer = g_value_get_variant (value);
            } else if (G_VALUE_HOLDS (value, G_TYPE_POINTER)) {
                arg.v_pointer = g_value_get_pointer (value);
            } else {
                PyErr_Format (
                    PyExc_NotImplementedError,
                    "Converting GValue's of type '%s' is not implemented.",
                    g_type_name (G_VALUE_TYPE (value)));
            }
        } else {
            PyErr_Format (
                PyExc_NotImplementedError,
                "Converting GValue's of type '%s' is not implemented.",
                g_type_name (G_TYPE_FROM_INSTANCE (info)));
        }

        gi_base_info_unref (info);

        break;
    }
    case GI_TYPE_TAG_ERROR:
        arg.v_pointer = g_value_get_boxed (value);
        break;
    case GI_TYPE_TAG_VOID:
        arg.v_pointer = g_value_get_pointer (value);
        break;
    default:
        break;
    }

    return arg;
}


/* Ignore g_value_array deprecations. Although they are deprecated,
 * we still need to support the marshaling of them in PyGObject.
 */
G_GNUC_BEGIN_IGNORE_DEPRECATIONS

static int
pyg_value_array_from_pyobject (GValue *value, PyObject *obj,
                               const GParamSpecValueArray *pspec)
{
    Py_ssize_t seq_len;
    GValueArray *value_array;
    guint len, i;

    seq_len = PySequence_Length (obj);
    if (seq_len == -1) {
        PyErr_Clear ();
        return -1;
    }
    len = (guint)seq_len;

    if (pspec && pspec->fixed_n_elements > 0 && len != pspec->fixed_n_elements)
        return -1;

    value_array = g_value_array_new (len);

    for (i = 0; i < len; ++i) {
        PyObject *item = PySequence_GetItem (obj, i);
        GType type;

        if (!item) {
            PyErr_Clear ();
            g_value_array_free (value_array);
            return -1;
        }

        if (pspec && pspec->element_spec)
            type = G_PARAM_SPEC_VALUE_TYPE (pspec->element_spec);
        else if (Py_IsNone (item))
            type = G_TYPE_POINTER; /* store None as NULL */
        else {
            type = pyg_type_from_object ((PyObject *)Py_TYPE (item));
            if (!type) {
                PyErr_Clear ();
                g_value_array_free (value_array);
                Py_DECREF (item);
                return -1;
            }
        }

        if (type == G_TYPE_VALUE) {
            const GValue *item_value = pyg_boxed_get (item, GValue);
            g_value_array_append (value_array, item_value);
        } else {
            GValue item_value = {
                0,
            };
            int status;

            g_value_init (&item_value, type);
            status = (pspec && pspec->element_spec)
                         ? pyg_param_gvalue_from_pyobject (&item_value, item,
                                                           pspec->element_spec)
                         : pyg_value_from_pyobject (&item_value, item);
            Py_DECREF (item);

            if (status == -1) {
                g_value_array_free (value_array);
                g_value_unset (&item_value);
                return -1;
            }
            g_value_array_append (value_array, &item_value);
            g_value_unset (&item_value);
        }
    }

    g_value_take_boxed (value, value_array);
    return 0;
}

G_GNUC_END_IGNORE_DEPRECATIONS

static int
pyg_array_from_pyobject (GValue *value, PyObject *obj)
{
    Py_ssize_t len, i;
    GArray *array;

    len = PySequence_Length (obj);
    if (len == -1) {
        PyErr_Clear ();
        return -1;
    }

    array = g_array_new (FALSE, TRUE, sizeof (GValue));

    for (i = 0; i < len; ++i) {
        PyObject *item = PySequence_GetItem (obj, i);
        GType type;
        GValue item_value = {
            0,
        };
        int status;

        if (!item) {
            PyErr_Clear ();
            g_array_free (array, FALSE);
            return -1;
        }

        if (Py_IsNone (item))
            type = G_TYPE_POINTER; /* store None as NULL */
        else {
            type = pyg_type_from_object ((PyObject *)Py_TYPE (item));
            if (!type) {
                PyErr_Clear ();
                g_array_free (array, FALSE);
                Py_DECREF (item);
                return -1;
            }
        }

        g_value_init (&item_value, type);
        status = pyg_value_from_pyobject (&item_value, item);
        Py_DECREF (item);

        if (status == -1) {
            g_array_free (array, FALSE);
            g_value_unset (&item_value);
            return -1;
        }

        g_array_append_val (array, item_value);
    }

    g_value_take_boxed (value, array);
    return 0;
}

/**
 * pyg_value_from_pyobject_with_error:
 * @value: the GValue object to store the converted value in.
 * @obj: the Python object to convert.
 *
 * This function converts a Python object and stores the result in a
 * GValue.  The GValue must be initialised in advance with
 * g_value_init().  If the Python object can't be converted to the
 * type of the GValue, then an error is returned.
 *
 * Returns: 0 on success, -1 on error.
 */
int
pyg_value_from_pyobject_with_error (GValue *value, PyObject *obj)
{
    GType value_type = G_VALUE_TYPE (value);

    switch (G_TYPE_FUNDAMENTAL (value_type)) {
    case G_TYPE_INTERFACE:
        /* we only handle interface types that have a GObject prereq */
        if (g_type_is_a (value_type, G_TYPE_OBJECT)) {
            if (Py_IsNone (obj))
                g_value_set_object (value, NULL);
            else {
                if (!PyObject_TypeCheck (obj, &PyGObject_Type)) {
                    PyErr_SetString (PyExc_TypeError, "GObject is required");
                    return -1;
                }
                if (!G_TYPE_CHECK_INSTANCE_TYPE (pygobject_get (obj),
                                                 value_type)) {
                    PyErr_SetString (PyExc_TypeError,
                                     "Invalid GObject type for assignment");
                    return -1;
                }
                g_value_set_object (value, pygobject_get (obj));
            }
        } else {
            PyErr_SetString (PyExc_TypeError, "Unsupported conversion");
            return -1;
        }
        break;
    case G_TYPE_CHAR: {
        gint8 temp;
        if (pygi_gschar_from_py (obj, &temp)) {
            g_value_set_schar (value, temp);
            return 0;
        } else
            return -1;
    }
    case G_TYPE_UCHAR: {
        guchar temp;
        if (pygi_guchar_from_py (obj, &temp)) {
            g_value_set_uchar (value, temp);
            return 0;
        } else
            return -1;
    }
    case G_TYPE_BOOLEAN: {
        gboolean temp;
        if (pygi_gboolean_from_py (obj, &temp)) {
            g_value_set_boolean (value, temp);
            return 0;
        } else
            return -1;
    }
    case G_TYPE_INT: {
        gint temp;
        if (pygi_gint_from_py (obj, &temp)) {
            g_value_set_int (value, temp);
            return 0;
        } else
            return -1;
    }
    case G_TYPE_UINT: {
        guint temp;
        if (pygi_guint_from_py (obj, &temp)) {
            g_value_set_uint (value, temp);
            return 0;
        } else
            return -1;
    }
    case G_TYPE_LONG: {
        glong temp;
        if (pygi_glong_from_py (obj, &temp)) {
            g_value_set_long (value, temp);
            return 0;
        } else
            return -1;
    }
    case G_TYPE_ULONG: {
        gulong temp;
        if (pygi_gulong_from_py (obj, &temp)) {
            g_value_set_ulong (value, temp);
            return 0;
        } else
            return -1;
    }
    case G_TYPE_INT64: {
        gint64 temp;
        if (pygi_gint64_from_py (obj, &temp)) {
            g_value_set_int64 (value, temp);
            return 0;
        } else
            return -1;
    }
    case G_TYPE_UINT64: {
        guint64 temp;
        if (pygi_guint64_from_py (obj, &temp)) {
            g_value_set_uint64 (value, temp);
            return 0;
        } else
            return -1;
    }
    case G_TYPE_ENUM: {
        gint val = 0;
        if (pyg_enum_get_value (G_VALUE_TYPE (value), obj, &val) < 0) {
            return -1;
        }
        g_value_set_enum (value, val);
        break;
    }
    case G_TYPE_FLAGS: {
        guint val = 0;
        if (pyg_flags_get_value (G_VALUE_TYPE (value), obj, &val) < 0) {
            return -1;
        }
        g_value_set_flags (value, val);
        return 0;
    }
    case G_TYPE_FLOAT: {
        gfloat temp;
        if (pygi_gfloat_from_py (obj, &temp)) {
            g_value_set_float (value, temp);
            return 0;
        } else
            return -1;
    }
    case G_TYPE_DOUBLE: {
        gdouble temp;
        if (pygi_gdouble_from_py (obj, &temp)) {
            g_value_set_double (value, temp);
            return 0;
        } else
            return -1;
    }
    case G_TYPE_STRING: {
        gchar *temp;
        if (pygi_utf8_from_py (obj, &temp)) {
            g_value_take_string (value, temp);
            return 0;
        } else {
            /* also allows setting anything implementing __str__ */
            PyObject *str;
            PyErr_Clear ();
            str = PyObject_Str (obj);
            if (str == NULL) return -1;
            if (pygi_utf8_from_py (str, &temp)) {
                Py_DECREF (str);
                g_value_take_string (value, temp);
                return 0;
            }
            Py_DECREF (str);
            return -1;
        }
    }
    case G_TYPE_POINTER:
        if (Py_IsNone (obj))
            g_value_set_pointer (value, NULL);
        else if (PyObject_TypeCheck (obj, &PyGPointer_Type)
                 && G_VALUE_HOLDS (value, ((PyGPointer *)obj)->gtype))
            g_value_set_pointer (value, pyg_pointer_get (obj, gpointer));
        else if (PyCapsule_CheckExact (obj))
            g_value_set_pointer (value, PyCapsule_GetPointer (obj, NULL));
        else if (G_VALUE_HOLDS_GTYPE (value))
            g_value_set_gtype (value, pyg_type_from_object (obj));
        else {
            PyErr_SetString (PyExc_TypeError, "Expected pointer");
            return -1;
        }
        break;
    case G_TYPE_BOXED: {
        PyGTypeMarshal *bm;
        gboolean holds_value_array;

        G_GNUC_BEGIN_IGNORE_DEPRECATIONS
        holds_value_array = G_VALUE_HOLDS (value, PYGI_TYPE_VALUE_ARRAY);
        G_GNUC_END_IGNORE_DEPRECATIONS

        if (Py_IsNone (obj))
            g_value_set_boxed (value, NULL);
        else if (G_VALUE_HOLDS (value, PY_TYPE_OBJECT))
            g_value_set_boxed (value, obj);
        else if (PyObject_TypeCheck (obj, &PyGBoxed_Type)
                 && G_VALUE_HOLDS (value, ((PyGBoxed *)obj)->gtype))
            g_value_set_boxed (value, pyg_boxed_get (obj, gpointer));
        else if (G_VALUE_HOLDS (value, G_TYPE_VALUE)) {
            GType type;
            GValue *n_value;

            type = pyg_type_from_object ((PyObject *)Py_TYPE (obj));
            if (G_UNLIKELY (!type)) {
                return -1;
            }
            n_value = g_new0 (GValue, 1);
            g_value_init (n_value, type);
            g_value_take_boxed (value, n_value);
            return pyg_value_from_pyobject_with_error (n_value, obj);
        } else if (PySequence_Check (obj) && holds_value_array)
            return pyg_value_array_from_pyobject (value, obj, NULL);

        else if (PySequence_Check (obj) && G_VALUE_HOLDS (value, G_TYPE_ARRAY))
            return pyg_array_from_pyobject (value, obj);
        else if (PyUnicode_Check (obj)
                 && G_VALUE_HOLDS (value, G_TYPE_GSTRING)) {
            GString *string;
            char *buffer;
            Py_ssize_t len;
            buffer = PyUnicode_AsUTF8AndSize (obj, &len);
            if (buffer == NULL) return -1;
            string = g_string_new_len (buffer, len);
            g_value_set_boxed (value, string);
            g_string_free (string, TRUE);
            break;
        } else if ((bm = pyg_type_lookup (G_VALUE_TYPE (value))) != NULL)
            return bm->tovalue (value, obj);
        else if (PyCapsule_CheckExact (obj))
            g_value_set_boxed (value, PyCapsule_GetPointer (obj, NULL));
        else {
            PyErr_SetString (PyExc_TypeError, "Expected Boxed");
            return -1;
        }
        break;
    }
    case G_TYPE_OBJECT:
        if (Py_IsNone (obj)) {
            g_value_set_object (value, NULL);
        } else if (PyObject_TypeCheck (obj, &PyGObject_Type)
                   && G_TYPE_CHECK_INSTANCE_TYPE (pygobject_get (obj),
                                                  G_VALUE_TYPE (value))) {
            g_value_set_object (value, pygobject_get (obj));
        } else {
            PyErr_SetString (PyExc_TypeError, "Expected GObject");
            return -1;
        }
        break;
    case G_TYPE_VARIANT: {
        if (Py_IsNone (obj))
            g_value_set_variant (value, NULL);
        else if (pyg_type_from_object_strict (obj, FALSE) == G_TYPE_VARIANT)
            g_value_set_variant (value, pyg_boxed_get (obj, GVariant));
        else {
            PyErr_SetString (PyExc_TypeError, "Expected Variant");
            return -1;
        }
        break;
    }
    default: {
        PyGTypeMarshal *bm;
        if ((bm = pyg_type_lookup (G_VALUE_TYPE (value))) != NULL) {
            return bm->tovalue (value, obj);
        } else {
            GIRepository *repository;
            GIBaseInfo *info;
            GIObjectInfoSetValueFunction set_value_func = NULL;

            if (!PyObject_TypeCheck (obj, &PyGIFundamental_Type)) {
                PyErr_SetString (PyExc_TypeError,
                                 "Fundamental type is required");
                return -1;
            }
            if (!G_TYPE_CHECK_INSTANCE_TYPE (pygi_fundamental_get (obj),
                                             value_type)) {
                PyErr_SetString (PyExc_TypeError,
                                 "Invalid fundamental type for assignment");
                return -1;
            }

            repository = pygi_repository_get_default ();
            info = gi_repository_find_by_gtype (repository, value_type);

            if (info && GI_IS_OBJECT_INFO (info)) {
                set_value_func =
                    gi_object_info_get_set_value_function_pointer (
                        (GIObjectInfo *)info);
                if (set_value_func) {
                    set_value_func (value, pygi_fundamental_get (obj));
                } else {
                    PyErr_SetString (
                        PyExc_TypeError,
                        "No set-value function for fundamental type");
                }
            } else {
                PyErr_SetString (PyExc_TypeError, "Unknown value type");
            }

            if (info) gi_base_info_unref (info);
        }
        break;
    }
    }

    /* If an error occurred, unset the GValue but don't clear the Python error. */
    if (PyErr_Occurred ()) {
        g_value_unset (value);
        return -1;
    }

    return 0;
}

/**
 * pyg_value_from_pyobject:
 * @value: the GValue object to store the converted value in.
 * @obj: the Python object to convert.
 *
 * Same basic function as pyg_value_from_pyobject_with_error but clears
 * any Python errors before returning.
 *
 * Returns: 0 on success, -1 on error.
 */
int
pyg_value_from_pyobject (GValue *value, PyObject *obj)
{
    int res = pyg_value_from_pyobject_with_error (value, obj);

    if (PyErr_Occurred ()) {
        PyErr_Clear ();
        return -1;
    }
    return res;
}

/**
 * pygi_value_to_py_basic_type:
 * @value: the GValue object.
 * @handled: (out): TRUE if the return value is defined
 *
 * This function creates/returns a Python wrapper object that
 * represents the GValue passed as an argument limited to supporting basic types
 * like ints, bools, and strings.
 *
 * Returns: a PyObject representing the value.
 */
PyObject *
pygi_value_to_py_basic_type (const GValue *value, GType fundamental,
                             gboolean *handled)
{
    *handled = TRUE;
    switch (fundamental) {
    case G_TYPE_CHAR:
        return PyLong_FromLong (g_value_get_schar (value));
    case G_TYPE_UCHAR:
        return PyLong_FromLong (g_value_get_uchar (value));
    case G_TYPE_BOOLEAN:
        return pygi_gboolean_to_py (g_value_get_boolean (value));
    case G_TYPE_INT:
        return pygi_gint_to_py (g_value_get_int (value));
    case G_TYPE_UINT:
        return pygi_guint_to_py (g_value_get_uint (value));
    case G_TYPE_LONG:
        return pygi_glong_to_py (g_value_get_long (value));
    case G_TYPE_ULONG:
        return pygi_gulong_to_py (g_value_get_ulong (value));
    case G_TYPE_INT64:
        return pygi_gint64_to_py (g_value_get_int64 (value));
    case G_TYPE_UINT64:
        return pygi_guint64_to_py (g_value_get_uint64 (value));
    case G_TYPE_ENUM:
        return pyg_enum_from_gtype (G_VALUE_TYPE (value),
                                    g_value_get_enum (value));
    case G_TYPE_FLAGS:
        return pyg_flags_from_gtype (G_VALUE_TYPE (value),
                                     g_value_get_flags (value));
    case G_TYPE_FLOAT:
        return pygi_gfloat_to_py (g_value_get_float (value));
    case G_TYPE_DOUBLE:
        return pygi_gdouble_to_py (g_value_get_double (value));
    case G_TYPE_STRING:
        return pygi_utf8_to_py (g_value_get_string (value));
    default:
        *handled = FALSE;
        return NULL;
    }
}

/**
 * value_to_py_structured_type:
 * @value: the GValue object.
 * @copy_boxed: true if boxed values should be copied.
 *
 * This function creates/returns a Python wrapper object that
 * represents the GValue passed as an argument.
 *
 * Returns: a PyObject representing the value or NULL and sets an error;
 */
static PyObject *
value_to_py_structured_type (const GValue *value, GType fundamental,
                             gboolean copy_boxed)
{
    const gchar *type_name;

    switch (fundamental) {
    case G_TYPE_INTERFACE:
        if (g_type_is_a (G_VALUE_TYPE (value), G_TYPE_OBJECT))
            return pygobject_new (g_value_get_object (value));
        else
            break;

    case G_TYPE_POINTER:
        if (G_VALUE_HOLDS_GTYPE (value))
            return pyg_type_wrapper_new (g_value_get_gtype (value));
        else
            return pyg_pointer_new (G_VALUE_TYPE (value),
                                    g_value_get_pointer (value));
    case G_TYPE_BOXED: {
        PyGTypeMarshal *bm;
        gboolean holds_value_array;

        G_GNUC_BEGIN_IGNORE_DEPRECATIONS
        holds_value_array = G_VALUE_HOLDS (value, PYGI_TYPE_VALUE_ARRAY);
        G_GNUC_END_IGNORE_DEPRECATIONS

        if (G_VALUE_HOLDS (value, PY_TYPE_OBJECT)) {
            PyObject *ret = (PyObject *)g_value_dup_boxed (value);
            if (ret == NULL) {
                Py_RETURN_NONE;
            }
            return ret;
        } else if (G_VALUE_HOLDS (value, G_TYPE_VALUE)) {
            GValue *n_value = g_value_get_boxed (value);
            return pyg_value_as_pyobject (n_value, copy_boxed);
        } else if (holds_value_array) {
            GValueArray *array = (GValueArray *)g_value_get_boxed (value);
            Py_ssize_t n_values = array ? array->n_values : 0;
            PyObject *ret = PyList_New (n_values);
            int i;
            for (i = 0; i < n_values; ++i)
                PyList_SET_ITEM (
                    ret, i,
                    pyg_value_as_pyobject (array->values + i, copy_boxed));
            return ret;
        } else if (G_VALUE_HOLDS (value, G_TYPE_GSTRING)) {
            GString *string = (GString *)g_value_get_boxed (value);
            PyObject *ret =
                PyUnicode_FromStringAndSize (string->str, string->len);
            return ret;
        }
        bm = pyg_type_lookup (G_VALUE_TYPE (value));
        if (bm) {
            return bm->fromvalue (value);
        } else {
            if (copy_boxed)
                return pygi_gboxed_new (G_VALUE_TYPE (value),
                                        g_value_get_boxed (value), TRUE, TRUE);
            else
                return pygi_gboxed_new (G_VALUE_TYPE (value),
                                        g_value_get_boxed (value), FALSE,
                                        FALSE);
        }
    }
    case G_TYPE_OBJECT:
        return pygobject_new (g_value_get_object (value));
    case G_TYPE_VARIANT: {
        GVariant *v = g_value_get_variant (value);
        if (v == NULL) {
            Py_RETURN_NONE;
        }
        return pygi_struct_new_from_g_type (G_TYPE_VARIANT, g_variant_ref (v),
                                            FALSE);
    }
    case G_TYPE_INVALID:
        PyErr_SetString (PyExc_TypeError, "Invalid type");
        return NULL;
    default: {
        PyGTypeMarshal *bm;
        GIRepository *repository;
        GIBaseInfo *info;
        GIObjectInfoGetValueFunction get_value_func = NULL;

        if ((bm = pyg_type_lookup (G_VALUE_TYPE (value))))
            return bm->fromvalue (value);

        repository = pygi_repository_get_default ();
        info = gi_repository_find_by_gtype (repository, fundamental);

        if (info == NULL) break;

        if (GI_IS_OBJECT_INFO (info))
            get_value_func = gi_object_info_get_get_value_function_pointer (
                (GIObjectInfo *)info);

        gi_base_info_unref (info);

        if (get_value_func)
            return pygi_fundamental_new (get_value_func (value));

        break;
    }
    }

    type_name = g_type_name (G_VALUE_TYPE (value));
    if (type_name == NULL) {
        type_name = "(null)";
    }
    PyErr_Format (PyExc_TypeError, "unknown type %s", type_name);
    return NULL;
}


/**
 * pyg_value_as_pyobject:
 * @value: the GValue object.
 * @copy_boxed: true if boxed values should be copied.
 *
 * This function creates/returns a Python wrapper object that
 * represents the GValue passed as an argument.
 *
 * Returns: a PyObject representing the value or %NULL and sets an exception.
 */
PyObject *
pyg_value_as_pyobject (const GValue *value, gboolean copy_boxed)
{
    PyObject *pyobj;
    gboolean handled;
    GType fundamental = G_TYPE_FUNDAMENTAL (G_VALUE_TYPE (value));

    /* HACK: special case char and uchar to return PyBytes intstead of integers
     * in the general case. Property access will skip this by calling
     * pygi_value_to_py_basic_type() directly.
     * See: https://bugzilla.gnome.org/show_bug.cgi?id=733893 */
    if (fundamental == G_TYPE_CHAR) {
        gint8 val = g_value_get_schar (value);
        return PyUnicode_FromStringAndSize ((char *)&val, 1);
    } else if (fundamental == G_TYPE_UCHAR) {
        guint8 val = g_value_get_uchar (value);
        return PyBytes_FromStringAndSize ((char *)&val, 1);
    }

    pyobj = pygi_value_to_py_basic_type (value, fundamental, &handled);
    if (handled) return pyobj;

    pyobj = value_to_py_structured_type (value, fundamental, copy_boxed);
    return pyobj;
}


G_GNUC_BEGIN_IGNORE_DEPRECATIONS
int
pyg_param_gvalue_from_pyobject (GValue *value, PyObject *py_obj,
                                const GParamSpec *pspec)
{
    if (G_IS_PARAM_SPEC_UNICHAR (pspec)) {
        gunichar u;

        if (!pyg_pyobj_to_unichar_conv (py_obj, &u)) {
            PyErr_Clear ();
            return -1;
        }
        g_value_set_uint (value, u);
        return 0;
    } else if (PYGI_IS_PARAM_SPEC_VALUE_ARRAY (pspec))
        return pyg_value_array_from_pyobject (
            value, py_obj, PYGI_PARAM_SPEC_VALUE_ARRAY (pspec));
    else {
        return pyg_value_from_pyobject (value, py_obj);
    }
}

G_GNUC_END_IGNORE_DEPRECATIONS

PyObject *
pyg_param_gvalue_as_pyobject (const GValue *gvalue, gboolean copy_boxed,
                              const GParamSpec *pspec)
{
    if (G_IS_PARAM_SPEC_UNICHAR (pspec)) {
        gunichar u;
        gchar *encoded;
        PyObject *retval;

        u = g_value_get_uint (gvalue);
        encoded = g_ucs4_to_utf8 (&u, 1, NULL, NULL, NULL);
        if (encoded == NULL) {
            PyErr_SetString (PyExc_ValueError, "Failed to decode");
            return NULL;
        }
        retval = PyUnicode_FromString (encoded);
        g_free (encoded);
        return retval;
    } else {
        return pyg_value_as_pyobject (gvalue, copy_boxed);
    }
}

PyObject *
pyg__gvalue_get (PyObject *module, PyObject *pygvalue)
{
    if (!pyg_boxed_check (pygvalue, G_TYPE_VALUE)) {
        PyErr_SetString (PyExc_TypeError, "Expected GValue argument.");
        return NULL;
    }

    return pyg_value_as_pyobject (pyg_boxed_get (pygvalue, GValue),
                                  /*copy_boxed=*/TRUE);
}

PyObject *
pyg__gvalue_get_type (PyObject *module, PyObject *pygvalue)
{
    GValue *value;
    GType type;

    if (!pyg_boxed_check (pygvalue, G_TYPE_VALUE)) {
        PyErr_SetString (PyExc_TypeError, "Expected GValue argument.");
        return NULL;
    }

    value = pyg_boxed_get (pygvalue, GValue);
    type = G_VALUE_TYPE (value);
    return pyg_type_wrapper_new (type);
}

PyObject *
pyg__gvalue_set (PyObject *module, PyObject *args)
{
    PyObject *pygvalue;
    PyObject *pyobject;

    if (!PyArg_ParseTuple (args, "OO:_gi._gvalue_set", &pygvalue, &pyobject))
        return NULL;

    if (!pyg_boxed_check (pygvalue, G_TYPE_VALUE)) {
        PyErr_SetString (PyExc_TypeError, "Expected GValue argument.");
        return NULL;
    }

    if (pyg_value_from_pyobject_with_error (pyg_boxed_get (pygvalue, GValue),
                                            pyobject)
        == -1)
        return NULL;

    Py_RETURN_NONE;
}
