/* -*- Mode: C; c-basic-offset: 4 -*-
 * pygtk- Python bindings for the GTK toolkit.
 * Copyright (C) 1998-2003  James Henstridge
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

#include "pygi-type.h"

#include "pygboxed.h"
#include "pygenum.h"
#include "pygflags.h"
#include "pygbasictype.h"
#include "pyginterface.h"
#include "pygobject-object.h"
#include "pygpointer.h"
#include "pygutil.h"
#include "pygi-repository.h"

PyObject *
pygi_type_import_by_name (const char *namespace_, const char *name)
{
    gchar *module_name;
    PyObject *py_module;
    PyObject *py_object;

    module_name = g_strconcat ("gi.repository.", namespace_, NULL);

    py_module = PyImport_ImportModule (module_name);

    g_free (module_name);

    if (py_module == NULL) {
        return NULL;
    }

    py_object = PyObject_GetAttrString (py_module, name);

    Py_DECREF (py_module);

    return py_object;
}

PyObject *
pygi_type_import_by_g_type (GType g_type)
{
    GIBaseInfo *info;
    PyObject *type;

    info = pygi_repository_find_by_gtype (g_type);
    if (info == NULL) {
        return NULL;
    }

    type = pygi_type_import_by_gi_info (info);
    gi_base_info_unref (info);

    return type;
}

PyObject *
pygi_type_import_by_gi_info (GIBaseInfo *info)
{
    return pygi_type_import_by_name (gi_base_info_get_namespace (info),
                                     gi_base_info_get_name (info));
}

/* -------------- __gtype__ objects ---------------------------- */

typedef struct {
    PyObject_HEAD
    GType type;
} PyGTypeWrapper;

PYG_DEFINE_TYPE ("gobject.GType", PyGTypeWrapper_Type, PyGTypeWrapper);

static PyObject *
generic_gsize_richcompare (gsize a, gsize b, int op)
{
    PyObject *res;

    switch (op) {
    case Py_EQ:
        res = (a == b) ? Py_True : Py_False;
        Py_INCREF (res);
        break;

    case Py_NE:
        res = (a != b) ? Py_True : Py_False;
        Py_INCREF (res);
        break;

    case Py_LT:
        res = (a < b) ? Py_True : Py_False;
        Py_INCREF (res);
        break;

    case Py_LE:
        res = (a <= b) ? Py_True : Py_False;
        Py_INCREF (res);
        break;

    case Py_GT:
        res = (a > b) ? Py_True : Py_False;
        Py_INCREF (res);
        break;

    case Py_GE:
        res = (a >= b) ? Py_True : Py_False;
        Py_INCREF (res);
        break;

    default:
        g_assert_not_reached ();
    }

    return res;
}

static PyObject *
pyg_type_wrapper_richcompare (PyObject *self, PyObject *other, int op)
{
    if (Py_TYPE (self) == Py_TYPE (other)
        && Py_TYPE (self) == &PyGTypeWrapper_Type)
        return generic_gsize_richcompare (((PyGTypeWrapper *)self)->type,
                                          ((PyGTypeWrapper *)other)->type, op);
    else {
        Py_RETURN_NOTIMPLEMENTED;
    }
}

static long
pyg_type_wrapper_hash (PyGTypeWrapper *self)
{
    return (long)self->type;
}

static PyObject *
pyg_type_wrapper_repr (PyGTypeWrapper *self)
{
    char buf[80];
    const gchar *name = g_type_name (self->type);

    g_snprintf (buf, sizeof (buf), "<GType %s (%lu)>", name ? name : "invalid",
                (unsigned long int)self->type);
    return PyUnicode_FromString (buf);
}

static void
pyg_type_wrapper_dealloc (PyGTypeWrapper *self)
{
    PyObject_Free (self);
}

static GQuark
_pyg_type_key (GType type)
{
    GQuark key;

    if (g_type_is_a (type, G_TYPE_INTERFACE)) {
        key = pyginterface_type_key;
    } else if (g_type_is_a (type, G_TYPE_ENUM)) {
        key = pygenum_class_key;
    } else if (g_type_is_a (type, G_TYPE_FLAGS)) {
        key = pygflags_class_key;
    } else if (g_type_is_a (type, G_TYPE_POINTER)) {
        key = pygpointer_class_key;
    } else if (g_type_is_a (type, G_TYPE_BOXED)) {
        key = pygboxed_type_key;
    } else {
        key = pygobject_class_key;
    }

    return key;
}

static PyObject *
_wrap_g_type_wrapper__get_pytype (PyGTypeWrapper *self, void *closure)
{
    GQuark key;
    PyObject *py_type;

    key = _pyg_type_key (self->type);

    py_type = g_type_get_qdata (self->type, key);
    if (!py_type) py_type = Py_None;

    return Py_NewRef (py_type);
}

static int
_wrap_g_type_wrapper__set_pytype (PyGTypeWrapper *self, PyObject *value,
                                  void *closure)
{
    GQuark key;
    PyObject *py_type;

    key = _pyg_type_key (self->type);

    py_type = g_type_get_qdata (self->type, key);
    Py_CLEAR (py_type);
    if (Py_IsNone (value))
        g_type_set_qdata (self->type, key, NULL);
    else if (PyType_Check (value)) {
        Py_INCREF (value);
        g_type_set_qdata (self->type, key, value);
    } else {
        PyErr_SetString (PyExc_TypeError,
                         "Value must be None or a type object");
        return -1;
    }

    return 0;
}

static PyObject *
_wrap_g_type_wrapper__get_name (PyGTypeWrapper *self, void *closure)
{
    const char *name = g_type_name (self->type);
    return PyUnicode_FromString (name ? name : "invalid");
}

static PyObject *
_wrap_g_type_wrapper__get_parent (PyGTypeWrapper *self, void *closure)
{
    return pyg_type_wrapper_new (g_type_parent (self->type));
}

static PyObject *
_wrap_g_type_wrapper__get_fundamental (PyGTypeWrapper *self, void *closure)
{
    return pyg_type_wrapper_new (g_type_fundamental (self->type));
}

static PyObject *
_wrap_g_type_wrapper__get_children (PyGTypeWrapper *self, void *closure)
{
    guint n_children, i;
    GType *children;
    PyObject *retval;

    children = g_type_children (self->type, &n_children);

    retval = PyList_New (n_children);
    for (i = 0; i < n_children; i++)
        PyList_SetItem (retval, i, pyg_type_wrapper_new (children[i]));
    g_free (children);

    return retval;
}

static PyObject *
_wrap_g_type_wrapper__get_interfaces (PyGTypeWrapper *self, void *closure)
{
    guint n_interfaces, i;
    GType *interfaces;
    PyObject *retval;

    interfaces = g_type_interfaces (self->type, &n_interfaces);

    retval = PyList_New (n_interfaces);
    for (i = 0; i < n_interfaces; i++)
        PyList_SetItem (retval, i, pyg_type_wrapper_new (interfaces[i]));
    g_free (interfaces);

    return retval;
}

static PyObject *
_wrap_g_type_wrapper__get_depth (PyGTypeWrapper *self, void *closure)
{
    return pyg_guint_to_py (g_type_depth (self->type));
}

static PyGetSetDef _PyGTypeWrapper_getsets[] = {
    { "pytype", (getter)_wrap_g_type_wrapper__get_pytype,
      (setter)_wrap_g_type_wrapper__set_pytype },
    { "name", (getter)_wrap_g_type_wrapper__get_name, (setter)0 },
    { "fundamental", (getter)_wrap_g_type_wrapper__get_fundamental,
      (setter)0 },
    { "parent", (getter)_wrap_g_type_wrapper__get_parent, (setter)0 },
    { "children", (getter)_wrap_g_type_wrapper__get_children, (setter)0 },
    { "interfaces", (getter)_wrap_g_type_wrapper__get_interfaces, (setter)0 },
    { "depth", (getter)_wrap_g_type_wrapper__get_depth, (setter)0 },
    { NULL, (getter)0, (setter)0 },
};

static PyObject *
_wrap_g_type_is_interface (PyGTypeWrapper *self)
{
    return pyg_gboolean_to_py (G_TYPE_IS_INTERFACE (self->type));
}

static PyObject *
_wrap_g_type_is_classed (PyGTypeWrapper *self)
{
    return pyg_gboolean_to_py (G_TYPE_IS_CLASSED (self->type));
}

static PyObject *
_wrap_g_type_is_instantiatable (PyGTypeWrapper *self)
{
    return pyg_gboolean_to_py (G_TYPE_IS_INSTANTIATABLE (self->type));
}

static PyObject *
_wrap_g_type_is_derivable (PyGTypeWrapper *self)
{
    return pyg_gboolean_to_py (G_TYPE_IS_DERIVABLE (self->type));
}

static PyObject *
_wrap_g_type_is_deep_derivable (PyGTypeWrapper *self)
{
    return pyg_gboolean_to_py (G_TYPE_IS_DEEP_DERIVABLE (self->type));
}

static PyObject *
_wrap_g_type_is_abstract (PyGTypeWrapper *self)
{
    return pyg_gboolean_to_py (G_TYPE_IS_ABSTRACT (self->type));
}

static PyObject *
_wrap_g_type_is_value_abstract (PyGTypeWrapper *self)
{
    return pyg_gboolean_to_py (G_TYPE_IS_VALUE_ABSTRACT (self->type));
}

static PyObject *
_wrap_g_type_is_value_type (PyGTypeWrapper *self)
{
    return pyg_gboolean_to_py (G_TYPE_IS_VALUE_TYPE (self->type));
}

static PyObject *
_wrap_g_type_has_value_table (PyGTypeWrapper *self)
{
    return pyg_gboolean_to_py (G_TYPE_HAS_VALUE_TABLE (self->type));
}

static PyObject *
_wrap_g_type_from_name (PyGTypeWrapper *_, PyObject *args)
{
    char *type_name;
    GType type;

    if (!PyArg_ParseTuple (args, "s:GType.from_name", &type_name)) return NULL;

    type = g_type_from_name (type_name);
    if (type == 0) {
        PyErr_SetString (PyExc_RuntimeError, "unknown type name");
        return NULL;
    }

    return pyg_type_wrapper_new (type);
}

static PyObject *
_wrap_g_type_is_a (PyGTypeWrapper *self, PyObject *args)
{
    PyObject *gparent;
    GType parent;

    if (!PyArg_ParseTuple (args, "O:GType.is_a", &gparent))
        return NULL;
    else if ((parent = pyg_type_from_object (gparent)) == 0)
        return NULL;

    return pyg_gboolean_to_py (g_type_is_a (self->type, parent));
}

static PyMethodDef _PyGTypeWrapper_methods[] = {
    { "is_interface", (PyCFunction)_wrap_g_type_is_interface, METH_NOARGS },
    { "is_classed", (PyCFunction)_wrap_g_type_is_classed, METH_NOARGS },
    { "is_instantiatable", (PyCFunction)_wrap_g_type_is_instantiatable,
      METH_NOARGS },
    { "is_derivable", (PyCFunction)_wrap_g_type_is_derivable, METH_NOARGS },
    { "is_deep_derivable", (PyCFunction)_wrap_g_type_is_deep_derivable,
      METH_NOARGS },
    { "is_abstract", (PyCFunction)_wrap_g_type_is_abstract, METH_NOARGS },
    { "is_value_abstract", (PyCFunction)_wrap_g_type_is_value_abstract,
      METH_NOARGS },
    { "is_value_type", (PyCFunction)_wrap_g_type_is_value_type, METH_NOARGS },
    { "has_value_table", (PyCFunction)_wrap_g_type_has_value_table,
      METH_NOARGS },
    { "from_name", (PyCFunction)_wrap_g_type_from_name,
      METH_VARARGS | METH_STATIC },
    { "is_a", (PyCFunction)_wrap_g_type_is_a, METH_VARARGS },
    { NULL, 0, 0 },
};

static int
pyg_type_wrapper_init (PyGTypeWrapper *self, PyObject *args, PyObject *kwargs)
{
    static char *kwlist[] = { "object", NULL };
    PyObject *py_object;
    GType type;

    if (!PyArg_ParseTupleAndKeywords (args, kwargs, "O:GType.__init__", kwlist,
                                      &py_object))
        return -1;

    if (!(type = pyg_type_from_object (py_object))) return -1;

    self->type = type;

    return 0;
}

/**
 * pyg_type_wrapper_new:
 * type: a GType
 *
 * Creates a Python wrapper for a GType.
 *
 * Returns: the Python wrapper.
 */
PyObject *
pyg_type_wrapper_new (GType type)
{
    PyGTypeWrapper *self;

    g_assert (Py_TYPE (&PyGTypeWrapper_Type) != NULL);
    self =
        (PyGTypeWrapper *)PyObject_New (PyGTypeWrapper, &PyGTypeWrapper_Type);
    if (self == NULL) return NULL;

    self->type = type;
    return (PyObject *)self;
}

/**
 * pyg_type_from_object_strict:
 * obj: a Python object
 * strict: if set to TRUE, raises an exception if it can't perform the
 *         conversion
 *
 * converts a python object to a GType.  If strict is set, raises an
 * exception if it can't perform the conversion, otherwise returns
 * PY_TYPE_OBJECT.
 *
 * Returns: the corresponding GType, or 0 on error.
 */
GType
pyg_type_from_object_strict (PyObject *obj, gboolean strict)
{
    PyObject *gtype;
    GType type;

    /* NULL check */
    if (!obj) {
        PyErr_SetString (PyExc_TypeError, "can't get type from NULL object");
        return 0;
    }

    /* map some standard types to primitive GTypes ... */
    if (Py_IsNone (obj)) return G_TYPE_NONE;
    if (PyType_Check (obj)) {
        PyTypeObject *tp = (PyTypeObject *)obj;

        if (tp == &PyLong_Type)
            return G_TYPE_INT;
        else if (tp == &PyBool_Type)
            return G_TYPE_BOOLEAN;
        else if (tp == &PyFloat_Type)
            return G_TYPE_DOUBLE;
        else if (tp == &PyUnicode_Type)
            return G_TYPE_STRING;
        else if (tp == &PyBaseObject_Type)
            return PY_TYPE_OBJECT;
    }

    if (Py_TYPE (obj) == &PyGTypeWrapper_Type) {
        return ((PyGTypeWrapper *)obj)->type;
    }

    /* handle strings */
    if (PyUnicode_Check (obj)) {
        gchar *name = PyUnicode_AsUTF8 (obj);

        type = g_type_from_name (name);
        if (type != 0) {
            return type;
        }
    }

    /* finally, look for a __gtype__ attribute on the object */
    gtype = PyObject_GetAttrString (obj, "__gtype__");

    if (gtype) {
        if (Py_TYPE (gtype) == &PyGTypeWrapper_Type) {
            type = ((PyGTypeWrapper *)gtype)->type;
            Py_DECREF (gtype);
            return type;
        }
        Py_DECREF (gtype);
    }

    PyErr_Clear ();

    /* Some API like those that take GValues can hold a python object as
     * a pointer.  This is potentially dangerous becuase everything is
     * passed in as a PyObject so we can't actually type check it.  Only
     * fallback to PY_TYPE_OBJECT if strict checking is disabled
     */
    if (!strict) return PY_TYPE_OBJECT;

    PyErr_SetString (PyExc_TypeError, "could not get typecode from object");
    return 0;
}

/**
 * pyg_type_from_object:
 * obj: a Python object
 *
 * converts a python object to a GType.  Raises an exception if it
 * can't perform the conversion.
 *
 * Returns: the corresponding GType, or 0 on error.
 */
GType
pyg_type_from_object (PyObject *obj)
{
    /* Legacy call always defaults to strict type checking */
    return pyg_type_from_object_strict (obj, TRUE);
}


/* ----- __doc__ descriptor for GObject and GInterface ----- */

static void
object_doc_dealloc (PyObject *self)
{
    PyObject_Free (self);
}

/* append information about signals of a particular gtype */
static void
add_signal_docs (GType gtype, GString *string)
{
    GTypeClass *class = NULL;
    guint *signal_ids, n_ids = 0, i;

    if (G_TYPE_IS_CLASSED (gtype)) class = g_type_class_ref (gtype);
    signal_ids = g_signal_list_ids (gtype, &n_ids);

    if (n_ids > 0) {
        g_string_append_printf (string, "Signals from %s:\n",
                                g_type_name (gtype));

        for (i = 0; i < n_ids; i++) {
            GSignalQuery query;
            guint j;

            g_signal_query (signal_ids[i], &query);

            g_string_append (string, "  ");
            g_string_append (string, query.signal_name);
            g_string_append (string, " (");
            for (j = 0; j < query.n_params; j++) {
                g_string_append (string, g_type_name (query.param_types[j]));
                if (j != query.n_params - 1) g_string_append (string, ", ");
            }
            g_string_append (string, ")");
            if (query.return_type && query.return_type != G_TYPE_NONE) {
                g_string_append (string, " -> ");
                g_string_append (string, g_type_name (query.return_type));
            }
            g_string_append (string, "\n");
        }
        g_free (signal_ids);
        g_string_append (string, "\n");
    }
    if (class) g_type_class_unref (class);
}

static void
add_property_docs (GType gtype, GString *string)
{
    GObjectClass *class;
    GParamSpec **props;
    guint n_props = 0, i;
    gboolean has_prop = FALSE;
    const gchar *blurb = NULL;

    class = g_type_class_ref (gtype);
    props = g_object_class_list_properties (class, &n_props);

    for (i = 0; i < n_props; i++) {
        if (props[i]->owner_type != gtype)
            continue; /* these are from a parent type */

        /* print out the heading first */
        if (!has_prop) {
            g_string_append_printf (string, "Properties from %s:\n",
                                    g_type_name (gtype));
            has_prop = TRUE;
        }
        g_string_append_printf (string, "  %s -> %s: %s\n",
                                g_param_spec_get_name (props[i]),
                                g_type_name (props[i]->value_type),
                                g_param_spec_get_nick (props[i]));

        /* g_string_append_printf crashes on win32 if the third
	   argument is NULL. */
        blurb = g_param_spec_get_blurb (props[i]);
        if (blurb) g_string_append_printf (string, "    %s\n", blurb);
    }
    g_free (props);
    if (has_prop) g_string_append (string, "\n");
    g_type_class_unref (class);
}

static PyObject *
object_doc_descr_get (PyObject *self, PyObject *obj, PyObject *type)
{
    GType gtype = 0;
    GString *string;
    PyObject *pystring;

    if (obj && pygobject_check (obj, &PyGObject_Type)) {
        gtype = G_OBJECT_TYPE (pygobject_get (obj));
        if (!gtype)
            PyErr_SetString (PyExc_RuntimeError, "could not get object type");
    } else {
        gtype = pyg_type_from_object (type);
    }
    if (!gtype) return NULL;

    string = g_string_new_len (NULL, 512);

    if (g_type_is_a (gtype, G_TYPE_INTERFACE))
        g_string_append_printf (string, "Interface %s\n\n",
                                g_type_name (gtype));
    else if (g_type_is_a (gtype, G_TYPE_OBJECT))
        g_string_append_printf (string, "Object %s\n\n", g_type_name (gtype));
    else
        g_string_append_printf (string, "%s\n\n", g_type_name (gtype));

    if (((PyTypeObject *)type)->tp_doc)
        g_string_append_printf (string, "%s\n\n",
                                ((PyTypeObject *)type)->tp_doc);

    if (g_type_is_a (gtype, G_TYPE_OBJECT)) {
        GType parent = G_TYPE_OBJECT;
        GArray *parents = g_array_new (FALSE, FALSE, sizeof (GType));
        int iparent;

        while (parent) {
            g_array_append_val (parents, parent);
            parent = g_type_next_base (gtype, parent);
        }

        for (iparent = parents->len - 1; iparent >= 0; --iparent) {
            GType *interfaces;
            guint n_interfaces, i;

            parent = g_array_index (parents, GType, iparent);
            add_signal_docs (parent, string);
            add_property_docs (parent, string);

            /* add docs for implemented interfaces */
            interfaces = g_type_interfaces (parent, &n_interfaces);
            for (i = 0; i < n_interfaces; i++)
                add_signal_docs (interfaces[i], string);
            g_free (interfaces);
        }
        g_array_free (parents, TRUE);
    }

    pystring = PyUnicode_FromStringAndSize (string->str, string->len);
    g_string_free (string, TRUE);
    return pystring;
}

PYG_DEFINE_TYPE ("gobject.GObject.__doc__", PyGObjectDoc_Type, PyObject);

static gpointer
init_once_doc_descr (gpointer user_data)
{
    Py_SET_TYPE (&PyGObjectDoc_Type, &PyType_Type);
    if (PyType_Ready (&PyGObjectDoc_Type)) return NULL;

    return PyObject_New (PyObject, &PyGObjectDoc_Type);
}

/**
 * pyg_object_descr_doc_get:
 *
 * Returns an object intended to be the __doc__ attribute of GObject
 * wrappers.  When read in the context of the object it will return
 * some documentation about the signals and properties of the object.
 *
 * Returns: the descriptor.
 */
PyObject *
pyg_object_descr_doc_get (void)
{
    static GOnce once_doc_descr = G_ONCE_INIT;

    g_once (&once_doc_descr, init_once_doc_descr, NULL);

    return once_doc_descr.retval;
}


gboolean
pyg_gtype_is_custom (GType gtype)
{
    return g_type_get_qdata (gtype, pygobject_custom_key) != NULL;
}

/**
 * Returns 0 on success, or -1 and sets an exception.
 */
int
pygi_type_register_types (PyObject *d)
{
    PyGTypeWrapper_Type.tp_dealloc = (destructor)pyg_type_wrapper_dealloc;
    PyGTypeWrapper_Type.tp_richcompare = pyg_type_wrapper_richcompare;
    PyGTypeWrapper_Type.tp_repr = (reprfunc)pyg_type_wrapper_repr;
    PyGTypeWrapper_Type.tp_hash = (hashfunc)pyg_type_wrapper_hash;
    PyGTypeWrapper_Type.tp_flags = Py_TPFLAGS_DEFAULT;
    PyGTypeWrapper_Type.tp_methods = _PyGTypeWrapper_methods;
    PyGTypeWrapper_Type.tp_getset = _PyGTypeWrapper_getsets;
    PyGTypeWrapper_Type.tp_init = (initproc)pyg_type_wrapper_init;
    PyGTypeWrapper_Type.tp_alloc = PyType_GenericAlloc;
    PyGTypeWrapper_Type.tp_new = PyType_GenericNew;
    if (PyType_Ready (&PyGTypeWrapper_Type)) return -1;

    PyDict_SetItemString (d, "GType", (PyObject *)&PyGTypeWrapper_Type);

    /* This type lazily registered in pyg_object_descr_doc_get */
    PyGObjectDoc_Type.tp_dealloc = (destructor)object_doc_dealloc;
    PyGObjectDoc_Type.tp_flags = Py_TPFLAGS_DEFAULT;
    PyGObjectDoc_Type.tp_descr_get = (descrgetfunc)object_doc_descr_get;

    return 0;
}

/**
 * pygi_interface_type_tag:
 *
 * A simple way to map types often obtained by calling
 * `gi_type_info_get_interface` to an enum.
 * This makes for easier to read code.
 */
PyGIInterfaceTypeTag
pygi_interface_type_tag (GIBaseInfo *info)
{
    PyGIInterfaceTypeTag iface_type_tag;

    if (GI_IS_FLAGS_INFO (info)) {
        /* Check flags before enums: flags are a subtype of enum. */
        iface_type_tag = PYGI_INTERFACE_TYPE_TAG_FLAGS;
    } else if (GI_IS_ENUM_INFO (info)) {
        iface_type_tag = PYGI_INTERFACE_TYPE_TAG_ENUM;
    } else if (GI_IS_INTERFACE_INFO (info)) {
        iface_type_tag = PYGI_INTERFACE_TYPE_TAG_INTERFACE;
    } else if (GI_IS_OBJECT_INFO (info)) {
        iface_type_tag = PYGI_INTERFACE_TYPE_TAG_OBJECT;
    } else if (GI_IS_STRUCT_INFO (info)) {
        iface_type_tag = PYGI_INTERFACE_TYPE_TAG_STRUCT;
    } else if (GI_IS_UNION_INFO (info)) {
        iface_type_tag = PYGI_INTERFACE_TYPE_TAG_UNION;
    } else if (GI_IS_CALLBACK_INFO (info)) {
        iface_type_tag = PYGI_INTERFACE_TYPE_TAG_CALLBACK;
    } else {
        g_critical ("Type %s is not handled",
                    g_type_name_from_instance ((GTypeInstance *)info));
        g_assert_not_reached ();
    }

    return iface_type_tag;
}
