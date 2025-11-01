/* -*- Mode: C; c-basic-offset: 4 -*-
 * vim: tabstop=4 shiftwidth=4 expandtab
 *
 * Copyright (C) 2015 Christoph Reiter <reiter.christoph@gmail.com>
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

#include <glib.h>
#include <pythoncapi_compat.h>

#include "pygi-resulttuple.h"
#include "pygi-util.h"

static char repr_format_key[] = "__repr_format";
static char tuple_indices_key[] = "__tuple_indices";

#define PYGI_USE_FREELIST

#ifdef PYPY_VERSION
#undef PYGI_USE_FREELIST
#endif

#ifdef PYGI_USE_FREELIST
/* A free list similar to the one used for the CPython tuple. Difference
 * is that zero length tuples aren't cached (as we don't need them)
 * and that the freelist is smaller as we don't free it with the cyclic GC
 * as CPython does. This wastes 21kB max.
 */
#define PyGIResultTuple_MAXSAVESIZE 10
#define PyGIResultTuple_MAXFREELIST 100
static PyObject *free_list[PyGIResultTuple_MAXSAVESIZE];
static int numfree[PyGIResultTuple_MAXSAVESIZE];
#endif

PYGI_DEFINE_TYPE ("gi._gi.ResultTuple", PyGIResultTuple_Type, PyTupleObject)

/**
 * ResultTuple.__repr__() implementation.
 * Takes the _ResultTuple.__repr_format format string and applies the tuple
 * values to it.
 */
static PyObject *
resulttuple_repr (PyObject *self)
{
    PyObject *format, *repr, *format_attr;

    format_attr = PyUnicode_FromString (repr_format_key);
    format = PyTuple_Type.tp_getattro (self, format_attr);
    Py_DECREF (format_attr);
    if (format == NULL) return NULL;
    repr = PyUnicode_Format (format, self);
    Py_DECREF (format);
    return repr;
}

/**
 * PyGIResultTuple_Type.tp_getattro implementation.
 * Looks up the tuple index in _ResultTuple.__tuple_indices and returns the
 * tuple item.
 */
static PyObject *
resulttuple_getattro (PyObject *self, PyObject *name)
{
    PyObject *mapping, *index, *mapping_attr, *item;

    mapping_attr = PyUnicode_FromString (tuple_indices_key);
    mapping = PyTuple_Type.tp_getattro (self, mapping_attr);
    Py_DECREF (mapping_attr);
    if (mapping == NULL) return NULL;
    g_assert (PyDict_Check (mapping));
    index = PyDict_GetItem (mapping, name);

    if (index != NULL) {
        item = PyTuple_GET_ITEM (self, PyLong_AsSsize_t (index));
        if (item == NULL) Py_RETURN_NONE;
        Py_INCREF (item);
    } else {
        item = PyTuple_Type.tp_getattro (self, name);
    }
    Py_DECREF (mapping);

    return item;
}

/**
 * ResultTuple.__reduce__() implementation.
 * Always returns (tuple, tuple(self))
 * Needed so that pickling doesn't depend on our tuple subclass and unpickling
 * works without it. As a result unpickle will give back in a normal tuple.
 */
static PyObject *
resulttuple_reduce (PyObject *self)
{
    PyObject *tuple = PySequence_Tuple (self);
    if (tuple == NULL) return NULL;
    return Py_BuildValue ("(O, (N))", &PyTuple_Type, tuple);
}

/**
 * Extends __dir__ with the extra attributes accessible through
 * resulttuple_getattro()
 */
static PyObject *
resulttuple_dir (PyObject *self)
{
    PyObject *mapping_attr;
    PyObject *items = NULL;
    PyObject *mapping = NULL;
    PyObject *mapping_values = NULL;
    PyObject *result = NULL;

    mapping_attr = PyUnicode_FromString (tuple_indices_key);
    mapping = PyTuple_Type.tp_getattro (self, mapping_attr);
    Py_DECREF (mapping_attr);
    if (mapping == NULL) goto error;
    items = PyObject_Dir ((PyObject *)Py_TYPE (self));
    if (items == NULL) goto error;
    mapping_values = PyDict_Keys (mapping);
    if (mapping_values == NULL) goto error;
    result = PySequence_InPlaceConcat (items, mapping_values);

error:
    Py_XDECREF (items);
    Py_XDECREF (mapping);
    Py_XDECREF (mapping_values);

    return result;
}

/**
 * resulttuple_new_type:
 * @args: one list object containing tuple item names and None
 *
 * Exposes pygi_resulttuple_new_type() as ResultTuple._new_type()
 * to allow creation of result types for unit tests.
 *
 * Returns: A new PyTypeObject which is a subclass of PyGIResultTuple_Type
 *    or %NULL in case of an error.
 */
static PyObject *
resulttuple_new_type (PyObject *self, PyObject *args)
{
    PyObject *tuple_names, *new_type;

    if (!PyArg_ParseTuple (args, "O:ResultTuple._new_type", &tuple_names))
        return NULL;

    if (!PyList_Check (tuple_names)) {
        PyErr_SetString (PyExc_TypeError, "not a list");
        return NULL;
    }

    new_type = (PyObject *)pygi_resulttuple_new_type (tuple_names);
    return new_type;
}

static PyMethodDef resulttuple_methods[] = {
    { "__reduce__", (PyCFunction)resulttuple_reduce, METH_NOARGS },
    { "__dir__", (PyCFunction)resulttuple_dir, METH_NOARGS },
    { "_new_type", (PyCFunction)resulttuple_new_type,
      METH_VARARGS | METH_STATIC },
    { NULL, NULL, 0 },
};

/**
 * pygi_resulttuple_new_type:
 * @tuple_names: A python list containing str or None items.
 *
 * Similar to namedtuple() creates a new tuple subclass which
 * allows to access items by name and have a pretty __repr__.
 * Each item in the passed name list corresponds to an item with
 * the same index in the tuple class. If the name is None the item/index
 * is unnamed.
 *
 * Returns: A new PyTypeObject which is a subclass of PyGIResultTuple_Type
 *    or %NULL in case of an error.
 */
PyTypeObject *
pygi_resulttuple_new_type (PyObject *tuple_names)
{
    PyTypeObject *new_type;
    PyObject *class_dict, *format_string, *empty_format, *named_format,
        *format_list, *sep, *index_dict, *slots, *paren_format, *new_type_args,
        *paren_string;
    Py_ssize_t len, i;

    g_assert (PyList_Check (tuple_names));

    class_dict = PyDict_New ();

    /* To save some memory don't use an instance dict */
    slots = PyTuple_New (0);
    PyDict_SetItemString (class_dict, "__slots__", slots);
    Py_DECREF (slots);

    format_list = PyList_New (0);
    index_dict = PyDict_New ();

    empty_format = PyUnicode_FromString ("%r");
    named_format = PyUnicode_FromString ("%s=%%r");
    len = PyList_Size (tuple_names);
    for (i = 0; i < len; i++) {
        PyObject *item, *named_args, *named_build, *index;
        item = PyList_GET_ITEM (tuple_names, i);
        if (Py_IsNone (item)) {
            PyList_Append (format_list, empty_format);
        } else {
            named_args = Py_BuildValue ("(O)", item);
            named_build = PyUnicode_Format (named_format, named_args);
            Py_DECREF (named_args);
            PyList_Append (format_list, named_build);
            Py_DECREF (named_build);
            index = PyLong_FromSsize_t (i);
            PyDict_SetItem (index_dict, item, index);
            Py_DECREF (index);
        }
    }
    Py_DECREF (empty_format);
    Py_DECREF (named_format);

    sep = PyUnicode_FromString (", ");
    format_string = PyObject_CallMethod (sep, "join", "O", format_list);
    Py_DECREF (sep);
    Py_DECREF (format_list);
    paren_format = PyUnicode_FromString ("(%s)");
    paren_string = PyUnicode_Format (paren_format, format_string);
    Py_DECREF (paren_format);
    Py_DECREF (format_string);

    PyDict_SetItemString (class_dict, repr_format_key, paren_string);
    Py_DECREF (paren_string);

    PyDict_SetItemString (class_dict, tuple_indices_key, index_dict);
    Py_DECREF (index_dict);

    new_type_args = Py_BuildValue ("s(O)O", "_ResultTuple",
                                   &PyGIResultTuple_Type, class_dict);
    new_type =
        (PyTypeObject *)PyType_Type.tp_new (&PyType_Type, new_type_args, NULL);
    Py_DECREF (new_type_args);
    Py_DECREF (class_dict);

    if (new_type != NULL) {
        /* disallow subclassing as that would break the free list caching
         * since we assume that all subclasses use PyTupleObject */
        new_type->tp_flags &= ~Py_TPFLAGS_BASETYPE;
    }

    return new_type;
}


/**
 * pygi_resulttuple_new:
 * @subclass: A PyGIResultTuple_Type subclass which will be the type of the
 *    returned instance.
 * @len: Length of the returned tuple
 *
 * Like PyTuple_New(). Return an uninitialized tuple of the given @length.
 *
 * Returns: An instance of @subclass or %NULL on error.
 */
PyObject *
pygi_resulttuple_new (PyTypeObject *subclass, Py_ssize_t len)
{
#ifdef PYGI_USE_FREELIST
    PyObject *self;
    Py_ssize_t i;

    /* Check the free list for a tuple object with the needed size;
     * clear it and change the class to ours.
     */
    if (len > 0 && len < PyGIResultTuple_MAXSAVESIZE) {
        self = free_list[len];
        if (self != NULL) {
            free_list[len] = PyTuple_GET_ITEM (self, 0);
            numfree[len]--;
            for (i = 0; i < len; i++) {
                PyTuple_SET_ITEM (self, i, NULL);
            }
            Py_SET_TYPE (self, subclass);
            Py_INCREF (subclass);
            _Py_NewReference (self);
            PyObject_GC_Track (self);
            return self;
        }
    }
#endif

    /* For zero length tuples and in case the free list is empty, alloc
     * as usual.
     */
    return subclass->tp_alloc (subclass, len);
}

#ifdef PYGI_USE_FREELIST
static void
resulttuple_dealloc (PyObject *self)
{
    Py_ssize_t i, len;

    PyObject_GC_UnTrack (self);
    CPy_TRASHCAN_BEGIN (self, resulttuple_dealloc)

        /* Free the tuple items and, if there is space, save the tuple object
     * pointer to the front of the free list for its size. Otherwise free it.
     */
        len = Py_SIZE (self);
    if (len > 0) {
        for (i = 0; i < len; i++) {
            Py_XDECREF (PyTuple_GET_ITEM (self, i));
        }

        if (len < PyGIResultTuple_MAXSAVESIZE
            && numfree[len] < PyGIResultTuple_MAXFREELIST) {
            PyTuple_SET_ITEM (self, 0, free_list[len]);
            numfree[len]++;
            free_list[len] = self;
            goto done;
        }
    }

    Py_TYPE (self)->tp_free (self);

done:
    CPy_TRASHCAN_END (self);
}
#endif

/**
 * pygi_resulttuple_register_types:
 * @module: A Python modules to which ResultTuple gets added to.
 *
 * Initializes the ResultTuple class and adds it to the passed @module.
 *
 * Returns: -1 on error, 0 on success.
 */
int
pygi_resulttuple_register_types (PyObject *module)
{
    PyGIResultTuple_Type.tp_base = &PyTuple_Type;
    PyGIResultTuple_Type.tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE;
    PyGIResultTuple_Type.tp_repr = (reprfunc)resulttuple_repr;
    PyGIResultTuple_Type.tp_getattro = (getattrofunc)resulttuple_getattro;
    PyGIResultTuple_Type.tp_methods = resulttuple_methods;
#ifdef PYGI_USE_FREELIST
    PyGIResultTuple_Type.tp_dealloc = (destructor)resulttuple_dealloc;
#endif

    if (PyType_Ready (&PyGIResultTuple_Type) < 0) return -1;

    Py_INCREF (&PyGIResultTuple_Type);
    if (PyModule_AddObject (module, "ResultTuple",
                            (PyObject *)&PyGIResultTuple_Type)
        < 0) {
        Py_DECREF (&PyGIResultTuple_Type);
        return -1;
    }

    return 0;
}
