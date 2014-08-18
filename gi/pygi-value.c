
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

#include "pygi-value.h"
#include "pyglib-python-compat.h"
#include "pygobject-private.h"
#include "pygtype.h"
#include "pygparamspec.h"

GIArgument
_pygi_argument_from_g_value(const GValue *value,
                            GITypeInfo *type_info)
{
    GIArgument arg = { 0, };

    GITypeTag type_tag = g_type_info_get_tag (type_info);

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
		arg.v_int = g_value_get_long (value);
	    else
		arg.v_int = g_value_get_int (value);
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
		arg.v_uint = g_value_get_ulong (value);
	    else
		arg.v_uint = g_value_get_uint (value);
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
            arg.v_long = g_value_get_gtype (value);
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
        case GI_TYPE_TAG_INTERFACE:
        {
            GIBaseInfo *info;
            GIInfoType info_type;

            info = g_type_info_get_interface (type_info);
            info_type = g_base_info_get_type (info);

            g_base_info_unref (info);

            switch (info_type) {
                case GI_INFO_TYPE_FLAGS:
                    arg.v_uint = g_value_get_flags (value);
                    break;
                case GI_INFO_TYPE_ENUM:
                    arg.v_int = g_value_get_enum (value);
                    break;
                case GI_INFO_TYPE_INTERFACE:
                case GI_INFO_TYPE_OBJECT:
                    if (G_VALUE_HOLDS_PARAM (value))
                      arg.v_pointer = g_value_get_param (value);
                    else
                      arg.v_pointer = g_value_get_object (value);
                    break;
                case GI_INFO_TYPE_BOXED:
                case GI_INFO_TYPE_STRUCT:
                case GI_INFO_TYPE_UNION:
                    if (G_VALUE_HOLDS (value, G_TYPE_BOXED)) {
                        arg.v_pointer = g_value_get_boxed (value);
                    } else if (G_VALUE_HOLDS (value, G_TYPE_VARIANT)) {
                        arg.v_pointer = g_value_get_variant (value);
                    } else if (G_VALUE_HOLDS (value, G_TYPE_POINTER)) {
                        arg.v_pointer = g_value_get_pointer (value);
                    } else {
                        PyErr_Format (PyExc_NotImplementedError,
                                      "Converting GValue's of type '%s' is not implemented.",
                                      g_type_name (G_VALUE_TYPE (value)));
                    }
                    break;
                default:
                    PyErr_Format (PyExc_NotImplementedError,
                                  "Converting GValue's of type '%s' is not implemented.",
                                  g_info_type_to_string (info_type));
                    break;
            }
            break;
        }
        case GI_TYPE_TAG_ERROR:
            arg.v_pointer = g_value_get_boxed (value);
            break;
        case GI_TYPE_TAG_VOID:
            arg.v_pointer = g_value_get_pointer (value);
            break;
    }

    return arg;
}


/* Ignore g_value_array deprecations. Although they are deprecated,
 * we still need to support the marshaling of them in PyGObject.
 */
G_GNUC_BEGIN_IGNORE_DEPRECATIONS

static int
pyg_value_array_from_pyobject(GValue *value,
                              PyObject *obj,
                              const GParamSpecValueArray *pspec)
{
    int len;
    GValueArray *value_array;
    int i;

    len = PySequence_Length(obj);
    if (len == -1) {
        PyErr_Clear();
        return -1;
    }

    if (pspec && pspec->fixed_n_elements > 0 && len != pspec->fixed_n_elements)
        return -1;

    value_array = g_value_array_new(len);

    for (i = 0; i < len; ++i) {
        PyObject *item = PySequence_GetItem(obj, i);
        GType type;
        GValue item_value = { 0, };
        int status;

        if (! item) {
            PyErr_Clear();
            g_value_array_free(value_array);
            return -1;
        }

        if (pspec && pspec->element_spec)
            type = G_PARAM_SPEC_VALUE_TYPE(pspec->element_spec);
        else if (item == Py_None)
            type = G_TYPE_POINTER; /* store None as NULL */
        else {
            type = pyg_type_from_object((PyObject*)Py_TYPE(item));
            if (! type) {
                PyErr_Clear();
                g_value_array_free(value_array);
                Py_DECREF(item);
                return -1;
            }
        }

        g_value_init(&item_value, type);
        status = (pspec && pspec->element_spec)
                 ? pyg_param_gvalue_from_pyobject(&item_value, item, pspec->element_spec)
                 : pyg_value_from_pyobject(&item_value, item);
        Py_DECREF(item);

        if (status == -1) {
            g_value_array_free(value_array);
            g_value_unset(&item_value);
            return -1;
        }

        g_value_array_append(value_array, &item_value);
        g_value_unset(&item_value);
    }

    g_value_take_boxed(value, value_array);
    return 0;
}

G_GNUC_END_IGNORE_DEPRECATIONS

static int
pyg_array_from_pyobject(GValue *value,
                        PyObject *obj)
{
    int len;
    GArray *array;
    int i;

    len = PySequence_Length(obj);
    if (len == -1) {
        PyErr_Clear();
        return -1;
    }

    array = g_array_new(FALSE, TRUE, sizeof(GValue));

    for (i = 0; i < len; ++i) {
        PyObject *item = PySequence_GetItem(obj, i);
        GType type;
        GValue item_value = { 0, };
        int status;

        if (! item) {
            PyErr_Clear();
            g_array_free(array, FALSE);
            return -1;
        }

        if (item == Py_None)
            type = G_TYPE_POINTER; /* store None as NULL */
        else {
            type = pyg_type_from_object((PyObject*)Py_TYPE(item));
            if (! type) {
                PyErr_Clear();
                g_array_free(array, FALSE);
                Py_DECREF(item);
                return -1;
            }
        }

        g_value_init(&item_value, type);
        status = pyg_value_from_pyobject(&item_value, item);
        Py_DECREF(item);

        if (status == -1) {
            g_array_free(array, FALSE);
            g_value_unset(&item_value);
            return -1;
        }

        g_array_append_val(array, item_value);
    }

    g_value_take_boxed(value, array);
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
pyg_value_from_pyobject_with_error(GValue *value, PyObject *obj)
{
    PyObject *tmp;
    GType value_type = G_VALUE_TYPE(value);

    switch (G_TYPE_FUNDAMENTAL(value_type)) {
    case G_TYPE_INTERFACE:
        /* we only handle interface types that have a GObject prereq */
        if (g_type_is_a(value_type, G_TYPE_OBJECT)) {
            if (obj == Py_None)
                g_value_set_object(value, NULL);
            else {
                if (!PyObject_TypeCheck(obj, &PyGObject_Type)) {
                    PyErr_SetString(PyExc_TypeError, "GObject is required");
                    return -1;
                }
                if (!G_TYPE_CHECK_INSTANCE_TYPE(pygobject_get(obj),
                        value_type)) {
                    PyErr_SetString(PyExc_TypeError, "Invalid GObject type for assignment");
                    return -1;
                }
                g_value_set_object(value, pygobject_get(obj));
            }
        } else {
            PyErr_SetString(PyExc_TypeError, "Unsupported conversion");
            return -1;
        }
        break;
    case G_TYPE_CHAR:
        if (PYGLIB_PyLong_Check(obj)) {
            glong val;
            val = PYGLIB_PyLong_AsLong(obj);
            if (val >= -128 && val <= 127)
                g_value_set_schar(value, (gchar) val);
            else
                return -1;
        }
#if PY_VERSION_HEX < 0x03000000
        else if (PyString_Check(obj)) {
            g_value_set_schar(value, PyString_AsString(obj)[0]);
        }
#endif
        else if (PyUnicode_Check(obj)) {
            tmp = PyUnicode_AsUTF8String(obj);
            g_value_set_schar(value, PYGLIB_PyBytes_AsString(tmp)[0]);
            Py_DECREF(tmp);
        } else {
            PyErr_SetString(PyExc_TypeError, "Cannot convert to TYPE_CHAR");
            return -1;
        }

        break;
    case G_TYPE_UCHAR:
        if (PYGLIB_PyLong_Check(obj)) {
            glong val;
            val = PYGLIB_PyLong_AsLong(obj);
            if (val >= 0 && val <= 255)
                g_value_set_uchar(value, (guchar) val);
            else
                return -1;
#if PY_VERSION_HEX < 0x03000000
        } else if (PyString_Check(obj)) {
            g_value_set_uchar(value, PyString_AsString(obj)[0]);
#endif
        } else if (PyUnicode_Check(obj)) {
            tmp = PyUnicode_AsUTF8String(obj);
            g_value_set_uchar(value, PYGLIB_PyBytes_AsString(tmp)[0]);
            Py_DECREF(tmp);
        } else {
            PyErr_Clear();
            return -1;
        }
        break;
    case G_TYPE_BOOLEAN:
        g_value_set_boolean(value, PyObject_IsTrue(obj));
        break;
    case G_TYPE_INT:
        g_value_set_int(value, PYGLIB_PyLong_AsLong(obj));
        break;
    case G_TYPE_UINT:
    {
        if (PYGLIB_PyLong_Check(obj)) {
            guint val;

            /* check that number is not negative */
            if (PyLong_AsLongLong(obj) < 0)
                return -1;

            val = PyLong_AsUnsignedLong(obj);
            if (val <= G_MAXUINT)
                g_value_set_uint(value, val);
            else
                return -1;
        } else {
            g_value_set_uint(value, PyLong_AsUnsignedLong(obj));
        }
    }
    break;
    case G_TYPE_LONG:
        g_value_set_long(value, PYGLIB_PyLong_AsLong(obj));
        break;
    case G_TYPE_ULONG:
#if PY_VERSION_HEX < 0x03000000
        if (PyInt_Check(obj)) {
            long val;

            val = PYGLIB_PyLong_AsLong(obj);
            if (val < 0) {
                PyErr_SetString(PyExc_OverflowError, "negative value not allowed for uint64 property");
                return -1;
            }
            g_value_set_ulong(value, (gulong)val);
        } else
#endif
            if (PyLong_Check(obj))
                g_value_set_ulong(value, PyLong_AsUnsignedLong(obj));
            else
                return -1;
        break;
    case G_TYPE_INT64:
        g_value_set_int64(value, PyLong_AsLongLong(obj));
        break;
    case G_TYPE_UINT64:
#if PY_VERSION_HEX < 0x03000000
        if (PyInt_Check(obj)) {
            long v = PyInt_AsLong(obj);
            if (v < 0) {
                PyErr_SetString(PyExc_OverflowError, "negative value not allowed for uint64 property");
                return -1;
            }
            g_value_set_uint64(value, v);
        } else
#endif
            if (PyLong_Check(obj))
                g_value_set_uint64(value, PyLong_AsUnsignedLongLong(obj));
            else
                return -1;
        break;
    case G_TYPE_ENUM:
    {
        gint val = 0;
        if (pyg_enum_get_value(G_VALUE_TYPE(value), obj, &val) < 0) {
            return -1;
        }
        g_value_set_enum(value, val);
    }
    break;
    case G_TYPE_FLAGS:
    {
        guint val = 0;
        if (pyg_flags_get_value(G_VALUE_TYPE(value), obj, &val) < 0) {
            return -1;
        }
        g_value_set_flags(value, val);
    }
    break;
    case G_TYPE_FLOAT:
        g_value_set_float(value, PyFloat_AsDouble(obj));
        break;
    case G_TYPE_DOUBLE:
        g_value_set_double(value, PyFloat_AsDouble(obj));
        break;
    case G_TYPE_STRING:
        if (obj == Py_None) {
            g_value_set_string(value, NULL);
        } else {
            PyObject* tmp_str = PyObject_Str(obj);
            if (tmp_str == NULL) {
                PyErr_Clear();
                if (PyUnicode_Check(obj)) {
                    tmp = PyUnicode_AsUTF8String(obj);
                    g_value_set_string(value, PYGLIB_PyBytes_AsString(tmp));
                    Py_DECREF(tmp);
                } else {
                    PyErr_SetString(PyExc_TypeError, "Expected string");
                    return -1;
                }
            } else {
#if PY_VERSION_HEX < 0x03000000
                g_value_set_string(value, PyString_AsString(tmp_str));
#else
                tmp = PyUnicode_AsUTF8String(tmp_str);
                g_value_set_string(value, PyBytes_AsString(tmp));
                Py_DECREF(tmp);
#endif
            }
            Py_XDECREF(tmp_str);
        }
        break;
    case G_TYPE_POINTER:
        if (obj == Py_None)
            g_value_set_pointer(value, NULL);
        else if (PyObject_TypeCheck(obj, &PyGPointer_Type) &&
                G_VALUE_HOLDS(value, ((PyGPointer *)obj)->gtype))
            g_value_set_pointer(value, pyg_pointer_get(obj, gpointer));
        else if (PYGLIB_CPointer_Check(obj))
            g_value_set_pointer(value, PYGLIB_CPointer_GetPointer(obj, NULL));
        else if (G_VALUE_HOLDS_GTYPE (value))
            g_value_set_gtype (value, pyg_type_from_object (obj));
        else {
            PyErr_SetString(PyExc_TypeError, "Expected pointer");
            return -1;
        }
        break;
    case G_TYPE_BOXED: {
        PyGTypeMarshal *bm;
        gboolean holds_value_array;

        G_GNUC_BEGIN_IGNORE_DEPRECATIONS
        holds_value_array = G_VALUE_HOLDS(value, G_TYPE_VALUE_ARRAY);
        G_GNUC_END_IGNORE_DEPRECATIONS

        if (obj == Py_None)
            g_value_set_boxed(value, NULL);
        else if (G_VALUE_HOLDS(value, PY_TYPE_OBJECT))
            g_value_set_boxed(value, obj);
        else if (PyObject_TypeCheck(obj, &PyGBoxed_Type) &&
                G_VALUE_HOLDS(value, ((PyGBoxed *)obj)->gtype))
            g_value_set_boxed(value, pyg_boxed_get(obj, gpointer));
        else if (G_VALUE_HOLDS(value, G_TYPE_VALUE)) {
            GType type;
            GValue *n_value;

            type = pyg_type_from_object((PyObject*)Py_TYPE(obj));
            if (G_UNLIKELY (! type)) {
                return -1;
            }
            n_value = g_new0 (GValue, 1);
            g_value_init (n_value, type);
            g_value_take_boxed (value, n_value);
            return pyg_value_from_pyobject_with_error (n_value, obj);
        }
        else if (PySequence_Check(obj) && holds_value_array)
            return pyg_value_array_from_pyobject(value, obj, NULL);

        else if (PySequence_Check(obj) &&
                G_VALUE_HOLDS(value, G_TYPE_ARRAY))
            return pyg_array_from_pyobject(value, obj);
        else if (PYGLIB_PyUnicode_Check(obj) &&
                G_VALUE_HOLDS(value, G_TYPE_GSTRING)) {
            GString *string;
            char *buffer;
            Py_ssize_t len;
            if (PYGLIB_PyUnicode_AsStringAndSize(obj, &buffer, &len))
                return -1;
            string = g_string_new_len(buffer, len);
            g_value_set_boxed(value, string);
            g_string_free (string, TRUE);
            break;
        }
        else if ((bm = pyg_type_lookup(G_VALUE_TYPE(value))) != NULL)
            return bm->tovalue(value, obj);
        else if (PYGLIB_CPointer_Check(obj))
            g_value_set_boxed(value, PYGLIB_CPointer_GetPointer(obj, NULL));
        else {
            PyErr_SetString(PyExc_TypeError, "Expected Boxed");
            return -1;
        }
        break;
    }
    case G_TYPE_PARAM:
        /* we need to support both the wrapped _gobject.GParamSpec and the GI
         * GObject.ParamSpec */
        if (G_IS_PARAM_SPEC (pygobject_get (obj)))
            g_value_set_param(value, G_PARAM_SPEC (pygobject_get (obj)));
        else if (pyg_param_spec_check (obj))
            g_value_set_param(value, PYGLIB_CPointer_GetPointer(obj, NULL));
        else {
            PyErr_SetString(PyExc_TypeError, "Expected ParamSpec");
            return -1;
        }
        break;
    case G_TYPE_OBJECT:
        if (obj == Py_None) {
            g_value_set_object(value, NULL);
        } else if (PyObject_TypeCheck(obj, &PyGObject_Type) &&
                G_TYPE_CHECK_INSTANCE_TYPE(pygobject_get(obj),
                        G_VALUE_TYPE(value))) {
            g_value_set_object(value, pygobject_get(obj));
        } else {
            PyErr_SetString(PyExc_TypeError, "Expected GObject");
            return -1;
        }
        break;
    case G_TYPE_VARIANT:
    {
        if (obj == Py_None)
            g_value_set_variant(value, NULL);
        else if (pyg_type_from_object_strict(obj, FALSE) == G_TYPE_VARIANT)
            g_value_set_variant(value, pyg_boxed_get(obj, GVariant));
        else {
            PyErr_SetString(PyExc_TypeError, "Expected Variant");
            return -1;
        }
        break;
    }
    default:
    {
        PyGTypeMarshal *bm;
        if ((bm = pyg_type_lookup(G_VALUE_TYPE(value))) != NULL) {
            return bm->tovalue(value, obj);
        } else {
            PyErr_SetString(PyExc_TypeError, "Unknown value type");
            return -1;
        }
        break;
    }
    }

    /* If an error occurred, unset the GValue but don't clear the Python error. */
    if (PyErr_Occurred()) {
        g_value_unset(value);
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
pyg_value_from_pyobject(GValue *value, PyObject *obj)
{
    int res = pyg_value_from_pyobject_with_error (value, obj);

    if (PyErr_Occurred()) {
        PyErr_Clear();
        return -1;
    }
    return res;
}

/**
 * pygi_value_to_py_basic_type:
 * @value: the GValue object.
 *
 * This function creates/returns a Python wrapper object that
 * represents the GValue passed as an argument limited to supporting basic types
 * like ints, bools, and strings.
 *
 * Returns: a PyObject representing the value.
 */
PyObject *
pygi_value_to_py_basic_type (const GValue *value, GType fundamental)
{
    switch (fundamental) {
    case G_TYPE_CHAR:
        return PYGLIB_PyLong_FromLong (g_value_get_schar (value));

    case G_TYPE_UCHAR:
        return PYGLIB_PyLong_FromLong (g_value_get_uchar (value));

    case G_TYPE_BOOLEAN: {
        return PyBool_FromLong(g_value_get_boolean(value));
    }
    case G_TYPE_INT:
        return PYGLIB_PyLong_FromLong(g_value_get_int(value));
    case G_TYPE_UINT:
    {
        /* in Python, the Int object is backed by a long.  If a
	       long can hold the whole value of an unsigned int, use
	       an Int.  Otherwise, use a Long object to avoid overflow.
	       This matches the ULongArg behavior in codegen/argtypes.h */
#if (G_MAXUINT <= G_MAXLONG)
        return PYGLIB_PyLong_FromLong((glong) g_value_get_uint(value));
#else
        return PyLong_FromUnsignedLong((gulong) g_value_get_uint(value));
#endif
    }
    case G_TYPE_LONG:
        return PYGLIB_PyLong_FromLong(g_value_get_long(value));
    case G_TYPE_ULONG:
    {
        gulong val = g_value_get_ulong(value);

        if (val <= G_MAXLONG)
            return PYGLIB_PyLong_FromLong((glong) val);
        else
            return PyLong_FromUnsignedLong(val);
    }
    case G_TYPE_INT64:
    {
        gint64 val = g_value_get_int64(value);

        if (G_MINLONG <= val && val <= G_MAXLONG)
            return PYGLIB_PyLong_FromLong((glong) val);
        else
            return PyLong_FromLongLong(val);
    }
    case G_TYPE_UINT64:
    {
        guint64 val = g_value_get_uint64(value);

        if (val <= G_MAXLONG)
            return PYGLIB_PyLong_FromLong((glong) val);
        else
            return PyLong_FromUnsignedLongLong(val);
    }
    case G_TYPE_ENUM:
        return pyg_enum_from_gtype(G_VALUE_TYPE(value), g_value_get_enum(value));
    case G_TYPE_FLAGS:
        return pyg_flags_from_gtype(G_VALUE_TYPE(value), g_value_get_flags(value));
    case G_TYPE_FLOAT:
        return PyFloat_FromDouble(g_value_get_float(value));
    case G_TYPE_DOUBLE:
        return PyFloat_FromDouble(g_value_get_double(value));
    case G_TYPE_STRING:
    {
        const gchar *str = g_value_get_string(value);

        if (str)
            return PYGLIB_PyUnicode_FromString(str);
        Py_INCREF(Py_None);
        return Py_None;
    }
    default:
        return NULL;
    }
}

/**
 * pygi_value_to_py_structured_type:
 * @value: the GValue object.
 * @copy_boxed: true if boxed values should be copied.
 *
 * This function creates/returns a Python wrapper object that
 * represents the GValue passed as an argument.
 *
 * Returns: a PyObject representing the value.
 */
PyObject *
pygi_value_to_py_structured_type (const GValue *value, GType fundamental, gboolean copy_boxed)
{
    switch (fundamental) {
    case G_TYPE_INTERFACE:
        if (g_type_is_a(G_VALUE_TYPE(value), G_TYPE_OBJECT))
            return pygobject_new(g_value_get_object(value));
        else
            break;

    case G_TYPE_POINTER:
        if (G_VALUE_HOLDS_GTYPE (value))
            return pyg_type_wrapper_new (g_value_get_gtype (value));
        else
            return pyg_pointer_new(G_VALUE_TYPE(value),
                    g_value_get_pointer(value));
    case G_TYPE_BOXED: {
        PyGTypeMarshal *bm;
        gboolean holds_value_array;

        G_GNUC_BEGIN_IGNORE_DEPRECATIONS
        holds_value_array = G_VALUE_HOLDS(value, G_TYPE_VALUE_ARRAY);
        G_GNUC_END_IGNORE_DEPRECATIONS

        if (G_VALUE_HOLDS(value, PY_TYPE_OBJECT)) {
            PyObject *ret = (PyObject *)g_value_dup_boxed(value);
            if (ret == NULL) {
                Py_INCREF(Py_None);
                return Py_None;
            }
            return ret;
        } else if (G_VALUE_HOLDS(value, G_TYPE_VALUE)) {
            GValue *n_value = g_value_get_boxed (value);
            return pyg_value_as_pyobject(n_value, copy_boxed);
        } else if (holds_value_array) {
            GValueArray *array = (GValueArray *) g_value_get_boxed(value);
            PyObject *ret = PyList_New(array->n_values);
            int i;
            for (i = 0; i < array->n_values; ++i)
                PyList_SET_ITEM(ret, i, pyg_value_as_pyobject
                        (array->values + i, copy_boxed));
            return ret;
        } else if (G_VALUE_HOLDS(value, G_TYPE_GSTRING)) {
            GString *string = (GString *) g_value_get_boxed(value);
            PyObject *ret = PYGLIB_PyUnicode_FromStringAndSize(string->str, string->len);
            return ret;
        }
        bm = pyg_type_lookup(G_VALUE_TYPE(value));
        if (bm) {
            return bm->fromvalue(value);
        } else {
            if (copy_boxed)
                return pyg_boxed_new(G_VALUE_TYPE(value),
                        g_value_get_boxed(value), TRUE, TRUE);
            else
                return pyg_boxed_new(G_VALUE_TYPE(value),
                        g_value_get_boxed(value),FALSE,FALSE);
        }
    }
    case G_TYPE_PARAM:
        return pyg_param_spec_new(g_value_get_param(value));
    case G_TYPE_OBJECT:
        return pygobject_new(g_value_get_object(value));
    case G_TYPE_VARIANT:
    {
        GVariant *v = g_value_get_variant(value);
        if (v == NULL) {
            Py_INCREF(Py_None);
            return Py_None;
        }
        return pyg_boxed_new(G_TYPE_VARIANT, g_variant_ref(v), FALSE, FALSE);
    }
    default:
    {
        PyGTypeMarshal *bm;
        if ((bm = pyg_type_lookup(G_VALUE_TYPE(value))))
            return bm->fromvalue(value);
        break;
    }
    }

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
 * Returns: a PyObject representing the value.
 */
PyObject *
pyg_value_as_pyobject (const GValue *value, gboolean copy_boxed)
{
    gchar buf[128];
    PyObject *pyobj;
    GType fundamental = G_TYPE_FUNDAMENTAL (G_VALUE_TYPE (value));

    /* HACK: special case char and uchar to return PyBytes intstead of integers
     * in the general case. Property access will skip this by calling
     * pygi_value_to_py_basic_type() directly.
     * See: https://bugzilla.gnome.org/show_bug.cgi?id=733893 */
    if (fundamental == G_TYPE_CHAR) {
        gint8 val = g_value_get_schar(value);
        return PYGLIB_PyUnicode_FromStringAndSize ((char *)&val, 1);
    } else if (fundamental == G_TYPE_UCHAR) {
        guint8 val = g_value_get_uchar(value);
        return PYGLIB_PyBytes_FromStringAndSize ((char *)&val, 1);
    }

    pyobj = pygi_value_to_py_basic_type (value, fundamental);
    if (pyobj) {
        return pyobj;
    }

    pyobj = pygi_value_to_py_structured_type (value, fundamental, copy_boxed);
    if (pyobj) {
        return pyobj;
    }

    g_snprintf(buf, sizeof(buf), "unknown type %s",
               g_type_name(G_VALUE_TYPE(value)));
    PyErr_SetString(PyExc_TypeError, buf);
    return NULL;
}


int
pyg_param_gvalue_from_pyobject(GValue* value,
                               PyObject* py_obj,
			       const GParamSpec* pspec)
{
    if (G_IS_PARAM_SPEC_UNICHAR(pspec)) {
	gunichar u;

	if (!pyg_pyobj_to_unichar_conv(py_obj, &u)) {
	    PyErr_Clear();
	    return -1;
	}
        g_value_set_uint(value, u);
	return 0;
    }
    else if (G_IS_PARAM_SPEC_VALUE_ARRAY(pspec))
	return pyg_value_array_from_pyobject(value, py_obj,
					     G_PARAM_SPEC_VALUE_ARRAY(pspec));
    else {
	return pyg_value_from_pyobject(value, py_obj);
    }
}

PyObject*
pyg_param_gvalue_as_pyobject(const GValue* gvalue,
                             gboolean copy_boxed,
			     const GParamSpec* pspec)
{
    if (G_IS_PARAM_SPEC_UNICHAR(pspec)) {
	gunichar u;
	Py_UNICODE uni_buffer[2] = { 0, 0 };

	u = g_value_get_uint(gvalue);
	uni_buffer[0] = u;
	return PyUnicode_FromUnicode(uni_buffer, 1);
    }
    else {
	return pyg_value_as_pyobject(gvalue, copy_boxed);
    }
}

PyObject *
pyg_strv_from_gvalue(const GValue *value)
{
    gchar    **argv = (gchar **) g_value_get_boxed(value);
    int        argc = 0, i;
    PyObject  *py_argv;

    if (argv) {
        while (argv[argc])
            argc++;
    }
    py_argv = PyList_New(argc);
    for (i = 0; i < argc; ++i)
	PyList_SET_ITEM(py_argv, i, PYGLIB_PyUnicode_FromString(argv[i]));
    return py_argv;
}

int
pyg_strv_to_gvalue(GValue *value, PyObject *obj)
{
    Py_ssize_t argc, i;
    gchar **argv;

    if (!(PyTuple_Check(obj) || PyList_Check(obj)))
        return -1;

    argc = PySequence_Length(obj);
    for (i = 0; i < argc; ++i)
	if (!PYGLIB_PyUnicode_Check(PySequence_Fast_GET_ITEM(obj, i)))
	    return -1;
    argv = g_new(gchar *, argc + 1);
    for (i = 0; i < argc; ++i)
	argv[i] = g_strdup(PYGLIB_PyUnicode_AsString(PySequence_Fast_GET_ITEM(obj, i)));
    argv[i] = NULL;
    g_value_take_boxed(value, argv);
    return 0;
}
