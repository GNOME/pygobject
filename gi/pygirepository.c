/* -*- Mode: C; c-basic-offset: 4 -*-
 * vim: tabstop=4 shiftwidth=4 expandtab
 *
 * Copyright (C) 2005-2009 Johan Dahlin <johan@gnome.org>
 *
 *   pygirepository.c: GIRepository wrapper.
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
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301
 * USA
 */

#include "pygi-private.h"

static PyMethodDef _PyGIRepository_methods[];

PyTypeObject PyGIRepository_Type = {
    PyObject_HEAD_INIT(NULL)
    0,
    "gi.Repository",         /* tp_name */
    sizeof(PyGIRepository),  /* tp_basicsize */
    0,                       /* tp_itemsize */
    (destructor)NULL,        /* tp_dealloc */
    (printfunc)NULL,         /* tp_print */
    (getattrfunc)NULL,       /* tp_getattr */
    (setattrfunc)NULL,       /* tp_setattr */
    (cmpfunc)NULL,           /* tp_compare */
    (reprfunc)NULL,          /* tp_repr */
    NULL,                    /* tp_as_number */
    NULL,                    /* tp_as_sequence */
    NULL,                    /* tp_as_mapping */
    (hashfunc)NULL,          /* tp_hash */
    (ternaryfunc)NULL,       /* tp_call */
    (reprfunc)NULL,          /* tp_str */
    (getattrofunc)NULL,      /* tp_getattro */
    (setattrofunc)NULL,      /* tp_setattro */
    NULL,                    /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT,      /* tp_flags */
    NULL,                    /* tp_doc */
    (traverseproc)NULL,      /* tp_traverse */
    (inquiry)NULL,           /* tp_clear */
    (richcmpfunc)NULL,       /* tp_richcompare */
    0,                       /* tp_weaklistoffset */
    (getiterfunc)NULL,       /* tp_iter */
    (iternextfunc)NULL,      /* tp_iternext */
    _PyGIRepository_methods, /* tp_methods */
};

static PyObject *
_wrap_g_irepository_require(PyGIRepository *self, PyObject *args, PyObject *kwargs)
{
    static char *kwlist[] = { "namespace", "lazy", NULL };
    gchar *namespace;
    PyObject *lazy_obj = NULL;
    int flags = 0;
    GTypelib *ret;
    PyObject *pyret;
    GError *error = NULL;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs,
				     "s|O:Repository.require",
				     kwlist, &namespace, &lazy_obj))
        return NULL;

    if (lazy_obj != NULL && PyObject_IsTrue(lazy_obj))
	flags |= G_IREPOSITORY_LOAD_FLAG_LAZY;

    /* TODO - handle versioning in some way, need to figure out what
     * this looks like Python side.
     */
    ret = g_irepository_require(self->repository, namespace, NULL, flags, &error);

    if (ret == NULL) {
#if 0
	g_print ("ERROR: %s (FIXME: raise GError exception)\n",
		 error->message);
	g_clear_error (&error);
#endif
	Py_INCREF(Py_None);
	return Py_None;
    }
    pyret = PyBool_FromLong(ret != NULL);
    Py_INCREF(pyret);
    return pyret;
}

static PyObject *
_wrap_g_irepository_find_by_name(PyGIRepository *self,
				 PyObject *args,
				 PyObject *kwargs)
{
    static char *kwlist[] = { "namespace", "name", NULL };
    char *namespace, *name;
    GIBaseInfo *info;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs,
				     "ss:Repository.find_by_name",
				     kwlist, &namespace, &name))
        return NULL;

    info = g_irepository_find_by_name (self->repository, namespace, name);
    if (!info) {
	Py_INCREF(Py_None);
	return Py_None;
    }

    return pyg_info_new(info);
}

static PyObject *
_wrap_g_irepository_get_namespaces(PyGIRepository *self)
{
    char ** namespaces;
    int i, length;
    PyObject *retval;

    namespaces = g_irepository_get_loaded_namespaces(self->repository);

    length = g_strv_length(namespaces);
    retval = PyTuple_New(length);

    for (i = 0; i < length; i++)
	PyTuple_SetItem(retval, i, PyString_FromString(namespaces[i]));

    g_strfreev (namespaces);

    return retval;
}

static PyObject *
_wrap_g_irepository_get_infos(PyGIRepository *self,
			      PyObject *args,
			      PyObject *kwargs)
{
    static char *kwlist[] = { "namespace", NULL };
    char *namespace;
    int i, length;
    PyObject *retval;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs,
				     "s:Repository.get_infos",
				     kwlist, &namespace))
        return NULL;

    length = g_irepository_get_n_infos(self->repository, namespace);

    retval = PyTuple_New(length);

    for (i = 0; i < length; i++) {
	GIBaseInfo *info = g_irepository_get_info(self->repository, namespace, i);
	PyTuple_SetItem(retval, i, pyg_info_new(info));
    }

    return retval;
}

static PyObject *
_wrap_g_irepository_is_registered(PyGIRepository *self,
				  PyObject *args,
				  PyObject *kwargs)
{
    static char *kwlist[] = { "namespace", NULL };
    char *namespace;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs,
				     "s:Repository.is_registered",
				     kwlist, &namespace))
        return NULL;

    return PyBool_FromLong(g_irepository_is_registered(self->repository, namespace, NULL));
}

static PyObject *
_wrap_g_irepository_get_c_prefix(PyGIRepository *self,
				 PyObject *args,
				 PyObject *kwargs)
{
    static char *kwlist[] = { "namespace", NULL };
    char *namespace;


    if (!PyArg_ParseTupleAndKeywords(args, kwargs,
				     "s:Repository.get_c_prefix",
				     kwlist, &namespace))
        return NULL;

    return PyString_FromString(g_irepository_get_c_prefix(self->repository, namespace));
}

static PyObject *
_wrap_g_irepository_get_default(PyObject *_)
{
    static PyGIRepository *self = NULL;

    if (!self) {
        self = (PyGIRepository *)PyObject_New(PyGIRepository,
					      &PyGIRepository_Type);
	if (self == NULL)
	    return NULL;

	self->repository = g_irepository_get_default();
    }

    return (PyObject*)self;
}

static PyMethodDef _PyGIRepository_methods[] = {
    { "require", (PyCFunction)_wrap_g_irepository_require, METH_VARARGS|METH_KEYWORDS },
    { "get_namespaces", (PyCFunction)_wrap_g_irepository_get_namespaces, METH_NOARGS },
    { "get_infos", (PyCFunction)_wrap_g_irepository_get_infos, METH_VARARGS|METH_KEYWORDS },
    { "get_default", (PyCFunction)_wrap_g_irepository_get_default, METH_STATIC|METH_NOARGS },
    { "find_by_name", (PyCFunction)_wrap_g_irepository_find_by_name, METH_VARARGS|METH_KEYWORDS },
    { "is_registered", (PyCFunction)_wrap_g_irepository_is_registered, METH_VARARGS|METH_KEYWORDS },
    { "get_c_prefix", (PyCFunction)_wrap_g_irepository_get_c_prefix, METH_VARARGS|METH_KEYWORDS },
    { NULL, NULL, 0 }
};

void
pygi_repository_register_types(PyObject *m)
{
    PyGIRepository_Type.ob_type = &PyType_Type;
    PyGIRepository_Type.tp_alloc = PyType_GenericAlloc;
    PyGIRepository_Type.tp_new = PyType_GenericNew;
    if (PyType_Ready(&PyGIRepository_Type)) {
        return;
    }
    if (PyModule_AddObject(m, "Repository", (PyObject *)&PyGIRepository_Type)) {
        return;
    }
    Py_INCREF(&PyGIRepository_Type);
}

