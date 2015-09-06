/* -*- Mode: C; c-basic-offset: 4 -*-
 * vim: tabstop=4 shiftwidth=4 expandtab
 *
 * Copyright (C) 2005-2009 Johan Dahlin <johan@gnome.org>
 *
 *   pygi-repository.c: GIRepository wrapper.
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

#include "pygi-private.h"

#include <pyglib-python-compat.h>

PyObject *PyGIRepositoryError;

PYGLIB_DEFINE_TYPE("gi.Repository", PyGIRepository_Type, PyGIRepository);

static PyObject *
_wrap_g_irepository_enumerate_versions (PyGIRepository *self,
                                        PyObject       *args,
                                        PyObject       *kwargs)
{
    static char *kwlist[] = { "namespace", NULL };
    const char *namespace_;
    GList *versions, *item;
    PyObject *ret = NULL;

    if (!PyArg_ParseTupleAndKeywords (args, kwargs, "s:Repository.enumerate_versions",
                                      kwlist, &namespace_)) {
        return NULL;
    }

    versions = g_irepository_enumerate_versions (self->repository, namespace_);
    ret = PyList_New(0);
    for (item = versions; item; item = item->next) {
        char *version = item->data;
        PyObject *py_version = PYGLIB_PyUnicode_FromString (version);
        PyList_Append(ret, py_version);
        Py_DECREF(py_version);
        g_free (version);
    }
    g_list_free(versions);

    return ret;
}

static PyObject *
_wrap_g_irepository_get_default (PyObject *self)
{
    static PyGIRepository *repository = NULL;

    if (!repository) {
        repository = (PyGIRepository *) PyObject_New (PyGIRepository, &PyGIRepository_Type);
        if (repository == NULL) {
            return NULL;
        }

        repository->repository = g_irepository_get_default();
    }

    Py_INCREF ( (PyObject *) repository);
    return (PyObject *) repository;
}

static PyObject *
_wrap_g_irepository_require (PyGIRepository *self,
                             PyObject       *args,
                             PyObject       *kwargs)
{
    static char *kwlist[] = { "namespace", "version", "lazy", NULL };

    const char *namespace_;
    const char *version = NULL;
    PyObject *lazy = NULL;
    GIRepositoryLoadFlags flags = 0;
    GError *error;

    if (!PyArg_ParseTupleAndKeywords (args, kwargs, "s|zO:Repository.require",
                                      kwlist, &namespace_, &version, &lazy)) {
        return NULL;
    }

    if (lazy != NULL && PyObject_IsTrue (lazy)) {
        flags |= G_IREPOSITORY_LOAD_FLAG_LAZY;
    }

    error = NULL;
    g_irepository_require (self->repository, namespace_, version, flags, &error);
    if (error != NULL) {
        PyErr_SetString (PyGIRepositoryError, error->message);
        g_error_free (error);
        return NULL;
    }

    Py_RETURN_NONE;
}

static PyObject *
_wrap_g_irepository_is_registered (PyGIRepository *self,
                                   PyObject       *args,
                                   PyObject       *kwargs)
{
    static char *kwlist[] = { "namespace", "version", NULL };
    const char *namespace_;
    const char *version = NULL;

    if (!PyArg_ParseTupleAndKeywords (args, kwargs, "s|z:Repository.is_registered",
                                      kwlist, &namespace_, &version)) {
        return NULL;
    }

    return PyBool_FromLong (g_irepository_is_registered (self->repository,
                                                         namespace_, version));
}

static PyObject *
_wrap_g_irepository_find_by_name (PyGIRepository *self,
                                  PyObject       *args,
                                  PyObject       *kwargs)
{
    static char *kwlist[] = { "namespace", "name", NULL };

    const char *namespace_;
    const char *name;
    GIBaseInfo *info;
    PyObject *py_info;
    size_t len;
    char *trimmed_name = NULL;

    if (!PyArg_ParseTupleAndKeywords (args, kwargs,
                                      "ss:Repository.find_by_name", kwlist, &namespace_, &name)) {
        return NULL;
    }

    /* If the given name ends with an underscore, it might be due to usage
     * as an accessible replacement for something in GI with the same name
     * as a Python keyword. Test for this and trim it out if necessary.
     */
    len = strlen (name);
    if (len > 0 && name[len-1] == '_') {
        trimmed_name = g_strndup (name, len-1);
        if (_pygi_is_python_keyword (trimmed_name)) {
            name = trimmed_name;
        }
    }

    info = g_irepository_find_by_name (self->repository, namespace_, name);
    g_free (trimmed_name);

    if (info == NULL) {
        Py_RETURN_NONE;
    }

    py_info = _pygi_info_new (info);

    g_base_info_unref (info);

    return py_info;
}

static PyObject *
_wrap_g_irepository_get_infos (PyGIRepository *self,
                               PyObject       *args,
                               PyObject       *kwargs)
{
    static char *kwlist[] = { "namespace", NULL };

    const char *namespace_;
    gssize n_infos;
    PyObject *infos;
    gssize i;

    if (!PyArg_ParseTupleAndKeywords (args, kwargs, "s:Repository.get_infos",
                                      kwlist, &namespace_)) {
        return NULL;
    }

    n_infos = g_irepository_get_n_infos (self->repository, namespace_);
    if (n_infos < 0) {
        PyErr_Format (PyExc_RuntimeError, "Namespace '%s' not loaded", namespace_);
        return NULL;
    }

    infos = PyTuple_New (n_infos);

    for (i = 0; i < n_infos; i++) {
        GIBaseInfo *info;
        PyObject *py_info;

        info = g_irepository_get_info (self->repository, namespace_, i);
        g_assert (info != NULL);

        py_info = _pygi_info_new (info);

        g_base_info_unref (info);

        if (py_info == NULL) {
            Py_CLEAR (infos);
            break;
        }

        PyTuple_SET_ITEM (infos, i, py_info);
    }

    return infos;
}

static PyObject *
_wrap_g_irepository_get_typelib_path (PyGIRepository *self,
                                      PyObject       *args,
                                      PyObject       *kwargs)
{
    static char *kwlist[] = { "namespace", NULL };
    const char *namespace_;
    const gchar *typelib_path;

    if (!PyArg_ParseTupleAndKeywords (args, kwargs,
                                      "s:Repository.get_typelib_path", kwlist, &namespace_)) {
        return NULL;
    }

    typelib_path = g_irepository_get_typelib_path (self->repository, namespace_);
    if (typelib_path == NULL) {
        PyErr_Format (PyExc_RuntimeError, "Namespace '%s' not loaded", namespace_);
        return NULL;
    }

    return PYGLIB_PyBytes_FromString (typelib_path);
}

static PyObject *
_wrap_g_irepository_get_version (PyGIRepository *self,
                                 PyObject       *args,
                                 PyObject       *kwargs)
{
    static char *kwlist[] = { "namespace", NULL };
    const char *namespace_;
    const gchar *version;

    if (!PyArg_ParseTupleAndKeywords (args, kwargs,
                                      "s:Repository.get_version", kwlist, &namespace_)) {
        return NULL;
    }

    version = g_irepository_get_version (self->repository, namespace_);
    if (version == NULL) {
        PyErr_Format (PyExc_RuntimeError, "Namespace '%s' not loaded", namespace_);
        return NULL;
    }

    return PYGLIB_PyUnicode_FromString (version);
}

static PyObject *
_wrap_g_irepository_get_loaded_namespaces (PyGIRepository *self)
{
    char **namespaces;
    PyObject *py_namespaces;
    gssize i;

    namespaces = g_irepository_get_loaded_namespaces (self->repository);

    py_namespaces = PyList_New (0);
    for (i = 0; namespaces[i] != NULL; i++) {
        PyObject *py_namespace = PYGLIB_PyUnicode_FromString (namespaces[i]);
        PyList_Append (py_namespaces, py_namespace);
        Py_DECREF(py_namespace);
        g_free (namespaces[i]);
    }

    g_free (namespaces);

    return py_namespaces;
}

static PyObject *
_wrap_g_irepository_get_dependencies (PyGIRepository *self,
                                      PyObject       *args,
                                      PyObject       *kwargs)
{
    static char *kwlist[] = { "namespace", NULL };
    const char *namespace_;
    char **namespaces;
    PyObject *py_namespaces;
    gssize i;

    if (!PyArg_ParseTupleAndKeywords (args, kwargs,
                                      "s:Repository.get_dependencies", kwlist, &namespace_)) {
        return NULL;
    }

    py_namespaces = PyList_New (0);
    /* Returns NULL in case of no dependencies */
    namespaces = g_irepository_get_dependencies (self->repository, namespace_);
    if (namespaces == NULL) {
        return py_namespaces;
    }

    for (i = 0; namespaces[i] != NULL; i++) {
        PyObject *py_namespace = PYGLIB_PyUnicode_FromString (namespaces[i]);
        PyList_Append (py_namespaces, py_namespace);
        Py_DECREF(py_namespace);
    }

    g_strfreev (namespaces);

    return py_namespaces;
}

static PyMethodDef _PyGIRepository_methods[] = {
    { "enumerate_versions", (PyCFunction) _wrap_g_irepository_enumerate_versions, METH_VARARGS | METH_KEYWORDS },
    { "get_default", (PyCFunction) _wrap_g_irepository_get_default, METH_STATIC | METH_NOARGS },
    { "require", (PyCFunction) _wrap_g_irepository_require, METH_VARARGS | METH_KEYWORDS },
    { "get_infos", (PyCFunction) _wrap_g_irepository_get_infos, METH_VARARGS | METH_KEYWORDS },
    { "find_by_name", (PyCFunction) _wrap_g_irepository_find_by_name, METH_VARARGS | METH_KEYWORDS },
    { "get_typelib_path", (PyCFunction) _wrap_g_irepository_get_typelib_path, METH_VARARGS | METH_KEYWORDS },
    { "get_version", (PyCFunction) _wrap_g_irepository_get_version, METH_VARARGS | METH_KEYWORDS },
    { "get_loaded_namespaces", (PyCFunction) _wrap_g_irepository_get_loaded_namespaces, METH_NOARGS },
    { "get_dependencies", (PyCFunction) _wrap_g_irepository_get_dependencies, METH_VARARGS | METH_KEYWORDS  },
    { "is_registered", (PyCFunction) _wrap_g_irepository_is_registered, METH_VARARGS | METH_KEYWORDS  },
    { NULL, NULL, 0 }
};

void
_pygi_repository_register_types (PyObject *m)
{
    Py_TYPE(&PyGIRepository_Type) = &PyType_Type;

    PyGIRepository_Type.tp_flags = Py_TPFLAGS_DEFAULT;
    PyGIRepository_Type.tp_methods = _PyGIRepository_methods;

    if (PyType_Ready (&PyGIRepository_Type)) {
        return;
    }

    if (PyModule_AddObject (m, "Repository", (PyObject *) &PyGIRepository_Type)) {
        return;
    }

    PyGIRepositoryError = PyErr_NewException ("gi.RepositoryError", NULL, NULL);
    if (PyModule_AddObject (m, "RepositoryError", PyGIRepositoryError)) {
        return;
    }
}

