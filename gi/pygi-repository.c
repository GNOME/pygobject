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

#include "pygi-basictype.h"
#include "pygi-info.h"
#include "pygi-repository.h"
#include "pygi-util.h"

PyObject *PyGIRepositoryError;

PYGI_DEFINE_TYPE ("gi.Repository", PyGIRepository_Type, PyGIRepository);


GIRepository *
pygi_repository_get_default (void)
{
    static GIRepository *default_repository = NULL;

    if (default_repository == NULL)
#if GLIB_CHECK_VERSION(2, 85, 0)
        default_repository = gi_repository_dup_default ();
#else
        default_repository = gi_repository_new ();
#endif

    return default_repository;
}

static PyObject *
_wrap_pygi_repository_get_default (PyObject *self)
{
    static PyGIRepository *repository = NULL;

    if (!repository) {
        repository = (PyGIRepository *)PyObject_New (PyGIRepository,
                                                     &PyGIRepository_Type);
        if (repository == NULL) {
            return NULL;
        }

        repository->repository = pygi_repository_get_default ();
    }

    Py_INCREF ((PyObject *)repository);
    return (PyObject *)repository;
}

static PyObject *
_wrap_gi_repository_prepend_library_path (PyGIRepository *self, PyObject *args,
                                          PyObject *kwargs)
{
    static char *kwlist[] = { "directory", NULL };
    const char *directory;

    if (!PyArg_ParseTupleAndKeywords (args, kwargs,
                                      "s:Repository.prepend_library_path",
                                      kwlist, &directory)) {
        return NULL;
    }

    gi_repository_prepend_library_path (self->repository, directory);

    Py_RETURN_NONE;
}

static PyObject *
_wrap_gi_repository_prepend_search_path (PyGIRepository *self, PyObject *args,
                                         PyObject *kwargs)
{
    static char *kwlist[] = { "directory", NULL };
    const char *directory;

    if (!PyArg_ParseTupleAndKeywords (args, kwargs,
                                      "s:Repository.prepend_search_path",
                                      kwlist, &directory)) {
        return NULL;
    }

    gi_repository_prepend_search_path (self->repository, directory);

    Py_RETURN_NONE;
}


static PyObject *
_wrap_gi_repository_enumerate_versions (PyGIRepository *self, PyObject *args,
                                        PyObject *kwargs)
{
    static char *kwlist[] = { "namespace", NULL };
    const char *namespace_;
    char **versions = NULL;
    PyObject *ret = NULL;

    if (!PyArg_ParseTupleAndKeywords (args, kwargs,
                                      "s:Repository.enumerate_versions",
                                      kwlist, &namespace_)) {
        return NULL;
    }

    versions =
        gi_repository_enumerate_versions (self->repository, namespace_, NULL);
    ret = PyList_New (0);
    for (size_t i = 0; versions[i] != NULL; i++) {
        char *version = g_steal_pointer (&versions[i]);
        PyObject *py_version = pygi_utf8_to_py (version);
        PyList_Append (ret, py_version);
        Py_DECREF (py_version);
        g_free (version);
    }
    g_free (versions);

    return ret;
}

static PyObject *
_wrap_gi_repository_require (PyGIRepository *self, PyObject *args,
                             PyObject *kwargs)
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
        flags |= GI_REPOSITORY_LOAD_FLAG_LAZY;
    }

    error = NULL;
    gi_repository_require (self->repository, namespace_, version, flags,
                           &error);
    if (error != NULL) {
        PyErr_SetString (PyGIRepositoryError, error->message);
        g_error_free (error);
        return NULL;
    }

    Py_RETURN_NONE;
}

static PyObject *
_wrap_gi_repository_is_registered (PyGIRepository *self, PyObject *args,
                                   PyObject *kwargs)
{
    static char *kwlist[] = { "namespace", "version", NULL };
    const char *namespace_;
    const char *version = NULL;

    if (!PyArg_ParseTupleAndKeywords (args, kwargs,
                                      "s|z:Repository.is_registered", kwlist,
                                      &namespace_, &version)) {
        return NULL;
    }

    return pygi_gboolean_to_py (
        gi_repository_is_registered (self->repository, namespace_, version));
}

static PyObject *
_wrap_gi_repository_find_by_name (PyGIRepository *self, PyObject *args,
                                  PyObject *kwargs)
{
    static char *kwlist[] = { "namespace", "name", NULL };

    const char *namespace_;
    const char *name;
    GIBaseInfo *info;
    PyObject *py_info;
    size_t len;
    char *trimmed_name = NULL;

    if (!PyArg_ParseTupleAndKeywords (args, kwargs,
                                      "ss:Repository.find_by_name", kwlist,
                                      &namespace_, &name)) {
        return NULL;
    }

    /* If the given name ends with an underscore, it might be due to usage
     * as an accessible replacement for something in GI with the same name
     * as a Python keyword. Test for this and trim it out if necessary.
     */
    len = strlen (name);
    if (len > 0 && name[len - 1] == '_') {
        PyObject *is_keyword;

        trimmed_name = g_strndup (name, len - 1);
        is_keyword = _pygi_is_python_keyword (trimmed_name);
        if (!is_keyword) return NULL;

        if (PyObject_IsTrue (is_keyword)) name = trimmed_name;

        Py_DECREF (is_keyword);
    }

    info = gi_repository_find_by_name (self->repository, namespace_, name);
    g_free (trimmed_name);

    if (info == NULL) {
        Py_RETURN_NONE;
    }

    py_info = _pygi_info_new (info);

    gi_base_info_unref (info);

    return py_info;
}

static PyObject *
_wrap_gi_repository_get_infos (PyGIRepository *self, PyObject *args,
                               PyObject *kwargs)
{
    static char *kwlist[] = { "namespace", NULL };

    const char *namespace_;
    gssize n_infos;
    PyObject *infos;
    gint i;

    if (!PyArg_ParseTupleAndKeywords (args, kwargs, "s:Repository.get_infos",
                                      kwlist, &namespace_)) {
        return NULL;
    }

    n_infos = gi_repository_get_n_infos (self->repository, namespace_);
    if (n_infos < 0) {
        PyErr_Format (PyExc_RuntimeError, "Namespace '%s' not loaded",
                      namespace_);
        return NULL;
    }

    infos = PyTuple_New (n_infos);

    for (i = 0; i < n_infos; i++) {
        GIBaseInfo *info;
        PyObject *py_info;

        info = gi_repository_get_info (self->repository, namespace_, i);
        g_assert (info != NULL);

        py_info = _pygi_info_new (info);

        gi_base_info_unref (info);

        if (py_info == NULL) {
            Py_CLEAR (infos);
            break;
        }

        PyTuple_SET_ITEM (infos, i, py_info);
    }

    return infos;
}

static PyObject *
_wrap_gi_repository_get_typelib_path (PyGIRepository *self, PyObject *args,
                                      PyObject *kwargs)
{
    static char *kwlist[] = { "namespace", NULL };
    const char *namespace_;
    const gchar *typelib_path;

    if (!PyArg_ParseTupleAndKeywords (args, kwargs,
                                      "s:Repository.get_typelib_path", kwlist,
                                      &namespace_)) {
        return NULL;
    }

    typelib_path =
        gi_repository_get_typelib_path (self->repository, namespace_);
    if (typelib_path == NULL) {
        PyErr_Format (PyExc_RuntimeError, "Namespace '%s' not loaded",
                      namespace_);
        return NULL;
    }

    return pygi_filename_to_py (typelib_path);
}

static PyObject *
_wrap_gi_repository_get_version (PyGIRepository *self, PyObject *args,
                                 PyObject *kwargs)
{
    static char *kwlist[] = { "namespace", NULL };
    const char *namespace_;
    const gchar *version;

    if (!PyArg_ParseTupleAndKeywords (args, kwargs, "s:Repository.get_version",
                                      kwlist, &namespace_)) {
        return NULL;
    }

    version = gi_repository_get_version (self->repository, namespace_);
    if (version == NULL) {
        PyErr_Format (PyExc_RuntimeError, "Namespace '%s' not loaded",
                      namespace_);
        return NULL;
    }

    return pygi_utf8_to_py (version);
}

static PyObject *
_wrap_gi_repository_get_loaded_namespaces (PyGIRepository *self)
{
    char **namespaces;
    PyObject *py_namespaces;
    gssize i;

    namespaces = gi_repository_get_loaded_namespaces (self->repository, NULL);

    py_namespaces = PyList_New (0);
    for (i = 0; namespaces[i] != NULL; i++) {
        PyObject *py_namespace = pygi_utf8_to_py (namespaces[i]);
        PyList_Append (py_namespaces, py_namespace);
        Py_DECREF (py_namespace);
        g_free (namespaces[i]);
    }

    g_free (namespaces);

    return py_namespaces;
}

static PyObject *
_wrap_gi_repository_get_dependencies (PyGIRepository *self, PyObject *args,
                                      PyObject *kwargs)
{
    static char *kwlist[] = { "namespace", NULL };
    const char *namespace_;
    char **namespaces;
    PyObject *py_namespaces;
    gssize i;

    if (!PyArg_ParseTupleAndKeywords (args, kwargs,
                                      "s:Repository.get_dependencies", kwlist,
                                      &namespace_)) {
        return NULL;
    }

    py_namespaces = PyList_New (0);
    /* Returns NULL in case of no dependencies */
    namespaces =
        gi_repository_get_dependencies (self->repository, namespace_, NULL);
    if (namespaces == NULL) {
        return py_namespaces;
    }

    for (i = 0; namespaces[i] != NULL; i++) {
        PyObject *py_namespace = pygi_utf8_to_py (namespaces[i]);
        PyList_Append (py_namespaces, py_namespace);
        Py_DECREF (py_namespace);
    }

    g_strfreev (namespaces);

    return py_namespaces;
}


static PyObject *
_wrap_gi_repository_get_immediate_dependencies (PyGIRepository *self,
                                                PyObject *args,
                                                PyObject *kwargs)
{
    static char *kwlist[] = { "namespace", NULL };
    const char *namespace_;
    char **namespaces;
    PyObject *py_namespaces;
    gssize i;

    if (!PyArg_ParseTupleAndKeywords (
            args, kwargs, "s:Repository.get_immediate_dependencies", kwlist,
            &namespace_)) {
        return NULL;
    }

    py_namespaces = PyList_New (0);
    namespaces = gi_repository_get_immediate_dependencies (self->repository,
                                                           namespace_, NULL);

    for (i = 0; namespaces[i] != NULL; i++) {
        PyObject *py_namespace = pygi_utf8_to_py (namespaces[i]);
        PyList_Append (py_namespaces, py_namespace);
        Py_DECREF (py_namespace);
    }

    g_strfreev (namespaces);

    return py_namespaces;
}


static PyMethodDef _PyGIRepository_methods[] = {
    { "get_default", (PyCFunction)_wrap_pygi_repository_get_default,
      METH_NOARGS | METH_STATIC },
    { "enumerate_versions",
      (PyCFunction)_wrap_gi_repository_enumerate_versions,
      METH_VARARGS | METH_KEYWORDS },
    { "require", (PyCFunction)_wrap_gi_repository_require,
      METH_VARARGS | METH_KEYWORDS },
    { "get_infos", (PyCFunction)_wrap_gi_repository_get_infos,
      METH_VARARGS | METH_KEYWORDS },
    { "find_by_name", (PyCFunction)_wrap_gi_repository_find_by_name,
      METH_VARARGS | METH_KEYWORDS },
    { "get_typelib_path", (PyCFunction)_wrap_gi_repository_get_typelib_path,
      METH_VARARGS | METH_KEYWORDS },
    { "get_version", (PyCFunction)_wrap_gi_repository_get_version,
      METH_VARARGS | METH_KEYWORDS },
    { "get_loaded_namespaces",
      (PyCFunction)_wrap_gi_repository_get_loaded_namespaces, METH_NOARGS },
    { "get_dependencies", (PyCFunction)_wrap_gi_repository_get_dependencies,
      METH_VARARGS | METH_KEYWORDS },
    { "get_immediate_dependencies",
      (PyCFunction)_wrap_gi_repository_get_immediate_dependencies,
      METH_VARARGS | METH_KEYWORDS },
    { "is_registered", (PyCFunction)_wrap_gi_repository_is_registered,
      METH_VARARGS | METH_KEYWORDS },
    { "prepend_library_path",
      (PyCFunction)_wrap_gi_repository_prepend_library_path,
      METH_VARARGS | METH_KEYWORDS },
    { "prepend_search_path",
      (PyCFunction)_wrap_gi_repository_prepend_search_path,
      METH_VARARGS | METH_KEYWORDS },
    { NULL, NULL, 0 },
};

/**
 * Returns 0 on success, or -1 and sets an exception.
 */
int
pygi_repository_register_types (PyObject *m)
{
    Py_SET_TYPE (&PyGIRepository_Type, &PyType_Type);

    PyGIRepository_Type.tp_flags = Py_TPFLAGS_DEFAULT;
    PyGIRepository_Type.tp_methods = _PyGIRepository_methods;

    if (PyType_Ready (&PyGIRepository_Type) < 0) return -1;

    Py_INCREF ((PyObject *)&PyGIRepository_Type);
    if (PyModule_AddObject (m, "Repository", (PyObject *)&PyGIRepository_Type)
        < 0) {
        Py_DECREF ((PyObject *)&PyGIRepository_Type);
        return -1;
    }

    PyGIRepositoryError =
        PyErr_NewException ("gi.RepositoryError", NULL, NULL);
    if (PyGIRepositoryError == NULL) return -1;

    Py_INCREF (PyGIRepositoryError);
    if (PyModule_AddObject (m, "RepositoryError", PyGIRepositoryError) < 0) {
        Py_DECREF (PyGIRepositoryError);
        return -1;
    }

    return 0;
}
