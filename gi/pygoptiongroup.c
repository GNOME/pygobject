/* -*- Mode: C; c-basic-offset: 4 -*-
 * pygobject - Python bindings for the GLib, GObject and GIO
 * Copyright (C) 2006  Johannes Hoelzl
 *
 *   pygoptiongroup.c: GOptionContext and GOptionGroup wrapper
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

#include "pygi-error.h"
#include "pygi-util.h"
#include "pygoptiongroup.h"

PYGI_DEFINE_TYPE ("gi._gi.OptionGroup", PyGOptionGroup_Type, PyGOptionGroup)

/**
 * pyg_option_group_new:
 * @group: a GOptionGroup
 *
 * The returned GOptionGroup can't be used to set any hooks, translation domains
 * or add entries. It's only intend is, to use for GOptionContext.add_group().
 *
 * Returns: the GOptionGroup wrapper.
 */
PyObject *
pyg_option_group_new (GOptionGroup *group)
{
    PyGOptionGroup *self;

    self =
        (PyGOptionGroup *)PyObject_New (PyGOptionGroup, &PyGOptionGroup_Type);
    if (self == NULL) return NULL;

    self->group = group;
    self->other_owner = TRUE;
    self->is_in_context = FALSE;

    return (PyObject *)self;
}

static gboolean
check_if_owned (PyGOptionGroup *self)
{
    if (self->other_owner) {
        PyErr_SetString (
            PyExc_ValueError,
            "The GOptionGroup was not created by "
            "gi._gi.OptionGroup(), so operation is not possible.");
        return TRUE;
    }
    return FALSE;
}

static void
destroy_g_group (PyGOptionGroup *self)
{
    PyGILState_STATE state;
    state = PyGILState_Ensure ();

    self->group = NULL;
    Py_CLEAR (self->callback);
    g_slist_foreach (self->strings, (GFunc)g_free, NULL);
    g_slist_free (self->strings);
    self->strings = NULL;

    if (self->is_in_context) {
        Py_DECREF (self);
    }

    PyGILState_Release (state);
}

static int
pyg_option_group_init (PyGOptionGroup *self, PyObject *args, PyObject *kwargs)
{
    static char *kwlist[] = { "name", "description", "help_description",
                              "callback", NULL };
    char *name, *description, *help_description;
    PyObject *callback;

    if (!PyArg_ParseTupleAndKeywords (
            args, kwargs, "zzzO:GOptionGroup.__init__", kwlist, &name,
            &description, &help_description, &callback))
        return -1;

    self->group = g_option_group_new (name, description, help_description,
                                      self, (GDestroyNotify)destroy_g_group);
    self->other_owner = FALSE;
    self->is_in_context = FALSE;

    self->callback = Py_NewRef (callback);

    return 0;
}

static void
pyg_option_group_dealloc (PyGOptionGroup *self)
{
    if (!self->other_owner && !self->is_in_context) {
        GOptionGroup *tmp = self->group;
        self->group = NULL;
        if (tmp) {
            G_GNUC_BEGIN_IGNORE_DEPRECATIONS
            g_option_group_free (tmp);
            G_GNUC_END_IGNORE_DEPRECATIONS
        }
    }

    PyObject_Free (self);
}

static gboolean
arg_func (const gchar *option_name, const gchar *value, PyGOptionGroup *self,
          GError **error)
{
    PyObject *ret;
    PyGILState_STATE state;
    gboolean no_error;

    state = PyGILState_Ensure ();
    if (value == NULL)
        ret = PyObject_CallFunction (self->callback, "sOO", option_name,
                                     Py_None, self);
    else
        ret = PyObject_CallFunction (self->callback, "ssO", option_name, value,
                                     self);

    if (ret != NULL) {
        Py_DECREF (ret);
        no_error = TRUE;
    } else
        no_error = pygi_gerror_exception_check (error) != -1;

    PyGILState_Release (state);
    return no_error;
}

static PyObject *
pyg_option_group_add_entries (PyGOptionGroup *self, PyObject *args,
                              PyObject *kwargs)
{
    static char *kwlist[] = { "entries", NULL };
    gssize entry_count, pos;
    PyObject *entry_tuple, *list;
    GOptionEntry *entries;

    if (check_if_owned (self)) return NULL;

    if (!PyArg_ParseTupleAndKeywords (
            args, kwargs, "O:GOptionGroup.add_entries", kwlist, &list))
        return NULL;

    if (!PyList_Check (list)) {
        PyErr_SetString (
            PyExc_TypeError,
            "GOptionGroup.add_entries expected a list of entries");
        return NULL;
    }

    entry_count = PyList_Size (list);
    if (entry_count == -1) {
        PyErr_SetString (
            PyExc_TypeError,
            "GOptionGroup.add_entries expected a list of entries");
        return NULL;
    }

    entries = g_new0 (GOptionEntry, entry_count + 1);
    for (pos = 0; pos < entry_count; pos++) {
        gchar *long_name, *description, *arg_description;
        entry_tuple = PyList_GetItem (list, pos);
        if (!PyTuple_Check (entry_tuple)) {
            PyErr_SetString (PyExc_TypeError,
                             "GOptionGroup.add_entries "
                             "expected a list of entries");
            g_free (entries);
            return NULL;
        }
        if (!PyArg_ParseTuple (
                entry_tuple, "scisz", &long_name, &(entries[pos].short_name),
                &(entries[pos].flags), &description, &arg_description)) {
            PyErr_SetString (PyExc_TypeError,
                             "GOptionGroup.add_entries "
                             "expected a list of entries");
            g_free (entries);
            return NULL;
        }
        long_name = g_strdup (long_name);
        self->strings = g_slist_prepend (self->strings, long_name);
        entries[pos].long_name = long_name;

        description = g_strdup (description);
        self->strings = g_slist_prepend (self->strings, description);
        entries[pos].description = description;

        arg_description = g_strdup (arg_description);
        self->strings = g_slist_prepend (self->strings, arg_description);
        entries[pos].arg_description = arg_description;

        entries[pos].arg = G_OPTION_ARG_CALLBACK;
        entries[pos].arg_data = arg_func;
    }

    g_option_group_add_entries (self->group, entries);

    g_free (entries);

    Py_RETURN_NONE;
}


static PyObject *
pyg_option_group_set_translation_domain (PyGOptionGroup *self, PyObject *args,
                                         PyObject *kwargs)
{
    static char *kwlist[] = { "domain", NULL };
    char *domain;

    if (check_if_owned (self)) return NULL;

    if (self->group == NULL) {
        PyErr_SetString (PyExc_RuntimeError,
                         "The corresponding GOptionGroup was already freed, "
                         "probably through the release of GOptionContext");
        return NULL;
    }

    if (!PyArg_ParseTupleAndKeywords (args, kwargs,
                                      "z:GOptionGroup.set_translate_domain",
                                      kwlist, &domain))
        return NULL;

    g_option_group_set_translation_domain (self->group, domain);

    Py_RETURN_NONE;
}

static PyObject *
pyg_option_group_richcompare (PyObject *self, PyObject *other, int op)
{
    if (Py_TYPE (self) == Py_TYPE (other)
        && Py_TYPE (self) == &PyGOptionGroup_Type) {
        return pyg_ptr_richcompare (((PyGOptionGroup *)self)->group,
                                    ((PyGOptionGroup *)other)->group, op);
    } else {
        Py_RETURN_NOTIMPLEMENTED;
    }
}

static PyMethodDef pyg_option_group_methods[] = {
    { "add_entries", (PyCFunction)pyg_option_group_add_entries,
      METH_VARARGS | METH_KEYWORDS },
    { "set_translation_domain",
      (PyCFunction)pyg_option_group_set_translation_domain,
      METH_VARARGS | METH_KEYWORDS },
    { NULL, NULL, 0 },
};

/**
 * Returns 0 on success, or -1 and sets an exception.
 */
int
pygi_option_group_register_types (PyObject *d)
{
    PyGOptionGroup_Type.tp_dealloc = (destructor)pyg_option_group_dealloc;
    PyGOptionGroup_Type.tp_richcompare = pyg_option_group_richcompare;
    PyGOptionGroup_Type.tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE;
    PyGOptionGroup_Type.tp_methods = pyg_option_group_methods;
    PyGOptionGroup_Type.tp_init = (initproc)pyg_option_group_init;
    PyGOptionGroup_Type.tp_alloc = PyType_GenericAlloc;
    PyGOptionGroup_Type.tp_new = PyType_GenericNew;

    if (PyType_Ready (&PyGOptionGroup_Type)) return -1;

    PyDict_SetItemString (d, "OptionGroup", (PyObject *)&PyGOptionGroup_Type);

    return 0;
}
