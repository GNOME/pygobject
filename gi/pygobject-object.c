/* -*- Mode: C; c-basic-offset: 4 -*-
 * pygtk- Python bindings for the GTK toolkit.
 * Copyright (C) 1998-2003  James Henstridge
 *
 *   pygobject.c: wrapper for the GObject type.
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

#include "gimodule.h"
#include "pygboxed.h"
#include "pygi-basictype.h"
#include "pygi-fundamental.h"
#include "pygi-property.h"
#include "pygi-signal-closure.h"
#include "pygi-type.h"
#include "pygi-util.h"
#include "pygi-value.h"
#include "pyginterface.h"
#include "pygobject-object.h"

extern PyObject *PyGIDeprecationWarning;

static void pygobject_dealloc (PyGObject *self);
static int pygobject_traverse (PyGObject *self, visitproc visit, void *arg);
static PyObject *pyg_type_get_bases (GType gtype);
static inline int pygobject_clear (PyGObject *self);
static PyObject *pygobject_weak_ref_new (GObject *obj, PyObject *callback,
                                         PyObject *user_data);
static void pygobject_inherit_slots (PyTypeObject *type, PyObject *bases,
                                     gboolean check_for_present);
static void pygobject_find_slot_for (PyTypeObject *type, PyObject *bases,
                                     int slot_offset,
                                     gboolean check_for_present);
GType PY_TYPE_OBJECT = 0;
GQuark pygobject_custom_key;
GQuark pygobject_class_key;
GQuark pygobject_class_init_key;
GQuark pygobject_wrapper_key;
GQuark pygobject_has_updated_constructor_key;
GQuark pygobject_instance_data_key;

/* PyPy doesn't support tp_dictoffset, so we have to work around it */
#ifndef PYPY_VERSION
#define PYGI_OBJECT_USE_CUSTOM_DICT
#endif

GClosure *
gclosure_from_pyfunc (PyGObject *object, PyObject *func)
{
    GSList *l;
    PyGObjectData *inst_data;
    inst_data = pyg_object_peek_inst_data (object->obj);
    if (inst_data) {
        for (l = inst_data->closures; l; l = l->next) {
            PyGClosure *pyclosure = l->data;
            int res =
                PyObject_RichCompareBool (pyclosure->callback, func, Py_EQ);
            if (res == -1) {
                PyErr_Clear (); /* Is there anything else to do? */
            } else if (res) {
                return (GClosure *)pyclosure;
            }
        }
    }
    return NULL;
}

/* Copied from glib. gobject uses hyphens in property names, but in Python
 * we can only represent hyphens as underscores. Convert underscores to
 * hyphens for glib compatibility. */
static void
canonicalize_key (gchar *key)
{
    gchar *p;

    for (p = key; *p != 0; p++) {
        gchar c = *p;

        if (c != '-' && (c < '0' || c > '9') && (c < 'A' || c > 'Z')
            && (c < 'a' || c > 'z'))
            *p = '-';
    }
}

/* -------------- class <-> wrapper manipulation --------------- */

static void
pygobject_data_free (PyGObjectData *data)
{
    /* This function may be called after the python interpreter has already
     * been shut down. If this happens, we cannot do any python calls, so just
     * free the memory. */
    PyGILState_STATE state = 0;
    PyThreadState *_save = NULL;
    gboolean state_saved;
    GSList *closures, *tmp;

    state_saved = Py_IsInitialized ();
    if (state_saved) {
        state = PyGILState_Ensure ();
        Py_DECREF (data->type);
        /* We cannot use Py_BEGIN_ALLOW_THREADS here because this is inside
	 * a branch. */
        Py_UNBLOCK_THREADS; /* Modifies _save */
    }

    tmp = closures = data->closures;
#ifndef NDEBUG
    data->closures = NULL;
    data->type = NULL;
#endif
    while (tmp) {
        GClosure *closure = tmp->data;

        /* we get next item first, because the current link gets
           * invalidated by pygobject_unwatch_closure */
        tmp = tmp->next;
        g_closure_invalidate (closure);
    }

    if (data->closures != NULL)
        g_warning ("invalidated all closures, but data->closures != NULL !");

    g_free (data);

    if (state_saved && Py_IsInitialized ()) {
        Py_BLOCK_THREADS; /* Restores _save */
        PyGILState_Release (state);
    }
}

static inline PyGObjectData *
pygobject_data_new (void)
{
    PyGObjectData *data;
    data = g_new0 (PyGObjectData, 1);
    return data;
}

static inline PyGObjectData *
pygobject_get_inst_data (PyGObject *self)
{
    PyGObjectData *inst_data;

    if (G_UNLIKELY (!self->obj)) return NULL;
    inst_data = g_object_get_qdata (self->obj, pygobject_instance_data_key);
    if (inst_data == NULL) {
        inst_data = pygobject_data_new ();

        inst_data->type = Py_TYPE (self);
        Py_INCREF ((PyObject *)inst_data->type);

        g_object_set_qdata_full (self->obj, pygobject_instance_data_key,
                                 inst_data,
                                 (GDestroyNotify)pygobject_data_free);
    }
    return inst_data;
}


PyTypeObject *PyGObject_MetaType = NULL;

typedef struct {
    PyObject_HEAD
    GParamSpec **props;
    guint n_props;
    guint index;
} PyGPropsIter;

PYGI_DEFINE_TYPE ("gi._gi.GPropsIter", PyGPropsIter_Type, PyGPropsIter);

static void
pyg_props_iter_dealloc (PyGPropsIter *self)
{
    g_free (self->props);
    PyObject_Free ((PyObject *)self);
}

static PyObject *
pygobject_props_iter_next (PyGPropsIter *iter)
{
    if (iter->index < iter->n_props)
        return pygi_fundamental_new (iter->props[iter->index++]);
    else {
        PyErr_SetNone (PyExc_StopIteration);
        return NULL;
    }
}

typedef struct {
    PyObject_HEAD
    /* a reference to the object containing the properties */
    PyGObject *pygobject;
    GType gtype;
} PyGProps;

static void
PyGProps_dealloc (PyGProps *self)
{
    PyObject_GC_UnTrack ((PyObject *)self);

    Py_XSETREF (self->pygobject, NULL);

    PyObject_GC_Del ((PyObject *)self);
}

static PyObject *
build_parameter_list (GObjectClass *class)
{
    GParamSpec **props;
    guint n_props = 0, i;
    PyObject *prop_str;
    PyObject *props_list;

    props = g_object_class_list_properties (class, &n_props);
    props_list = PyList_New (n_props);
    for (i = 0; i < n_props; i++) {
        char *name;
        name = g_strdup (g_param_spec_get_name (props[i]));
        /* hyphens cannot belong in identifiers */
        g_strdelimit (name, "-", '_');
        prop_str = PyUnicode_FromString (name);

        PyList_SetItem (props_list, i, prop_str);
        g_free (name);
    }

    if (props) g_free (props);

    return props_list;
}

static PyObject *
PyGProps_getattro (PyGProps *self, PyObject *attr)
{
    char *attr_name, *property_name;
    GObjectClass *class;
    GParamSpec *pspec;

    attr_name = PyUnicode_AsUTF8 (attr);
    if (!attr_name) {
        PyErr_Clear ();
        return PyObject_GenericGetAttr ((PyObject *)self, attr);
    }

    class = g_type_class_ref (self->gtype);

    /* g_object_class_find_property recurses through the class hierarchy,
     * so the resulting pspec tells us the owner_type that owns the property
     * we're dealing with. */
    property_name = g_strdup (attr_name);
    canonicalize_key (property_name);
    pspec = g_object_class_find_property (class, property_name);
    g_free (property_name);
    g_type_class_unref (class);

    if (!pspec) {
        return PyObject_GenericGetAttr ((PyObject *)self, attr);
    }

    if (!self->pygobject) {
        /* If we're doing it without an instance, return a GParamSpec */
        return pygi_fundamental_new (pspec);
    }

    return pygi_get_property_value (self->pygobject, pspec);
}

static gboolean
set_property_from_pspec (GObject *obj, GParamSpec *pspec, PyObject *pvalue)
{
    GValue value = {
        0,
    };

    if (pspec->flags & G_PARAM_CONSTRUCT_ONLY) {
        PyErr_Format (PyExc_TypeError,
                      "property '%s' can only be set in constructor",
                      pspec->name);
        return FALSE;
    }

    if (!(pspec->flags & G_PARAM_WRITABLE)) {
        PyErr_Format (PyExc_TypeError, "property '%s' is not writable",
                      pspec->name);
        return FALSE;
    }

    g_value_init (&value, G_PARAM_SPEC_VALUE_TYPE (pspec));
    if (pyg_param_gvalue_from_pyobject (&value, pvalue, pspec) < 0) {
        PyObject *pvalue_str = PyObject_Repr (pvalue);
        PyErr_Format (
            PyExc_TypeError,
            "could not convert %s to type '%s' when setting property '%s.%s'",
            PyUnicode_AsUTF8 (pvalue_str),
            g_type_name (G_PARAM_SPEC_VALUE_TYPE (pspec)),
            G_OBJECT_TYPE_NAME (obj), pspec->name);
        Py_DECREF (pvalue_str);
        return FALSE;
    }

    Py_BEGIN_ALLOW_THREADS;
    g_object_set_property (obj, pspec->name, &value);
    g_value_unset (&value);
    Py_END_ALLOW_THREADS;

    return TRUE;
}

PYGI_DEFINE_TYPE ("gi._gi.GProps", PyGProps_Type, PyGProps);

static int
PyGProps_setattro (PyGProps *self, PyObject *attr, PyObject *pvalue)
{
    GParamSpec *pspec;
    char *attr_name, *property_name;
    GObject *obj;
    int ret = -1;

    if (pvalue == NULL) {
        PyErr_SetString (PyExc_TypeError,
                         "properties cannot be "
                         "deleted");
        return -1;
    }

    attr_name = PyUnicode_AsUTF8 (attr);
    if (!attr_name) {
        PyErr_Clear ();
        return PyObject_GenericSetAttr ((PyObject *)self, attr, pvalue);
    }

    if (!self->pygobject) {
        PyErr_SetString (PyExc_TypeError,
                         "cannot set GOject properties without an instance");
        return -1;
    }

    obj = self->pygobject->obj;

    property_name = g_strdup (attr_name);
    canonicalize_key (property_name);

    /* g_object_class_find_property recurses through the class hierarchy,
     * so the resulting pspec tells us the owner_type that owns the property
     * we're dealing with. */
    pspec =
        g_object_class_find_property (G_OBJECT_GET_CLASS (obj), property_name);
    g_free (property_name);
    if (!pspec) {
        return PyObject_GenericSetAttr ((PyObject *)self, attr, pvalue);
    }
    if (!pyg_gtype_is_custom (pspec->owner_type)) {
        /* This GType is not implemented in Python: see if we can set the
         * property via gi. */
        ret = pygi_set_property_value (self->pygobject, pspec, pvalue);
        if (ret == 0)
            return 0;
        else if (ret == -1 && PyErr_Occurred ())
            return -1;
    }

    /* This GType is implemented in Python, or we failed to set it via gi:
     * do a straightforward set. */
    if (!set_property_from_pspec (obj, pspec, pvalue)) return -1;

    return 0;
}

static int
pygobject_props_traverse (PyGProps *self, visitproc visit, void *arg)
{
    if (self->pygobject && visit ((PyObject *)self->pygobject, arg) < 0)
        return -1;
    return 0;
}

static PyObject *
pygobject_props_get_iter (PyGProps *self)
{
    PyGPropsIter *iter;
    GObjectClass *class;

    iter = PyObject_New (PyGPropsIter, &PyGPropsIter_Type);
    class = g_type_class_ref (self->gtype);
    iter->props = g_object_class_list_properties (class, &iter->n_props);
    iter->index = 0;
    g_type_class_unref (class);
    return (PyObject *)iter;
}

static PyObject *
pygobject_props_dir (PyGProps *self)
{
    PyObject *ret;
    GObjectClass *class;

    class = g_type_class_ref (self->gtype);
    ret = build_parameter_list (class);
    g_type_class_unref (class);

    return ret;
}

static PyMethodDef pygobject_props_methods[] = {
    { "__dir__", (PyCFunction)pygobject_props_dir, METH_NOARGS },
    { NULL, NULL, 0 },
};


static Py_ssize_t
PyGProps_length (PyGProps *self)
{
    GObjectClass *class;
    GParamSpec **props;
    guint n_props;

    class = g_type_class_ref (self->gtype);
    props = g_object_class_list_properties (class, &n_props);
    g_type_class_unref (class);
    g_free (props);

    return (Py_ssize_t)n_props;
}

static PySequenceMethods _PyGProps_as_sequence = {
    (lenfunc)PyGProps_length, 0, 0, 0, 0, 0, 0,
};

PYGI_DEFINE_TYPE ("gi._gi.GPropsDescr", PyGPropsDescr_Type, PyObject);

static PyObject *
pyg_props_descr_descr_get (PyObject *self, PyObject *obj, PyObject *type)
{
    PyGProps *gprops;

    gprops = PyObject_GC_New (PyGProps, &PyGProps_Type);
    if (obj == NULL || Py_IsNone (obj)) {
        gprops->pygobject = NULL;
        gprops->gtype = pyg_type_from_object (type);
    } else {
        if (!PyObject_IsInstance (obj, (PyObject *)&PyGObject_Type)) {
            PyErr_SetString (PyExc_TypeError,
                             "cannot use GObject property"
                             " descriptor on non-GObject instances");
            return NULL;
        }
        gprops->pygobject = (PyGObject *)Py_NewRef (obj);
        gprops->gtype = pyg_type_from_object (obj);
    }
    return (PyObject *)gprops;
}

/**
 * pygobject_register_class:
 * @dict: the module dictionary.  A reference to the type will be stored here.
 * @type_name: not used ?
 * @gtype: the GType of the GObject subclass.
 * @type: the Python type object for this wrapper.
 * @static_bases: a tuple of Python type objects that are the bases of
 * this type
 *
 * This function is used to register a Python type as the wrapper for
 * a particular GObject subclass.  It will also insert a reference to
 * the wrapper class into the module dictionary passed as a reference,
 * which simplifies initialisation.
 */
void
pygobject_register_class (PyObject *dict, const gchar *type_name, GType gtype,
                          PyTypeObject *type, PyObject *static_bases)
{
    PyObject *o;
    const char *class_name, *s;
    PyObject *runtime_bases;
    PyObject *bases_list, *bases, *mod_name;
    int i;

    class_name = type->tp_name;
    s = strrchr (class_name, '.');
    if (s != NULL) class_name = s + 1;

    runtime_bases = pyg_type_get_bases (gtype);
    if (static_bases) {
        PyTypeObject *py_parent_type =
            (PyTypeObject *)PyTuple_GET_ITEM (static_bases, 0);
        bases_list = PySequence_List (static_bases);
        /* we start at index 1 because we want to skip the primary
           * base, otherwise we might get MRO conflict */
        for (i = 1; i < PyTuple_GET_SIZE (runtime_bases); ++i) {
            PyObject *base = PyTuple_GET_ITEM (runtime_bases, i);
            int contains = PySequence_Contains (bases_list, base);
            if (contains < 0)
                PyErr_Print ();
            else if (!contains) {
                if (!PySequence_Contains (py_parent_type->tp_mro, base)) {
#if 0
                    g_message("Adding missing base %s to type %s",
                              ((PyTypeObject *)base)->tp_name, type->tp_name);
#endif
                    PyList_Append (bases_list, base);
                }
            }
        }
        bases = PySequence_Tuple (bases_list);
        Py_DECREF (bases_list);
        Py_DECREF (runtime_bases);
    } else
        bases = runtime_bases;

    Py_SET_TYPE (type, PyGObject_MetaType);
    type->tp_bases = bases;
    if (G_LIKELY (bases)) {
        type->tp_base = (PyTypeObject *)PyTuple_GetItem (bases, 0);
        Py_INCREF (type->tp_base);
    }

    pygobject_inherit_slots (type, bases, TRUE);

    if (PyType_Ready (type) < 0) {
        g_warning ("couldn't make the type `%s' ready", type->tp_name);
        return;
    }

    /* Set type.__module__ to the name of the module,
     * otherwise it'll default to 'gobject', see #376099
     */
    s = strrchr (type->tp_name, '.');
    if (s != NULL) {
        mod_name = PyUnicode_FromStringAndSize (type->tp_name,
                                                (int)(s - type->tp_name));
        PyDict_SetItemString (type->tp_dict, "__module__", mod_name);
        Py_DECREF (mod_name);
    }

    if (gtype) {
        o = pyg_type_wrapper_new (gtype);
        PyDict_SetItemString (type->tp_dict, "__gtype__", o);
        Py_DECREF (o);

        /* stash a pointer to the python class with the GType */
        Py_INCREF (type);
        g_type_set_qdata (gtype, pygobject_class_key, type);
    }

    /* set up __doc__ descriptor on type */
    PyDict_SetItemString (type->tp_dict, "__doc__",
                          pyg_object_descr_doc_get ());

    PyDict_SetItemString (dict, (char *)class_name, (PyObject *)type);
}

static void
pyg_toggle_notify (gpointer data, GObject *object, gboolean is_last_ref)
{
    PyGObject *self;
    PyGILState_STATE state;

    state = PyGILState_Ensure ();

    /* Avoid thread safety problems by using qdata for wrapper retrieval
     * instead of the user data argument.
     * See: https://bugzilla.gnome.org/show_bug.cgi?id=709223
     */
    self = (PyGObject *)g_object_get_qdata (object, pygobject_wrapper_key);
    if (self) {
        if (is_last_ref)
            Py_DECREF (self);
        else
            Py_INCREF (self);
    }

    PyGILState_Release (state);
}

static inline gboolean
pygobject_toggle_ref_is_required (PyGObject *self)
{
#ifdef PYGI_OBJECT_USE_CUSTOM_DICT
    return self->inst_dict != NULL;
#else
    PyObject *dict;
    gboolean result;
    dict = PyObject_GetAttrString ((PyObject *)self, "__dict__");
    if (!dict) {
        PyErr_Clear ();
        return FALSE;
    }
    result = PyDict_Size (dict) != 0;
    Py_DECREF (dict);
    return result;
#endif
}

static inline gboolean
pygobject_toggle_ref_is_active (PyGObject *self)
{
    return self->private_flags.flags & PYGOBJECT_USING_TOGGLE_REF;
}

/* Called when the inst_dict is first created; switches the
     reference counting strategy to start using toggle ref to keep the
     wrapper alive while the GObject lives.  In contrast, while
     inst_dict was NULL the python wrapper is allowed to die at
     will and is recreated on demand. */
static inline void
pygobject_toggle_ref_ensure (PyGObject *self)
{
    if (pygobject_toggle_ref_is_active (self)) return;

    if (!pygobject_toggle_ref_is_required (self)) return;

    if (self->obj == NULL) return;

    g_assert (self->obj->ref_count >= 1);
    self->private_flags.flags |= PYGOBJECT_USING_TOGGLE_REF;
    /* Note that add_toggle_ref will never immediately call back into pyg_toggle_notify */
    Py_INCREF ((PyObject *)self);
    g_object_add_toggle_ref (self->obj, pyg_toggle_notify, NULL);
    g_object_unref (self->obj);
}

/**
 * pygobject_register_wrapper:
 * @self: the wrapper instance
 *
 * In the constructor of PyGTK wrappers, this function should be
 * called after setting the obj member.  It will tie the wrapper
 * instance to the GObject so that the same wrapper instance will
 * always be used for this GObject instance.
 */
void
pygobject_register_wrapper (PyObject *self)
{
    PyGObject *gself;

    g_return_if_fail (self != NULL);
    g_return_if_fail (PyObject_TypeCheck (self, &PyGObject_Type));

    gself = (PyGObject *)self;

    g_assert (gself->obj->ref_count >= 1);
    /* save wrapper pointer so we can access it later */
    g_object_set_qdata_full (gself->obj, pygobject_wrapper_key, gself, NULL);

    pygobject_toggle_ref_ensure (gself);
}

static PyObject *
pyg_type_get_bases (GType gtype)
{
    GType *interfaces, parent_type, interface_type;
    guint n_interfaces;
    PyTypeObject *py_parent_type, *py_interface_type;
    PyObject *bases;
    guint i;

    if (G_UNLIKELY (gtype == G_TYPE_OBJECT)) return NULL;

    /* Lookup the parent type */
    parent_type = g_type_parent (gtype);
    py_parent_type = pygobject_lookup_class (parent_type);
    interfaces = g_type_interfaces (gtype, &n_interfaces);
    bases = PyTuple_New (n_interfaces + 1);
    /* We will always put the parent at the first position in bases */
    Py_INCREF (py_parent_type); /* PyTuple_SetItem steals a reference */
    PyTuple_SetItem (bases, 0, (PyObject *)py_parent_type);

    /* And traverse interfaces */
    if (n_interfaces) {
        for (i = 0; i < n_interfaces; i++) {
            interface_type = interfaces[i];
            py_interface_type = pygobject_lookup_class (interface_type);
            Py_INCREF (
                py_interface_type); /* PyTuple_SetItem steals a reference */
            PyTuple_SetItem (bases, i + 1, (PyObject *)py_interface_type);
        }
    }
    g_free (interfaces);
    return bases;
}

/**
 * pygobject_new_with_interfaces
 * @gtype: the GType of the GObject subclass.
 *
 * Creates a new PyTypeObject from the given GType with interfaces attached in
 * bases.
 *
 * Returns: a PyTypeObject for the new type or NULL if it couldn't be created
 */
static PyTypeObject *
pygobject_new_with_interfaces (GType gtype)
{
    PyGILState_STATE state;
    PyObject *o;
    PyTypeObject *type;
    PyObject *dict;
    PyTypeObject *py_parent_type;
    PyObject *bases;

    state = PyGILState_Ensure ();

    bases = pyg_type_get_bases (gtype);
    py_parent_type = (PyTypeObject *)PyTuple_GetItem (bases, 0);

    dict = PyDict_New ();

    o = pyg_type_wrapper_new (gtype);
    PyDict_SetItemString (dict, "__gtype__", o);
    Py_DECREF (o);

    /* set up __doc__ descriptor on type */
    PyDict_SetItemString (dict, "__doc__", pyg_object_descr_doc_get ());

    /* Something special to point out that it's not accessible through
     * gi.repository */
    o = PyUnicode_FromString ("__gi__");
    PyDict_SetItemString (dict, "__module__", o);
    Py_DECREF (o);

    type = (PyTypeObject *)PyObject_CallFunction (
        (PyObject *)Py_TYPE (py_parent_type), "sNN", g_type_name (gtype),
        bases, dict);

    if (type == NULL) {
        PyErr_Print ();
        PyGILState_Release (state);
        return NULL;
    }

    /* Workaround python tp_(get|set)attr slot inheritance bug.
       * Fixes bug #144135. */
    if (!type->tp_getattr && py_parent_type->tp_getattr) {
        type->tp_getattro = NULL;
        type->tp_getattr = py_parent_type->tp_getattr;
    }
    if (!type->tp_setattr && py_parent_type->tp_setattr) {
        type->tp_setattro = NULL;
        type->tp_setattr = py_parent_type->tp_setattr;
    }
    /* override more python stupid hacks behind our back */
    type->tp_dealloc = py_parent_type->tp_dealloc;
    type->tp_alloc = py_parent_type->tp_alloc;
    type->tp_free = py_parent_type->tp_free;
    type->tp_traverse = py_parent_type->tp_traverse;
    type->tp_clear = py_parent_type->tp_clear;

    pygobject_inherit_slots (type, bases, FALSE);

    if (PyType_Ready (type) < 0) {
        g_warning ("couldn't make the type `%s' ready", type->tp_name);
        PyGILState_Release (state);
        return NULL;
    }

    /* stash a pointer to the python class with the GType */
    Py_INCREF (type);
    g_type_set_qdata (gtype, pygobject_class_key, type);

    PyGILState_Release (state);

    return type;
}

/* Pick appropriate value for given slot (at slot_offset inside
 * PyTypeObject structure).  It must be a pointer, e.g. a pointer to a
 * function.  We use the following heuristic:
 *
 * - Scan all types listed as bases of the type.
 * - If for exactly one base type slot value is non-NULL and
 *   different from that of 'object' and 'GObject', set current type
 *   slot into that value.
 * - Otherwise (if there is more than one such base type or none at
 *   all) don't touch it and live with Python default.
 *
 * The intention here is to propagate slot from custom wrappers to
 * wrappers created at runtime when appropriate.  We prefer to be on
 * the safe side, so if there is potential collision (more than one
 * custom slot value), we discard custom overrides altogether.
 *
 * When registering type with pygobject_register_class(), i.e. a type
 * that has been manually created (likely with Codegen help),
 * `check_for_present' should be set to TRUE.  In this case, the
 * function will never overwrite any non-NULL slots already present in
 * the type.  If `check_for_present' is FALSE, such non-NULL slots are
 * though to be set by Python interpreter and so will be overwritten
 * if heuristic above says so.
 */
static void
pygobject_inherit_slots (PyTypeObject *type, PyObject *bases,
                         gboolean check_for_present)
{
    static int slot_offsets[] = {
        offsetof (PyTypeObject, tp_richcompare),
        offsetof (PyTypeObject, tp_richcompare),
        offsetof (PyTypeObject, tp_hash),
        offsetof (PyTypeObject, tp_iter),
        offsetof (PyTypeObject, tp_repr),
        offsetof (PyTypeObject, tp_str),
    };
    gsize i;

    /* Happens when registering gobject.GObject itself, at least. */
    if (!bases) return;

    for (i = 0; i < G_N_ELEMENTS (slot_offsets); ++i)
        pygobject_find_slot_for (type, bases, slot_offsets[i],
                                 check_for_present);
}

static void
pygobject_find_slot_for (PyTypeObject *type, PyObject *bases, int slot_offset,
                         gboolean check_for_present)
{
#define TYPE_SLOT(type) (*(void **)(void *)(((char *)(type)) + slot_offset))

    void *found_slot = NULL;
    Py_ssize_t num_bases = PyTuple_Size (bases);
    Py_ssize_t i;

    if (check_for_present && TYPE_SLOT (type) != NULL) {
        /* We are requested to check if there is any custom slot value
	 * in this type already and there actually is.  Don't
	 * overwrite it.
	 */
        return;
    }

    for (i = 0; i < num_bases; ++i) {
        PyTypeObject *base_type = (PyTypeObject *)PyTuple_GetItem (bases, i);
        void *slot = TYPE_SLOT (base_type);

        if (slot == NULL) continue;
        if (slot == TYPE_SLOT (&PyGObject_Type)
            || slot == TYPE_SLOT (&PyBaseObject_Type))
            continue;

        if (found_slot != NULL && found_slot != slot) {
            /* We have a conflict: more than one base use different
	     * custom slots.  To be on the safe side, we bail out.
	     */
            return;
        }

        found_slot = slot;
    }

    /* Only perform the final assignment if at least one base has a
     * custom value.  Otherwise just leave this type's slot untouched.
     */
    if (found_slot != NULL) TYPE_SLOT (type) = found_slot;

#undef TYPE_SLOT
}

/**
 * pygobject_lookup_class:
 * @gtype: the GType of the GObject subclass.
 *
 * This function looks up the wrapper class used to represent
 * instances of a GObject represented by @gtype.  If no wrapper class
 * or interface has been registered for the given GType, then a new
 * type will be created.
 *
 * Does not set an exception when NULL is returned.
 *
 * Returns: The wrapper class for the GObject or NULL if the
 *          GType has no registered type and a new type couldn't be created
 */
PyTypeObject *
pygobject_lookup_class (GType gtype)
{
    PyTypeObject *py_type;

    if (gtype == G_TYPE_INTERFACE) return &PyGInterface_Type;

    py_type = g_type_get_qdata (gtype, pygobject_class_key);
    if (py_type == NULL) {
        py_type = g_type_get_qdata (gtype, pyginterface_type_key);

        if (py_type == NULL) {
            py_type = (PyTypeObject *)pygi_type_import_by_g_type (gtype);
            PyErr_Clear ();
        }

        if (py_type == NULL) {
            py_type = pygobject_new_with_interfaces (gtype);
            PyErr_Clear ();
            g_type_set_qdata (gtype, pyginterface_type_key, py_type);
        }
    }

    return py_type;
}

/**
 * pygobject_new_full:
 * @obj: a GObject instance.
 * @steal: whether to steal a ref from the GObject or add (sink) a new one.
 * @g_class: the GObjectClass
 *
 * This function gets a reference to a wrapper for the given GObject
 * instance.  If a wrapper has already been created, a new reference
 * to that wrapper will be returned.  Otherwise, a wrapper instance
 * will be created.
 *
 * Returns: a reference to the wrapper for the GObject.
 */
PyObject *
pygobject_new_full (GObject *obj, gboolean steal, gpointer g_class)
{
    PyGObject *self;

    if (obj == NULL) {
        Py_RETURN_NONE;
    }

    /* If the GObject already has a PyObject wrapper stashed in its qdata, re-use it.
     */
    self = (PyGObject *)g_object_get_qdata (obj, pygobject_wrapper_key);
    if (self != NULL) {
        Py_INCREF ((PyObject *)self);

        /* If steal is true, we also want to decref the incoming GObjects which
         * already have a Python wrapper because the wrapper is already holding a
         * strong reference.
         */
        if (steal) g_object_unref (obj);

    } else {
        /* create wrapper */
        PyGObjectData *inst_data = pyg_object_peek_inst_data (obj);
        PyTypeObject *tp;
        if (inst_data)
            tp = inst_data->type;
        else {
            if (g_class)
                tp = pygobject_lookup_class (G_OBJECT_CLASS_TYPE (g_class));
            else
                tp = pygobject_lookup_class (G_OBJECT_TYPE (obj));
        }
        g_assert (tp != NULL);

        /* need to bump type refcount if created with
           pygobject_new_with_interfaces(). fixes bug #141042 */
        if (tp->tp_flags & Py_TPFLAGS_HEAPTYPE) Py_INCREF (tp);
        self = PyObject_GC_New (PyGObject, tp);
        if (self == NULL) return NULL;
        self->inst_dict = NULL;
        self->weakreflist = NULL;
        self->private_flags.flags = 0;
        self->obj = obj;

        /* If we are not stealing a ref or the object is floating,
         * add a regular ref or sink the object. */
        if (!steal || g_object_is_floating (obj)) g_object_ref_sink (obj);

        pygobject_register_wrapper ((PyObject *)self);
        PyObject_GC_Track ((PyObject *)self);
    }

    return (PyObject *)self;
}


PyObject *
pygobject_new (GObject *obj)
{
    return pygobject_new_full (obj,
                               /*steal=*/FALSE, NULL);
}

static void
pygobject_unwatch_closure (gpointer data, GClosure *closure)
{
    PyGObjectData *inst_data = data;

    /* Despite no Python API is called the list inst_data->closures
     * must be protected by GIL as it is used by GC in
     * pygobject_traverse */
    PyGILState_STATE state = PyGILState_Ensure ();
    inst_data->closures = g_slist_remove (inst_data->closures, closure);
    PyGILState_Release (state);
}

/**
 * pygobject_watch_closure:
 * @self: a GObject wrapper instance
 * @closure: a GClosure to watch
 *
 * Adds a closure to the list of watched closures for the wrapper.
 * The closure must be one returned by pyg_closure_new().  When the
 * cycle GC traverses the wrapper instance, it will enumerate the
 * references to Python objects stored in watched closures.  If the
 * cycle GC tells the wrapper to clear itself, the watched closures
 * will be invalidated.
 */
void
pygobject_watch_closure (PyObject *self, GClosure *closure)
{
    PyGObject *gself;
    PyGObjectData *data;

    g_return_if_fail (self != NULL);
    g_return_if_fail (PyObject_TypeCheck (self, &PyGObject_Type));
    g_return_if_fail (closure != NULL);

    gself = (PyGObject *)self;
    data = pygobject_get_inst_data (gself);
    g_return_if_fail (data != NULL);
    g_return_if_fail (g_slist_find (data->closures, closure) == NULL);
    data->closures = g_slist_prepend (data->closures, closure);
    g_closure_add_invalidate_notifier (closure, data,
                                       pygobject_unwatch_closure);
}


/* -------------- PyGObject behaviour ----------------- */

PYGI_DEFINE_TYPE ("gi._gi.GObject", PyGObject_Type, PyGObject);

static void
pygobject_dealloc (PyGObject *self)
{
    /* Untrack must be done first. This is because followup calls such as
     * ClearWeakRefs could call into Python and cause new allocations to
     * happen, which could in turn could trigger the garbage collector,
     * which would then get confused as it is tracking this half-deallocated
     * object. */
    PyObject_GC_UnTrack ((PyObject *)self);

    if (self->weakreflist != NULL) PyObject_ClearWeakRefs ((PyObject *)self);

    /* this forces inst_data->type to be updated, which could prove
       * important if a new wrapper has to be created and it is of a
       * unregistered type */
    pygobject_get_inst_data (self);
    pygobject_clear (self);
    /* the following causes problems with subclassed types */
    /* Py_TYPE(self)->tp_free((PyObject *)self); */
    PyObject_GC_Del (self);
}

static PyObject *
pygobject_richcompare (PyObject *self, PyObject *other, int op)
{
    int isinst;

    isinst = PyObject_IsInstance (self, (PyObject *)&PyGObject_Type);
    if (isinst == -1) return NULL;
    if (!isinst) {
        Py_RETURN_NOTIMPLEMENTED;
    }
    isinst = PyObject_IsInstance (other, (PyObject *)&PyGObject_Type);
    if (isinst == -1) return NULL;
    if (!isinst) {
        Py_RETURN_NOTIMPLEMENTED;
    }

    return pyg_ptr_richcompare (((PyGObject *)self)->obj,
                                ((PyGObject *)other)->obj, op);
}

static Py_hash_t
pygobject_hash (PyGObject *self)
{
    return (Py_hash_t)(gintptr)(self->obj);
}

static PyObject *
pygobject_repr (PyGObject *self)
{
    PyObject *module, *repr;
    gchar *module_str, *namespace;

    module = PyObject_GetAttrString ((PyObject *)self, "__module__");
    if (module == NULL) return NULL;

    if (!PyUnicode_Check (module)) {
        Py_DECREF (module);
        return NULL;
    }

    module_str = PyUnicode_AsUTF8 (module);
    namespace = g_strrstr (module_str, ".");
    if (namespace == NULL) {
        namespace = module_str;
    } else {
        namespace += 1;
    }

    repr = PyUnicode_FromFormat (
        "<%s.%s object at %p (%s at %p)>", namespace, Py_TYPE (self)->tp_name,
        self, self->obj ? G_OBJECT_TYPE_NAME (self->obj) : "uninitialized",
        self->obj);
    Py_DECREF (module);
    return repr;
}


static int
pygobject_traverse (PyGObject *self, visitproc visit, void *arg)
{
    int ret = 0;
    GSList *tmp;
    PyGObjectData *data = pygobject_get_inst_data (self);

    if (self->inst_dict) ret = visit (self->inst_dict, arg);
    if (ret != 0) return ret;

    /* Only let the GC track the closures when tp_clear() would free them.
     * https://bugzilla.gnome.org/show_bug.cgi?id=731501
     */
    if (data && self->obj->ref_count == 1) {
        for (tmp = data->closures; tmp != NULL; tmp = tmp->next) {
            PyGClosure *closure = tmp->data;

            if (closure->callback) ret = visit (closure->callback, arg);
            if (ret != 0) return ret;

            if (closure->extra_args) ret = visit (closure->extra_args, arg);
            if (ret != 0) return ret;

            if (closure->swap_data) ret = visit (closure->swap_data, arg);
            if (ret != 0) return ret;
        }
    }
    return ret;
}

static inline int
pygobject_clear (PyGObject *self)
{
    if (self->obj) {
        g_object_set_qdata_full (self->obj, pygobject_wrapper_key, NULL, NULL);
        if (pygobject_toggle_ref_is_active (self)) {
            g_object_remove_toggle_ref (self->obj, pyg_toggle_notify, NULL);
            self->private_flags.flags &= ~PYGOBJECT_USING_TOGGLE_REF;
        } else {
            Py_BEGIN_ALLOW_THREADS;
            g_object_unref (self->obj);
            Py_END_ALLOW_THREADS;
        }
        self->obj = NULL;
    }
    Py_CLEAR (self->inst_dict);
    return 0;
}

static void
pygobject_free (PyObject *op)
{
    PyObject_GC_Del (op);
}

static gboolean
pygobject_prepare_construct_properties (GObjectClass *class, PyObject *kwargs,
                                        guint *n_properties,
                                        const char **names[],
                                        const GValue **values)
{
    *n_properties = 0;
    *names = NULL;
    *values = NULL;

    if (kwargs) {
        Py_ssize_t pos = 0;
        PyObject *key;
        PyObject *value;
        Py_ssize_t len;

        len = PyDict_Size (kwargs);
        *names = g_new (const char *, len);
        *values = g_new0 (GValue, len);
        while (PyDict_Next (kwargs, &pos, &key, &value)) {
            GParamSpec *pspec;
            GValue *gvalue = &(*values)[*n_properties];

            const gchar *key_str = PyUnicode_AsUTF8 (key);

            pspec = g_object_class_find_property (class, key_str);
            if (!pspec) {
                PyErr_Format (PyExc_TypeError,
                              "gobject `%s' doesn't support property `%s'",
                              G_OBJECT_CLASS_NAME (class), key_str);
                return FALSE;
            }
            g_value_init (gvalue, G_PARAM_SPEC_VALUE_TYPE (pspec));
            if (pyg_param_gvalue_from_pyobject (gvalue, value, pspec) < 0) {
                PyErr_Format (
                    PyExc_TypeError,
                    "could not convert value for property `%s' from %s to %s",
                    key_str, Py_TYPE (value)->tp_name,
                    g_type_name (G_PARAM_SPEC_VALUE_TYPE (pspec)));
                return FALSE;
            }
            (*names)[*n_properties] = g_strdup (key_str);
            ++(*n_properties);
        }
    }
    return TRUE;
}

/* ---------------- PyGObject methods ----------------- */

static int
pygobject_init (PyGObject *self, PyObject *args, PyObject *kwargs)
{
    GType object_type;
    guint n_properties = 0, i;
    const GValue *values = NULL;
    const char **names = NULL;
    GObjectClass *class;

    /* Only do GObject creation and property setting if the GObject hasn't
     * already been created. The case where self->obj already exists can occur
     * when C constructors are called directly (Gtk.Button.new_with_label)
     * and we are simply wrapping the result with a PyGObject.
     * In these cases we want to ignore any keyword arguments passed along
     * to __init__ and simply return.
     *
     * See: https://bugzilla.gnome.org/show_bug.cgi?id=705810
     */
    if (self->obj != NULL) return 0;

    if (!PyArg_ParseTuple (args, ":GObject.__init__", NULL)) return -1;

    object_type = pyg_type_from_object ((PyObject *)self);
    if (!object_type) return -1;

    if (G_TYPE_IS_ABSTRACT (object_type)) {
        PyErr_Format (PyExc_TypeError,
                      "cannot create instance of abstract "
                      "(non-instantiable) type `%s'",
                      g_type_name (object_type));
        return -1;
    }

    if ((class = g_type_class_ref (object_type)) == NULL) {
        PyErr_SetString (PyExc_TypeError,
                         "could not get a reference to type class");
        return -1;
    }

    if (!pygobject_prepare_construct_properties (class, kwargs, &n_properties,
                                                 &names, &values))
        goto cleanup;

    if (pygobject_constructv (self, n_properties, names, values))
        PyErr_SetString (PyExc_RuntimeError, "could not create object");

cleanup:
    for (i = 0; i < n_properties; i++) {
        g_free (names[i]);
        g_value_unset (&values[i]);
    }
    g_free (names);
    g_free (values);

    g_type_class_unref (class);

    return (self->obj) ? 0 : -1;
}

#define CHECK_GOBJECT(self)                                                   \
    if (!G_IS_OBJECT (self->obj)) {                                           \
        PyErr_Format (PyExc_TypeError,                                        \
                      "object at %p of type %s is not initialized", self,     \
                      Py_TYPE (self)->tp_name);                               \
        return NULL;                                                          \
    }

static PyObject *
pygobject_get_property (PyGObject *self, PyObject *args)
{
    gchar *param_name;

    if (!PyArg_ParseTuple (args, "s:GObject.get_property", &param_name)) {
        return NULL;
    }

    CHECK_GOBJECT (self);

    return pygi_get_property_value_by_name (self, param_name);
}

static PyObject *
pygobject_get_properties (PyGObject *self, PyObject *args)
{
    Py_ssize_t len, i;
    PyObject *tuple;

    if ((len = PyTuple_Size (args)) < 1) {
        PyErr_SetString (PyExc_TypeError, "requires at least one argument");
        return NULL;
    }

    tuple = PyTuple_New (len);
    for (i = 0; i < len; i++) {
        PyObject *py_property = PyTuple_GetItem (args, i);
        gchar *property_name;
        PyObject *item;

        if (!PyUnicode_Check (py_property)) {
            PyErr_SetString (PyExc_TypeError,
                             "Expected string argument for property.");
            goto fail;
        }

        property_name = PyUnicode_AsUTF8 (py_property);
        item = pygi_get_property_value_by_name (self, property_name);
        PyTuple_SetItem (tuple, i, item);
    }

    return tuple;

fail:
    Py_DECREF (tuple);
    return NULL;
}

static PyObject *
pygobject_set_property (PyGObject *self, PyObject *args)
{
    gchar *param_name;
    GParamSpec *pspec;
    PyObject *pvalue;
    int ret = -1;

    if (!PyArg_ParseTuple (args, "sO:GObject.set_property", &param_name,
                           &pvalue))
        return NULL;

    CHECK_GOBJECT (self);

    pspec = g_object_class_find_property (G_OBJECT_GET_CLASS (self->obj),
                                          param_name);
    if (!pspec) {
        PyErr_Format (PyExc_TypeError,
                      "object of type `%s' does not have property `%s'",
                      g_type_name (G_OBJECT_TYPE (self->obj)), param_name);
        return NULL;
    }

    ret = pygi_set_property_value (self, pspec, pvalue);
    if (ret == 0)
        goto done;
    else if (PyErr_Occurred ())
        return NULL;

    if (!set_property_from_pspec (self->obj, pspec, pvalue)) return NULL;

done:

    Py_RETURN_NONE;
}

static PyObject *
pygobject_set_properties (PyGObject *self, PyObject *args, PyObject *kwargs)
{
    GObjectClass *class;
    Py_ssize_t pos;
    PyObject *value;
    PyObject *key;
    PyObject *result = NULL;

    CHECK_GOBJECT (self);

    class = G_OBJECT_GET_CLASS (self->obj);

    g_object_freeze_notify (G_OBJECT (self->obj));
    pos = 0;

    while (kwargs && PyDict_Next (kwargs, &pos, &key, &value)) {
        gchar *key_str = PyUnicode_AsUTF8 (key);
        GParamSpec *pspec;
        int ret = -1;

        pspec = g_object_class_find_property (class, key_str);
        if (!pspec) {
            gchar buf[512];

            g_snprintf (buf, sizeof (buf),
                        "object `%s' doesn't support property `%s'",
                        g_type_name (G_OBJECT_TYPE (self->obj)), key_str);
            PyErr_SetString (PyExc_TypeError, buf);
            goto exit;
        }

        ret = pygi_set_property_value (self, pspec, value);
        if (ret != 0) {
            /* Non-zero return code means that either an error occured ...*/
            if (PyErr_Occurred ()) goto exit;

            /* ... or the property couldn't be found , so let's try the default
             * call. */
            if (!set_property_from_pspec (G_OBJECT (self->obj), pspec, value))
                goto exit;
        }
    }

    result = Py_None;

exit:
    g_object_thaw_notify (G_OBJECT (self->obj));
    return Py_XNewRef (result);
}

/* custom closure for gobject bindings */
static void
pygbinding_closure_invalidate (gpointer data, GClosure *closure)
{
    PyGClosure *pc = (PyGClosure *)closure;
    PyGILState_STATE state;

    state = PyGILState_Ensure ();
    Py_XDECREF (pc->callback);
    Py_XDECREF (pc->extra_args);
    PyGILState_Release (state);

    pc->callback = NULL;
    pc->extra_args = NULL;
}

static void
pygbinding_marshal (GClosure *closure, GValue *return_value,
                    guint n_param_values, const GValue *param_values,
                    gpointer invocation_hint, gpointer marshal_data)
{
    PyGILState_STATE state;
    PyGClosure *pc = (PyGClosure *)closure;
    PyObject *params, *ret;
    GValue *out_value;

    state = PyGILState_Ensure ();

    /* construct Python tuple for the parameter values */
    params = PyTuple_New (2);
    PyTuple_SetItem (params, 0,
                     pyg_value_as_pyobject (&param_values[0], FALSE));
    PyTuple_SetItem (params, 1,
                     pyg_value_as_pyobject (&param_values[1], FALSE));

    /* params passed to function may have extra arguments */
    if (pc->extra_args) {
        PyObject *tuple = params;
        params = PySequence_Concat (tuple, pc->extra_args);
        Py_DECREF (tuple);
    }
    ret = PyObject_CallObject (pc->callback, params);
    if (!ret) {
        PyErr_Print ();
        goto out;
    } else if (Py_IsNone (ret)) {
        g_value_set_boolean (return_value, FALSE);
        goto out;
    }

    out_value = g_value_get_boxed (&param_values[2]);
    if (pyg_value_from_pyobject (out_value, ret) != 0) {
        PyErr_SetString (PyExc_ValueError, "can't convert value");
        PyErr_Print ();
        g_value_set_boolean (return_value, FALSE);
    } else {
        g_value_set_boolean (return_value, TRUE);
    }

    Py_DECREF (ret);

out:
    Py_DECREF (params);
    PyGILState_Release (state);
}

static GClosure *
pygbinding_closure_new (PyObject *callback, PyObject *extra_args)
{
    GClosure *closure;

    g_return_val_if_fail (callback != NULL, NULL);
    closure = g_closure_new_simple (sizeof (PyGClosure), NULL);
    g_closure_add_invalidate_notifier (closure, NULL,
                                       pygbinding_closure_invalidate);
    g_closure_set_marshal (closure, pygbinding_marshal);
    Py_INCREF (callback);
    ((PyGClosure *)closure)->callback = callback;
    if (extra_args && !Py_IsNone (extra_args)) {
        Py_INCREF (extra_args);
        if (!PyTuple_Check (extra_args)) {
            PyObject *tmp = PyTuple_New (1);
            PyTuple_SetItem (tmp, 0, extra_args);
            extra_args = tmp;
        }
        ((PyGClosure *)closure)->extra_args = extra_args;
    }
    return closure;
}

static PyObject *
pygobject_bind_property (PyGObject *self, PyObject *args, PyObject *kwargs)
{
    gchar *source_name, *target_name;
    gchar *source_canon, *target_canon;
    PyObject *target, *source_repr, *target_repr;
    PyObject *transform_to, *transform_from, *user_data = NULL;
    GBinding *binding;
    GBindingFlags flags = G_BINDING_DEFAULT;
    GClosure *to_closure = NULL, *from_closure = NULL;

    transform_from = NULL;
    transform_to = NULL;

    static char *kwlist[] = { "source_property", "target",
                              "target_property", "flags",
                              "transform_to",    "transform_from",
                              "user_data",       NULL };

    if (!PyArg_ParseTupleAndKeywords (
            args, kwargs, "sOs|iOOO:GObject.bind_property", kwlist,
            &source_name, &target, &target_name, &flags, &transform_to,
            &transform_from, &user_data))
        return NULL;

    CHECK_GOBJECT (self);
    if (!PyObject_TypeCheck (target, &PyGObject_Type)) {
        PyErr_SetString (PyExc_TypeError, "Second argument must be a GObject");
        return NULL;
    }

    if (transform_to && !Py_IsNone (transform_to)) {
        if (!PyCallable_Check (transform_to)) {
            PyErr_SetString (PyExc_TypeError,
                             "transform_to must be callable or None");
            return NULL;
        }
        to_closure = pygbinding_closure_new (transform_to, user_data);
    }

    if (transform_from && !Py_IsNone (transform_from)) {
        if (!PyCallable_Check (transform_from)) {
            PyErr_SetString (PyExc_TypeError,
                             "transform_from must be callable or None");
            return NULL;
        }
        from_closure = pygbinding_closure_new (transform_from, user_data);
    }

    /* Canonicalize underscores to hyphens. Note the results must be freed. */
    source_canon = g_strdelimit (g_strdup (source_name), "_", '-');
    target_canon = g_strdelimit (g_strdup (target_name), "_", '-');

    binding = g_object_bind_property_with_closures (
        G_OBJECT (self->obj), source_canon, pygobject_get (target),
        target_canon, flags, to_closure, from_closure);
    g_free (source_canon);
    g_free (target_canon);
    source_canon = target_canon = NULL;

    if (binding == NULL) {
        source_repr = PyObject_Repr ((PyObject *)self);
        target_repr = PyObject_Repr (target);
        PyErr_Format (PyExc_TypeError,
                      "Cannot create binding from %s.%s to %s.%s",
                      PyUnicode_AsUTF8 (source_repr), source_name,
                      PyUnicode_AsUTF8 (target_repr), target_name);
        Py_DECREF (source_repr);
        Py_DECREF (target_repr);
        return NULL;
    }

    return pygobject_new (G_OBJECT (binding));
}

static PyObject *
connect_helper (PyGObject *self, gchar *name, PyObject *callback,
                PyObject *extra_args, PyObject *object, gboolean after)
{
    guint sigid;
    GQuark detail = 0;
    GClosure *closure = NULL;
    gulong handlerid;
    GSignalQuery query_info;

    if (!g_signal_parse_name (name, G_OBJECT_TYPE (self->obj), &sigid, &detail,
                              TRUE)) {
        PyObject *repr = PyObject_Repr ((PyObject *)self);
        PyErr_Format (PyExc_TypeError, "%s: unknown signal name: %s",
                      PyUnicode_AsUTF8 (repr), name);
        Py_DECREF (repr);
        return NULL;
    }

    if (object && !PyObject_TypeCheck (object, &PyGObject_Type)) {
        if (PyErr_WarnEx (PyGIDeprecationWarning,
                          "Using non GObject arguments for connect_object() "
                          "is deprecated, use: "
                          "connect_data(signal, callback, data, "
                          "connect_flags=GObject.ConnectFlags.SWAPPED)",
                          1)) {
            return NULL;
        }
    }

    g_signal_query (sigid, &query_info);
    if (!pyg_gtype_is_custom (query_info.itype)) {
        /* The signal is implemented by a non-Python class, probably
         * something in the gi repository. */
        closure = pygi_signal_closure_new (self, query_info.itype,
                                           query_info.signal_name, callback,
                                           extra_args, object);
    }

    if (!closure) {
        /* The signal is either implemented at the Python level, or it comes
         * from a foreign class that we don't have introspection data for. */
        closure = pyg_closure_new (callback, extra_args, object);
    }

    pygobject_watch_closure ((PyObject *)self, closure);
    handlerid = g_signal_connect_closure_by_id (self->obj, sigid, detail,
                                                closure, after);
    return pygi_gulong_to_py (handlerid);
}

static PyObject *
pygobject_connect (PyGObject *self, PyObject *args)
{
    PyObject *first, *callback, *extra_args, *ret;
    gchar *name;
    Py_ssize_t len;

    len = PyTuple_Size (args);
    if (len < 2) {
        PyErr_SetString (PyExc_TypeError,
                         "GObject.connect requires at least 2 arguments");
        return NULL;
    }
    first = PySequence_GetSlice (args, 0, 2);
    if (!PyArg_ParseTuple (first, "sO:GObject.connect", &name, &callback)) {
        Py_DECREF (first);
        return NULL;
    }
    Py_DECREF (first);
    if (!PyCallable_Check (callback)) {
        PyErr_SetString (PyExc_TypeError, "second argument must be callable");
        return NULL;
    }

    CHECK_GOBJECT (self);

    extra_args = PySequence_GetSlice (args, 2, len);
    if (extra_args == NULL) return NULL;

    ret = connect_helper (self, name, callback, extra_args, NULL, FALSE);
    Py_DECREF (extra_args);
    return ret;
}

static PyObject *
pygobject_connect_after (PyGObject *self, PyObject *args)
{
    PyObject *first, *callback, *extra_args, *ret;
    gchar *name;
    Py_ssize_t len;

    len = PyTuple_Size (args);
    if (len < 2) {
        PyErr_SetString (
            PyExc_TypeError,
            "GObject.connect_after requires at least 2 arguments");
        return NULL;
    }
    first = PySequence_GetSlice (args, 0, 2);
    if (!PyArg_ParseTuple (first, "sO:GObject.connect_after", &name,
                           &callback)) {
        Py_DECREF (first);
        return NULL;
    }
    Py_DECREF (first);
    if (!PyCallable_Check (callback)) {
        PyErr_SetString (PyExc_TypeError, "second argument must be callable");
        return NULL;
    }

    CHECK_GOBJECT (self);

    extra_args = PySequence_GetSlice (args, 2, len);
    if (extra_args == NULL) return NULL;

    ret = connect_helper (self, name, callback, extra_args, NULL, TRUE);
    Py_DECREF (extra_args);
    return ret;
}

static PyObject *
pygobject_connect_object (PyGObject *self, PyObject *args)
{
    PyObject *first, *callback, *extra_args, *object, *ret;
    gchar *name;
    Py_ssize_t len;

    len = PyTuple_Size (args);
    if (len < 3) {
        PyErr_SetString (
            PyExc_TypeError,
            "GObject.connect_object requires at least 3 arguments");
        return NULL;
    }
    first = PySequence_GetSlice (args, 0, 3);
    if (!PyArg_ParseTuple (first, "sOO:GObject.connect_object", &name,
                           &callback, &object)) {
        Py_DECREF (first);
        return NULL;
    }
    Py_DECREF (first);
    if (!PyCallable_Check (callback)) {
        PyErr_SetString (PyExc_TypeError, "second argument must be callable");
        return NULL;
    }

    CHECK_GOBJECT (self);

    extra_args = PySequence_GetSlice (args, 3, len);
    if (extra_args == NULL) return NULL;

    ret = connect_helper (self, name, callback, extra_args, object, FALSE);
    Py_DECREF (extra_args);
    return ret;
}

static PyObject *
pygobject_connect_object_after (PyGObject *self, PyObject *args)
{
    PyObject *first, *callback, *extra_args, *object, *ret;
    gchar *name;
    Py_ssize_t len;

    len = PyTuple_Size (args);
    if (len < 3) {
        PyErr_SetString (
            PyExc_TypeError,
            "GObject.connect_object_after requires at least 3 arguments");
        return NULL;
    }
    first = PySequence_GetSlice (args, 0, 3);
    if (!PyArg_ParseTuple (first, "sOO:GObject.connect_object_after", &name,
                           &callback, &object)) {
        Py_DECREF (first);
        return NULL;
    }
    Py_DECREF (first);
    if (!PyCallable_Check (callback)) {
        PyErr_SetString (PyExc_TypeError, "second argument must be callable");
        return NULL;
    }

    CHECK_GOBJECT (self);

    extra_args = PySequence_GetSlice (args, 3, len);
    if (extra_args == NULL) return NULL;

    ret = connect_helper (self, name, callback, extra_args, object, TRUE);
    Py_DECREF (extra_args);
    return ret;
}

static PyObject *
pygobject_emit (PyGObject *self, PyObject *args)
{
    guint signal_id, i, j;
    Py_ssize_t len;
    GQuark detail;
    PyObject *first, *py_ret, *repr = NULL;
    gchar *name;
    GSignalQuery query;
    GValue *params, ret = {
        0,
    };

    len = PyTuple_Size (args);
    if (len < 1) {
        PyErr_SetString (PyExc_TypeError,
                         "GObject.emit needs at least one arg");
        return NULL;
    }
    first = PySequence_GetSlice (args, 0, 1);
    if (!PyArg_ParseTuple (first, "s:GObject.emit", &name)) {
        Py_DECREF (first);
        return NULL;
    }
    Py_DECREF (first);

    CHECK_GOBJECT (self);

    if (!g_signal_parse_name (name, G_OBJECT_TYPE (self->obj), &signal_id,
                              &detail, TRUE)) {
        repr = PyObject_Repr ((PyObject *)self);
        PyErr_Format (PyExc_TypeError, "%s: unknown signal name: %s",
                      PyUnicode_AsUTF8 (repr), name);
        Py_DECREF (repr);
        return NULL;
    }
    g_signal_query (signal_id, &query);
    if ((gsize)len != query.n_params + 1) {
        gchar buf[128];

        g_snprintf (buf, sizeof (buf),
                    "%d parameters needed for signal %s; %ld given",
                    query.n_params, name, (long int)(len - 1));
        PyErr_SetString (PyExc_TypeError, buf);
        return NULL;
    }

    params = g_new0 (GValue, query.n_params + 1);
    g_value_init (&params[0], G_OBJECT_TYPE (self->obj));
    g_value_set_object (&params[0], G_OBJECT (self->obj));

    for (i = 0; i < query.n_params; i++)
        g_value_init (&params[i + 1],
                      query.param_types[i] & ~G_SIGNAL_TYPE_STATIC_SCOPE);
    for (i = 0; i < query.n_params; i++) {
        PyObject *item = PyTuple_GetItem (args, i + 1);

        if (pyg_value_from_pyobject (&params[i + 1], item) < 0) {
            gchar buf[128];
            g_snprintf (
                buf, sizeof (buf),
                "could not convert type %s to %s required for parameter %d",
                Py_TYPE (item)->tp_name, G_VALUE_TYPE_NAME (&params[i + 1]),
                i);
            PyErr_SetString (PyExc_TypeError, buf);

            for (j = 0; j <= i; j++) g_value_unset (&params[j]);

            g_free (params);
            return NULL;
        }
    }

    if (query.return_type != G_TYPE_NONE)
        g_value_init (&ret, query.return_type & ~G_SIGNAL_TYPE_STATIC_SCOPE);

    Py_BEGIN_ALLOW_THREADS;
    g_signal_emitv (params, signal_id, detail, &ret);
    Py_END_ALLOW_THREADS;

    for (i = 0; i < query.n_params + 1; i++) g_value_unset (&params[i]);

    g_free (params);
    if ((query.return_type & ~G_SIGNAL_TYPE_STATIC_SCOPE) != G_TYPE_NONE) {
        gboolean was_floating = FALSE;

        if (G_VALUE_HOLDS_OBJECT (&ret)) {
            GObject *obj = g_value_get_object (&ret);
            if (obj != NULL && G_IS_OBJECT (obj)) {
                was_floating = g_object_is_floating (obj);
            }
        }
        py_ret = pyg_value_as_pyobject (&ret, TRUE);
        if (!was_floating) g_value_unset (&ret);
    } else {
        py_ret = Py_NewRef (Py_None);
    }

    return py_ret;
}

static PyObject *
pygobject_chain_from_overridden (PyGObject *self, PyObject *args)
{
    GSignalInvocationHint *ihint;
    guint signal_id, i;
    Py_ssize_t len;
    PyObject *py_ret;
    const gchar *name;
    GSignalQuery query;
    GValue *params, ret = {
        0,
    };

    CHECK_GOBJECT (self);

    ihint = g_signal_get_invocation_hint (self->obj);
    if (!ihint) {
        PyErr_SetString (PyExc_TypeError,
                         "could not find signal invocation "
                         "information for this object.");
        return NULL;
    }

    signal_id = ihint->signal_id;
    name = g_signal_name (signal_id);

    len = PyTuple_Size (args);
    if (signal_id == 0) {
        PyErr_SetString (PyExc_TypeError, "unknown signal name");
        return NULL;
    }
    g_signal_query (signal_id, &query);
    if (len < 0 || (gsize)len != query.n_params) {
        gchar buf[128];

        g_snprintf (buf, sizeof (buf),
                    "%d parameters needed for signal %s; %ld given",
                    query.n_params, name, (long int)len);
        PyErr_SetString (PyExc_TypeError, buf);
        return NULL;
    }
    params = g_new0 (GValue, query.n_params + 1);
    g_value_init (&params[0], G_OBJECT_TYPE (self->obj));
    g_value_set_object (&params[0], G_OBJECT (self->obj));

    for (i = 0; i < query.n_params; i++)
        g_value_init (&params[i + 1],
                      query.param_types[i] & ~G_SIGNAL_TYPE_STATIC_SCOPE);
    for (i = 0; i < query.n_params; i++) {
        PyObject *item = PyTuple_GetItem (args, i);

        if (pyg_boxed_check (
                item, query.param_types[i] & ~G_SIGNAL_TYPE_STATIC_SCOPE)) {
            g_value_set_static_boxed (&params[i + 1],
                                      pyg_boxed_get (item, void));
        } else if (pyg_value_from_pyobject (&params[i + 1], item) < 0) {
            gchar buf[128];

            g_snprintf (
                buf, sizeof (buf),
                "could not convert type %s to %s required for parameter %d",
                Py_TYPE (item)->tp_name,
                g_type_name (G_VALUE_TYPE (&params[i + 1])), i);
            PyErr_SetString (PyExc_TypeError, buf);
            for (i = 0; i < query.n_params + 1; i++)
                g_value_unset (&params[i]);
            g_free (params);
            return NULL;
        }
    }
    if (query.return_type != G_TYPE_NONE)
        g_value_init (&ret, query.return_type & ~G_SIGNAL_TYPE_STATIC_SCOPE);
    g_signal_chain_from_overridden (params, &ret);
    for (i = 0; i < query.n_params + 1; i++) g_value_unset (&params[i]);
    g_free (params);
    if (query.return_type != G_TYPE_NONE) {
        py_ret = pyg_value_as_pyobject (&ret, TRUE);
        g_value_unset (&ret);
    } else {
        py_ret = Py_NewRef (Py_None);
    }
    return py_ret;
}


static PyObject *
pygobject_weak_ref (PyGObject *self, PyObject *args)
{
    Py_ssize_t len;
    PyObject *callback = NULL, *user_data = NULL;
    PyObject *retval;

    CHECK_GOBJECT (self);

    if ((len = PySequence_Length (args)) >= 1) {
        callback = PySequence_ITEM (args, 0);
        user_data = PySequence_GetSlice (args, 1, len);
    }
    retval = pygobject_weak_ref_new (self->obj, callback, user_data);
    Py_XDECREF (callback);
    Py_XDECREF (user_data);
    return retval;
}


static PyObject *
pygobject_do_dispose (PyObject *self)
{
    Py_RETURN_NONE;
}

static PyObject *
pygobject_copy (PyGObject *self)
{
    PyErr_SetString (PyExc_TypeError,
                     "GObject descendants' instances are non-copyable");
    return NULL;
}

static PyObject *
pygobject_deepcopy (PyGObject *self, PyObject *args)
{
    PyErr_SetString (PyExc_TypeError,
                     "GObject descendants' instances are non-copyable");
    return NULL;
}


static PyObject *
pygobject_disconnect_by_func (PyGObject *self, PyObject *args)
{
    PyObject *pyfunc = NULL, *repr = NULL;
    GClosure *closure = NULL;
    guint retval;

    CHECK_GOBJECT (self);

    if (!PyArg_ParseTuple (args, "O:GObject.disconnect_by_func", &pyfunc))
        return NULL;

    if (!PyCallable_Check (pyfunc)) {
        PyErr_SetString (PyExc_TypeError, "first argument must be callable");
        return NULL;
    }

    closure = gclosure_from_pyfunc (self, pyfunc);
    if (!closure) {
        repr = PyObject_Repr ((PyObject *)pyfunc);
        PyErr_Format (PyExc_TypeError, "nothing connected to %s",
                      PyUnicode_AsUTF8 (repr));
        Py_DECREF (repr);
        return NULL;
    }

    retval = g_signal_handlers_disconnect_matched (
        self->obj, G_SIGNAL_MATCH_CLOSURE, 0, 0, closure, NULL, NULL);
    return pygi_guint_to_py (retval);
}

static PyObject *
pygobject_handler_block_by_func (PyGObject *self, PyObject *args)
{
    PyObject *pyfunc = NULL, *repr = NULL;
    GClosure *closure = NULL;
    guint retval;

    CHECK_GOBJECT (self);

    if (!PyArg_ParseTuple (args, "O:GObject.handler_block_by_func", &pyfunc))
        return NULL;

    if (!PyCallable_Check (pyfunc)) {
        PyErr_SetString (PyExc_TypeError, "first argument must be callable");
        return NULL;
    }

    closure = gclosure_from_pyfunc (self, pyfunc);
    if (!closure) {
        repr = PyObject_Repr ((PyObject *)pyfunc);
        PyErr_Format (PyExc_TypeError, "nothing connected to %s",
                      PyUnicode_AsUTF8 (repr));
        Py_DECREF (repr);
        return NULL;
    }

    retval = g_signal_handlers_block_matched (
        self->obj, G_SIGNAL_MATCH_CLOSURE, 0, 0, closure, NULL, NULL);
    return pygi_guint_to_py (retval);
}

static PyObject *
pygobject_handler_unblock_by_func (PyGObject *self, PyObject *args)
{
    PyObject *pyfunc = NULL, *repr = NULL;
    GClosure *closure = NULL;
    guint retval;

    CHECK_GOBJECT (self);

    if (!PyArg_ParseTuple (args, "O:GObject.handler_unblock_by_func", &pyfunc))
        return NULL;

    if (!PyCallable_Check (pyfunc)) {
        PyErr_SetString (PyExc_TypeError, "first argument must be callable");
        return NULL;
    }

    closure = gclosure_from_pyfunc (self, pyfunc);
    if (!closure) {
        repr = PyObject_Repr ((PyObject *)pyfunc);
        PyErr_Format (PyExc_TypeError, "nothing connected to %s",
                      PyUnicode_AsUTF8 (repr));
        Py_DECREF (repr);
        return NULL;
    }

    retval = g_signal_handlers_unblock_matched (
        self->obj, G_SIGNAL_MATCH_CLOSURE, 0, 0, closure, NULL, NULL);
    return pygi_guint_to_py (retval);
}


static PyMethodDef pygobject_methods[] = {
    { "get_property", (PyCFunction)pygobject_get_property, METH_VARARGS },
    { "get_properties", (PyCFunction)pygobject_get_properties, METH_VARARGS },
    { "set_property", (PyCFunction)pygobject_set_property, METH_VARARGS },
    { "set_properties", (PyCFunction)pygobject_set_properties,
      METH_VARARGS | METH_KEYWORDS },
    { "bind_property", (PyCFunction)pygobject_bind_property,
      METH_VARARGS | METH_KEYWORDS },
    { "connect", (PyCFunction)pygobject_connect, METH_VARARGS },
    { "connect_after", (PyCFunction)pygobject_connect_after, METH_VARARGS },
    { "connect_object", (PyCFunction)pygobject_connect_object, METH_VARARGS },
    { "connect_object_after", (PyCFunction)pygobject_connect_object_after,
      METH_VARARGS },
    { "disconnect_by_func", (PyCFunction)pygobject_disconnect_by_func,
      METH_VARARGS },
    { "handler_block_by_func", (PyCFunction)pygobject_handler_block_by_func,
      METH_VARARGS },
    { "handler_unblock_by_func",
      (PyCFunction)pygobject_handler_unblock_by_func, METH_VARARGS },
    { "emit", (PyCFunction)pygobject_emit, METH_VARARGS },
    { "chain", (PyCFunction)pygobject_chain_from_overridden, METH_VARARGS },
    { "weak_ref", (PyCFunction)pygobject_weak_ref, METH_VARARGS },
    { "do_dispose", (PyCFunction)pygobject_do_dispose, METH_NOARGS },
    { "__copy__", (PyCFunction)pygobject_copy, METH_NOARGS },
    { "__deepcopy__", (PyCFunction)pygobject_deepcopy, METH_VARARGS },
    { NULL, NULL, 0 },
};

#ifdef PYGI_OBJECT_USE_CUSTOM_DICT
static PyObject *
pygobject_get_dict (PyGObject *self, void *closure)
{
    if (self->inst_dict == NULL) {
        self->inst_dict = PyDict_New ();
        pygobject_toggle_ref_ensure (self);
    }
    return Py_NewRef (self->inst_dict);
}
#endif

static PyObject *
pygobject_get_refcount (PyGObject *self, void *closure)
{
    if (self->obj == NULL) {
        PyErr_Format (PyExc_TypeError, "GObject instance is not yet created");
        return NULL;
    }
    return pygi_guint_to_py (self->obj->ref_count);
}

static PyObject *
pygobject_get_pointer (PyGObject *self, void *closure)
{
    return PyCapsule_New (self->obj, NULL, NULL);
}

static int
pygobject_setattro (PyObject *self, PyObject *name, PyObject *value)
{
    int res;
    res = PyGObject_Type.tp_base->tp_setattro (self, name, value);
    pygobject_toggle_ref_ensure ((PyGObject *)self);
    return res;
}

static PyGetSetDef pygobject_getsets[] = {
#ifdef PYGI_OBJECT_USE_CUSTOM_DICT
    { "__dict__", (getter)pygobject_get_dict, (setter)0 },
#endif
    {
        "__grefcount__",
        (getter)pygobject_get_refcount,
        (setter)0,
    },
    {
        "__gpointer__",
        (getter)pygobject_get_pointer,
        (setter)0,
    },
    { NULL, 0, 0 },
};

/* ------------------------------------ */
/* ****** GObject weak reference ****** */
/* ------------------------------------ */

typedef struct {
    PyObject_HEAD
    GObject *obj;
    PyObject *callback;
    PyObject *user_data;
    gboolean have_floating_ref;
} PyGObjectWeakRef;

PYGI_DEFINE_TYPE ("gi._gi.GObjectWeakRef", PyGObjectWeakRef_Type,
                  PyGObjectWeakRef);

static int
pygobject_weak_ref_traverse (PyGObjectWeakRef *self, visitproc visit,
                             void *arg)
{
    if (self->callback && visit (self->callback, arg) < 0) return -1;
    if (self->user_data && visit (self->user_data, arg) < 0) return -1;
    return 0;
}

static void
pygobject_weak_ref_notify (PyGObjectWeakRef *self, GObject *dummy)
{
    self->obj = NULL;
    if (self->callback) {
        PyObject *retval;
        PyGILState_STATE state = PyGILState_Ensure ();
        retval = PyObject_Call (self->callback, self->user_data, NULL);
        if (retval) {
            if (!Py_IsNone (retval))
                PyErr_Format (PyExc_TypeError,
                              "GObject weak notify callback returned a value"
                              " of type %s, should return None",
                              Py_TYPE (retval)->tp_name);
            Py_DECREF (retval);
            PyErr_Print ();
        } else
            PyErr_Print ();
        Py_CLEAR (self->callback);
        Py_CLEAR (self->user_data);
        if (self->have_floating_ref) {
            self->have_floating_ref = FALSE;
            Py_DECREF ((PyObject *)self);
        }
        PyGILState_Release (state);
    }
}

static inline int
pygobject_weak_ref_clear (PyGObjectWeakRef *self)
{
    Py_CLEAR (self->callback);
    Py_CLEAR (self->user_data);
    if (self->obj) {
        g_object_weak_unref (self->obj, (GWeakNotify)pygobject_weak_ref_notify,
                             self);
        self->obj = NULL;
    }
    return 0;
}

static void
pygobject_weak_ref_dealloc (PyGObjectWeakRef *self)
{
    PyObject_GC_UnTrack ((PyObject *)self);
    pygobject_weak_ref_clear (self);
    PyObject_GC_Del (self);
}

static PyObject *
pygobject_weak_ref_new (GObject *obj, PyObject *callback, PyObject *user_data)
{
    PyGObjectWeakRef *self;

    self = PyObject_GC_New (PyGObjectWeakRef, &PyGObjectWeakRef_Type);
    self->callback = callback;
    self->user_data = user_data;
    Py_XINCREF (self->callback);
    Py_XINCREF (self->user_data);
    self->obj = obj;
    g_object_weak_ref (self->obj, (GWeakNotify)pygobject_weak_ref_notify,
                       self);
    if (callback != NULL) {
        /* when we have a callback, we should INCREF the weakref
           * object to make it stay alive even if it goes out of scope */
        self->have_floating_ref = TRUE;
        Py_INCREF ((PyObject *)self);
    }
    return (PyObject *)self;
}

static PyObject *
pygobject_weak_ref_unref (PyGObjectWeakRef *self, PyObject *args)
{
    if (!self->obj) {
        PyErr_SetString (PyExc_ValueError, "weak ref already unreffed");
        return NULL;
    }
    g_object_weak_unref (self->obj, (GWeakNotify)pygobject_weak_ref_notify,
                         self);
    self->obj = NULL;
    if (self->have_floating_ref) {
        self->have_floating_ref = FALSE;
        Py_DECREF (self);
    }
    Py_RETURN_NONE;
}

static PyMethodDef pygobject_weak_ref_methods[] = {
    { "unref", (PyCFunction)pygobject_weak_ref_unref, METH_NOARGS },
    { NULL, NULL, 0 },
};

static PyObject *
pygobject_weak_ref_call (PyGObjectWeakRef *self, PyObject *args, PyObject *kw)
{
    static char *argnames[] = { NULL };

    if (!PyArg_ParseTupleAndKeywords (args, kw, ":__call__", argnames))
        return NULL;

    if (self->obj)
        return pygobject_new (self->obj);
    else {
        Py_RETURN_NONE;
    }
}

static gpointer
pyobject_copy (gpointer boxed)
{
    PyObject *object = boxed;
    PyGILState_STATE state;

    state = PyGILState_Ensure ();
    Py_INCREF (object);
    PyGILState_Release (state);
    return object;
}

static void
pyobject_free (gpointer boxed)
{
    PyObject *object = boxed;
    PyGILState_STATE state;

    state = PyGILState_Ensure ();
    Py_DECREF (object);
    PyGILState_Release (state);
}

/**
 * Returns 0 on success, or -1 and sets an exception.
 */
int
pyi_object_register_types (PyObject *d)
{
    PyObject *o, *descr;

    pygobject_custom_key = g_quark_from_static_string ("PyGObject::custom");
    pygobject_class_key = g_quark_from_static_string ("PyGObject::class");
    pygobject_class_init_key =
        g_quark_from_static_string ("PyGObject::class-init");
    pygobject_wrapper_key = g_quark_from_static_string ("PyGObject::wrapper");
    pygobject_has_updated_constructor_key =
        g_quark_from_static_string ("PyGObject::has-updated-constructor");
    pygobject_instance_data_key =
        g_quark_from_static_string ("PyGObject::instance-data");

    /* GObject */
    if (!PY_TYPE_OBJECT)
        PY_TYPE_OBJECT = g_boxed_type_register_static (
            "PyObject", pyobject_copy, pyobject_free);
    PyGObject_Type.tp_dealloc = (destructor)pygobject_dealloc;
    PyGObject_Type.tp_richcompare = pygobject_richcompare;
    PyGObject_Type.tp_repr = (reprfunc)pygobject_repr;
    PyGObject_Type.tp_hash = (hashfunc)pygobject_hash;
    PyGObject_Type.tp_setattro = (setattrofunc)pygobject_setattro;
    PyGObject_Type.tp_flags =
        (Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC);
    PyGObject_Type.tp_traverse = (traverseproc)pygobject_traverse;
    PyGObject_Type.tp_clear = (inquiry)pygobject_clear;
    PyGObject_Type.tp_weaklistoffset = offsetof (PyGObject, weakreflist);
    PyGObject_Type.tp_methods = pygobject_methods;
    PyGObject_Type.tp_getset = pygobject_getsets;
#ifdef PYGI_OBJECT_USE_CUSTOM_DICT
    PyGObject_Type.tp_dictoffset = offsetof (PyGObject, inst_dict);
#endif
    PyGObject_Type.tp_init = (initproc)pygobject_init;
    PyGObject_Type.tp_free = (freefunc)pygobject_free;
    PyGObject_Type.tp_alloc = PyType_GenericAlloc;
    PyGObject_Type.tp_new = PyType_GenericNew;
    pygobject_register_class (d, "GObject", G_TYPE_OBJECT, &PyGObject_Type,
                              NULL);
    PyDict_SetItemString (PyGObject_Type.tp_dict, "__gdoc__",
                          pyg_object_descr_doc_get ());

    /* GProps */
    PyGProps_Type.tp_dealloc = (destructor)PyGProps_dealloc;
    PyGProps_Type.tp_as_sequence = (PySequenceMethods *)&_PyGProps_as_sequence;
    PyGProps_Type.tp_getattro = (getattrofunc)PyGProps_getattro;
    PyGProps_Type.tp_setattro = (setattrofunc)PyGProps_setattro;
    PyGProps_Type.tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC;
    PyGProps_Type.tp_doc =
        "The properties of the GObject accessible as "
        "Python attributes.";
    PyGProps_Type.tp_traverse = (traverseproc)pygobject_props_traverse;
    PyGProps_Type.tp_iter = (getiterfunc)pygobject_props_get_iter;
    PyGProps_Type.tp_methods = pygobject_props_methods;
    if (PyType_Ready (&PyGProps_Type) < 0) return -1;

    /* GPropsDescr */
    PyGPropsDescr_Type.tp_flags = Py_TPFLAGS_DEFAULT;
    PyGPropsDescr_Type.tp_descr_get = pyg_props_descr_descr_get;
    if (PyType_Ready (&PyGPropsDescr_Type) < 0) return -1;
    descr = PyObject_New (PyObject, &PyGPropsDescr_Type);
    PyDict_SetItemString (PyGObject_Type.tp_dict, "props", descr);
    PyDict_SetItemString (PyGObject_Type.tp_dict, "__module__",
                          o = PyUnicode_FromString ("gi._gi"));
    Py_DECREF (o);

    /* GPropsIter */
    PyGPropsIter_Type.tp_dealloc = (destructor)pyg_props_iter_dealloc;
    PyGPropsIter_Type.tp_flags = Py_TPFLAGS_DEFAULT;
    PyGPropsIter_Type.tp_doc = "GObject properties iterator";
    PyGPropsIter_Type.tp_iter = PyObject_SelfIter;
    PyGPropsIter_Type.tp_iternext = (iternextfunc)pygobject_props_iter_next;
    if (PyType_Ready (&PyGPropsIter_Type) < 0) return -1;

    PyGObjectWeakRef_Type.tp_dealloc = (destructor)pygobject_weak_ref_dealloc;
    PyGObjectWeakRef_Type.tp_call = (ternaryfunc)pygobject_weak_ref_call;
    PyGObjectWeakRef_Type.tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC;
    PyGObjectWeakRef_Type.tp_doc = "A GObject weak reference";
    PyGObjectWeakRef_Type.tp_traverse =
        (traverseproc)pygobject_weak_ref_traverse;
    PyGObjectWeakRef_Type.tp_clear = (inquiry)pygobject_weak_ref_clear;
    PyGObjectWeakRef_Type.tp_methods = pygobject_weak_ref_methods;
    if (PyType_Ready (&PyGObjectWeakRef_Type) < 0) return -1;
    PyDict_SetItemString (d, "GObjectWeakRef",
                          (PyObject *)&PyGObjectWeakRef_Type);

    return 0;
}

PyObject *
pyg_object_new (PyGObject *self, PyObject *args, PyObject *kwargs)
{
    PyObject *pytype;
    GType type;
    GObject *obj = NULL;
    GObjectClass *class;
    guint n_properties = 0, i;
    const GValue *values = NULL;
    const char **names = NULL;

    if (!PyArg_ParseTuple (args, "O:gobject.new", &pytype)) {
        return NULL;
    }

    if ((type = pyg_type_from_object (pytype)) == 0) return NULL;

    if (G_TYPE_IS_ABSTRACT (type)) {
        PyErr_Format (PyExc_TypeError,
                      "cannot create instance of abstract "
                      "(non-instantiable) type `%s'",
                      g_type_name (type));
        return NULL;
    }

    if ((class = g_type_class_ref (type)) == NULL) {
        PyErr_SetString (PyExc_TypeError,
                         "could not get a reference to type class");
        return NULL;
    }

    if (pygobject_prepare_construct_properties (class, kwargs, &n_properties,
                                                &names, &values)) {
        obj = g_object_new_with_properties (type, n_properties, names, values);
    }

    for (i = 0; i < n_properties; i++) {
        g_free (names[i]);
        g_value_unset (&values[i]);
    }
    g_free (names);
    g_free (values);

    g_type_class_unref (class);

    if (obj) {
        if (G_IS_INITIALLY_UNOWNED (obj)) {
            g_object_ref_sink (obj);
        }
        self = g_object_get_qdata (obj, pygobject_wrapper_key);
        if (self == NULL) {
            self = (PyGObject *)pygobject_new ((GObject *)obj);
            g_object_unref (obj);
        }
    } else {
        PyErr_SetString (PyExc_RuntimeError, "could not create object");
        self = NULL;
    }

    return (PyObject *)self;
}
