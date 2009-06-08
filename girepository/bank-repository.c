/* -*- Mode: C; c-basic-offset: 4 -*-
 * vim: set ts=8 sts=4 sw=4 noet ai cindent :
 * 
 * Copyright (C) 2005  Johan Dahlin <johan@gnome.org>
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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include "bank.h"

static PyMethodDef _PyGIRepository_methods[];

PyTypeObject PyGIRepository_Type = {
    PyObject_HEAD_INIT(NULL)
    0,
    "bank.IRepository",
    sizeof(PyGIRepository),
    0,
    /* methods */
    (destructor)0,
    (printfunc)0,
    (getattrfunc)0,
    (setattrfunc)0,
    (cmpfunc)0,
    (reprfunc)0,
    0,
    0,
    0,
    (hashfunc)0,
    (ternaryfunc)0,
    (reprfunc)0,
    (getattrofunc)0,
    (setattrofunc)0,
    0,
    Py_TPFLAGS_DEFAULT,
    NULL,
    (traverseproc)0,
    (inquiry)0,
    (richcmpfunc)0,
    0,
    (getiterfunc)0,
    (iternextfunc)0,
    _PyGIRepository_methods,
    0,
    0,
    NULL,
    NULL,
    (descrgetfunc)0,
    (descrsetfunc)0,
    0,
    (initproc)0,
};

static PyObject *
_wrap_g_irepository_require(PyGIRepository *self,
			    PyObject *args,
			    PyObject *kwargs)
{
    static char *kwlist[] = { "namespace", "lazy", NULL };
    gchar *namespace;
    PyObject *lazy_obj = NULL;
    int flags = 0;
    GTypelib *ret;
    PyObject *pyret;
    GError *error = NULL;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs,
				     "s|O:GIRepository.require",
				     kwlist, &namespace, &lazy_obj))
        return NULL;

    if (lazy_obj != NULL && PyObject_IsTrue(lazy_obj))
	flags |= G_IREPOSITORY_LOAD_FLAG_LAZY;

    /* TODO - handle versioning in some way, need to figure out what
     * this looks like Python side.
     */
    ret = g_irepository_require(self->repo, namespace, NULL, flags, &error);

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
				     "ss:GIRepository.findByName",
				     kwlist, &namespace, &name))
        return NULL;

    info = g_irepository_find_by_name (self->repo, namespace, name);
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

    namespaces = g_irepository_get_loaded_namespaces(self->repo);

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
				     "s:GIRepository.getInfos",
				     kwlist, &namespace))
        return NULL;

    length = g_irepository_get_n_infos(self->repo, namespace);

    retval = PyTuple_New(length);

    for (i = 0; i < length; i++) {
	GIBaseInfo *info = g_irepository_get_info(self->repo, namespace, i);
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
				     "s:GIRepository.isRegistered",
				     kwlist, &namespace))
        return NULL;

    return PyBool_FromLong(g_irepository_is_registered(self->repo, namespace, NULL));
}

static PyObject *
_wrap_g_irepository_get_c_prefix(PyGIRepository *self,
				 PyObject *args,
				 PyObject *kwargs)
{
    static char *kwlist[] = { "namespace", NULL };
    char *namespace;


    if (!PyArg_ParseTupleAndKeywords(args, kwargs,
				     "s:GIRepository.getCPrefix",
				     kwlist, &namespace))
        return NULL;

    return PyString_FromString(g_irepository_get_c_prefix(self->repo, namespace));
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

	self->repo = g_irepository_get_default();
    }

    return (PyObject*)self;
}

static PyMethodDef _PyGIRepository_methods[] = {
    { "require", (PyCFunction)_wrap_g_irepository_require, METH_VARARGS|METH_KEYWORDS },
    { "getNamespaces", (PyCFunction)_wrap_g_irepository_get_namespaces, METH_NOARGS },
    { "getInfos", (PyCFunction)_wrap_g_irepository_get_infos, METH_VARARGS|METH_KEYWORDS },
    { "getDefault", (PyCFunction)_wrap_g_irepository_get_default, METH_STATIC|METH_NOARGS },
    { "findByName", (PyCFunction)_wrap_g_irepository_find_by_name, METH_VARARGS|METH_KEYWORDS },
    { "isRegistered", (PyCFunction)_wrap_g_irepository_is_registered, METH_VARARGS|METH_KEYWORDS },
    { "getCPrefix", (PyCFunction)_wrap_g_irepository_get_c_prefix, METH_VARARGS|METH_KEYWORDS },
    { NULL, NULL, 0 }
};

