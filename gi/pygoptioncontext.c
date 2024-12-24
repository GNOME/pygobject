/* -*- Mode: C; c-basic-offset: 4 -*-
 * pygtk- Python bindings for the GTK toolkit.
 * Copyright (C) 2006  Johannes Hoelzl
 *
 *   pygoptioncontext.c: GOptionContext wrapper
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

#include "pygi-basictype.h"
#include "pygi-error.h"
#include "pygi-util.h"
#include "pygoptioncontext.h"

PYGI_DEFINE_TYPE ("gi._gi.OptionContext", PyGOptionContext_Type,
                  PyGOptionContext)

/**
 * pyg_option_group_transfer_group:
 * @group: a GOptionGroup wrapper
 *
 * This is used to transfer the GOptionGroup to a GOptionContext. After this
 * is called, the calle must handle the release of the GOptionGroup.
 *
 * When #NULL is returned, the GOptionGroup was already transfered.
 *
 * Returns: Either #NULL or the wrapped GOptionGroup.
 */
static GOptionGroup *
pyglib_option_group_transfer_group (PyObject *obj)
{
    PyGOptionGroup *self = (PyGOptionGroup *)obj;

    if (self->is_in_context) return NULL;

    self->is_in_context = TRUE;

    /* Here we increase the reference count of the PyGOptionGroup, because now
     * the GOptionContext holds an reference to us (it is the userdata passed
     * to g_option_group_new().
     *
     * The GOptionGroup is freed with the GOptionContext.
     *
     * We set it here because if we would do this in the init method we would
     * hold two references and the PyGOptionGroup would never be freed.
     */
    Py_INCREF (self);

    return self->group;
}

/**
 * pyg_option_context_new:
 * @context: a GOptionContext
 *
 * Returns: A new GOptionContext wrapper.
 */
PyObject *
pyg_option_context_new (GOptionContext *context)
{
    PyGOptionContext *self;

    self = (PyGOptionContext *)PyObject_New (PyGOptionContext,
                                             &PyGOptionContext_Type);
    if (self == NULL) return NULL;

    self->context = context;
    self->main_group = NULL;

    return (PyObject *)self;
}

static int
pyg_option_context_init (PyGOptionContext *self, PyObject *args,
                         PyObject *kwargs)
{
    char *parameter_string;

    if (!PyArg_ParseTuple (args, "s:gi._gi.GOptionContext.__init__",
                           &parameter_string))
        return -1;

    self->context = g_option_context_new (parameter_string);
    return 0;
}

static void
pyg_option_context_dealloc (PyGOptionContext *self)
{
    Py_CLEAR (self->main_group);

    if (self->context != NULL) {
        GOptionContext *tmp = self->context;
        self->context = NULL;
        g_option_context_free (tmp);
    }

    PyObject_Free (self);
}

static PyObject *
pyg_option_context_parse (PyGOptionContext *self, PyObject *args,
                          PyObject *kwargs)
{
    static char *kwlist[] = { "argv", NULL };
    PyObject *arg;
    PyObject *new_argv, *argv;
    Py_ssize_t argv_length, pos;
    gint argv_length_int;
    char **argv_content, **original;
    GError *error = NULL;
    gboolean result;

    if (!PyArg_ParseTupleAndKeywords (args, kwargs, "O:GOptionContext.parse",
                                      kwlist, &argv))
        return NULL;

    if (!PyList_Check (argv)) {
        PyErr_SetString (PyExc_TypeError,
                         "GOptionContext.parse expects a list of strings.");
        return NULL;
    }

    argv_length = PyList_Size (argv);
    if (argv_length == -1) {
        PyErr_SetString (PyExc_TypeError,
                         "GOptionContext.parse expects a list of strings.");
        return NULL;
    }

    argv_content = g_new (char *, argv_length + 1);
    argv_content[argv_length] = NULL;
    for (pos = 0; pos < argv_length; pos++) {
        arg = PyList_GetItem (argv, pos);
        argv_content[pos] = g_strdup (PyUnicode_AsUTF8 (arg));
        if (argv_content[pos] == NULL) {
            g_strfreev (argv_content);
            return NULL;
        }
    }
    original = g_strdupv (argv_content);

    g_assert (argv_length <= G_MAXINT);
    argv_length_int = (gint)argv_length;
    Py_BEGIN_ALLOW_THREADS;
    result = g_option_context_parse (self->context, &argv_length_int,
                                     &argv_content, &error);
    Py_END_ALLOW_THREADS;
    argv_length = argv_length_int;

    if (!result) {
        g_strfreev (argv_content);
        g_strfreev (original);
        pygi_error_check (&error);
        return NULL;
    }

    new_argv = PyList_New (g_strv_length (argv_content));
    for (pos = 0; pos < argv_length; pos++) {
        arg = PyUnicode_FromString (argv_content[pos]);
        PyList_SetItem (new_argv, pos, arg);
    }

    g_strfreev (original);
    g_strfreev (argv_content);
    return new_argv;
}

static PyObject *
pyg_option_context_set_help_enabled (PyGOptionContext *self, PyObject *args,
                                     PyObject *kwargs)
{
    static char *kwlist[] = { "help_enable", NULL };

    PyObject *help_enabled;
    if (!PyArg_ParseTupleAndKeywords (args, kwargs,
                                      "O:GOptionContext.set_help_enabled",
                                      kwlist, &help_enabled))
        return NULL;

    g_option_context_set_help_enabled (self->context,
                                       PyObject_IsTrue (help_enabled));

    Py_RETURN_NONE;
}

static PyObject *
pyg_option_context_get_help_enabled (PyGOptionContext *self)
{
    return pygi_gboolean_to_py (
        g_option_context_get_help_enabled (self->context));
}

static PyObject *
pyg_option_context_set_ignore_unknown_options (PyGOptionContext *self,
                                               PyObject *args,
                                               PyObject *kwargs)
{
    static char *kwlist[] = { "ignore_unknown_options", NULL };
    PyObject *ignore_unknown_options;

    if (!PyArg_ParseTupleAndKeywords (
            args, kwargs, "O:GOptionContext.set_ignore_unknown_options",
            kwlist, &ignore_unknown_options))
        return NULL;

    g_option_context_set_ignore_unknown_options (
        self->context, PyObject_IsTrue (ignore_unknown_options));


    Py_RETURN_NONE;
}

static PyObject *
pyg_option_context_get_ignore_unknown_options (PyGOptionContext *self)
{
    return pygi_gboolean_to_py (
        g_option_context_get_ignore_unknown_options (self->context));
}

static PyObject *
pyg_option_context_set_main_group (PyGOptionContext *self, PyObject *args,
                                   PyObject *kwargs)
{
    static char *kwlist[] = { "group", NULL };
    GOptionGroup *g_group;
    PyObject *group;

    if (!PyArg_ParseTupleAndKeywords (
            args, kwargs, "O:GOptionContext.set_main_group", kwlist, &group))
        return NULL;

    if (PyObject_IsInstance (group, (PyObject *)&PyGOptionGroup_Type) != 1) {
        PyErr_SetString (
            PyExc_TypeError,
            "GOptionContext.set_main_group expects a GOptionGroup.");
        return NULL;
    }

    g_group = pyglib_option_group_transfer_group (group);
    if (g_group == NULL) {
        PyErr_SetString (PyExc_RuntimeError,
                         "Group is already in a OptionContext.");
        return NULL;
    }

    g_option_context_set_main_group (self->context, g_group);

    self->main_group = (PyGOptionGroup *)Py_NewRef (group);

    Py_RETURN_NONE;
}

static PyObject *
pyg_option_context_get_main_group (PyGOptionContext *self)
{
    if (self->main_group == NULL) {
        Py_RETURN_NONE;
    }
    return Py_NewRef (self->main_group);
}

static PyObject *
pyg_option_context_add_group (PyGOptionContext *self, PyObject *args,
                              PyObject *kwargs)
{
    static char *kwlist[] = { "group", NULL };
    GOptionGroup *g_group;
    PyObject *group;

    if (!PyArg_ParseTupleAndKeywords (
            args, kwargs, "O:GOptionContext.add_group", kwlist, &group))
        return NULL;

    if (PyObject_IsInstance (group, (PyObject *)&PyGOptionGroup_Type) != 1) {
        PyErr_SetString (PyExc_TypeError,
                         "GOptionContext.add_group expects a GOptionGroup.");
        return NULL;
    }

    g_group = pyglib_option_group_transfer_group (group);
    if (g_group == NULL) {
        PyErr_SetString (PyExc_RuntimeError,
                         "Group is already in a OptionContext.");
        return NULL;
    }
    Py_INCREF (group);

    g_option_context_add_group (self->context, g_group);

    Py_RETURN_NONE;
}

static PyObject *
pyg_option_context_richcompare (PyObject *self, PyObject *other, int op)
{
    if (Py_TYPE (self) == Py_TYPE (other)
        && Py_TYPE (self) == &PyGOptionContext_Type)
        return pyg_ptr_richcompare (((PyGOptionContext *)self)->context,
                                    ((PyGOptionContext *)other)->context, op);
    else {
        Py_RETURN_NOTIMPLEMENTED;
    }
}

static PyObject *
pyg_option_get_context (PyGOptionContext *self)
{
    return PyCapsule_New (self->context, "goption.context", NULL);
}

static PyMethodDef pyg_option_context_methods[] = {
    { "parse", (PyCFunction)pyg_option_context_parse,
      METH_VARARGS | METH_KEYWORDS },
    { "set_help_enabled", (PyCFunction)pyg_option_context_set_help_enabled,
      METH_VARARGS | METH_KEYWORDS },
    { "get_help_enabled", (PyCFunction)pyg_option_context_get_help_enabled,
      METH_NOARGS },
    { "set_ignore_unknown_options",
      (PyCFunction)pyg_option_context_set_ignore_unknown_options,
      METH_VARARGS | METH_KEYWORDS },
    { "get_ignore_unknown_options",
      (PyCFunction)pyg_option_context_get_ignore_unknown_options,
      METH_NOARGS },
    { "set_main_group", (PyCFunction)pyg_option_context_set_main_group,
      METH_VARARGS | METH_KEYWORDS },
    { "get_main_group", (PyCFunction)pyg_option_context_get_main_group,
      METH_NOARGS },
    { "add_group", (PyCFunction)pyg_option_context_add_group,
      METH_VARARGS | METH_KEYWORDS },
    { "_get_context", (PyCFunction)pyg_option_get_context, METH_NOARGS },
    { NULL, NULL, 0 },
};

/**
 * Returns 0 on success, or -1 and sets an exception.
 */
int
pygi_option_context_register_types (PyObject *d)
{
    PyGOptionContext_Type.tp_dealloc = (destructor)pyg_option_context_dealloc;
    PyGOptionContext_Type.tp_richcompare = pyg_option_context_richcompare;
    PyGOptionContext_Type.tp_flags = Py_TPFLAGS_DEFAULT;
    PyGOptionContext_Type.tp_methods = pyg_option_context_methods;
    PyGOptionContext_Type.tp_init = (initproc)pyg_option_context_init;
    PyGOptionContext_Type.tp_alloc = PyType_GenericAlloc;
    PyGOptionContext_Type.tp_new = PyType_GenericNew;
    if (PyType_Ready (&PyGOptionContext_Type)) return -1;

    PyDict_SetItemString (d, "OptionContext",
                          (PyObject *)&PyGOptionContext_Type);

    return 0;
}
