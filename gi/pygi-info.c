/* -*- Mode: C; c-basic-offset: 4 -*-
 * vim: tabstop=4 shiftwidth=4 expandtab
 *
 * Copyright (C) 2005-2009 Johan Dahlin <johan@gnome.org>
 * Copyright (C) 2013 Simon Feltman <sfeltman@gnome.org>
 *
 *   pygi-info.c: GI.*Info wrappers.
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
#include "pygi-cache.h"
#include "pygobject-private.h"

#include <pyglib-python-compat.h>


/* _generate_doc_string
 *
 * C wrapper to call Python implemented "gi.docstring.generate_doc_string"
 */
static PyObject *
_generate_doc_string(PyGIBaseInfo *self)
{
    static PyObject *_py_generate_doc_string = NULL;

    if (_py_generate_doc_string == NULL) {
        PyObject *mod = PyImport_ImportModule ("gi.docstring");
        if (!mod)
            return NULL;

        _py_generate_doc_string = PyObject_GetAttrString (mod, "generate_doc_string");
        if (_py_generate_doc_string == NULL) {
            Py_DECREF (mod);
            return NULL;
        }
        Py_DECREF (mod);
    }

    return PyObject_CallFunctionObjArgs (_py_generate_doc_string, self, NULL);
}

static PyObject *
_get_info_string (PyGIBaseInfo *self,
                  const gchar* (*get_info_string)(GIBaseInfo*))
{
    const gchar *value = get_info_string ((GIBaseInfo*)self->info);
    if (value == NULL) {
        Py_RETURN_NONE;
    }
    return PYGLIB_PyUnicode_FromString (value);
}

static PyObject *
_get_child_info (PyGIBaseInfo *self,
                 GIBaseInfo* (*get_child_info)(GIBaseInfo*))
{
    GIBaseInfo *info;
    PyObject *py_info;

    info = get_child_info ((GIBaseInfo*)self->info);
    if (info == NULL) {
        Py_RETURN_NONE;
    }

    py_info = _pygi_info_new (info);
    g_base_info_unref (info);
    return py_info;
}


static PyObject *
_get_child_info_by_name (PyGIBaseInfo *self, PyObject *py_name,
                         GIBaseInfo* (*get_child_info_by_name)(GIBaseInfo*, const gchar*))
{
    GIBaseInfo *info;
    PyObject *py_info;
    char *name;

    if (!PYGLIB_PyUnicode_Check (py_name)) {
        PyErr_SetString (PyExc_TypeError, "expected string name");
        return NULL;
    }

    name = PYGLIB_PyUnicode_AsString (py_name);
    info = get_child_info_by_name ((GIObjectInfo*)self->info, name);
    if (info == NULL) {
        Py_RETURN_NONE;
    }

    py_info = _pygi_info_new (info);
    g_base_info_unref (info);
    return py_info;
}


/* _make_infos_tuple
 *
 * Build a tuple from the common API pattern in GI of having a
 * function which returns a count and an indexed GIBaseInfo
 * in the range of 0 to count;
 */
static PyObject *
_make_infos_tuple (PyGIBaseInfo *self,
                   gint (*get_n_infos)(GIBaseInfo*),
                   GIBaseInfo* (*get_info)(GIBaseInfo*, gint))
{
    gint n_infos;
    PyObject *infos;
    gint i;

    n_infos = get_n_infos ( (GIBaseInfo *) self->info);

    infos = PyTuple_New (n_infos);
    if (infos == NULL) {
        return NULL;
    }

    for (i = 0; i < n_infos; i++) {
        GIBaseInfo *info;
        PyObject *py_info;

        info = (GIBaseInfo *) get_info (self->info, i);
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


/* BaseInfo */

/* We need to be careful about calling g_base_info_get_name because
 * calling it with a GI_INFO_TYPE_TYPE will crash.
 * See: https://bugzilla.gnome.org/show_bug.cgi?id=709456
 */
static const char *
_safe_base_info_get_name (GIBaseInfo *info)
{
    if (g_base_info_get_type (info) == GI_INFO_TYPE_TYPE) {
        return "type_type_instance";
    } else {
        return g_base_info_get_name (info);
    }
}

static void
_base_info_dealloc (PyGIBaseInfo *self)
{
    if (self->inst_weakreflist != NULL)
        PyObject_ClearWeakRefs ( (PyObject *) self);

    g_base_info_unref (self->info);

    if (self->cache != NULL)
        pygi_callable_cache_free ( (PyGICallableCache *) self->cache);

    Py_TYPE (self)->tp_free ((PyObject *)self);
}

static PyObject *
_base_info_repr (PyGIBaseInfo *self)
{

    return PYGLIB_PyUnicode_FromFormat ("%s(%s)",
                                        Py_TYPE( (PyObject *) self)->tp_name,
                                        _safe_base_info_get_name (self->info));
}

static PyObject *
_wrap_g_base_info_equal (PyGIBaseInfo *self, PyObject *other)
{
    GIBaseInfo *other_info;

    if (!PyObject_TypeCheck (other, &PyGIBaseInfo_Type)) {
        Py_INCREF (Py_NotImplemented);
        return Py_NotImplemented;
    }

    other_info = ((PyGIBaseInfo *)other)->info;
    if (g_base_info_equal (self->info, other_info)) {
        Py_RETURN_TRUE;
    } else {
        Py_RETURN_FALSE;
    }
}

static PyObject *
_base_info_richcompare (PyGIBaseInfo *self, PyObject *other, int op)
{
    PyObject *res;

    switch (op) {
        case Py_EQ:
            return _wrap_g_base_info_equal (self, other);
        case Py_NE:
            res = _wrap_g_base_info_equal (self, other);
            if (res == Py_True) {
                Py_DECREF (res);
                Py_RETURN_FALSE;
            } else {
                Py_DECREF (res);
                Py_RETURN_TRUE;
            }
        default:
            res = Py_NotImplemented;
            break;
    }
    Py_INCREF(res);
    return res;
}

PYGLIB_DEFINE_TYPE("gi.BaseInfo", PyGIBaseInfo_Type, PyGIBaseInfo);

gboolean
_pygi_is_python_keyword (const gchar *name)
{
    /* It may be better to use keyword.iskeyword(); keep in sync with
     * python -c 'import keyword; print(keyword.kwlist)' */
#if PY_VERSION_HEX < 0x03000000
    /* Python 2.x */
    static const gchar* keywords[] = {"and", "as", "assert", "break", "class",
        "continue", "def", "del", "elif", "else", "except", "exec", "finally",
        "for", "from", "global", "if", "import", "in", "is", "lambda", "not",
        "or", "pass", "print", "raise", "return", "try", "while", "with",
        "yield", NULL};
#elif PY_VERSION_HEX < 0x04000000
    /* Python 3.x; note that we explicitly keep "print"; it is not a keyword
     * any more, but we do not want to break API between Python versions */
    static const gchar* keywords[] = {"False", "None", "True", "and", "as",
        "assert", "break", "class", "continue", "def", "del", "elif", "else",
        "except", "finally", "for", "from", "global", "if", "import", "in",
        "is", "lambda", "nonlocal", "not", "or", "pass", "raise", "return",
        "try", "while", "with", "yield",
        "print", NULL};
#else
    #error Need keyword list for this major Python version
#endif

    const gchar **i;

    for (i = keywords; *i != NULL; ++i) {
        if (strcmp (name, *i) == 0) {
            return TRUE;
        }
    }

    return FALSE;
}

static PyObject *
_wrap_g_base_info_get_type (PyGIBaseInfo *self)
{
    return PYGLIB_PyLong_FromLong (g_base_info_get_type (self->info));
}

static PyObject *
_wrap_g_base_info_get_name (PyGIBaseInfo *self)
{
    const gchar *name;

    name = _safe_base_info_get_name (self->info);

    /* escape keywords */
    if (_pygi_is_python_keyword (name)) {
        gchar *escaped = g_strconcat (name, "_", NULL);
        PyObject *obj = PYGLIB_PyUnicode_FromString (escaped);
        g_free (escaped);
        return obj;
    }

    return PYGLIB_PyUnicode_FromString (name);
}

static PyObject *
_wrap_g_base_info_get_name_unescaped (PyGIBaseInfo *self)
{
    return _get_info_string (self, _safe_base_info_get_name);
}

static PyObject *
_wrap_g_base_info_get_namespace (PyGIBaseInfo *self)
{
    return _get_info_string (self, g_base_info_get_namespace);
}

static PyObject *
_wrap_g_base_info_is_deprecated (PyGIBaseInfo *self)
{
    if (g_base_info_is_deprecated (self->info))
        Py_RETURN_TRUE;
    else
        Py_RETURN_FALSE;
}

static PyObject *
_wrap_g_base_info_get_attribute (PyGIBaseInfo *self, PyObject *arg)
{
    char *name;
    const char *value;

    if (!PYGLIB_PyUnicode_Check (arg)) {
        PyErr_SetString (PyExc_TypeError, "expected string name");
        return NULL;
    }

    name = PYGLIB_PyUnicode_AsString (arg);
    value = g_base_info_get_attribute (self->info, name);
    if (value == NULL) {
        Py_RETURN_NONE;
    }
    return PYGLIB_PyUnicode_FromString (value);
}

static PyObject *
_wrap_g_base_info_get_container (PyGIBaseInfo *self)
{
    /* Note: don't use _get_child_info because g_base_info_get_container
     * is marked as [transfer none] and therefore returns a borrowed ref.
     */
    GIBaseInfo *info;

    info = g_base_info_get_container (self->info);

    if (info == NULL) {
        Py_RETURN_NONE;
    }

    return _pygi_info_new (info);
}


static PyMethodDef _PyGIBaseInfo_methods[] = {
    { "get_type", (PyCFunction) _wrap_g_base_info_get_type, METH_NOARGS },
    { "get_name", (PyCFunction) _wrap_g_base_info_get_name, METH_NOARGS },
    { "get_name_unescaped", (PyCFunction) _wrap_g_base_info_get_name_unescaped, METH_NOARGS },
    { "get_namespace", (PyCFunction) _wrap_g_base_info_get_namespace, METH_NOARGS },
    { "is_deprecated", (PyCFunction) _wrap_g_base_info_is_deprecated, METH_NOARGS },
    { "get_attribute", (PyCFunction) _wrap_g_base_info_get_attribute, METH_O },
    { "get_container", (PyCFunction) _wrap_g_base_info_get_container, METH_NOARGS },
    { "equal", (PyCFunction) _wrap_g_base_info_equal, METH_O },
    { NULL, NULL, 0 }
};

/* _base_info_getattro:
 *
 * The usage of __getattr__ is needed because the get/set method table
 * does not work for __doc__.
 */
static PyObject *
_base_info_getattro(PyGIBaseInfo *self, PyObject *name)
{
    PyObject *result;

    static PyObject *docstr;
    if (docstr == NULL) {
        docstr= PYGLIB_PyUnicode_InternFromString("__doc__");
        if (docstr == NULL)
            return NULL;
    }

    Py_INCREF (name);
    PYGLIB_PyUnicode_InternInPlace (&name);

    if (name == docstr) {
        result = _generate_doc_string (self);
    } else {
        result = PyObject_GenericGetAttr ((PyObject *)self, name);
    }

    Py_DECREF (name);
    return result;
}

static PyObject *
_base_info_attr_name(PyGIBaseInfo *self, void *closure)
{
    return _wrap_g_base_info_get_name (self);
}

static PyObject *
_base_info_attr_module(PyGIBaseInfo *self, void *closure)
{
    return PYGLIB_PyUnicode_FromFormat ("gi.repository.%s",
                                        g_base_info_get_namespace (self->info));
}

static PyGetSetDef _base_info_getsets[] = {
        { "__name__", (getter)_base_info_attr_name, (setter)0, "Name", NULL},
        { "__module__", (getter)_base_info_attr_module, (setter)0, "Module name", NULL},
    { NULL, 0, 0 }
};

PyObject *
_pygi_info_new (GIBaseInfo *info)
{
    GIInfoType info_type;
    PyTypeObject *type = NULL;
    PyGIBaseInfo *self;

    info_type = g_base_info_get_type (info);

    switch (info_type)
    {
        case GI_INFO_TYPE_INVALID:
            PyErr_SetString (PyExc_RuntimeError, "Invalid info type");
            return NULL;
        case GI_INFO_TYPE_FUNCTION:
            type = &PyGIFunctionInfo_Type;
            break;
        case GI_INFO_TYPE_CALLBACK:
            type = &PyGICallbackInfo_Type;
            break;
        case GI_INFO_TYPE_STRUCT:
        case GI_INFO_TYPE_BOXED:
            type = &PyGIStructInfo_Type;
            break;
        case GI_INFO_TYPE_ENUM:
        case GI_INFO_TYPE_FLAGS:
            type = &PyGIEnumInfo_Type;
            break;
        case GI_INFO_TYPE_OBJECT:
            type = &PyGIObjectInfo_Type;
            break;
        case GI_INFO_TYPE_INTERFACE:
            type = &PyGIInterfaceInfo_Type;
            break;
        case GI_INFO_TYPE_CONSTANT:
            type = &PyGIConstantInfo_Type;
            break;
        case GI_INFO_TYPE_UNION:
            type = &PyGIUnionInfo_Type;
            break;
        case GI_INFO_TYPE_VALUE:
            type = &PyGIValueInfo_Type;
            break;
        case GI_INFO_TYPE_SIGNAL:
            type = &PyGISignalInfo_Type;
            break;
        case GI_INFO_TYPE_VFUNC:
            type = &PyGIVFuncInfo_Type;
            break;
        case GI_INFO_TYPE_PROPERTY:
            type = &PyGIPropertyInfo_Type;
            break;
        case GI_INFO_TYPE_FIELD:
            type = &PyGIFieldInfo_Type;
            break;
        case GI_INFO_TYPE_ARG:
            type = &PyGIArgInfo_Type;
            break;
        case GI_INFO_TYPE_TYPE:
            type = &PyGITypeInfo_Type;
            break;
        case GI_INFO_TYPE_UNRESOLVED:
            type = &PyGIUnresolvedInfo_Type;
            break;
        default:
            g_assert_not_reached();
            break;
    }

    self = (PyGIBaseInfo *) type->tp_alloc (type, 0);
    if (self == NULL) {
        return NULL;
    }

    self->info = g_base_info_ref (info);
    self->inst_weakreflist = NULL;
    self->cache = NULL;

    return (PyObject *) self;
}

GIBaseInfo *
_pygi_object_get_gi_info (PyObject     *object,
                          PyTypeObject *type)
{
    PyObject *py_info;
    GIBaseInfo *info = NULL;

    py_info = PyObject_GetAttrString (object, "__info__");
    if (py_info == NULL) {
        return NULL;
    }
    if (!PyObject_TypeCheck (py_info, type)) {
        PyErr_Format (PyExc_TypeError, "attribute '__info__' must be %s, not %s",
                      type->tp_name, Py_TYPE(py_info)->tp_name);
        goto out;
    }

    info = ( (PyGIBaseInfo *) py_info)->info;
    g_base_info_ref (info);

out:
    Py_DECREF (py_info);

    return info;
}


/* CallableInfo */
PYGLIB_DEFINE_TYPE ("gi.CallableInfo", PyGICallableInfo_Type, PyGICallableInfo);

/* _callable_info_call:
 *
 * Shared wrapper for invoke which can be bound (instance method or class constructor)
 * or unbound (function or static method).
 */
static PyObject *
_callable_info_call (PyGICallableInfo *self, PyObject *args, PyObject *kwargs)
{
    /* Insert the bound arg at the beginning of the invoke method args. */
    if (self->py_bound_arg) {
        int i;
        PyObject *result;
        Py_ssize_t argcount = PyTuple_Size (args);
        PyObject *newargs = PyTuple_New (argcount + 1);
        if (newargs == NULL)
            return NULL;

        Py_INCREF (self->py_bound_arg);
        PyTuple_SET_ITEM (newargs, 0, self->py_bound_arg);

        for (i = 0; i < argcount; i++) {
            PyObject *v = PyTuple_GET_ITEM (args, i);
            Py_XINCREF (v);
            PyTuple_SET_ITEM (newargs, i+1, v);
        }

        /* Invoke with the original GI info struct this wrapper was based upon.
         * This is necessary to maintain the same cache for all bound versions.
         */
        result = _wrap_g_callable_info_invoke ((PyGIBaseInfo *)self->py_unbound_info,
                                               newargs, kwargs);
        Py_DECREF (newargs);
        return result;

    } else {
        /* We should never have an unbound info when calling when calling invoke
         * at this point because the descriptor implementation on sub-classes
         * should return "self" not a copy when there is no bound arg.
         */
        g_assert (self->py_unbound_info == NULL);
        return _wrap_g_callable_info_invoke ((PyGIBaseInfo *)self, args, kwargs);
    }
}


/* _function_info_call:
 *
 * Specialization of _callable_info_call for GIFunctionInfo which
 * handles constructor error conditions.
 */
static PyObject *
_function_info_call (PyGICallableInfo *self, PyObject *args, PyObject *kwargs)
{
    if (self->py_bound_arg) {
        GIFunctionInfoFlags flags;

        /* Ensure constructors are only called as class methods on the class
         * implementing the constructor and not on sub-classes.
         */
        flags = g_function_info_get_flags ( (GIFunctionInfo*) self->base.info);
        if (flags & GI_FUNCTION_IS_CONSTRUCTOR) {
            PyObject *py_str_name;
            const gchar *str_name;
            GIBaseInfo *container_info = g_base_info_get_container (self->base.info);
            g_assert (container_info != NULL);

            py_str_name = PyObject_GetAttrString (self->py_bound_arg, "__name__");
            if (py_str_name == NULL)
                return NULL;

            if (PyUnicode_Check (py_str_name) ) {
                PyObject *tmp = PyUnicode_AsUTF8String (py_str_name);
                Py_DECREF (py_str_name);
                py_str_name = tmp;
            }

#if PY_VERSION_HEX < 0x03000000
            str_name = PyString_AsString (py_str_name);
#else
            str_name = PyBytes_AsString (py_str_name);
#endif

            if (strcmp (str_name, _safe_base_info_get_name (container_info))) {
                PyErr_Format (PyExc_TypeError,
                              "%s constructor cannot be used to create instances of "
                              "a subclass %s",
                              _safe_base_info_get_name (container_info),
                              str_name);
                Py_DECREF (py_str_name);
                return NULL;
            }
            Py_DECREF (py_str_name);
        }
    }

    return _callable_info_call (self, args, kwargs);
}

/* _new_bound_callable_info
 *
 * Utility function for sub-classes to create a bound version of themself.
 */
static PyGICallableInfo *
_new_bound_callable_info (PyGICallableInfo *self, PyObject *bound_arg)
{
    PyGICallableInfo *new_self;

    /* Return self if this is already bound or there is nothing passed to bind.  */
    if (self->py_bound_arg != NULL || bound_arg == NULL || bound_arg == Py_None) {
        Py_INCREF ((PyObject *)self);
        return self;
    }

    new_self = (PyGICallableInfo *)_pygi_info_new (self->base.info);
    if (new_self == NULL)
        return NULL;

    Py_INCREF ((PyObject *)self);
    new_self->py_unbound_info = (struct PyGICallableInfo *)self;

    Py_INCREF (bound_arg);
    new_self->py_bound_arg = bound_arg;

    return new_self;
}

/* _function_info_descr_get
 *
 * Descriptor protocol implementation for functions, methods, and constructors.
 */
static PyObject *
_function_info_descr_get (PyGICallableInfo *self, PyObject *obj, PyObject *type) {
    GIFunctionInfoFlags flags;
    PyObject *bound_arg = NULL;

    flags = g_function_info_get_flags ( (GIFunctionInfo*) self->base.info);
    if (flags & GI_FUNCTION_IS_CONSTRUCTOR) {
        if (type == NULL)
            bound_arg = (PyObject *)(Py_TYPE(obj));
        else
            bound_arg = type;
    } else if (flags & GI_FUNCTION_IS_METHOD) {
        bound_arg = obj;
    }

    return (PyObject *)_new_bound_callable_info (self, bound_arg);
}

/* _vfunc_info_descr_get
 *
 * Descriptor protocol implementation for virtual functions.
 */
static PyObject *
_vfunc_info_descr_get (PyGICallableInfo *self, PyObject *obj, PyObject *type) {
    PyObject *result;
    PyObject *bound_arg = NULL;

    bound_arg = PyObject_GetAttrString (type, "__gtype__");
    if (bound_arg == NULL)
        return NULL;

    /* _new_bound_callable_info adds its own ref so free the one from GetAttrString */
    result = (PyObject *)_new_bound_callable_info (self, bound_arg);
    Py_DECREF (bound_arg);
    return result;
}

static void
_callable_info_dealloc (PyGICallableInfo *self)
{
    Py_CLEAR (self->py_unbound_info);
    Py_CLEAR (self->py_bound_arg);

    PyGIBaseInfo_Type.tp_dealloc ((PyObject *) self);
}

static PyObject *
_wrap_g_callable_info_get_arguments (PyGIBaseInfo *self)
{
    return _make_infos_tuple (self, g_callable_info_get_n_args, g_callable_info_get_arg);
}

static PyObject *
_wrap_g_callable_info_get_return_type (PyGIBaseInfo *self)
{
    return _get_child_info (self, g_callable_info_get_return_type);
}

static PyObject *
_wrap_g_callable_info_get_caller_owns (PyGIBaseInfo *self)
{
    return PYGLIB_PyLong_FromLong (
            g_callable_info_get_caller_owns (self->info) );
}

static PyObject *
_wrap_g_callable_info_may_return_null (PyGIBaseInfo *self)
{
    return PyBool_FromLong (
            g_callable_info_may_return_null (self->info) );
}

static PyObject *
_wrap_g_callable_info_skip_return (PyGIBaseInfo *self)
{
    return PyBool_FromLong (g_callable_info_skip_return (self->info));
}

static PyObject *
_wrap_g_callable_info_get_return_attribute (PyGIBaseInfo *self, PyObject *py_name)
{
    gchar *name;
    const gchar *attr;

    if (!PYGLIB_PyUnicode_Check (py_name)) {
        PyErr_SetString (PyExc_TypeError, "expected string name");
        return NULL;
    }

    name = PYGLIB_PyUnicode_AsString (py_name);
    attr = g_callable_info_get_return_attribute (self->info, name);
    if (attr) {
        return PYGLIB_PyUnicode_FromString (
                g_callable_info_get_return_attribute (self->info, name));
    } else {
        PyErr_Format(PyExc_AttributeError, "return attribute %s not found", name);
        return NULL;
    }
}

static PyObject *
_wrap_g_callable_info_can_throw_gerror (PyGIBaseInfo *self)
{
    if (g_callable_info_can_throw_gerror (self->info))
        Py_RETURN_TRUE;
    else
        Py_RETURN_FALSE;
}

static PyMethodDef _PyGICallableInfo_methods[] = {
    { "invoke", (PyCFunction) _wrap_g_callable_info_invoke, METH_VARARGS | METH_KEYWORDS },
    { "get_arguments", (PyCFunction) _wrap_g_callable_info_get_arguments, METH_NOARGS },
    { "get_return_type", (PyCFunction) _wrap_g_callable_info_get_return_type, METH_NOARGS },
    { "get_caller_owns", (PyCFunction) _wrap_g_callable_info_get_caller_owns, METH_NOARGS },
    { "may_return_null", (PyCFunction) _wrap_g_callable_info_may_return_null, METH_NOARGS },
    { "skip_return", (PyCFunction) _wrap_g_callable_info_skip_return, METH_NOARGS },
    { "get_return_attribute", (PyCFunction) _wrap_g_callable_info_get_return_attribute, METH_O },
    { "can_throw_gerror", (PyCFunction) _wrap_g_callable_info_can_throw_gerror, METH_NOARGS },
    { NULL, NULL, 0 }
};

/* CallbackInfo */
PYGLIB_DEFINE_TYPE ("gi.CallbackInfo", PyGICallbackInfo_Type, PyGICallableInfo);

static PyMethodDef _PyGICallbackInfo_methods[] = {
    { NULL, NULL, 0 }
};

/* ErrorDomainInfo */
PYGLIB_DEFINE_TYPE ("gi.ErrorDomainInfo", PyGIErrorDomainInfo_Type, PyGIBaseInfo);

static PyMethodDef _PyGIErrorDomainInfo_methods[] = {
    { NULL, NULL, 0 }
};

/* SignalInfo */
PYGLIB_DEFINE_TYPE ("gi.SignalInfo", PyGISignalInfo_Type, PyGICallableInfo);

static PyObject *
_wrap_g_signal_info_get_flags (PyGIBaseInfo *self)
{
    return PYGLIB_PyLong_FromLong (
            g_signal_info_get_flags ((GISignalInfo *)self->info) );
}

static PyObject *
_wrap_g_signal_info_get_class_closure (PyGIBaseInfo *self)
{
    return _get_child_info (self, g_signal_info_get_class_closure);
}

static PyObject *
_wrap_g_signal_info_true_stops_emit (PyGIBaseInfo *self)
{
    return PyBool_FromLong (
            g_signal_info_true_stops_emit ((GISignalInfo *)self->info) );
}

static PyMethodDef _PyGISignalInfo_methods[] = {
    { "get_flags", (PyCFunction) _wrap_g_signal_info_get_flags, METH_NOARGS },
    { "get_class_closure", (PyCFunction) _wrap_g_signal_info_get_class_closure, METH_NOARGS },
    { "true_stops_emit", (PyCFunction) _wrap_g_signal_info_true_stops_emit, METH_NOARGS },
    { NULL, NULL, 0 }
};

/* PropertyInfo */
PYGLIB_DEFINE_TYPE ("gi.PropertyInfo", PyGIPropertyInfo_Type, PyGIBaseInfo);

static PyObject *
_wrap_g_property_info_get_flags (PyGIBaseInfo *self)
{
    return PYGLIB_PyLong_FromLong (
            g_property_info_get_flags ((GIPropertyInfo *)self->info) );
}

static PyObject *
_wrap_g_property_info_get_type (PyGIBaseInfo *self)
{
    return _get_child_info (self, g_property_info_get_type);
}

static PyObject *
_wrap_g_property_info_get_ownership_transfer (PyGIBaseInfo *self)
{
    return PYGLIB_PyLong_FromLong (
            g_property_info_get_ownership_transfer ((GIPropertyInfo *)self->info) );
}

static PyMethodDef _PyGIPropertyInfo_methods[] = {
    { "get_flags", (PyCFunction) _wrap_g_property_info_get_flags, METH_NOARGS },
    { "get_type", (PyCFunction) _wrap_g_property_info_get_type, METH_NOARGS },
    { "get_ownership_transfer", (PyCFunction) _wrap_g_property_info_get_ownership_transfer, METH_NOARGS },
    { NULL, NULL, 0 }
};


/* ArgInfo */
PYGLIB_DEFINE_TYPE ("gi.ArgInfo", PyGIArgInfo_Type, PyGIBaseInfo);

static PyObject *
_wrap_g_arg_info_get_direction (PyGIBaseInfo *self)
{
    return PYGLIB_PyLong_FromLong (
	    g_arg_info_get_direction ((GIArgInfo*)self->info) );
}

static PyObject *
_wrap_g_arg_info_is_caller_allocates (PyGIBaseInfo *self)
{
    return PyBool_FromLong (
	    g_arg_info_is_caller_allocates ((GIArgInfo*)self->info) );
}

static PyObject *
_wrap_g_arg_info_is_return_value (PyGIBaseInfo *self)
{
    return PyBool_FromLong (
	    g_arg_info_is_return_value ((GIArgInfo*)self->info) );
}

static PyObject *
_wrap_g_arg_info_is_optional (PyGIBaseInfo *self)
{
    return PyBool_FromLong (
	    g_arg_info_is_optional ((GIArgInfo*)self->info) );
}

static PyObject *
_wrap_g_arg_info_may_be_null (PyGIBaseInfo *self)
{
    return PyBool_FromLong (
	    g_arg_info_may_be_null ((GIArgInfo*)self->info) );
}

static PyObject *
_wrap_g_arg_info_get_ownership_transfer (PyGIBaseInfo *self)
{
    return PYGLIB_PyLong_FromLong (
            g_arg_info_get_ownership_transfer ((GIArgInfo *)self->info) );
}

static PyObject *
_wrap_g_arg_info_get_scope (PyGIBaseInfo *self)
{
    return PYGLIB_PyLong_FromLong (
            g_arg_info_get_scope ((GIArgInfo *)self->info) );
}

static PyObject *
_wrap_g_arg_info_get_closure (PyGIBaseInfo *self)
{
    return PYGLIB_PyLong_FromLong (
            g_arg_info_get_closure ((GIArgInfo *)self->info) );
}

static PyObject *
_wrap_g_arg_info_get_destroy (PyGIBaseInfo *self)
{
    return PYGLIB_PyLong_FromLong (
            g_arg_info_get_destroy ((GIArgInfo *)self->info) );
}

static PyObject *
_wrap_g_arg_info_get_type (PyGIBaseInfo *self)
{
    return _get_child_info (self, g_arg_info_get_type);
}

static PyMethodDef _PyGIArgInfo_methods[] = {
    { "get_direction", (PyCFunction) _wrap_g_arg_info_get_direction, METH_NOARGS },
    { "is_caller_allocates", (PyCFunction) _wrap_g_arg_info_is_caller_allocates, METH_NOARGS },
    { "is_return_value", (PyCFunction) _wrap_g_arg_info_is_return_value, METH_NOARGS },
    { "is_optional", (PyCFunction) _wrap_g_arg_info_is_optional, METH_NOARGS },
    { "may_be_null", (PyCFunction) _wrap_g_arg_info_may_be_null, METH_NOARGS },
    { "get_ownership_transfer", (PyCFunction) _wrap_g_arg_info_get_ownership_transfer, METH_NOARGS },
    { "get_scope", (PyCFunction) _wrap_g_arg_info_get_scope, METH_NOARGS },
    { "get_closure", (PyCFunction) _wrap_g_arg_info_get_closure, METH_NOARGS },
    { "get_destroy", (PyCFunction) _wrap_g_arg_info_get_destroy, METH_NOARGS },
    { "get_type", (PyCFunction) _wrap_g_arg_info_get_type, METH_NOARGS },
    { NULL, NULL, 0 }
};


/* TypeInfo */
PYGLIB_DEFINE_TYPE ("gi.TypeInfo", PyGITypeInfo_Type, PyGIBaseInfo);

static PyObject *
_wrap_g_type_info_is_pointer (PyGIBaseInfo *self)
{
    return PyBool_FromLong (g_type_info_is_pointer (self->info));
}

static PyObject *
_wrap_g_type_info_get_tag (PyGIBaseInfo *self)
{
    return PYGLIB_PyLong_FromLong (g_type_info_get_tag (self->info));
}

static PyObject *
_wrap_g_type_info_get_tag_as_string (PyGIBaseInfo *self)
{
    GITypeTag tag = g_type_info_get_tag (self->info);
    return PYGLIB_PyUnicode_FromString (g_type_tag_to_string(tag));
}

static PyObject *
_wrap_g_type_info_get_param_type (PyGIBaseInfo *self, PyObject *py_n)
{
    GIBaseInfo *info;
    PyObject *py_info;
    gint n;

    if (!PYGLIB_PyLong_Check (py_n)) {
        PyErr_SetString(PyExc_TypeError, "expected integer value");
        return NULL;
    }

    n = PYGLIB_PyLong_AsLong (py_n);
    info = (GIBaseInfo *) g_type_info_get_param_type ( (GITypeInfo *) self->info, n);
    if (info == NULL) {
        Py_RETURN_NONE;
    }

    py_info = _pygi_info_new (info);
    g_base_info_unref (info);
    return py_info;
}

static PyObject *
_wrap_g_type_info_get_interface (PyGIBaseInfo *self)
{
    return _get_child_info (self, g_type_info_get_interface);
}

static PyObject *
_wrap_g_type_info_get_array_length (PyGIBaseInfo *self)
{
    return PYGLIB_PyLong_FromLong (g_type_info_get_array_length (self->info));
}

static PyObject *
_wrap_g_type_info_get_array_fixed_size (PyGIBaseInfo *self)
{
    return PYGLIB_PyLong_FromLong (g_type_info_get_array_fixed_size (self->info));
}

static PyObject *
_wrap_g_type_info_is_zero_terminated (PyGIBaseInfo *self)
{
    return PyBool_FromLong (g_type_info_is_zero_terminated (self->info));
}

static PyObject *
_wrap_g_type_info_get_array_type (PyGIBaseInfo *self)
{
    return PYGLIB_PyLong_FromLong (g_type_info_get_array_type (self->info));
}

static PyMethodDef _PyGITypeInfo_methods[] = {
    { "is_pointer", (PyCFunction) _wrap_g_type_info_is_pointer, METH_NOARGS },
    { "get_tag", (PyCFunction) _wrap_g_type_info_get_tag, METH_NOARGS },
    { "get_tag_as_string", (PyCFunction) _wrap_g_type_info_get_tag_as_string, METH_NOARGS },
    { "get_param_type", (PyCFunction) _wrap_g_type_info_get_param_type, METH_O },
    { "get_interface", (PyCFunction) _wrap_g_type_info_get_interface, METH_NOARGS },
    { "get_array_length", (PyCFunction) _wrap_g_type_info_get_array_length, METH_NOARGS },
    { "get_array_fixed_size", (PyCFunction) _wrap_g_type_info_get_array_fixed_size, METH_NOARGS },
    { "is_zero_terminated", (PyCFunction) _wrap_g_type_info_is_zero_terminated, METH_NOARGS },
    { "get_array_type", (PyCFunction) _wrap_g_type_info_get_array_type, METH_NOARGS },
    { NULL, NULL, 0 }
};


/* FunctionInfo */
PYGLIB_DEFINE_TYPE ("gi.FunctionInfo", PyGIFunctionInfo_Type, PyGICallableInfo);

static PyObject *
_wrap_g_function_info_is_constructor (PyGIBaseInfo *self)
{
    GIFunctionInfoFlags flags;
    gboolean is_constructor;

    flags = g_function_info_get_flags ( (GIFunctionInfo*) self->info);
    is_constructor = flags & GI_FUNCTION_IS_CONSTRUCTOR;

    return PyBool_FromLong (is_constructor);
}

static PyObject *
_wrap_g_function_info_is_method (PyGIBaseInfo *self)
{
    GIFunctionInfoFlags flags;
    gboolean is_method;

    flags = g_function_info_get_flags ( (GIFunctionInfo*) self->info);
    is_method = flags & GI_FUNCTION_IS_METHOD;

    return PyBool_FromLong (is_method);
}

gsize
_pygi_g_type_tag_size (GITypeTag type_tag)
{
    gsize size = 0;

    switch (type_tag) {
        case GI_TYPE_TAG_BOOLEAN:
            size = sizeof (gboolean);
            break;
        case GI_TYPE_TAG_INT8:
        case GI_TYPE_TAG_UINT8:
            size = sizeof (gint8);
            break;
        case GI_TYPE_TAG_INT16:
        case GI_TYPE_TAG_UINT16:
            size = sizeof (gint16);
            break;
        case GI_TYPE_TAG_INT32:
        case GI_TYPE_TAG_UINT32:
            size = sizeof (gint32);
            break;
        case GI_TYPE_TAG_INT64:
        case GI_TYPE_TAG_UINT64:
            size = sizeof (gint64);
            break;
        case GI_TYPE_TAG_FLOAT:
            size = sizeof (gfloat);
            break;
        case GI_TYPE_TAG_DOUBLE:
            size = sizeof (gdouble);
            break;
        case GI_TYPE_TAG_GTYPE:
            size = sizeof (GType);
            break;
        case GI_TYPE_TAG_UNICHAR:
            size = sizeof (gunichar);
            break;
        case GI_TYPE_TAG_VOID:
        case GI_TYPE_TAG_UTF8:
        case GI_TYPE_TAG_FILENAME:
        case GI_TYPE_TAG_ARRAY:
        case GI_TYPE_TAG_INTERFACE:
        case GI_TYPE_TAG_GLIST:
        case GI_TYPE_TAG_GSLIST:
        case GI_TYPE_TAG_GHASH:
        case GI_TYPE_TAG_ERROR:
            PyErr_Format (PyExc_TypeError,
                          "Unable to know the size (assuming %s is not a pointer)",
                          g_type_tag_to_string (type_tag));
            break;
    }

    return size;
}

gsize
_pygi_g_type_info_size (GITypeInfo *type_info)
{
    gsize size = 0;

    GITypeTag type_tag;

    type_tag = g_type_info_get_tag (type_info);
    switch (type_tag) {
        case GI_TYPE_TAG_BOOLEAN:
        case GI_TYPE_TAG_INT8:
        case GI_TYPE_TAG_UINT8:
        case GI_TYPE_TAG_INT16:
        case GI_TYPE_TAG_UINT16:
        case GI_TYPE_TAG_INT32:
        case GI_TYPE_TAG_UINT32:
        case GI_TYPE_TAG_INT64:
        case GI_TYPE_TAG_UINT64:
        case GI_TYPE_TAG_FLOAT:
        case GI_TYPE_TAG_DOUBLE:
        case GI_TYPE_TAG_GTYPE:
        case GI_TYPE_TAG_UNICHAR:
            size = _pygi_g_type_tag_size (type_tag);
            g_assert (size > 0);
            break;
        case GI_TYPE_TAG_INTERFACE:
        {
            GIBaseInfo *info;
            GIInfoType info_type;

            info = g_type_info_get_interface (type_info);
            info_type = g_base_info_get_type (info);

            switch (info_type) {
                case GI_INFO_TYPE_STRUCT:
                    if (g_type_info_is_pointer (type_info)) {
                        size = sizeof (gpointer);
                    } else {
                        size = g_struct_info_get_size ( (GIStructInfo *) info);
                    }
                    break;
                case GI_INFO_TYPE_UNION:
                    if (g_type_info_is_pointer (type_info)) {
                        size = sizeof (gpointer);
                    } else {
                        size = g_union_info_get_size ( (GIUnionInfo *) info);
                    }
                    break;
                case GI_INFO_TYPE_ENUM:
                case GI_INFO_TYPE_FLAGS:
                    if (g_type_info_is_pointer (type_info)) {
                        size = sizeof (gpointer);
                    } else {
                        GITypeTag type_tag;

                        type_tag = g_enum_info_get_storage_type ( (GIEnumInfo *) info);
                        size = _pygi_g_type_tag_size (type_tag);
                    }
                    break;
                case GI_INFO_TYPE_BOXED:
                case GI_INFO_TYPE_OBJECT:
                case GI_INFO_TYPE_INTERFACE:
                case GI_INFO_TYPE_CALLBACK:
                    size = sizeof (gpointer);
                    break;
                case GI_INFO_TYPE_VFUNC:
                case GI_INFO_TYPE_INVALID:
                case GI_INFO_TYPE_FUNCTION:
                case GI_INFO_TYPE_CONSTANT:
                case GI_INFO_TYPE_VALUE:
                case GI_INFO_TYPE_SIGNAL:
                case GI_INFO_TYPE_PROPERTY:
                case GI_INFO_TYPE_FIELD:
                case GI_INFO_TYPE_ARG:
                case GI_INFO_TYPE_TYPE:
                case GI_INFO_TYPE_UNRESOLVED:
                default:
                    g_assert_not_reached();
                    break;
            }

            g_base_info_unref (info);
            break;
        }
        case GI_TYPE_TAG_ARRAY:
        case GI_TYPE_TAG_VOID:
        case GI_TYPE_TAG_UTF8:
        case GI_TYPE_TAG_FILENAME:
        case GI_TYPE_TAG_GLIST:
        case GI_TYPE_TAG_GSLIST:
        case GI_TYPE_TAG_GHASH:
        case GI_TYPE_TAG_ERROR:
            size = sizeof (gpointer);
            break;
    }

    return size;
}

static PyObject *
_wrap_g_function_info_get_symbol (PyGIBaseInfo *self)
{
    return _get_info_string (self, g_function_info_get_symbol);
}

static PyObject *
_wrap_g_function_info_get_flags (PyGIBaseInfo *self)
{
    return PYGLIB_PyLong_FromLong (g_function_info_get_flags (self->info));
}

static PyObject *
_wrap_g_function_info_get_property (PyGIBaseInfo *self)
{
    return _get_child_info (self, g_function_info_get_property);
}

static PyObject *
_wrap_g_function_info_get_vfunc (PyGIBaseInfo *self)
{
    return _get_child_info (self, g_function_info_get_vfunc);
}

static PyMethodDef _PyGIFunctionInfo_methods[] = {
    { "is_constructor", (PyCFunction) _wrap_g_function_info_is_constructor, METH_NOARGS },
    { "is_method", (PyCFunction) _wrap_g_function_info_is_method, METH_NOARGS },
    { "get_symbol", (PyCFunction) _wrap_g_function_info_get_symbol, METH_NOARGS },
    { "get_flags", (PyCFunction) _wrap_g_function_info_get_flags, METH_NOARGS },
    { "get_property", (PyCFunction) _wrap_g_function_info_get_property, METH_NOARGS },
    { "get_vfunc", (PyCFunction) _wrap_g_function_info_get_vfunc, METH_NOARGS },
    { NULL, NULL, 0 }
};

/* RegisteredTypeInfo */
PYGLIB_DEFINE_TYPE ("gi.RegisteredTypeInfo", PyGIRegisteredTypeInfo_Type, PyGIBaseInfo);

static PyObject *
_wrap_g_registered_type_info_get_type_name (PyGIBaseInfo *self)
{
    return _get_info_string (self, g_registered_type_info_get_type_name);
}

static PyObject *
_wrap_g_registered_type_info_get_type_init (PyGIBaseInfo *self)
{
    return _get_info_string (self, g_registered_type_info_get_type_init);
}

static PyObject *
_wrap_g_registered_type_info_get_g_type (PyGIBaseInfo *self)
{
    GType type;

    type = g_registered_type_info_get_g_type ( (GIRegisteredTypeInfo *) self->info);

    return pyg_type_wrapper_new (type);
}

static PyMethodDef _PyGIRegisteredTypeInfo_methods[] = {
    { "get_type_name", (PyCFunction) _wrap_g_registered_type_info_get_type_name, METH_NOARGS },
    { "get_type_init", (PyCFunction) _wrap_g_registered_type_info_get_type_init, METH_NOARGS },
    { "get_g_type", (PyCFunction) _wrap_g_registered_type_info_get_g_type, METH_NOARGS },
    { NULL, NULL, 0 }
};


/* GIStructInfo */
PYGLIB_DEFINE_TYPE ("StructInfo", PyGIStructInfo_Type, PyGIBaseInfo);

static PyObject *
_wrap_g_struct_info_get_fields (PyGIBaseInfo *self)
{
    return _make_infos_tuple (self, g_struct_info_get_n_fields, g_struct_info_get_field);
}

static PyObject *
_wrap_g_struct_info_get_methods (PyGIBaseInfo *self)
{
    return _make_infos_tuple (self, g_struct_info_get_n_methods, g_struct_info_get_method);
}

static PyObject *
_wrap_g_struct_info_get_size (PyGIBaseInfo *self)
{
    return PYGLIB_PyLong_FromSize_t (g_struct_info_get_size (self->info));
}

static PyObject *
_wrap_g_struct_info_get_alignment (PyGIBaseInfo *self)
{
    return PYGLIB_PyLong_FromSize_t (g_struct_info_get_alignment (self->info));
}

static PyObject *
_wrap_g_struct_info_is_gtype_struct (PyGIBaseInfo *self)
{
    return PyBool_FromLong (g_struct_info_is_gtype_struct (self->info));
}

static PyObject *
_wrap_g_struct_info_is_foreign (PyGIBaseInfo *self)
{
    return PyBool_FromLong (g_struct_info_is_foreign (self->info));
}

static PyMethodDef _PyGIStructInfo_methods[] = {
    { "get_fields", (PyCFunction) _wrap_g_struct_info_get_fields, METH_NOARGS },
    { "get_methods", (PyCFunction) _wrap_g_struct_info_get_methods, METH_NOARGS },
    { "get_size", (PyCFunction) _wrap_g_struct_info_get_size, METH_NOARGS },
    { "get_alignment", (PyCFunction) _wrap_g_struct_info_get_alignment, METH_NOARGS },
    { "is_gtype_struct", (PyCFunction) _wrap_g_struct_info_is_gtype_struct, METH_NOARGS },
    { "is_foreign", (PyCFunction) _wrap_g_struct_info_is_foreign, METH_NOARGS },
    { NULL, NULL, 0 }
};

gboolean
pygi_g_struct_info_is_simple (GIStructInfo *struct_info)
{
    gboolean is_simple;
    gsize n_field_infos;
    gsize i;

    is_simple = TRUE;

    n_field_infos = g_struct_info_get_n_fields (struct_info);

    for (i = 0; i < n_field_infos && is_simple; i++) {
        GIFieldInfo *field_info;
        GITypeInfo *field_type_info;
        GITypeTag field_type_tag;

        field_info = g_struct_info_get_field (struct_info, i);
        field_type_info = g_field_info_get_type (field_info);


        field_type_tag = g_type_info_get_tag (field_type_info);

        switch (field_type_tag) {
            case GI_TYPE_TAG_BOOLEAN:
            case GI_TYPE_TAG_INT8:
            case GI_TYPE_TAG_UINT8:
            case GI_TYPE_TAG_INT16:
            case GI_TYPE_TAG_UINT16:
            case GI_TYPE_TAG_INT32:
            case GI_TYPE_TAG_UINT32:
            case GI_TYPE_TAG_INT64:
            case GI_TYPE_TAG_UINT64:
            case GI_TYPE_TAG_FLOAT:
            case GI_TYPE_TAG_DOUBLE:
            case GI_TYPE_TAG_UNICHAR:
                if (g_type_info_is_pointer (field_type_info)) {
                    is_simple = FALSE;
                }
                break;
            case GI_TYPE_TAG_VOID:
            case GI_TYPE_TAG_GTYPE:
            case GI_TYPE_TAG_ERROR:
            case GI_TYPE_TAG_UTF8:
            case GI_TYPE_TAG_FILENAME:
            case GI_TYPE_TAG_ARRAY:
            case GI_TYPE_TAG_GLIST:
            case GI_TYPE_TAG_GSLIST:
            case GI_TYPE_TAG_GHASH:
                is_simple = FALSE;
                break;
            case GI_TYPE_TAG_INTERFACE:
            {
                GIBaseInfo *info;
                GIInfoType info_type;

                info = g_type_info_get_interface (field_type_info);
                info_type = g_base_info_get_type (info);

                switch (info_type) {
                    case GI_INFO_TYPE_STRUCT:
                        if (g_type_info_is_pointer (field_type_info)) {
                            is_simple = FALSE;
                        } else {
                            is_simple = pygi_g_struct_info_is_simple ( (GIStructInfo *) info);
                        }
                        break;
                    case GI_INFO_TYPE_UNION:
                        /* TODO */
                        is_simple = FALSE;
                        break;
                    case GI_INFO_TYPE_ENUM:
                    case GI_INFO_TYPE_FLAGS:
                        if (g_type_info_is_pointer (field_type_info)) {
                            is_simple = FALSE;
                        }
                        break;
                    case GI_INFO_TYPE_BOXED:
                    case GI_INFO_TYPE_OBJECT:
                    case GI_INFO_TYPE_CALLBACK:
                    case GI_INFO_TYPE_INTERFACE:
                        is_simple = FALSE;
                        break;
                    case GI_INFO_TYPE_VFUNC:
                    case GI_INFO_TYPE_INVALID:
                    case GI_INFO_TYPE_FUNCTION:
                    case GI_INFO_TYPE_CONSTANT:
                    case GI_INFO_TYPE_VALUE:
                    case GI_INFO_TYPE_SIGNAL:
                    case GI_INFO_TYPE_PROPERTY:
                    case GI_INFO_TYPE_FIELD:
                    case GI_INFO_TYPE_ARG:
                    case GI_INFO_TYPE_TYPE:
                    case GI_INFO_TYPE_UNRESOLVED:
                    default:
                        g_assert_not_reached();
                        break;
                }

                g_base_info_unref (info);
                break;
            }
        }

        g_base_info_unref ( (GIBaseInfo *) field_type_info);
        g_base_info_unref ( (GIBaseInfo *) field_info);
    }

    return is_simple;
}


/* EnumInfo */
PYGLIB_DEFINE_TYPE ("gi.EnumInfo", PyGIEnumInfo_Type, PyGIBaseInfo);

static PyObject *
_wrap_g_enum_info_get_values (PyGIBaseInfo *self)
{
    return _make_infos_tuple (self, g_enum_info_get_n_values, g_enum_info_get_value);
}

static PyObject *
_wrap_g_enum_info_is_flags (PyGIBaseInfo *self)
{
    GIInfoType info_type = g_base_info_get_type ((GIBaseInfo *) self->info);

    if (info_type == GI_INFO_TYPE_ENUM) {
        Py_RETURN_FALSE;
    } else if (info_type == GI_INFO_TYPE_FLAGS) {
        Py_RETURN_TRUE;
    } else {
        g_assert_not_reached();
    }
}

static PyObject *
_wrap_g_enum_info_get_methods (PyGIBaseInfo *self)
{
    return _make_infos_tuple (self, g_enum_info_get_n_methods, g_enum_info_get_method);
}

static PyObject *
_wrap_g_enum_info_get_storage_type (PyGIBaseInfo *self)
{
    return PYGLIB_PyLong_FromLong (g_enum_info_get_storage_type ((GIBaseInfo *) self->info));
}

static PyMethodDef _PyGIEnumInfo_methods[] = {
    { "get_values", (PyCFunction) _wrap_g_enum_info_get_values, METH_NOARGS },
    { "is_flags", (PyCFunction) _wrap_g_enum_info_is_flags, METH_NOARGS },
    { "get_methods", (PyCFunction) _wrap_g_enum_info_get_methods, METH_NOARGS },
    { "get_storage_type", (PyCFunction) _wrap_g_enum_info_get_storage_type, METH_NOARGS },
    { NULL, NULL, 0 }
};


/* ObjectInfo */
PYGLIB_DEFINE_TYPE ("ObjectInfo", PyGIObjectInfo_Type, PyGIBaseInfo);

static PyObject *
_wrap_g_object_info_get_parent (PyGIBaseInfo *self)
{
    return _get_child_info (self, g_object_info_get_parent);
}

static PyObject *
_wrap_g_object_info_get_methods (PyGIBaseInfo *self)
{
    return _make_infos_tuple (self, g_object_info_get_n_methods, g_object_info_get_method);
}

static PyObject *
_wrap_g_object_info_find_method (PyGIBaseInfo *self, PyObject *py_name)
{
    return _get_child_info_by_name (self, py_name, g_object_info_find_method);
}

static PyObject *
_wrap_g_object_info_get_fields (PyGIBaseInfo *self)
{
    return _make_infos_tuple (self, g_object_info_get_n_fields, g_object_info_get_field);
}

static PyObject *
_wrap_g_object_info_get_properties (PyGIBaseInfo *self)
{
    return _make_infos_tuple (self, g_object_info_get_n_properties, g_object_info_get_property);
}

static PyObject *
_wrap_g_object_info_get_signals (PyGIBaseInfo *self)
{
    return _make_infos_tuple (self, g_object_info_get_n_signals, g_object_info_get_signal);
}

static PyObject *
_wrap_g_object_info_get_interfaces (PyGIBaseInfo *self)
{
    return _make_infos_tuple (self, g_object_info_get_n_interfaces, g_object_info_get_interface);
}

static PyObject *
_wrap_g_object_info_get_constants (PyGIBaseInfo *self)
{
    return _make_infos_tuple (self, g_object_info_get_n_constants, g_object_info_get_constant);
}

static PyObject *
_wrap_g_object_info_get_vfuncs (PyGIBaseInfo *self)
{
    return _make_infos_tuple (self, g_object_info_get_n_vfuncs, g_object_info_get_vfunc);
}

static PyObject *
_wrap_g_object_info_get_abstract (PyGIBaseInfo *self)
{
    gboolean is_abstract  = g_object_info_get_abstract ( (GIObjectInfo*) self->info);
    return PyBool_FromLong (is_abstract);
}

static PyObject *
_wrap_g_object_info_get_type_name (PyGIBaseInfo *self)
{
    return _get_info_string (self, g_object_info_get_type_name);
}

static PyObject *
_wrap_g_object_info_get_type_init (PyGIBaseInfo *self)
{
    return _get_info_string (self, g_object_info_get_type_init);
}

static PyObject *
_wrap_g_object_info_get_fundamental (PyGIBaseInfo *self)
{
    return PyBool_FromLong (g_object_info_get_fundamental ( (GIObjectInfo*) self->info));
}

static PyObject *
_wrap_g_object_info_get_class_struct (PyGIBaseInfo *self)
{
    return _get_child_info (self, g_object_info_get_class_struct);
}

static PyObject *
_wrap_g_object_info_find_vfunc (PyGIBaseInfo *self, PyObject *py_name)
{
    return _get_child_info_by_name (self, py_name, g_object_info_find_vfunc);
}

static PyObject *
_wrap_g_object_info_get_unref_function (PyGIBaseInfo *self)
{
    return _get_info_string (self, g_object_info_get_unref_function);
}

static PyObject *
_wrap_g_object_info_get_ref_function (PyGIBaseInfo *self)
{
    return _get_info_string (self, g_object_info_get_ref_function);
}

static PyObject *
_wrap_g_object_info_get_set_value_function (PyGIBaseInfo *self)
{
    return _get_info_string (self, g_object_info_get_set_value_function);
}

static PyObject *
_wrap_g_object_info_get_get_value_function (PyGIBaseInfo *self)
{
    return _get_info_string (self, g_object_info_get_get_value_function);
}

static PyMethodDef _PyGIObjectInfo_methods[] = {
    { "get_parent", (PyCFunction) _wrap_g_object_info_get_parent, METH_NOARGS },
    { "get_methods", (PyCFunction) _wrap_g_object_info_get_methods, METH_NOARGS },
    { "find_method", (PyCFunction) _wrap_g_object_info_find_method, METH_O },
    { "get_fields", (PyCFunction) _wrap_g_object_info_get_fields, METH_NOARGS },
    { "get_properties", (PyCFunction) _wrap_g_object_info_get_properties, METH_NOARGS },
    { "get_signals", (PyCFunction) _wrap_g_object_info_get_signals, METH_NOARGS },
    { "get_interfaces", (PyCFunction) _wrap_g_object_info_get_interfaces, METH_NOARGS },
    { "get_constants", (PyCFunction) _wrap_g_object_info_get_constants, METH_NOARGS },
    { "get_vfuncs", (PyCFunction) _wrap_g_object_info_get_vfuncs, METH_NOARGS },
    { "find_vfunc", (PyCFunction) _wrap_g_object_info_find_vfunc, METH_O },
    { "get_abstract", (PyCFunction) _wrap_g_object_info_get_abstract, METH_NOARGS },
    { "get_type_name", (PyCFunction) _wrap_g_object_info_get_type_name, METH_NOARGS },
    { "get_type_init", (PyCFunction) _wrap_g_object_info_get_type_init, METH_NOARGS },
    { "get_fundamental", (PyCFunction) _wrap_g_object_info_get_fundamental, METH_NOARGS },
    { "get_class_struct", (PyCFunction) _wrap_g_object_info_get_class_struct, METH_NOARGS },
    { "get_unref_function", (PyCFunction) _wrap_g_object_info_get_unref_function, METH_NOARGS },
    { "get_ref_function", (PyCFunction) _wrap_g_object_info_get_ref_function, METH_NOARGS },
    { "get_set_value_function", (PyCFunction) _wrap_g_object_info_get_set_value_function, METH_NOARGS },
    { "get_get_value_function", (PyCFunction) _wrap_g_object_info_get_get_value_function, METH_NOARGS },
    { NULL, NULL, 0 }
};


/* GIInterfaceInfo */
PYGLIB_DEFINE_TYPE ("InterfaceInfo", PyGIInterfaceInfo_Type, PyGIBaseInfo);

static PyObject *
_wrap_g_interface_info_get_methods (PyGIBaseInfo *self)
{
    return _make_infos_tuple (self, g_interface_info_get_n_methods, g_interface_info_get_method);
}

static PyObject *
_wrap_g_interface_info_find_method (PyGIBaseInfo *self, PyObject *py_name)
{
    return _get_child_info_by_name (self, py_name, g_interface_info_find_method);
}

static PyObject *
_wrap_g_interface_info_get_constants (PyGIBaseInfo *self)
{
    return _make_infos_tuple (self, g_interface_info_get_n_constants, g_interface_info_get_constant);
}

static PyObject *
_wrap_g_interface_info_get_vfuncs (PyGIBaseInfo *self)
{
    return _make_infos_tuple (self, g_interface_info_get_n_vfuncs, g_interface_info_get_vfunc);
}

static PyObject *
_wrap_g_interface_info_find_vfunc (PyGIBaseInfo *self, PyObject *py_name)
{
    return _get_child_info_by_name (self, py_name, g_interface_info_find_vfunc);
}

static PyObject *
_wrap_g_interface_info_get_prerequisites (PyGIBaseInfo *self)
{
    return _make_infos_tuple (self, g_interface_info_get_n_prerequisites, g_interface_info_get_prerequisite);
}

static PyObject *
_wrap_g_interface_info_get_properties (PyGIBaseInfo *self)
{
    return _make_infos_tuple (self, g_interface_info_get_n_properties, g_interface_info_get_property);
}

static PyObject *
_wrap_g_interface_info_get_iface_struct (PyGIBaseInfo *self)
{
    return _get_child_info (self, g_interface_info_get_iface_struct);
}

static PyObject *
_wrap_g_interface_info_get_signals (PyGIBaseInfo *self)
{
    return _make_infos_tuple (self, g_interface_info_get_n_signals, g_interface_info_get_signal);
}

static PyObject *
_wrap_g_interface_info_find_signal (PyGIBaseInfo *self, PyObject *py_name)
{
    return _get_child_info_by_name (self, py_name, g_interface_info_find_signal);
}

static PyMethodDef _PyGIInterfaceInfo_methods[] = {
    { "get_prerequisites", (PyCFunction) _wrap_g_interface_info_get_prerequisites, METH_NOARGS },
    { "get_properties", (PyCFunction) _wrap_g_interface_info_get_properties, METH_NOARGS },
    { "get_methods", (PyCFunction) _wrap_g_interface_info_get_methods, METH_NOARGS },
    { "find_method", (PyCFunction) _wrap_g_interface_info_find_method, METH_O },
    { "get_signals", (PyCFunction) _wrap_g_interface_info_get_signals, METH_NOARGS },
    { "find_signal", (PyCFunction) _wrap_g_interface_info_find_signal, METH_O },
    { "get_vfuncs", (PyCFunction) _wrap_g_interface_info_get_vfuncs, METH_NOARGS },
    { "get_constants", (PyCFunction) _wrap_g_interface_info_get_constants, METH_NOARGS },
    { "get_iface_struct", (PyCFunction) _wrap_g_interface_info_get_iface_struct, METH_NOARGS },
    { "find_vfunc", (PyCFunction) _wrap_g_interface_info_find_vfunc, METH_O },
    { NULL, NULL, 0 }
};

/* GIConstantInfo */
PYGLIB_DEFINE_TYPE ("gi.ConstantInfo", PyGIConstantInfo_Type, PyGIBaseInfo);

static PyObject *
_wrap_g_constant_info_get_value (PyGIBaseInfo *self)
{
    GITypeInfo *type_info;
    GIArgument value;
    PyObject *py_value;
    gboolean free_array = FALSE;

    if (g_constant_info_get_value ( (GIConstantInfo *) self->info, &value) < 0) {
        PyErr_SetString (PyExc_RuntimeError, "unable to get value");
        return NULL;
    }

    type_info = g_constant_info_get_type ( (GIConstantInfo *) self->info);

    if (g_type_info_get_tag (type_info) == GI_TYPE_TAG_ARRAY) {
        value.v_pointer = _pygi_argument_to_array (&value, NULL, NULL, NULL,
                                                   type_info, &free_array);
    }

    py_value = _pygi_argument_to_object (&value, type_info, GI_TRANSFER_NOTHING);
    
    if (free_array) {
        g_array_free (value.v_pointer, FALSE);
    }

    g_constant_info_free_value (self->info, &value);
    g_base_info_unref ( (GIBaseInfo *) type_info);

    return py_value;
}

static PyMethodDef _PyGIConstantInfo_methods[] = {
    { "get_value", (PyCFunction) _wrap_g_constant_info_get_value, METH_NOARGS },
    { NULL, NULL, 0 }
};

/* GIValueInfo */
PYGLIB_DEFINE_TYPE ("gi.ValueInfo", PyGIValueInfo_Type, PyGIBaseInfo);

static PyObject *
_wrap_g_value_info_get_value (PyGIBaseInfo *self)
{
    glong value;

    value = g_value_info_get_value ( (GIValueInfo *) self->info);

    return PYGLIB_PyLong_FromLong (value);
}


static PyMethodDef _PyGIValueInfo_methods[] = {
    { "get_value", (PyCFunction) _wrap_g_value_info_get_value, METH_NOARGS },
    { NULL, NULL, 0 }
};


/* GIFieldInfo */
PYGLIB_DEFINE_TYPE ("gi.FieldInfo", PyGIFieldInfo_Type, PyGIBaseInfo);

static gssize
_struct_field_array_length_marshal (gsize length_index,
                                    void *container_ptr,
                                    void *struct_data_ptr)
{
    gssize array_len = -1;
    GIFieldInfo *array_len_field = NULL;
    GIArgument arg = {0};
    GIBaseInfo *container_info = (GIBaseInfo *)container_ptr;

    switch (g_base_info_get_type (container_info)) {
        case GI_INFO_TYPE_UNION:
            array_len_field = g_union_info_get_field ((GIUnionInfo *)container_info, length_index);
            break;
        case GI_INFO_TYPE_STRUCT:
            array_len_field = g_struct_info_get_field ((GIStructInfo *)container_info, length_index);
            break;
        case GI_INFO_TYPE_OBJECT:
            array_len_field = g_object_info_get_field ((GIObjectInfo *)container_info, length_index);
            break;
        default:
            /* Other types don't have fields. */
            g_assert_not_reached();
    }

    if (array_len_field == NULL) {
        return -1;
    }

    if (g_field_info_get_field (array_len_field, struct_data_ptr, &arg)) {
        GITypeInfo *array_len_type_info;

        array_len_type_info = g_field_info_get_type (array_len_field);
        if (array_len_type_info == NULL) {
            goto out;
        }

        if (!pygi_argument_to_gssize (&arg,
                                      g_type_info_get_tag (array_len_type_info),
                                      &array_len)) {
            array_len = -1;
        }

        g_base_info_unref (array_len_type_info);
    }

out:
    g_base_info_unref (array_len_field);
    return array_len;
}

static PyObject *
_wrap_g_field_info_get_value (PyGIBaseInfo *self,
                              PyObject     *args)
{
    PyObject *instance;
    GIBaseInfo *container_info;
    GIInfoType container_info_type;
    gpointer pointer;
    GITypeInfo *field_type_info;
    GIArgument value;
    PyObject *py_value = NULL;
    gboolean free_array = FALSE;

    memset(&value, 0, sizeof(GIArgument));

    if (!PyArg_ParseTuple (args, "O:FieldInfo.get_value", &instance)) {
        return NULL;
    }

    container_info = g_base_info_get_container (self->info);
    g_assert (container_info != NULL);

    /* Check the instance. */
    if (!_pygi_g_registered_type_info_check_object ( (GIRegisteredTypeInfo *) container_info, TRUE, instance)) {
        _PyGI_ERROR_PREFIX ("argument 1: ");
        return NULL;
    }

    /* Get the pointer to the container. */
    container_info_type = g_base_info_get_type (container_info);
    switch (container_info_type) {
        case GI_INFO_TYPE_UNION:
        case GI_INFO_TYPE_STRUCT:
            pointer = pyg_boxed_get (instance, void);
            break;
        case GI_INFO_TYPE_OBJECT:
            pointer = pygobject_get (instance);
            break;
        default:
            /* Other types don't have fields. */
            g_assert_not_reached();
    }

    /* Get the field's value. */
    field_type_info = g_field_info_get_type ( (GIFieldInfo *) self->info);

    /* A few types are not handled by g_field_info_get_field, so do it here. */
    if (!g_type_info_is_pointer (field_type_info)
            && g_type_info_get_tag (field_type_info) == GI_TYPE_TAG_INTERFACE) {
        GIBaseInfo *info;
        GIInfoType info_type;

        if (! (g_field_info_get_flags ( (GIFieldInfo *) self->info) & GI_FIELD_IS_READABLE)) {
            PyErr_SetString (PyExc_RuntimeError, "field is not readable");
            goto out;
        }

        info = g_type_info_get_interface (field_type_info);

        info_type = g_base_info_get_type (info);

        g_base_info_unref (info);

        switch (info_type) {
            case GI_INFO_TYPE_UNION:
                PyErr_SetString (PyExc_NotImplementedError, "getting an union is not supported yet");
                goto out;
            case GI_INFO_TYPE_STRUCT:
            {
                gsize offset;

                offset = g_field_info_get_offset ( (GIFieldInfo *) self->info);

                value.v_pointer = (char*) pointer + offset;

                goto argument_to_object;
            }
            default:
                /* Fallback. */
                break;
        }
    }

    if (!g_field_info_get_field ( (GIFieldInfo *) self->info, pointer, &value)) {
        PyErr_SetString (PyExc_RuntimeError, "unable to get the value");
        goto out;
    }

    if (g_type_info_get_tag (field_type_info) == GI_TYPE_TAG_ARRAY) {
        value.v_pointer = _pygi_argument_to_array (&value,
                                                   _struct_field_array_length_marshal,
                                                   container_info,
                                                   pointer,
                                                   field_type_info,
                                                   &free_array);
    }

argument_to_object:
    py_value = _pygi_argument_to_object (&value, field_type_info, GI_TRANSFER_NOTHING);

    if (free_array) {
        g_array_free (value.v_pointer, FALSE);
    }

out:
    g_base_info_unref ( (GIBaseInfo *) field_type_info);

    return py_value;
}

static PyObject *
_wrap_g_field_info_set_value (PyGIBaseInfo *self,
                              PyObject     *args)
{
    PyObject *instance;
    PyObject *py_value;
    GIBaseInfo *container_info;
    GIInfoType container_info_type;
    gpointer pointer;
    GITypeInfo *field_type_info;
    GIArgument value;
    PyObject *retval = NULL;

    if (!PyArg_ParseTuple (args, "OO:FieldInfo.set_value", &instance, &py_value)) {
        return NULL;
    }

    container_info = g_base_info_get_container (self->info);
    g_assert (container_info != NULL);

    /* Check the instance. */
    if (!_pygi_g_registered_type_info_check_object ( (GIRegisteredTypeInfo *) container_info, TRUE, instance)) {
        _PyGI_ERROR_PREFIX ("argument 1: ");
        return NULL;
    }

    /* Get the pointer to the container. */
    container_info_type = g_base_info_get_type (container_info);
    switch (container_info_type) {
        case GI_INFO_TYPE_UNION:
        case GI_INFO_TYPE_STRUCT:
            pointer = pyg_boxed_get (instance, void);
            break;
        case GI_INFO_TYPE_OBJECT:
            pointer = pygobject_get (instance);
            break;
        default:
            /* Other types don't have fields. */
            g_assert_not_reached();
    }

    field_type_info = g_field_info_get_type ( (GIFieldInfo *) self->info);

    /* Check the value. */
    {
        gboolean retval;

        retval = _pygi_g_type_info_check_object (field_type_info, py_value, TRUE);
        if (retval < 0) {
            goto out;
        }

        if (!retval) {
            _PyGI_ERROR_PREFIX ("argument 2: ");
            goto out;
        }
    }

    /* Set the field's value. */
    /* A few types are not handled by g_field_info_set_field, so do it here. */
    if (!g_type_info_is_pointer (field_type_info)
            && g_type_info_get_tag (field_type_info) == GI_TYPE_TAG_INTERFACE) {
        GIBaseInfo *info;
        GIInfoType info_type;

        if (! (g_field_info_get_flags ( (GIFieldInfo *) self->info) & GI_FIELD_IS_WRITABLE)) {
            PyErr_SetString (PyExc_RuntimeError, "field is not writable");
            goto out;
        }

        info = g_type_info_get_interface (field_type_info);

        info_type = g_base_info_get_type (info);

        switch (info_type) {
            case GI_INFO_TYPE_UNION:
                PyErr_SetString (PyExc_NotImplementedError, "setting an union is not supported yet");
                goto out;
            case GI_INFO_TYPE_STRUCT:
            {
                gboolean is_simple;
                gsize offset;
                gssize size;

                is_simple = pygi_g_struct_info_is_simple ( (GIStructInfo *) info);

                if (!is_simple) {
                    PyErr_SetString (PyExc_TypeError,
                                     "cannot set a structure which has no well-defined ownership transfer rules");
                    g_base_info_unref (info);
                    goto out;
                }

                value = _pygi_argument_from_object (py_value, field_type_info, GI_TRANSFER_NOTHING);
                if (PyErr_Occurred()) {
                    g_base_info_unref (info);
                    goto out;
                }

                offset = g_field_info_get_offset ( (GIFieldInfo *) self->info);
                size = g_struct_info_get_size ( (GIStructInfo *) info);
                g_assert (size > 0);

                g_memmove ((char*) pointer + offset, value.v_pointer, size);

                g_base_info_unref (info);

                retval = Py_None;
                goto out;
            }
            default:
                /* Fallback. */
                break;
        }

        g_base_info_unref (info);
    } else if (g_type_info_is_pointer (field_type_info)
            && (g_type_info_get_tag (field_type_info) == GI_TYPE_TAG_VOID
                || g_type_info_get_tag (field_type_info) == GI_TYPE_TAG_UTF8)) {
        int offset;
        value = _pygi_argument_from_object (py_value, field_type_info, GI_TRANSFER_NOTHING);
        if (PyErr_Occurred()) {
            goto out;
        }

        offset = g_field_info_get_offset ((GIFieldInfo *) self->info);
        G_STRUCT_MEMBER (gpointer, pointer, offset) = (gpointer)value.v_pointer;

        retval = Py_None;
        goto out;
    }

    value = _pygi_argument_from_object (py_value, field_type_info, GI_TRANSFER_EVERYTHING);
    if (PyErr_Occurred()) {
        goto out;
    }

    if (!g_field_info_set_field ( (GIFieldInfo *) self->info, pointer, &value)) {
        _pygi_argument_release (&value, field_type_info, GI_TRANSFER_NOTHING, GI_DIRECTION_IN);
        PyErr_SetString (PyExc_RuntimeError, "unable to set value for field");
        goto out;
    }

    retval = Py_None;

out:
    g_base_info_unref ( (GIBaseInfo *) field_type_info);

    Py_XINCREF (retval);
    return retval;
}

static PyObject *
_wrap_g_field_info_get_flags (PyGIBaseInfo *self)
{
    return PYGLIB_PyLong_FromLong (g_field_info_get_flags (self->info));
}

static PyObject *
_wrap_g_field_info_get_size (PyGIBaseInfo *self)
{
    return PYGLIB_PyLong_FromLong (g_field_info_get_size (self->info));
}

static PyObject *
_wrap_g_field_info_get_offset (PyGIBaseInfo *self)
{
    return PYGLIB_PyLong_FromLong (g_field_info_get_offset (self->info));
}

static PyObject *
_wrap_g_field_info_get_type (PyGIBaseInfo *self)
{
    return _get_child_info (self, g_field_info_get_type);
}

static PyMethodDef _PyGIFieldInfo_methods[] = {
    { "get_value", (PyCFunction) _wrap_g_field_info_get_value, METH_VARARGS },
    { "set_value", (PyCFunction) _wrap_g_field_info_set_value, METH_VARARGS },
    { "get_flags", (PyCFunction) _wrap_g_field_info_get_flags, METH_VARARGS },
    { "get_size", (PyCFunction) _wrap_g_field_info_get_size, METH_VARARGS },
    { "get_offset", (PyCFunction) _wrap_g_field_info_get_offset, METH_VARARGS },
    { "get_type", (PyCFunction) _wrap_g_field_info_get_type, METH_VARARGS },
    { NULL, NULL, 0 }
};


/* GIUnresolvedInfo */
PYGLIB_DEFINE_TYPE ("gi.UnresolvedInfo", PyGIUnresolvedInfo_Type, PyGIBaseInfo);

static PyMethodDef _PyGIUnresolvedInfo_methods[] = {
    { NULL, NULL, 0 }
};

/* GIVFuncInfo */
PYGLIB_DEFINE_TYPE ("gi.VFuncInfo", PyGIVFuncInfo_Type, PyGICallableInfo);

static PyObject *
_wrap_g_vfunc_info_get_flags (PyGIBaseInfo *self)
{
    return PYGLIB_PyLong_FromLong (g_vfunc_info_get_flags ((GIVFuncInfo *) self->info));
}

static PyObject *
_wrap_g_vfunc_info_get_offset (PyGIBaseInfo *self)
{
    return PYGLIB_PyLong_FromLong (g_vfunc_info_get_offset ((GIVFuncInfo *) self->info));
}

static PyObject *
_wrap_g_vfunc_info_get_signal (PyGIBaseInfo *self)
{
    return _get_child_info (self, g_vfunc_info_get_signal);
}

static PyObject *
_wrap_g_vfunc_info_get_invoker (PyGIBaseInfo *self)
{
    return _get_child_info (self, g_vfunc_info_get_invoker);
}

static PyMethodDef _PyGIVFuncInfo_methods[] = {
    { "get_flags", (PyCFunction) _wrap_g_vfunc_info_get_flags, METH_NOARGS },
    { "get_offset", (PyCFunction) _wrap_g_vfunc_info_get_offset, METH_NOARGS },
    { "get_signal", (PyCFunction) _wrap_g_vfunc_info_get_signal, METH_NOARGS },
    { "get_invoker", (PyCFunction) _wrap_g_vfunc_info_get_invoker, METH_NOARGS },
    { NULL, NULL, 0 }
};


/* GIUnionInfo */
PYGLIB_DEFINE_TYPE ("gi.UnionInfo", PyGIUnionInfo_Type, PyGIBaseInfo);

static PyObject *
_wrap_g_union_info_get_fields (PyGIBaseInfo *self)
{
    return _make_infos_tuple (self, g_union_info_get_n_fields, g_union_info_get_field);
}

static PyObject *
_wrap_g_union_info_get_methods (PyGIBaseInfo *self)
{
    return _make_infos_tuple (self, g_union_info_get_n_methods, g_union_info_get_method);
}

static PyMethodDef _PyGIUnionInfo_methods[] = {
    { "get_fields", (PyCFunction) _wrap_g_union_info_get_fields, METH_NOARGS },
    { "get_methods", (PyCFunction) _wrap_g_union_info_get_methods, METH_NOARGS },
    { NULL, NULL, 0 }
};

/* Private */

gchar *
_pygi_g_base_info_get_fullname (GIBaseInfo *info)
{
    GIBaseInfo *container_info;
    gchar *fullname;

    container_info = g_base_info_get_container (info);
    if (container_info != NULL) {
        fullname = g_strdup_printf ("%s.%s.%s",
                                    g_base_info_get_namespace (container_info),
                                    _safe_base_info_get_name (container_info),
                                    _safe_base_info_get_name (info));
    } else {
        fullname = g_strdup_printf ("%s.%s",
                                    g_base_info_get_namespace (info),
                                    _safe_base_info_get_name (info));
    }

    if (fullname == NULL) {
        PyErr_NoMemory();
    }

    return fullname;
}


void
_pygi_info_register_types (PyObject *m)
{
#define _PyGI_REGISTER_TYPE(m, type, cname, base) \
    Py_TYPE(&type) = &PyType_Type; \
    type.tp_flags = (Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE); \
    type.tp_weaklistoffset = offsetof(PyGIBaseInfo, inst_weakreflist); \
    type.tp_methods = _PyGI##cname##_methods; \
    type.tp_base = &base; \
    if (PyType_Ready(&type)) \
        return; \
    if (PyModule_AddObject(m, #cname, (PyObject *)&type)) \
        return

    Py_TYPE(&PyGIBaseInfo_Type) = &PyType_Type;

    PyGIBaseInfo_Type.tp_dealloc = (destructor) _base_info_dealloc;
    PyGIBaseInfo_Type.tp_repr = (reprfunc) _base_info_repr;
    PyGIBaseInfo_Type.tp_flags = (Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE);
    PyGIBaseInfo_Type.tp_weaklistoffset = offsetof(PyGIBaseInfo, inst_weakreflist);
    PyGIBaseInfo_Type.tp_methods = _PyGIBaseInfo_methods;
    PyGIBaseInfo_Type.tp_richcompare = (richcmpfunc)_base_info_richcompare;
    PyGIBaseInfo_Type.tp_getset = _base_info_getsets;
    PyGIBaseInfo_Type.tp_getattro = (getattrofunc) _base_info_getattro;

    if (PyType_Ready(&PyGIBaseInfo_Type))
        return;
    if (PyModule_AddObject(m, "BaseInfo", (PyObject *)&PyGIBaseInfo_Type))
        return;

    _PyGI_REGISTER_TYPE (m, PyGICallableInfo_Type, CallableInfo,
                         PyGIBaseInfo_Type);
    PyGICallableInfo_Type.tp_call = (ternaryfunc) _callable_info_call;
    PyGICallableInfo_Type.tp_dealloc = (destructor) _callable_info_dealloc;

    _PyGI_REGISTER_TYPE (m, PyGIFunctionInfo_Type, FunctionInfo,
                         PyGICallableInfo_Type);
    PyGIFunctionInfo_Type.tp_call = (ternaryfunc) _function_info_call;
    PyGIFunctionInfo_Type.tp_descr_get = (descrgetfunc) _function_info_descr_get;

    _PyGI_REGISTER_TYPE (m, PyGIVFuncInfo_Type, VFuncInfo,
                         PyGICallableInfo_Type);
    PyGIVFuncInfo_Type.tp_descr_get = (descrgetfunc) _vfunc_info_descr_get;

    _PyGI_REGISTER_TYPE (m, PyGISignalInfo_Type, SignalInfo,
                         PyGICallableInfo_Type);

    _PyGI_REGISTER_TYPE (m, PyGIUnresolvedInfo_Type, UnresolvedInfo,
                         PyGIBaseInfo_Type);
    _PyGI_REGISTER_TYPE (m, PyGICallbackInfo_Type, CallbackInfo,
                         PyGICallableInfo_Type);
    _PyGI_REGISTER_TYPE (m, PyGIRegisteredTypeInfo_Type, RegisteredTypeInfo,
                         PyGIBaseInfo_Type);
    _PyGI_REGISTER_TYPE (m, PyGIStructInfo_Type, StructInfo,
                         PyGIRegisteredTypeInfo_Type);
    _PyGI_REGISTER_TYPE (m, PyGIEnumInfo_Type, EnumInfo,
                         PyGIRegisteredTypeInfo_Type);
    _PyGI_REGISTER_TYPE (m, PyGIObjectInfo_Type, ObjectInfo,
                         PyGIRegisteredTypeInfo_Type);
    _PyGI_REGISTER_TYPE (m, PyGIInterfaceInfo_Type, InterfaceInfo,
                         PyGIRegisteredTypeInfo_Type);
    _PyGI_REGISTER_TYPE (m, PyGIConstantInfo_Type, ConstantInfo,
                         PyGIBaseInfo_Type);
    _PyGI_REGISTER_TYPE (m, PyGIValueInfo_Type, ValueInfo,
                         PyGIBaseInfo_Type);
    _PyGI_REGISTER_TYPE (m, PyGIFieldInfo_Type, FieldInfo,
                         PyGIBaseInfo_Type);
    _PyGI_REGISTER_TYPE (m, PyGIUnionInfo_Type, UnionInfo,
                         PyGIRegisteredTypeInfo_Type);
    _PyGI_REGISTER_TYPE (m, PyGIErrorDomainInfo_Type, ErrorDomainInfo,
                         PyGIBaseInfo_Type);
    _PyGI_REGISTER_TYPE (m, PyGIPropertyInfo_Type, PropertyInfo,
                         PyGIBaseInfo_Type);
    _PyGI_REGISTER_TYPE (m, PyGIArgInfo_Type, ArgInfo,
                         PyGIBaseInfo_Type);
    _PyGI_REGISTER_TYPE (m, PyGITypeInfo_Type, TypeInfo,
                         PyGIBaseInfo_Type);

#undef _PyGI_REGISTER_TYPE

#define _PyGI_ENUM_BEGIN(name) \
        { \
            const char *__enum_name = #name; \
            PyObject *__enum_value = NULL; \
            PyObject *__new_enum_cls = NULL; \
            PyObject *__enum_instance_dict = PyDict_New(); \
            PyObject *__module_name = PyObject_GetAttrString (m, "__name__"); \
            PyDict_SetItemString (__enum_instance_dict, "__module__", __module_name); \
            Py_DECREF (__module_name);

#define _PyGI_ENUM_ADD_VALUE(prefix, name) \
            __enum_value = PYGLIB_PyLong_FromLong (prefix##_##name); \
            if (PyDict_SetItemString(__enum_instance_dict, #name, __enum_value)) { \
                Py_DECREF (__enum_instance_dict); \
                Py_DECREF (__enum_value); \
                return; \
            } \
            Py_DECREF (__enum_value);

#define _PyGI_ENUM_END \
            __new_enum_cls = PyObject_CallFunction ((PyObject *)&PyType_Type, "s(O)O", \
                                                    __enum_name, (PyObject *)&PyType_Type, \
                                                    __enum_instance_dict); \
            Py_DECREF (__enum_instance_dict); \
            PyModule_AddObject (m, __enum_name, __new_enum_cls); /* steals ref */ \
        }


    /* GIDirection */
    _PyGI_ENUM_BEGIN (Direction)
        _PyGI_ENUM_ADD_VALUE (GI_DIRECTION, IN)
        _PyGI_ENUM_ADD_VALUE (GI_DIRECTION, OUT)
        _PyGI_ENUM_ADD_VALUE (GI_DIRECTION, INOUT)
    _PyGI_ENUM_END


    /* GITransfer */
    _PyGI_ENUM_BEGIN (Transfer)
        _PyGI_ENUM_ADD_VALUE (GI_TRANSFER, NOTHING)
        _PyGI_ENUM_ADD_VALUE (GI_TRANSFER, CONTAINER)
        _PyGI_ENUM_ADD_VALUE (GI_TRANSFER, EVERYTHING)
    _PyGI_ENUM_END

    /* GIArrayType */
    _PyGI_ENUM_BEGIN (ArrayType)
        _PyGI_ENUM_ADD_VALUE (GI_ARRAY_TYPE, C)
        _PyGI_ENUM_ADD_VALUE (GI_ARRAY_TYPE, ARRAY)
        _PyGI_ENUM_ADD_VALUE (GI_ARRAY_TYPE, PTR_ARRAY)
        _PyGI_ENUM_ADD_VALUE (GI_ARRAY_TYPE, BYTE_ARRAY)
    _PyGI_ENUM_END

    /* GIScopeType */
    _PyGI_ENUM_BEGIN (ScopeType)
        _PyGI_ENUM_ADD_VALUE (GI_SCOPE_TYPE, INVALID)
        _PyGI_ENUM_ADD_VALUE (GI_SCOPE_TYPE, CALL)
        _PyGI_ENUM_ADD_VALUE (GI_SCOPE_TYPE, ASYNC)
        _PyGI_ENUM_ADD_VALUE (GI_SCOPE_TYPE, NOTIFIED)
    _PyGI_ENUM_END

    /* GIVFuncInfoFlags */
    _PyGI_ENUM_BEGIN (VFuncInfoFlags)
        _PyGI_ENUM_ADD_VALUE (GI_VFUNC_MUST, CHAIN_UP)
        _PyGI_ENUM_ADD_VALUE (GI_VFUNC_MUST, OVERRIDE)
        _PyGI_ENUM_ADD_VALUE (GI_VFUNC_MUST, NOT_OVERRIDE)
    _PyGI_ENUM_END

    /* GIFieldInfoFlags */
    _PyGI_ENUM_BEGIN (FieldInfoFlags)
        _PyGI_ENUM_ADD_VALUE (GI_FIELD, IS_READABLE)
        _PyGI_ENUM_ADD_VALUE (GI_FIELD, IS_WRITABLE)
    _PyGI_ENUM_END

    /* GIFunctionInfoFlags */
    _PyGI_ENUM_BEGIN (FunctionInfoFlags)
        _PyGI_ENUM_ADD_VALUE (GI_FUNCTION, IS_METHOD)
        _PyGI_ENUM_ADD_VALUE (GI_FUNCTION, IS_CONSTRUCTOR)
        _PyGI_ENUM_ADD_VALUE (GI_FUNCTION, IS_GETTER)
        _PyGI_ENUM_ADD_VALUE (GI_FUNCTION, IS_SETTER)
        _PyGI_ENUM_ADD_VALUE (GI_FUNCTION, WRAPS_VFUNC)
        _PyGI_ENUM_ADD_VALUE (GI_FUNCTION, THROWS)
    _PyGI_ENUM_END

    /* GITypeTag */
    _PyGI_ENUM_BEGIN (TypeTag)
        /* Basic types */
        _PyGI_ENUM_ADD_VALUE (GI_TYPE_TAG, VOID)
        _PyGI_ENUM_ADD_VALUE (GI_TYPE_TAG, BOOLEAN)
        _PyGI_ENUM_ADD_VALUE (GI_TYPE_TAG, INT8)
        _PyGI_ENUM_ADD_VALUE (GI_TYPE_TAG, UINT8)
        _PyGI_ENUM_ADD_VALUE (GI_TYPE_TAG, INT16)
        _PyGI_ENUM_ADD_VALUE (GI_TYPE_TAG, UINT16)
        _PyGI_ENUM_ADD_VALUE (GI_TYPE_TAG, INT32)
        _PyGI_ENUM_ADD_VALUE (GI_TYPE_TAG, UINT32)
        _PyGI_ENUM_ADD_VALUE (GI_TYPE_TAG, INT64)
        _PyGI_ENUM_ADD_VALUE (GI_TYPE_TAG, UINT64)
        _PyGI_ENUM_ADD_VALUE (GI_TYPE_TAG, FLOAT)
        _PyGI_ENUM_ADD_VALUE (GI_TYPE_TAG, DOUBLE)
        _PyGI_ENUM_ADD_VALUE (GI_TYPE_TAG, GTYPE)
        _PyGI_ENUM_ADD_VALUE (GI_TYPE_TAG, UTF8)
        _PyGI_ENUM_ADD_VALUE (GI_TYPE_TAG, FILENAME)

        /* Non-basic types; compare with G_TYPE_TAG_IS_BASIC */
        _PyGI_ENUM_ADD_VALUE (GI_TYPE_TAG, ARRAY)
        _PyGI_ENUM_ADD_VALUE (GI_TYPE_TAG, INTERFACE)
        _PyGI_ENUM_ADD_VALUE (GI_TYPE_TAG, GLIST)
        _PyGI_ENUM_ADD_VALUE (GI_TYPE_TAG, GSLIST)
        _PyGI_ENUM_ADD_VALUE (GI_TYPE_TAG, GHASH)
        _PyGI_ENUM_ADD_VALUE (GI_TYPE_TAG, ERROR)

        /* Another basic type */
        _PyGI_ENUM_ADD_VALUE (GI_TYPE_TAG, UNICHAR)
    _PyGI_ENUM_END

    /* GIInfoType */
    _PyGI_ENUM_BEGIN (InfoType)
        _PyGI_ENUM_ADD_VALUE (GI_INFO_TYPE, INVALID)
        _PyGI_ENUM_ADD_VALUE (GI_INFO_TYPE, FUNCTION)
        _PyGI_ENUM_ADD_VALUE (GI_INFO_TYPE, CALLBACK)
        _PyGI_ENUM_ADD_VALUE (GI_INFO_TYPE, STRUCT)
        _PyGI_ENUM_ADD_VALUE (GI_INFO_TYPE, BOXED)
        _PyGI_ENUM_ADD_VALUE (GI_INFO_TYPE, ENUM)
        _PyGI_ENUM_ADD_VALUE (GI_INFO_TYPE, FLAGS)
        _PyGI_ENUM_ADD_VALUE (GI_INFO_TYPE, OBJECT)
        _PyGI_ENUM_ADD_VALUE (GI_INFO_TYPE, INTERFACE)
        _PyGI_ENUM_ADD_VALUE (GI_INFO_TYPE, CONSTANT)
        _PyGI_ENUM_ADD_VALUE (GI_INFO_TYPE, INVALID_0)
        _PyGI_ENUM_ADD_VALUE (GI_INFO_TYPE, UNION)
        _PyGI_ENUM_ADD_VALUE (GI_INFO_TYPE, VALUE)
        _PyGI_ENUM_ADD_VALUE (GI_INFO_TYPE, SIGNAL)
        _PyGI_ENUM_ADD_VALUE (GI_INFO_TYPE, VFUNC)
        _PyGI_ENUM_ADD_VALUE (GI_INFO_TYPE, PROPERTY)
        _PyGI_ENUM_ADD_VALUE (GI_INFO_TYPE, FIELD)
        _PyGI_ENUM_ADD_VALUE (GI_INFO_TYPE, ARG)
        _PyGI_ENUM_ADD_VALUE (GI_INFO_TYPE, TYPE)
        _PyGI_ENUM_ADD_VALUE (GI_INFO_TYPE, UNRESOLVED)
    _PyGI_ENUM_END

#undef _PyGI_ENUM_BEGIN
#undef _PyGI_ENUM_ADD_VALUE
#undef _PyGI_ENUM_END

}
