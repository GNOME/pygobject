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

#include <pythoncapi_compat.h>

#include "pygi-type.h"

#include "pygi-argument.h"
#include "pygi-basictype.h"
#include "pygi-cache.h"
#include "pygi-fundamental.h"
#include "pygi-info.h"
#include "pygi-invoke.h"
#include "pygi-util.h"

/* _generate_doc_string
 *
 * C wrapper to call Python implemented "gi.docstring.generate_doc_string"
 */
static PyObject *
_generate_doc_string (PyGIBaseInfo *self)
{
    static PyObject *_py_generate_doc_string = NULL;

    if (_py_generate_doc_string == NULL) {
        PyObject *mod = PyImport_ImportModule ("gi.docstring");
        if (!mod) return NULL;

        _py_generate_doc_string =
            PyObject_GetAttrString (mod, "generate_doc_string");
        if (_py_generate_doc_string == NULL) {
            Py_DECREF (mod);
            return NULL;
        }
        Py_DECREF (mod);
    }

    return PyObject_CallFunctionObjArgs (_py_generate_doc_string, self, NULL);
}

static PyObject *
_generate_signature (PyGICallableInfo *self)
{
    static PyObject *_py_generate_signature = NULL;

    if (_py_generate_signature == NULL) {
        PyObject *mod = PyImport_ImportModule ("gi._signature");
        if (!mod) return NULL;

        _py_generate_signature =
            PyObject_GetAttrString (mod, "generate_signature");
        if (_py_generate_signature == NULL) {
            Py_DECREF (mod);
            return NULL;
        }
        Py_DECREF (mod);
    }

    return PyObject_CallFunctionObjArgs (_py_generate_signature, self, NULL);
}

static PyObject *
_get_info_string (const gchar *value)
{
    if (value == NULL) {
        Py_RETURN_NONE;
    }
    return pygi_utf8_to_py (value);
}

/* TODO Maybe rework this to be a macro which can do the type safety properly */
typedef GIBaseInfo *(*GetChildInfoCallback) (GIBaseInfo *info);

static PyObject *
_get_child_info (PyGIBaseInfo *self, GetChildInfoCallback get_child_info)
{
    GIBaseInfo *info;
    PyObject *py_info;

    info = get_child_info ((GIBaseInfo *)self->info);
    if (info == NULL) {
        Py_RETURN_NONE;
    }

    py_info = _pygi_info_new (info);
    gi_base_info_unref (info);
    return py_info;
}

typedef GIBaseInfo *(*GetChildInfoByNameCallback) (GIBaseInfo *info,
                                                   const char *name);

static PyObject *
_get_child_info_by_name (PyGIBaseInfo *self, PyObject *py_name,
                         GetChildInfoByNameCallback get_child_info_by_name)
{
    GIBaseInfo *info;
    PyObject *py_info;
    char *name;

    if (!pygi_utf8_from_py (py_name, &name)) return NULL;

    info = get_child_info_by_name (self->info, name);
    g_free (name);
    if (info == NULL) {
        Py_RETURN_NONE;
    }

    py_info = _pygi_info_new (info);
    gi_base_info_unref (info);
    return py_info;
}


/* _make_infos_tuple
 *
 * Build a tuple from the common API pattern in GI of having a
 * function which returns a count and an indexed GIBaseInfo
 * in the range of 0 to count;
 */
/* TODO Maybe rework this to be a macro which can do the type safety properly */
typedef unsigned int (*GetNInfosCallback) (GIBaseInfo *info);
typedef GIBaseInfo *(*MakeInfosCallback) (GIBaseInfo *info, unsigned int idx);

static PyObject *
_make_infos_tuple (PyGIBaseInfo *self, GetNInfosCallback get_n_infos,
                   MakeInfosCallback get_info)
{
    gint n_infos;
    PyObject *infos;
    gint i;

    n_infos = get_n_infos ((GIBaseInfo *)self->info);

    infos = PyTuple_New (n_infos);
    if (infos == NULL) {
        return NULL;
    }

    for (i = 0; i < n_infos; i++) {
        GIBaseInfo *info;
        PyObject *py_info;

        info = (GIBaseInfo *)get_info (self->info, i);
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


/* BaseInfo */

/* We need to be careful about calling gi_base_info_get_name because
 * calling it with a GI_INFO_TYPE_TYPE will crash.
 * See: https://bugzilla.gnome.org/show_bug.cgi?id=709456
 */
static const char *
_safe_base_info_get_name (GIBaseInfo *info)
{
    if (GI_IS_TYPE_INFO (info)) {
        return "type_type_instance";
    } else {
        return gi_base_info_get_name (info);
    }
}

static void
_base_info_dealloc (PyGIBaseInfo *self)
{
    if (self->inst_weakreflist != NULL)
        PyObject_ClearWeakRefs ((PyObject *)self);

    gi_base_info_unref (self->info);

    Py_TYPE (self)->tp_free ((PyObject *)self);
}

static PyObject *
_base_info_repr (PyGIBaseInfo *self)
{
    return PyUnicode_FromFormat ("%s(%s)", Py_TYPE ((PyObject *)self)->tp_name,
                                 _safe_base_info_get_name (self->info));
}

static PyObject *
_wrap_gi_base_info_equal (PyGIBaseInfo *self, PyObject *other)
{
    GIBaseInfo *other_info;

    if (!PyObject_TypeCheck (other, &PyGIBaseInfo_Type)) {
        Py_INCREF (Py_NotImplemented);
        return Py_NotImplemented;
    }

    other_info = ((PyGIBaseInfo *)other)->info;
    if (gi_base_info_equal (self->info, other_info)) {
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
        return _wrap_gi_base_info_equal (self, other);
    case Py_NE:
        res = _wrap_gi_base_info_equal (self, other);
        if (Py_IsTrue (res)) {
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
    return Py_NewRef (res);
}

PYGI_DEFINE_TYPE ("gi.BaseInfo", PyGIBaseInfo_Type, PyGIBaseInfo);

PyObject *
_pygi_is_python_keyword (const gchar *name)
{
    static PyObject *iskeyword = NULL;
    PyObject *pyname, *result;

    if (!iskeyword) {
        PyObject *keyword_module = PyImport_ImportModule ("keyword");
        if (!keyword_module) return NULL;

        iskeyword = PyObject_GetAttrString (keyword_module, "iskeyword");
        Py_DECREF (keyword_module);
        if (!iskeyword) return NULL;
    }

    /* Python 3.x; note that we explicitly keep "print"; it is not a keyword
     * any more, but we do not want to break API between Python versions */
    if (strcmp (name, "print") == 0) Py_RETURN_TRUE;

    pyname = PyUnicode_FromString (name);
    if (!pyname) return NULL;

    result = PyObject_CallOneArg (iskeyword, pyname);
    Py_DECREF (pyname);

    return result;
}

static PyObject *
_wrap_gi_base_info_get_name (PyGIBaseInfo *self)
{
    const gchar *name;
    PyObject *is_keyword, *obj;

    name = _safe_base_info_get_name (self->info);
    is_keyword = _pygi_is_python_keyword (name);
    if (!is_keyword) return NULL;

    /* escape keywords */
    if (PyObject_IsTrue (is_keyword)) {
        gchar *escaped = g_strconcat (name, "_", NULL);
        obj = pygi_utf8_to_py (escaped);
        g_free (escaped);
    } else {
        obj = pygi_utf8_to_py (name);
    }
    Py_DECREF (is_keyword);

    return obj;
}

static PyObject *
_wrap_gi_base_info_get_name_unescaped (PyGIBaseInfo *self)
{
    return _get_info_string (_safe_base_info_get_name (self->info));
}

static PyObject *
_wrap_gi_base_info_get_namespace (PyGIBaseInfo *self)
{
    return _get_info_string (gi_base_info_get_namespace (self->info));
}

static PyObject *
_wrap_gi_base_info_is_deprecated (PyGIBaseInfo *self)
{
    if (gi_base_info_is_deprecated (self->info))
        Py_RETURN_TRUE;
    else
        Py_RETURN_FALSE;
}

static PyObject *
_wrap_gi_base_info_get_attribute (PyGIBaseInfo *self, PyObject *arg)
{
    char *name;
    const char *value;

    if (!pygi_utf8_from_py (arg, &name)) return NULL;

    value = gi_base_info_get_attribute (self->info, name);
    g_free (name);
    if (value == NULL) {
        Py_RETURN_NONE;
    }
    return pygi_utf8_to_py (value);
}

static PyObject *
_wrap_gi_base_info_get_container (PyGIBaseInfo *self)
{
    /* Note: don't use _get_child_info because gi_base_info_get_container
     * is marked as [transfer none] and therefore returns a borrowed ref.
     */
    GIBaseInfo *info;

    info = gi_base_info_get_container (self->info);

    if (info == NULL) {
        Py_RETURN_NONE;
    }

    return _pygi_info_new (info);
}


static PyMethodDef _PyGIBaseInfo_methods[] = {
    { "get_name", (PyCFunction)_wrap_gi_base_info_get_name, METH_NOARGS },
    { "get_name_unescaped", (PyCFunction)_wrap_gi_base_info_get_name_unescaped,
      METH_NOARGS },
    { "get_namespace", (PyCFunction)_wrap_gi_base_info_get_namespace,
      METH_NOARGS },
    { "is_deprecated", (PyCFunction)_wrap_gi_base_info_is_deprecated,
      METH_NOARGS },
    { "get_attribute", (PyCFunction)_wrap_gi_base_info_get_attribute, METH_O },
    { "get_container", (PyCFunction)_wrap_gi_base_info_get_container,
      METH_NOARGS },
    { "equal", (PyCFunction)_wrap_gi_base_info_equal, METH_O },
    { NULL, NULL, 0 },
};

/* _base_info_getattro:
 *
 * The usage of __getattr__ is needed because the get/set method table
 * does not work for __doc__.
 */
static PyObject *
_base_info_getattro (PyGIBaseInfo *self, PyObject *name)
{
    PyObject *result;

    static PyObject *docstr;
    if (docstr == NULL) {
        docstr = PyUnicode_InternFromString ("__doc__");
        if (docstr == NULL) return NULL;
    }

    Py_INCREF (name);
    PyUnicode_InternInPlace (&name);

    if (name == docstr) {
        result = _generate_doc_string (self);
    } else {
        result = PyObject_GenericGetAttr ((PyObject *)self, name);
    }

    Py_DECREF (name);
    return result;
}

static PyObject *
_base_info_attr_name (PyGIBaseInfo *self, void *closure)
{
    return _wrap_gi_base_info_get_name (self);
}

static PyObject *
_base_info_attr_module (PyGIBaseInfo *self, void *closure)
{
    return PyUnicode_FromFormat ("gi.repository.%s",
                                 gi_base_info_get_namespace (self->info));
}

static PyGetSetDef _base_info_getsets[] = {
    { "__name__", (getter)_base_info_attr_name, (setter)0, "Name", NULL },
    { "__module__", (getter)_base_info_attr_module, (setter)0, "Module name",
      NULL },
    { NULL, 0, 0 },
};

static PyObject *_callable_info_vectorcall (PyGICallableInfo *self,
                                            PyObject *const *args,
                                            size_t nargsf, PyObject *kwnames);
static PyObject *_function_info_vectorcall (PyGICallableInfo *self,
                                            PyObject *const *args,
                                            size_t nargsf, PyObject *kwnames);

PyObject *
_pygi_info_new (GIBaseInfo *info)
{
    PyTypeObject *type = NULL;
    vectorcallfunc vectorcall = NULL;
    PyGIBaseInfo *self;

    if (GI_IS_FUNCTION_INFO (info)) {
        type = &PyGIFunctionInfo_Type;
        vectorcall = (vectorcallfunc)_function_info_vectorcall;
    } else if (GI_IS_CALLBACK_INFO (info)) {
        type = &PyGICallbackInfo_Type;
        vectorcall = (vectorcallfunc)_callable_info_vectorcall;
    } else if (GI_IS_STRUCT_INFO (info)) {
        type = &PyGIStructInfo_Type;
    } else if (GI_IS_ENUM_INFO (info)) {
        type = &PyGIEnumInfo_Type;
    } else if (GI_IS_OBJECT_INFO (info)) {
        type = &PyGIObjectInfo_Type;
    } else if (GI_IS_INTERFACE_INFO (info)) {
        type = &PyGIInterfaceInfo_Type;
    } else if (GI_IS_CONSTANT_INFO (info)) {
        type = &PyGIConstantInfo_Type;
    } else if (GI_IS_UNION_INFO (info)) {
        type = &PyGIUnionInfo_Type;
    } else if (GI_IS_VALUE_INFO (info)) {
        type = &PyGIValueInfo_Type;
    } else if (GI_IS_SIGNAL_INFO (info)) {
        type = &PyGISignalInfo_Type;
        vectorcall = (vectorcallfunc)_callable_info_vectorcall;
    } else if (GI_IS_VFUNC_INFO (info)) {
        type = &PyGIVFuncInfo_Type;
        vectorcall = (vectorcallfunc)_callable_info_vectorcall;
    } else if (GI_IS_PROPERTY_INFO (info)) {
        type = &PyGIPropertyInfo_Type;
    } else if (GI_IS_FIELD_INFO (info)) {
        type = &PyGIFieldInfo_Type;
    } else if (GI_IS_ARG_INFO (info)) {
        type = &PyGIArgInfo_Type;
    } else if (GI_IS_TYPE_INFO (info)) {
        type = &PyGITypeInfo_Type;
    } else if (GI_IS_UNRESOLVED_INFO (info)) {
        type = &PyGIUnresolvedInfo_Type;
    } else {
        PyErr_SetString (PyExc_RuntimeError, "Invalid info type");
        return NULL;
    }

    self = (PyGIBaseInfo *)type->tp_alloc (type, 0);
    if (self == NULL) {
        return NULL;
    }

    self->info = gi_base_info_ref (info);
    self->inst_weakreflist = NULL;

    if (vectorcall != NULL) {
        ((PyGICallableInfo *)self)->vectorcall = vectorcall;
    }

    return (PyObject *)self;
}

GIBaseInfo *
_pygi_object_get_gi_info (PyObject *object, PyTypeObject *type)
{
    PyObject *py_info;
    GIBaseInfo *info = NULL;

    py_info = PyObject_GetAttrString (object, "__info__");
    if (py_info == NULL) {
        return NULL;
    }
    if (!PyObject_TypeCheck (py_info, type)) {
        PyErr_Format (PyExc_TypeError,
                      "attribute '__info__' must be %s, not %s", type->tp_name,
                      Py_TYPE (py_info)->tp_name);
        goto out;
    }

    info = ((PyGIBaseInfo *)py_info)->info;
    gi_base_info_ref (info);

out:
    Py_DECREF (py_info);

    return info;
}


/* CallableInfo */
PYGI_DEFINE_TYPE ("gi.CallableInfo", PyGICallableInfo_Type, PyGICallableInfo);

static void
_callable_info_dealloc (PyGICallableInfo *self)
{
    if (self->cache != NULL)
        pygi_callable_cache_free ((PyGICallableCache *)self->cache);
    _base_info_dealloc ((PyGIBaseInfo *)self);
}

/* _callable_info_call:
 *
 * Shared wrapper for invoke which can be bound (instance method or class constructor)
 * or unbound (function or static method).
 */
static PyObject *
_callable_info_vectorcall (PyGICallableInfo *self, PyObject *const *args,
                           size_t nargsf, PyObject *kwnames)
{
    return pygi_callable_info_invoke (self, args, nargsf, kwnames);
}

static PyObject *
_callable_info_repr (PyGICallableInfo *self)
{
    return PyUnicode_FromFormat ("%s(%s)", Py_TYPE ((PyObject *)self)->tp_name,
                                 _safe_base_info_get_name (self->base.info));
}

/* _function_info_vectorcall:
 *
 * Specialization of _callable_info_call for GIFunctionInfo which
 * handles constructor error conditions.
 */
static PyObject *
_function_info_vectorcall (PyGICallableInfo *self, PyObject *const *args,
                           size_t nargsf, PyObject *kwnames)
{
    GIFunctionInfoFlags flags;

    /* Ensure constructors are only called as class methods on the class
     * implementing the constructor and not on sub-classes.
     */
    flags = gi_function_info_get_flags ((GIFunctionInfo *)self->base.info);
    if (flags & GI_FUNCTION_IS_CONSTRUCTOR) {
        Py_ssize_t nargs;
        PyObject *cls;
        PyObject *py_str_name;
        const gchar *str_name;
        GIBaseInfo *container_info =
            gi_base_info_get_container (self->base.info);

        g_assert (container_info != NULL);

        nargs = PyVectorcall_NARGS (nargsf);
        cls = nargs > 0 ? args[0] : NULL;
        if (cls == NULL) {
            PyErr_BadArgument ();
            return NULL;
        }
        py_str_name = PyObject_GetAttrString (cls, "__name__");
        if (py_str_name == NULL) return NULL;

        if (!PyUnicode_Check (py_str_name)) {
            PyErr_SetString (PyExc_TypeError,
                             "cls.__name__ attribute is not a string");
            Py_DECREF (py_str_name);
            return NULL;
        }

        str_name = PyUnicode_AsUTF8 (py_str_name);
        if (strcmp (str_name, _safe_base_info_get_name (container_info))) {
            PyErr_Format (
                PyExc_TypeError,
                "%s constructor cannot be used to create instances of "
                "a subclass %s",
                _safe_base_info_get_name (container_info), str_name);
            Py_DECREF (py_str_name);
            return NULL;
        }
        Py_DECREF (py_str_name);
    }

    return _callable_info_vectorcall (self, args, nargsf, kwnames);
}

/* _function_info_descr_get
 *
 * Descriptor protocol implementation for functions, methods, and constructors.
 */
static PyObject *
_function_info_descr_get (PyGICallableInfo *self, PyObject *obj,
                          PyObject *type)
{
    if (obj != NULL && obj != Py_None)
        return PyMethod_New ((PyObject *)self, obj);

    return Py_NewRef (self);
}

/* _vfunc_info_descr_get
 *
 * Descriptor protocol implementation for virtual functions.
 */
static PyObject *
_vfunc_info_descr_get (PyGICallableInfo *self, PyObject *obj, PyObject *type)
{
    PyObject *result;
    PyObject *bound_arg = NULL;

    if (type == NULL) type = (PyObject *)Py_TYPE (obj);

    bound_arg = PyObject_GetAttrString (type, "__gtype__");
    if (bound_arg == NULL) return NULL;

    /* _new_bound_callable_info adds its own ref so free the one from GetAttrString */
    result = PyMethod_New ((PyObject *)self, bound_arg);
    Py_DECREF (bound_arg);
    return result;
}

static PyObject *
_wrap_gi_callable_info_get_arguments (PyGIBaseInfo *self)
{
    return _make_infos_tuple (self,
                              (GetNInfosCallback)gi_callable_info_get_n_args,
                              (MakeInfosCallback)gi_callable_info_get_arg);
}

static PyObject *
_wrap_gi_callable_info_get_return_type (PyGIBaseInfo *self)
{
    return _get_child_info (
        self, (GetChildInfoCallback)gi_callable_info_get_return_type);
}

static PyObject *
_wrap_gi_callable_info_get_caller_owns (PyGIBaseInfo *self)
{
    return pygi_guint_to_py (
        gi_callable_info_get_caller_owns (GI_CALLABLE_INFO (self->info)));
}

static PyObject *
_wrap_gi_callable_info_may_return_null (PyGIBaseInfo *self)
{
    return pygi_gboolean_to_py (
        gi_callable_info_may_return_null (GI_CALLABLE_INFO (self->info)));
}

static PyObject *
_wrap_gi_callable_info_skip_return (PyGIBaseInfo *self)
{
    return pygi_gboolean_to_py (
        gi_callable_info_skip_return (GI_CALLABLE_INFO (self->info)));
}

static PyObject *
_wrap_gi_callable_info_get_return_attribute (PyGIBaseInfo *self,
                                             PyObject *py_name)
{
    gchar *name;
    const gchar *attr;

    if (!pygi_utf8_from_py (py_name, &name)) return NULL;

    attr = gi_callable_info_get_return_attribute (
        GI_CALLABLE_INFO (self->info), name);
    if (attr) {
        g_free (name);
        return pygi_utf8_to_py (attr);
    } else {
        PyErr_Format (PyExc_AttributeError, "return attribute %s not found",
                      name);
        g_free (name);
        return NULL;
    }
}

static PyObject *
_wrap_gi_callable_info_can_throw_gerror (PyGIBaseInfo *self)
{
    if (gi_callable_info_can_throw_gerror (GI_CALLABLE_INFO (self->info)))
        Py_RETURN_TRUE;
    else
        Py_RETURN_FALSE;
}

static PyMethodDef _PyGICallableInfo_methods[] = {
    { "invoke", (PyCFunction)PyObject_Call, METH_VARARGS | METH_KEYWORDS },
    { "get_arguments", (PyCFunction)_wrap_gi_callable_info_get_arguments,
      METH_NOARGS },
    { "get_return_type", (PyCFunction)_wrap_gi_callable_info_get_return_type,
      METH_NOARGS },
    { "get_caller_owns", (PyCFunction)_wrap_gi_callable_info_get_caller_owns,
      METH_NOARGS },
    { "may_return_null", (PyCFunction)_wrap_gi_callable_info_may_return_null,
      METH_NOARGS },
    { "skip_return", (PyCFunction)_wrap_gi_callable_info_skip_return,
      METH_NOARGS },
    { "get_return_attribute",
      (PyCFunction)_wrap_gi_callable_info_get_return_attribute, METH_O },
    { "can_throw_gerror", (PyCFunction)_wrap_gi_callable_info_can_throw_gerror,
      METH_NOARGS },
    { NULL, NULL, 0 },
};

static PyObject *
_callable_info_signature (PyGICallableInfo *self)
{
    return _generate_signature (self);
}

static PyGetSetDef _PyGICallableInfo_getsets[] = {
    { "__signature__", (getter)_callable_info_signature, (setter)NULL },
    { NULL, NULL, NULL },
};


PyGIFunctionCache *
pygi_callable_info_get_cache (PyGICallableInfo *self)
{
    PyGIFunctionCache *function_cache;
    GIBaseInfo *info = self->base.info;

    if (self->cache != NULL) return self->cache;

    if (GI_IS_FUNCTION_INFO (info)) {
        GIFunctionInfoFlags flags;

        flags = gi_function_info_get_flags (GI_FUNCTION_INFO (info));

        if (flags & GI_FUNCTION_IS_CONSTRUCTOR) {
            function_cache =
                pygi_constructor_cache_new (GI_CALLABLE_INFO (info));
        } else if (flags & GI_FUNCTION_IS_METHOD) {
            function_cache = pygi_method_cache_new (GI_CALLABLE_INFO (info));
        } else {
            function_cache = pygi_function_cache_new (GI_CALLABLE_INFO (info));
        }
    } else if (GI_IS_VFUNC_INFO (info)) {
        function_cache = pygi_vfunc_cache_new (GI_CALLABLE_INFO (info));
    } else if (GI_IS_CALLBACK_INFO (info)) {
        g_error ("Cannot invoke callback types");
    } else {
        function_cache = pygi_method_cache_new (GI_CALLABLE_INFO (info));
    }

    self->cache = function_cache;

    return function_cache;
}

/* CallbackInfo */
PYGI_DEFINE_TYPE ("gi.CallbackInfo", PyGICallbackInfo_Type, PyGICallableInfo);

static PyMethodDef _PyGICallbackInfo_methods[] = {
    { NULL, NULL, 0 },
};

/* ErrorDomainInfo */
PYGI_DEFINE_TYPE ("gi.ErrorDomainInfo", PyGIErrorDomainInfo_Type,
                  PyGIBaseInfo);

static PyMethodDef _PyGIErrorDomainInfo_methods[] = {
    { NULL, NULL, 0 },
};

/* SignalInfo */
PYGI_DEFINE_TYPE ("gi.SignalInfo", PyGISignalInfo_Type, PyGICallableInfo);

static PyObject *
_wrap_gi_signal_info_get_flags (PyGIBaseInfo *self)
{
    return pygi_guint_to_py (
        gi_signal_info_get_flags ((GISignalInfo *)self->info));
}

static PyObject *
_wrap_gi_signal_info_get_class_closure (PyGIBaseInfo *self)
{
    return _get_child_info (
        self, (GetChildInfoCallback)gi_signal_info_get_class_closure);
}

static PyObject *
_wrap_gi_signal_info_true_stops_emit (PyGIBaseInfo *self)
{
    return pygi_gboolean_to_py (
        gi_signal_info_true_stops_emit ((GISignalInfo *)self->info));
}

static PyMethodDef _PyGISignalInfo_methods[] = {
    { "get_flags", (PyCFunction)_wrap_gi_signal_info_get_flags, METH_NOARGS },
    { "get_class_closure", (PyCFunction)_wrap_gi_signal_info_get_class_closure,
      METH_NOARGS },
    { "true_stops_emit", (PyCFunction)_wrap_gi_signal_info_true_stops_emit,
      METH_NOARGS },
    { NULL, NULL, 0 },
};

/* PropertyInfo */
PYGI_DEFINE_TYPE ("gi.PropertyInfo", PyGIPropertyInfo_Type, PyGIBaseInfo);

static PyObject *
_wrap_gi_property_info_get_flags (PyGIBaseInfo *self)
{
    return pygi_guint_to_py (
        gi_property_info_get_flags ((GIPropertyInfo *)self->info));
}

static PyObject *
_wrap_gi_property_info_get_type_info (PyGIBaseInfo *self)
{
    return _get_child_info (
        self, (GetChildInfoCallback)gi_property_info_get_type_info);
}

static PyObject *
_wrap_gi_property_info_get_ownership_transfer (PyGIBaseInfo *self)
{
    return pygi_guint_to_py (gi_property_info_get_ownership_transfer (
        (GIPropertyInfo *)self->info));
}

static PyMethodDef _PyGIPropertyInfo_methods[] = {
    { "get_flags", (PyCFunction)_wrap_gi_property_info_get_flags,
      METH_NOARGS },
    { "get_type_info", (PyCFunction)_wrap_gi_property_info_get_type_info,
      METH_NOARGS },
    { "get_ownership_transfer",
      (PyCFunction)_wrap_gi_property_info_get_ownership_transfer,
      METH_NOARGS },
    { NULL, NULL, 0 },
};


/* ArgInfo */
PYGI_DEFINE_TYPE ("gi.ArgInfo", PyGIArgInfo_Type, PyGIBaseInfo);

static PyObject *
_wrap_gi_arg_info_get_direction (PyGIBaseInfo *self)
{
    return pygi_guint_to_py (
        gi_arg_info_get_direction ((GIArgInfo *)self->info));
}

static PyObject *
_wrap_gi_arg_info_is_caller_allocates (PyGIBaseInfo *self)
{
    return pygi_gboolean_to_py (
        gi_arg_info_is_caller_allocates ((GIArgInfo *)self->info));
}

static PyObject *
_wrap_gi_arg_info_is_return_value (PyGIBaseInfo *self)
{
    return pygi_gboolean_to_py (
        gi_arg_info_is_return_value ((GIArgInfo *)self->info));
}

static PyObject *
_wrap_gi_arg_info_is_optional (PyGIBaseInfo *self)
{
    return pygi_gboolean_to_py (
        gi_arg_info_is_optional ((GIArgInfo *)self->info));
}

static PyObject *
_wrap_gi_arg_info_may_be_null (PyGIBaseInfo *self)
{
    return pygi_gboolean_to_py (
        gi_arg_info_may_be_null ((GIArgInfo *)self->info));
}

static PyObject *
_wrap_gi_arg_info_get_ownership_transfer (PyGIBaseInfo *self)
{
    return pygi_guint_to_py (
        gi_arg_info_get_ownership_transfer ((GIArgInfo *)self->info));
}

static PyObject *
_wrap_gi_arg_info_get_scope (PyGIBaseInfo *self)
{
    return pygi_guint_to_py (gi_arg_info_get_scope ((GIArgInfo *)self->info));
}

static PyObject *
_wrap_gi_arg_info_get_closure_index (PyGIBaseInfo *self)
{
    unsigned int closure_index;

    if (gi_arg_info_get_closure_index ((GIArgInfo *)self->info,
                                       &closure_index))
        return pygi_guint_to_py (closure_index);
    else
        return pygi_gint_to_py (-1);
}

static PyObject *
_wrap_gi_arg_info_get_destroy_index (PyGIBaseInfo *self)
{
    unsigned int destroy_index;
    if (gi_arg_info_get_destroy_index ((GIArgInfo *)self->info,
                                       &destroy_index))
        return pygi_guint_to_py (destroy_index);
    else
        return pygi_gint_to_py (-1);
}

static PyObject *
_wrap_gi_arg_info_get_type_info (PyGIBaseInfo *self)
{
    return _get_child_info (self,
                            (GetChildInfoCallback)gi_arg_info_get_type_info);
}

static PyMethodDef _PyGIArgInfo_methods[] = {
    { "get_direction", (PyCFunction)_wrap_gi_arg_info_get_direction,
      METH_NOARGS },
    { "is_caller_allocates",
      (PyCFunction)_wrap_gi_arg_info_is_caller_allocates, METH_NOARGS },
    { "is_return_value", (PyCFunction)_wrap_gi_arg_info_is_return_value,
      METH_NOARGS },
    { "is_optional", (PyCFunction)_wrap_gi_arg_info_is_optional, METH_NOARGS },
    { "may_be_null", (PyCFunction)_wrap_gi_arg_info_may_be_null, METH_NOARGS },
    { "get_ownership_transfer",
      (PyCFunction)_wrap_gi_arg_info_get_ownership_transfer, METH_NOARGS },
    { "get_scope", (PyCFunction)_wrap_gi_arg_info_get_scope, METH_NOARGS },
    { "get_closure_index", (PyCFunction)_wrap_gi_arg_info_get_closure_index,
      METH_NOARGS },
    { "get_destroy_index", (PyCFunction)_wrap_gi_arg_info_get_destroy_index,
      METH_NOARGS },
    { "get_type_info", (PyCFunction)_wrap_gi_arg_info_get_type_info,
      METH_NOARGS },
    { NULL, NULL, 0 },
};


/* TypeInfo */
PYGI_DEFINE_TYPE ("gi.TypeInfo", PyGITypeInfo_Type, PyGIBaseInfo);

static PyObject *
_wrap_gi_type_info_is_pointer (PyGIBaseInfo *self)
{
    return pygi_gboolean_to_py (
        gi_type_info_is_pointer (GI_TYPE_INFO (self->info)));
}

static PyObject *
_wrap_gi_type_info_get_tag (PyGIBaseInfo *self)
{
    return pygi_guint_to_py (gi_type_info_get_tag (GI_TYPE_INFO (self->info)));
}

static PyObject *
_wrap_gi_type_info_get_tag_as_string (PyGIBaseInfo *self)
{
    GITypeTag tag = gi_type_info_get_tag (GI_TYPE_INFO (self->info));
    return pygi_utf8_to_py (gi_type_tag_to_string (tag));
}

static PyObject *
_wrap_gi_type_info_get_param_type (PyGIBaseInfo *self, PyObject *py_n)
{
    GIBaseInfo *info;
    PyObject *py_info;
    gint n;

    if (!pygi_gint_from_py (py_n, &n)) return NULL;

    info = (GIBaseInfo *)gi_type_info_get_param_type ((GITypeInfo *)self->info,
                                                      n);
    if (info == NULL) {
        Py_RETURN_NONE;
    }

    py_info = _pygi_info_new (info);
    gi_base_info_unref (info);
    return py_info;
}

static PyObject *
_wrap_gi_type_info_get_interface (PyGIBaseInfo *self)
{
    return _get_child_info (self,
                            (GetChildInfoCallback)gi_type_info_get_interface);
}

static PyObject *
_wrap_gi_type_info_get_array_length_index (PyGIBaseInfo *self)
{
    unsigned int idx;

    if (gi_type_info_get_array_length_index (GI_TYPE_INFO (self->info), &idx))
        return pygi_guint_to_py (idx);
    else
        return pygi_gint_to_py (-1);
}

static PyObject *
_wrap_gi_type_info_get_array_fixed_size (PyGIBaseInfo *self)
{
    size_t size;
    if (gi_type_info_get_array_fixed_size (GI_TYPE_INFO (self->info), &size))
        return pygi_gint_to_py (size);

    g_assert_not_reached ();
}

static PyObject *
_wrap_gi_type_info_is_zero_terminated (PyGIBaseInfo *self)
{
    return pygi_gboolean_to_py (
        gi_type_info_is_zero_terminated (GI_TYPE_INFO (self->info)));
}

static PyObject *
_wrap_gi_type_info_get_array_type (PyGIBaseInfo *self)
{
    return pygi_guint_to_py (
        gi_type_info_get_array_type (GI_TYPE_INFO (self->info)));
}

static PyMethodDef _PyGITypeInfo_methods[] = {
    { "is_pointer", (PyCFunction)_wrap_gi_type_info_is_pointer, METH_NOARGS },
    { "get_tag", (PyCFunction)_wrap_gi_type_info_get_tag, METH_NOARGS },
    { "get_tag_as_string", (PyCFunction)_wrap_gi_type_info_get_tag_as_string,
      METH_NOARGS },
    { "get_param_type", (PyCFunction)_wrap_gi_type_info_get_param_type,
      METH_O },
    { "get_interface", (PyCFunction)_wrap_gi_type_info_get_interface,
      METH_NOARGS },
    { "get_array_length_index",
      (PyCFunction)_wrap_gi_type_info_get_array_length_index, METH_NOARGS },
    { "get_array_fixed_size",
      (PyCFunction)_wrap_gi_type_info_get_array_fixed_size, METH_NOARGS },
    { "is_zero_terminated", (PyCFunction)_wrap_gi_type_info_is_zero_terminated,
      METH_NOARGS },
    { "get_array_type", (PyCFunction)_wrap_gi_type_info_get_array_type,
      METH_NOARGS },
    { NULL, NULL, 0 },
};


/* FunctionInfo */
PYGI_DEFINE_TYPE ("gi.FunctionInfo", PyGIFunctionInfo_Type, PyGICallableInfo);

static PyObject *
_wrap_gi_function_info_is_constructor (PyGIBaseInfo *self)
{
    GIFunctionInfoFlags flags;
    gboolean is_constructor;

    flags = gi_function_info_get_flags ((GIFunctionInfo *)self->info);
    is_constructor = flags & GI_FUNCTION_IS_CONSTRUCTOR;

    return pygi_gboolean_to_py (is_constructor);
}

static PyObject *
_wrap_gi_function_info_is_method (PyGIBaseInfo *self)
{
    GIFunctionInfoFlags flags;
    gboolean is_method;

    flags = gi_function_info_get_flags ((GIFunctionInfo *)self->info);
    is_method = flags & GI_FUNCTION_IS_METHOD;

    return pygi_gboolean_to_py (is_method);
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
                      gi_type_tag_to_string (type_tag));
        break;
    default:
        break;
    }

    return size;
}

gsize
_pygi_gi_type_info_size (GITypeInfo *type_info)
{
    gsize size = 0;

    GITypeTag type_tag;

    type_tag = gi_type_info_get_tag (type_info);
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
    case GI_TYPE_TAG_INTERFACE: {
        GIBaseInfo *info;

        info = gi_type_info_get_interface (type_info);

        if (GI_IS_STRUCT_INFO (info)) {
            if (gi_type_info_is_pointer (type_info)) {
                size = sizeof (gpointer);
            } else {
                size = gi_struct_info_get_size ((GIStructInfo *)info);
            }
        } else if (GI_IS_UNION_INFO (info)) {
            if (gi_type_info_is_pointer (type_info)) {
                size = sizeof (gpointer);
            } else {
                size = gi_union_info_get_size ((GIUnionInfo *)info);
            }
        } else if (GI_IS_ENUM_INFO (info)) {
            if (gi_type_info_is_pointer (type_info)) {
                size = sizeof (gpointer);
            } else {
                GITypeTag enum_type_tag;

                enum_type_tag =
                    gi_enum_info_get_storage_type ((GIEnumInfo *)info);
                size = _pygi_g_type_tag_size (enum_type_tag);
            }
        } else if (GI_IS_OBJECT_INFO (info) || GI_IS_INTERFACE_INFO (info)
                   || GI_IS_CALLBACK_INFO (info)) {
            size = sizeof (gpointer);
        } else {
            g_assert_not_reached ();
        }

        gi_base_info_unref (info);
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
    default:
        break;
    }

    return size;
}

static PyObject *
_wrap_gi_function_info_get_symbol (PyGIBaseInfo *self)
{
    return _get_info_string (
        gi_function_info_get_symbol (GI_FUNCTION_INFO (self->info)));
}

static PyObject *
_wrap_gi_function_info_get_flags (PyGIBaseInfo *self)
{
    return pygi_guint_to_py (
        gi_function_info_get_flags (GI_FUNCTION_INFO (self->info)));
}

static PyObject *
_wrap_gi_function_info_get_property (PyGIBaseInfo *self)
{
    return _get_child_info (
        self, (GetChildInfoCallback)gi_function_info_get_property);
}

static PyObject *
_wrap_gi_function_info_get_vfunc (PyGIBaseInfo *self)
{
    return _get_child_info (self,
                            (GetChildInfoCallback)gi_function_info_get_vfunc);
}

static PyObject *
_wrap_gi_function_info_get_finish_func (PyGICallableInfo *self)
{
    PyGIFunctionCache *cache = pygi_callable_info_get_cache (self);

    if (PyErr_Occurred ()) return NULL;

    if (cache == NULL) return NULL;

    if (cache->async_finish) {
        Py_INCREF (cache->async_finish);
        return cache->async_finish;
    }

    Py_RETURN_NONE;
}

static PyMethodDef _PyGIFunctionInfo_methods[] = {
    { "is_constructor", (PyCFunction)_wrap_gi_function_info_is_constructor,
      METH_NOARGS },
    { "is_method", (PyCFunction)_wrap_gi_function_info_is_method,
      METH_NOARGS },
    { "get_symbol", (PyCFunction)_wrap_gi_function_info_get_symbol,
      METH_NOARGS },
    { "get_flags", (PyCFunction)_wrap_gi_function_info_get_flags,
      METH_NOARGS },
    { "get_property", (PyCFunction)_wrap_gi_function_info_get_property,
      METH_NOARGS },
    { "get_vfunc", (PyCFunction)_wrap_gi_function_info_get_vfunc,
      METH_NOARGS },
    { "get_finish_func", (PyCFunction)_wrap_gi_function_info_get_finish_func,
      METH_NOARGS },
    { NULL, NULL, 0 },
};

/* RegisteredTypeInfo */
PYGI_DEFINE_TYPE ("gi.RegisteredTypeInfo", PyGIRegisteredTypeInfo_Type,
                  PyGIBaseInfo);

static PyObject *
_wrap_gi_registered_type_info_get_type_name (PyGIBaseInfo *self)
{
    return _get_info_string (gi_registered_type_info_get_type_name (
        GI_REGISTERED_TYPE_INFO (self->info)));
}

static PyObject *
_wrap_gi_registered_type_info_get_type_init (PyGIBaseInfo *self)
{
    return _get_info_string (
        gi_registered_type_info_get_type_init_function_name (
            GI_REGISTERED_TYPE_INFO (self->info)));
}

static PyObject *
_wrap_gi_registered_type_info_get_g_type (PyGIBaseInfo *self)
{
    GType type;

    type = gi_registered_type_info_get_g_type (
        (GIRegisteredTypeInfo *)self->info);

    return pyg_type_wrapper_new (type);
}

static PyMethodDef _PyGIRegisteredTypeInfo_methods[] = {
    { "get_type_name",
      (PyCFunction)_wrap_gi_registered_type_info_get_type_name, METH_NOARGS },
    { "get_type_init",
      (PyCFunction)_wrap_gi_registered_type_info_get_type_init, METH_NOARGS },
    { "get_g_type", (PyCFunction)_wrap_gi_registered_type_info_get_g_type,
      METH_NOARGS },
    { NULL, NULL, 0 },
};


/* GIStructInfo */
PYGI_DEFINE_TYPE ("StructInfo", PyGIStructInfo_Type, PyGIBaseInfo);

static PyObject *
_wrap_gi_struct_info_get_fields (PyGIBaseInfo *self)
{
    return _make_infos_tuple (self,
                              (GetNInfosCallback)gi_struct_info_get_n_fields,
                              (MakeInfosCallback)gi_struct_info_get_field);
}

static PyObject *
_wrap_gi_struct_info_get_methods (PyGIBaseInfo *self)
{
    return _make_infos_tuple (self,
                              (GetNInfosCallback)gi_struct_info_get_n_methods,
                              (MakeInfosCallback)gi_struct_info_get_method);
}

static PyObject *
_wrap_gi_struct_info_get_size (PyGIBaseInfo *self)
{
    return pygi_gsize_to_py (
        gi_struct_info_get_size (GI_STRUCT_INFO (self->info)));
}

static PyObject *
_wrap_gi_struct_info_get_alignment (PyGIBaseInfo *self)
{
    return pygi_gsize_to_py (
        gi_struct_info_get_alignment (GI_STRUCT_INFO (self->info)));
}

static PyObject *
_wrap_gi_struct_info_is_gtype_struct (PyGIBaseInfo *self)
{
    return pygi_gboolean_to_py (
        gi_struct_info_is_gtype_struct (GI_STRUCT_INFO (self->info)));
}

static PyObject *
_wrap_gi_struct_info_is_foreign (PyGIBaseInfo *self)
{
    return pygi_gboolean_to_py (
        gi_struct_info_is_foreign (GI_STRUCT_INFO (self->info)));
}

static PyObject *
_wrap_gi_struct_info_find_method (PyGIBaseInfo *self, PyObject *py_name)
{
    return _get_child_info_by_name (
        self, py_name, (GetChildInfoByNameCallback)gi_struct_info_find_method);
}

static PyObject *
_wrap_gi_struct_info_find_field (PyGIBaseInfo *self, PyObject *py_name)
{
    return _get_child_info_by_name (
        self, py_name, (GetChildInfoByNameCallback)gi_struct_info_find_field);
}

static PyMethodDef _PyGIStructInfo_methods[] = {
    { "get_fields", (PyCFunction)_wrap_gi_struct_info_get_fields,
      METH_NOARGS },
    { "find_field", (PyCFunction)_wrap_gi_struct_info_find_field, METH_O },
    { "get_methods", (PyCFunction)_wrap_gi_struct_info_get_methods,
      METH_NOARGS },
    { "find_method", (PyCFunction)_wrap_gi_struct_info_find_method, METH_O },
    { "get_size", (PyCFunction)_wrap_gi_struct_info_get_size, METH_NOARGS },
    { "get_alignment", (PyCFunction)_wrap_gi_struct_info_get_alignment,
      METH_NOARGS },
    { "is_gtype_struct", (PyCFunction)_wrap_gi_struct_info_is_gtype_struct,
      METH_NOARGS },
    { "is_foreign", (PyCFunction)_wrap_gi_struct_info_is_foreign,
      METH_NOARGS },
    { NULL, NULL, 0 },
};

gboolean
pygi_gi_struct_info_is_simple (GIStructInfo *struct_info)
{
    gboolean is_simple;
    gint n_field_infos;
    gint i;

    is_simple = TRUE;

    n_field_infos = gi_struct_info_get_n_fields (struct_info);

    for (i = 0; i < n_field_infos && is_simple; i++) {
        GIFieldInfo *field_info;
        GITypeInfo *field_type_info;
        GITypeTag field_type_tag;

        field_info = gi_struct_info_get_field (struct_info, i);
        field_type_info = gi_field_info_get_type_info (field_info);


        field_type_tag = gi_type_info_get_tag (field_type_info);

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
            if (gi_type_info_is_pointer (field_type_info)) {
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
        case GI_TYPE_TAG_INTERFACE: {
            GIBaseInfo *info;

            info = gi_type_info_get_interface (field_type_info);

            if (GI_IS_STRUCT_INFO (info)) {
                if (gi_type_info_is_pointer (field_type_info)) {
                    is_simple = FALSE;
                } else {
                    is_simple =
                        pygi_gi_struct_info_is_simple ((GIStructInfo *)info);
                }
            } else if (GI_IS_UNION_INFO (info)) {
                /* TODO */
                is_simple = FALSE;
            } else if (GI_IS_ENUM_INFO (info)) {
                if (gi_type_info_is_pointer (field_type_info)) {
                    is_simple = FALSE;
                }
            } else if (GI_IS_OBJECT_INFO (info) || GI_IS_CALLBACK_INFO (info)
                       || GI_IS_INTERFACE_INFO (info)) {
                is_simple = FALSE;
            } else {
                g_assert_not_reached ();
            }

            gi_base_info_unref (info);
            break;
        }
        default:
            g_assert_not_reached ();
            break;
        }

        gi_base_info_unref ((GIBaseInfo *)field_type_info);
        gi_base_info_unref ((GIBaseInfo *)field_info);
    }

    return is_simple;
}


/* EnumInfo */
PYGI_DEFINE_TYPE ("gi.EnumInfo", PyGIEnumInfo_Type, PyGIBaseInfo);

static PyObject *
_wrap_gi_enum_info_get_values (PyGIBaseInfo *self)
{
    return _make_infos_tuple (self,
                              (GetNInfosCallback)gi_enum_info_get_n_values,
                              (MakeInfosCallback)gi_enum_info_get_value);
}

static PyObject *
_wrap_gi_enum_info_is_flags (PyGIBaseInfo *self)
{
    if (GI_IS_FLAGS_INFO (self->info)) {
        /* Check flags before enums: flags are a subtype of enum. */
        Py_RETURN_TRUE;
    } else if (GI_IS_ENUM_INFO (self->info)) {
        Py_RETURN_FALSE;
    } else {
        g_assert_not_reached ();
    }
}

static PyObject *
_wrap_gi_enum_info_get_methods (PyGIBaseInfo *self)
{
    return _make_infos_tuple (self,
                              (GetNInfosCallback)gi_enum_info_get_n_methods,
                              (MakeInfosCallback)gi_enum_info_get_method);
}

static PyObject *
_wrap_gi_enum_info_get_storage_type (PyGIBaseInfo *self)
{
    return pygi_guint_to_py (
        gi_enum_info_get_storage_type (GI_ENUM_INFO (self->info)));
}

static PyMethodDef _PyGIEnumInfo_methods[] = {
    { "get_values", (PyCFunction)_wrap_gi_enum_info_get_values, METH_NOARGS },
    { "is_flags", (PyCFunction)_wrap_gi_enum_info_is_flags, METH_NOARGS },
    { "get_methods", (PyCFunction)_wrap_gi_enum_info_get_methods,
      METH_NOARGS },
    { "get_storage_type", (PyCFunction)_wrap_gi_enum_info_get_storage_type,
      METH_NOARGS },
    { NULL, NULL, 0 },
};


/* ObjectInfo */
PYGI_DEFINE_TYPE ("ObjectInfo", PyGIObjectInfo_Type, PyGIBaseInfo);

static PyObject *
_wrap_gi_object_info_get_parent (PyGIBaseInfo *self)
{
    return _get_child_info (self,
                            (GetChildInfoCallback)gi_object_info_get_parent);
}

static PyObject *
_wrap_gi_object_info_get_methods (PyGIBaseInfo *self)
{
    return _make_infos_tuple (self,
                              (GetNInfosCallback)gi_object_info_get_n_methods,
                              (MakeInfosCallback)gi_object_info_get_method);
}

static PyObject *
_wrap_gi_object_info_find_method (PyGIBaseInfo *self, PyObject *py_name)
{
    return _get_child_info_by_name (
        self, py_name, (GetChildInfoByNameCallback)gi_object_info_find_method);
}

static PyObject *
_wrap_gi_object_info_get_fields (PyGIBaseInfo *self)
{
    return _make_infos_tuple (self,
                              (GetNInfosCallback)gi_object_info_get_n_fields,
                              (MakeInfosCallback)gi_object_info_get_field);
}

static PyObject *
_wrap_gi_object_info_get_properties (PyGIBaseInfo *self)
{
    return _make_infos_tuple (
        self, (GetNInfosCallback)gi_object_info_get_n_properties,
        (MakeInfosCallback)gi_object_info_get_property);
}

static PyObject *
_wrap_gi_object_info_get_signals (PyGIBaseInfo *self)
{
    return _make_infos_tuple (self,
                              (GetNInfosCallback)gi_object_info_get_n_signals,
                              (MakeInfosCallback)gi_object_info_get_signal);
}

static PyObject *
_wrap_gi_object_info_get_interfaces (PyGIBaseInfo *self)
{
    return _make_infos_tuple (
        self, (GetNInfosCallback)gi_object_info_get_n_interfaces,
        (MakeInfosCallback)gi_object_info_get_interface);
}

static PyObject *
_wrap_gi_object_info_get_constants (PyGIBaseInfo *self)
{
    return _make_infos_tuple (
        self, (GetNInfosCallback)gi_object_info_get_n_constants,
        (MakeInfosCallback)gi_object_info_get_constant);
}

static PyObject *
_wrap_gi_object_info_get_vfuncs (PyGIBaseInfo *self)
{
    return _make_infos_tuple (self,
                              (GetNInfosCallback)gi_object_info_get_n_vfuncs,
                              (MakeInfosCallback)gi_object_info_get_vfunc);
}

static PyObject *
_wrap_gi_object_info_get_abstract (PyGIBaseInfo *self)
{
    gboolean is_abstract =
        gi_object_info_get_abstract ((GIObjectInfo *)self->info);
    return pygi_gboolean_to_py (is_abstract);
}

static PyObject *
_wrap_gi_object_info_get_type_name (PyGIBaseInfo *self)
{
    return _get_info_string (
        gi_object_info_get_type_name (GI_OBJECT_INFO (self->info)));
}

static PyObject *
_wrap_gi_object_info_get_type_init (PyGIBaseInfo *self)
{
    return _get_info_string (gi_object_info_get_type_init_function_name (
        GI_OBJECT_INFO (self->info)));
}

static PyObject *
_wrap_gi_object_info_get_fundamental (PyGIBaseInfo *self)
{
    return pygi_gboolean_to_py (
        gi_object_info_get_fundamental ((GIObjectInfo *)self->info));
}

static PyObject *
_wrap_gi_object_info_get_class_struct (PyGIBaseInfo *self)
{
    return _get_child_info (
        self, (GetChildInfoCallback)gi_object_info_get_class_struct);
}

static PyObject *
_wrap_gi_object_info_find_vfunc (PyGIBaseInfo *self, PyObject *py_name)
{
    return _get_child_info_by_name (
        self, py_name, (GetChildInfoByNameCallback)gi_object_info_find_vfunc);
}

static PyObject *
_wrap_gi_object_info_get_unref_function (PyGIBaseInfo *self)
{
    return _get_info_string (
        gi_object_info_get_unref_function_name (GI_OBJECT_INFO (self->info)));
}

static PyObject *
_wrap_gi_object_info_get_ref_function (PyGIBaseInfo *self)
{
    return _get_info_string (
        gi_object_info_get_ref_function_name (GI_OBJECT_INFO (self->info)));
}

static PyObject *
_wrap_gi_object_info_get_set_value_function (PyGIBaseInfo *self)
{
    return _get_info_string (gi_object_info_get_set_value_function_name (
        GI_OBJECT_INFO (self->info)));
}

static PyObject *
_wrap_gi_object_info_get_get_value_function (PyGIBaseInfo *self)
{
    return _get_info_string (gi_object_info_get_get_value_function_name (
        GI_OBJECT_INFO (self->info)));
}

static PyMethodDef _PyGIObjectInfo_methods[] = {
    { "get_parent", (PyCFunction)_wrap_gi_object_info_get_parent,
      METH_NOARGS },
    { "get_methods", (PyCFunction)_wrap_gi_object_info_get_methods,
      METH_NOARGS },
    { "find_method", (PyCFunction)_wrap_gi_object_info_find_method, METH_O },
    { "get_fields", (PyCFunction)_wrap_gi_object_info_get_fields,
      METH_NOARGS },
    { "get_properties", (PyCFunction)_wrap_gi_object_info_get_properties,
      METH_NOARGS },
    { "get_signals", (PyCFunction)_wrap_gi_object_info_get_signals,
      METH_NOARGS },
    { "get_interfaces", (PyCFunction)_wrap_gi_object_info_get_interfaces,
      METH_NOARGS },
    { "get_constants", (PyCFunction)_wrap_gi_object_info_get_constants,
      METH_NOARGS },
    { "get_vfuncs", (PyCFunction)_wrap_gi_object_info_get_vfuncs,
      METH_NOARGS },
    { "find_vfunc", (PyCFunction)_wrap_gi_object_info_find_vfunc, METH_O },
    { "get_abstract", (PyCFunction)_wrap_gi_object_info_get_abstract,
      METH_NOARGS },
    { "get_type_name", (PyCFunction)_wrap_gi_object_info_get_type_name,
      METH_NOARGS },
    { "get_type_init", (PyCFunction)_wrap_gi_object_info_get_type_init,
      METH_NOARGS },
    { "get_fundamental", (PyCFunction)_wrap_gi_object_info_get_fundamental,
      METH_NOARGS },
    { "get_class_struct", (PyCFunction)_wrap_gi_object_info_get_class_struct,
      METH_NOARGS },
    { "get_unref_function",
      (PyCFunction)_wrap_gi_object_info_get_unref_function, METH_NOARGS },
    { "get_ref_function", (PyCFunction)_wrap_gi_object_info_get_ref_function,
      METH_NOARGS },
    { "get_set_value_function",
      (PyCFunction)_wrap_gi_object_info_get_set_value_function, METH_NOARGS },
    { "get_get_value_function",
      (PyCFunction)_wrap_gi_object_info_get_get_value_function, METH_NOARGS },
    { NULL, NULL, 0 },
};


/* GIInterfaceInfo */
PYGI_DEFINE_TYPE ("InterfaceInfo", PyGIInterfaceInfo_Type, PyGIBaseInfo);

static PyObject *
_wrap_gi_interface_info_get_methods (PyGIBaseInfo *self)
{
    return _make_infos_tuple (
        self, (GetNInfosCallback)gi_interface_info_get_n_methods,
        (MakeInfosCallback)gi_interface_info_get_method);
}

static PyObject *
_wrap_gi_interface_info_find_method (PyGIBaseInfo *self, PyObject *py_name)
{
    return _get_child_info_by_name (
        self, py_name,
        (GetChildInfoByNameCallback)gi_interface_info_find_method);
}

static PyObject *
_wrap_gi_interface_info_get_constants (PyGIBaseInfo *self)
{
    return _make_infos_tuple (
        self, (GetNInfosCallback)gi_interface_info_get_n_constants,
        (MakeInfosCallback)gi_interface_info_get_constant);
}

static PyObject *
_wrap_gi_interface_info_get_vfuncs (PyGIBaseInfo *self)
{
    return _make_infos_tuple (
        self, (GetNInfosCallback)gi_interface_info_get_n_vfuncs,
        (MakeInfosCallback)gi_interface_info_get_vfunc);
}

static PyObject *
_wrap_gi_interface_info_find_vfunc (PyGIBaseInfo *self, PyObject *py_name)
{
    return _get_child_info_by_name (
        self, py_name,
        (GetChildInfoByNameCallback)gi_interface_info_find_vfunc);
}

static PyObject *
_wrap_gi_interface_info_get_prerequisites (PyGIBaseInfo *self)
{
    return _make_infos_tuple (
        self, (GetNInfosCallback)gi_interface_info_get_n_prerequisites,
        (MakeInfosCallback)gi_interface_info_get_prerequisite);
}

static PyObject *
_wrap_gi_interface_info_get_properties (PyGIBaseInfo *self)
{
    return _make_infos_tuple (
        self, (GetNInfosCallback)gi_interface_info_get_n_properties,
        (MakeInfosCallback)gi_interface_info_get_property);
}

static PyObject *
_wrap_gi_interface_info_get_iface_struct (PyGIBaseInfo *self)
{
    return _get_child_info (
        self, (GetChildInfoCallback)gi_interface_info_get_iface_struct);
}

static PyObject *
_wrap_gi_interface_info_get_signals (PyGIBaseInfo *self)
{
    return _make_infos_tuple (
        self, (GetNInfosCallback)gi_interface_info_get_n_signals,
        (MakeInfosCallback)gi_interface_info_get_signal);
}

static PyObject *
_wrap_gi_interface_info_find_signal (PyGIBaseInfo *self, PyObject *py_name)
{
    return _get_child_info_by_name (
        self, py_name,
        (GetChildInfoByNameCallback)gi_interface_info_find_signal);
}

static PyMethodDef _PyGIInterfaceInfo_methods[] = {
    { "get_prerequisites",
      (PyCFunction)_wrap_gi_interface_info_get_prerequisites, METH_NOARGS },
    { "get_properties", (PyCFunction)_wrap_gi_interface_info_get_properties,
      METH_NOARGS },
    { "get_methods", (PyCFunction)_wrap_gi_interface_info_get_methods,
      METH_NOARGS },
    { "find_method", (PyCFunction)_wrap_gi_interface_info_find_method,
      METH_O },
    { "get_signals", (PyCFunction)_wrap_gi_interface_info_get_signals,
      METH_NOARGS },
    { "find_signal", (PyCFunction)_wrap_gi_interface_info_find_signal,
      METH_O },
    { "get_vfuncs", (PyCFunction)_wrap_gi_interface_info_get_vfuncs,
      METH_NOARGS },
    { "get_constants", (PyCFunction)_wrap_gi_interface_info_get_constants,
      METH_NOARGS },
    { "get_iface_struct",
      (PyCFunction)_wrap_gi_interface_info_get_iface_struct, METH_NOARGS },
    { "find_vfunc", (PyCFunction)_wrap_gi_interface_info_find_vfunc, METH_O },
    { NULL, NULL, 0 },
};

/* GIConstantInfo */
PYGI_DEFINE_TYPE ("gi.ConstantInfo", PyGIConstantInfo_Type, PyGIBaseInfo);

static PyObject *
_wrap_gi_constant_info_get_value (PyGIBaseInfo *self)
{
    GITypeInfo *type_info;
    GIArgument value = { 0 };
    PyObject *py_value;
    gboolean free_array = FALSE;

    gi_constant_info_get_value ((GIConstantInfo *)self->info, &value);

    type_info = gi_constant_info_get_type_info ((GIConstantInfo *)self->info);

    if (gi_type_info_get_tag (type_info) == GI_TYPE_TAG_ARRAY) {
        value.v_pointer = _pygi_argument_to_array (&value, NULL, NULL, NULL,
                                                   type_info, &free_array);
    }

    py_value =
        _pygi_argument_to_object (&value, type_info, GI_TRANSFER_NOTHING);

    if (free_array) {
        g_array_free (value.v_pointer, FALSE);
    }

    gi_constant_info_free_value (GI_CONSTANT_INFO (self->info), &value);
    gi_base_info_unref ((GIBaseInfo *)type_info);

    return py_value;
}

static PyMethodDef _PyGIConstantInfo_methods[] = {
    { "get_value", (PyCFunction)_wrap_gi_constant_info_get_value,
      METH_NOARGS },
    { NULL, NULL, 0 },
};

/* GIValueInfo */
PYGI_DEFINE_TYPE ("gi.ValueInfo", PyGIValueInfo_Type, PyGIBaseInfo);

static PyObject *
_wrap_gi_value_info_get_value (PyGIBaseInfo *self)
{
    gint64 value;

    value = gi_value_info_get_value ((GIValueInfo *)self->info);

    return pygi_gint64_to_py (value);
}


static PyMethodDef _PyGIValueInfo_methods[] = {
    { "get_value", (PyCFunction)_wrap_gi_value_info_get_value, METH_NOARGS },
    { NULL, NULL, 0 },
};


/* GIFieldInfo */
PYGI_DEFINE_TYPE ("gi.FieldInfo", PyGIFieldInfo_Type, PyGIBaseInfo);

static gssize
_struct_field_array_length_marshal (gsize length_index, void *container_ptr,
                                    void *struct_data_ptr)
{
    gssize array_len = -1;
    GIFieldInfo *array_len_field = NULL;
    GIArgument arg = { 0 };
    GIBaseInfo *container_info = (GIBaseInfo *)container_ptr;

    if (GI_IS_UNION_INFO (container_info)) {
        array_len_field = gi_union_info_get_field (
            (GIUnionInfo *)container_info, (gint)length_index);
    } else if (GI_IS_STRUCT_INFO (container_info)) {
        array_len_field = gi_struct_info_get_field (
            (GIStructInfo *)container_info, (gint)length_index);
    } else if (GI_IS_OBJECT_INFO (container_info)) {
        array_len_field = gi_object_info_get_field (
            (GIObjectInfo *)container_info, (gint)length_index);
    } else {
        /* Other types don't have fields. */
        g_assert_not_reached ();
    }

    if (array_len_field == NULL) {
        return -1;
    }

    if (gi_field_info_get_field (array_len_field, struct_data_ptr, &arg)) {
        GITypeInfo *array_len_type_info;

        array_len_type_info = gi_field_info_get_type_info (array_len_field);
        if (array_len_type_info == NULL) {
            goto out;
        }

        if (!pygi_argument_to_gssize (
                &arg, gi_type_info_get_tag (array_len_type_info),
                &array_len)) {
            array_len = -1;
        }

        gi_base_info_unref (array_len_type_info);
    }

out:
    gi_base_info_unref (array_len_field);
    return array_len;
}

static gint
_pygi_gi_registered_type_info_check_object (GIRegisteredTypeInfo *info,
                                            PyObject *object)
{
    gint retval;

    GType g_type;
    PyObject *py_type;
    gchar *type_name_expected = NULL;

    if ((GI_IS_STRUCT_INFO (info))
        && (gi_struct_info_is_foreign ((GIStructInfo *)info))) {
        /* TODO: Could we check is the correct foreign type? */
        return 1;
    }

    g_type = gi_registered_type_info_get_g_type (info);
    if (g_type != G_TYPE_NONE) {
        py_type = pygi_type_get_from_g_type (g_type);
    } else {
        py_type = pygi_type_import_by_gi_info ((GIBaseInfo *)info);
    }

    if (py_type == NULL) {
        return 0;
    }

    g_assert (PyType_Check (py_type));

    retval = PyObject_IsInstance (object, py_type);
    if (!retval) {
        type_name_expected =
            _pygi_gi_base_info_get_fullname ((GIBaseInfo *)info);
    }

    Py_DECREF (py_type);

    if (!retval) {
        PyTypeObject *object_type;

        if (type_name_expected == NULL) {
            return -1;
        }

        object_type = (PyTypeObject *)PyObject_Type (object);
        if (object_type == NULL) {
            g_free (type_name_expected);
            return -1;
        }

        PyErr_Format (PyExc_TypeError, "Must be %s, not %s",
                      type_name_expected, object_type->tp_name);

        g_free (type_name_expected);
    }

    return retval;
}

static PyObject *
_wrap_gi_field_info_get_value (PyGIBaseInfo *self, PyObject *args)
{
    PyObject *instance;
    GIBaseInfo *container_info;
    gpointer pointer;
    GITypeInfo *field_type_info;
    GIArgument value;
    PyObject *py_value = NULL;
    gboolean free_array = FALSE;

    memset (&value, 0, sizeof (GIArgument));

    if (!PyArg_ParseTuple (args, "O:FieldInfo.get_value", &instance)) {
        return NULL;
    }

    container_info = gi_base_info_get_container (self->info);
    g_assert (container_info != NULL);

    /* Check the instance. */
    if (!_pygi_gi_registered_type_info_check_object (
            (GIRegisteredTypeInfo *)container_info, instance)) {
        _PyGI_ERROR_PREFIX ("argument 1: ");
        return NULL;
    }

    /* Get the pointer to the container. */
    if (GI_IS_UNION_INFO (container_info)
        || GI_IS_STRUCT_INFO (container_info)) {
        pointer = pyg_boxed_get (instance, void);
    } else if (GI_IS_OBJECT_INFO (container_info)) {
        if (pygi_check_fundamental (container_info))
            pointer = pygi_fundamental_get (instance);
        else
            pointer = pygobject_get (instance);
    } else {
        /* Other types don't have fields. */
        g_assert_not_reached ();
    }

    if (pointer == NULL) {
        PyErr_Format (PyExc_RuntimeError,
                      "object at %p of type %s is not initialized", instance,
                      Py_TYPE (instance)->tp_name);
        return NULL;
    }

    /* Get the field's value. */
    field_type_info = gi_field_info_get_type_info ((GIFieldInfo *)self->info);

    /* A few types are not handled by gi_field_info_get_field, so do it here. */
    if (!gi_type_info_is_pointer (field_type_info)
        && gi_type_info_get_tag (field_type_info) == GI_TYPE_TAG_INTERFACE) {
        GIBaseInfo *info;

        if (!(gi_field_info_get_flags ((GIFieldInfo *)self->info)
              & GI_FIELD_IS_READABLE)) {
            PyErr_SetString (PyExc_RuntimeError, "field is not readable");
            goto out;
        }

        info = gi_type_info_get_interface (field_type_info);

        if (GI_IS_UNION_INFO (info)) {
            PyErr_SetString (PyExc_NotImplementedError,
                             "getting an union is not supported yet");
            gi_base_info_unref (info);

            goto out;
        } else if (GI_IS_STRUCT_INFO (info)) {
            gsize offset;

            offset = gi_field_info_get_offset ((GIFieldInfo *)self->info);

            value.v_pointer = (char *)pointer + offset;

            gi_base_info_unref (info);

            goto argument_to_object;
        } else {
            gi_base_info_unref (info);

            /* Fallback. */
        }
    }

    if (!gi_field_info_get_field ((GIFieldInfo *)self->info, pointer,
                                  &value)) {
        PyErr_SetString (PyExc_RuntimeError, "unable to get the value");
        goto out;
    }

    if (gi_type_info_get_tag (field_type_info) == GI_TYPE_TAG_ARRAY) {
        value.v_pointer = _pygi_argument_to_array (
            &value, _struct_field_array_length_marshal, container_info,
            pointer, field_type_info, &free_array);
    }

argument_to_object:
    py_value = _pygi_argument_to_object (&value, field_type_info,
                                         GI_TRANSFER_NOTHING);

    if (free_array) {
        g_array_free (value.v_pointer, FALSE);
    }

out:
    gi_base_info_unref ((GIBaseInfo *)field_type_info);

    return py_value;
}

static PyObject *
_wrap_gi_field_info_set_value (PyGIBaseInfo *self, PyObject *args)
{
    PyObject *instance;
    PyObject *py_value;
    GIBaseInfo *container_info;
    gpointer pointer;
    GITypeInfo *field_type_info;
    GIArgument value;
    PyObject *retval = NULL;

    if (!PyArg_ParseTuple (args, "OO:FieldInfo.set_value", &instance,
                           &py_value)) {
        return NULL;
    }

    container_info = gi_base_info_get_container (self->info);
    g_assert (container_info != NULL);

    /* Check the instance. */
    if (!_pygi_gi_registered_type_info_check_object (
            (GIRegisteredTypeInfo *)container_info, instance)) {
        _PyGI_ERROR_PREFIX ("argument 1: ");
        return NULL;
    }

    /* Get the pointer to the container. */
    if (GI_IS_UNION_INFO (container_info)
        || GI_IS_STRUCT_INFO (container_info)) {
        pointer = pyg_boxed_get (instance, void);
    } else if (GI_IS_OBJECT_INFO (container_info)) {
        if (pygi_check_fundamental (container_info))
            pointer = pygi_fundamental_get (instance);
        else
            pointer = pygobject_get (instance);
    } else {
        /* Other types don't have fields. */
        g_assert_not_reached ();
    }

    if (pointer == NULL) {
        PyErr_Format (PyExc_RuntimeError,
                      "object at %p of type %s is not initialized", instance,
                      Py_TYPE (instance)->tp_name);
        return NULL;
    }

    field_type_info = gi_field_info_get_type_info ((GIFieldInfo *)self->info);

    /* Set the field's value. */
    /* A few types are not handled by gi_field_info_set_field, so do it here. */
    if (!gi_type_info_is_pointer (field_type_info)
        && gi_type_info_get_tag (field_type_info) == GI_TYPE_TAG_INTERFACE) {
        GIBaseInfo *info;

        if (!(gi_field_info_get_flags ((GIFieldInfo *)self->info)
              & GI_FIELD_IS_WRITABLE)) {
            PyErr_SetString (PyExc_RuntimeError, "field is not writable");
            goto out;
        }

        info = gi_type_info_get_interface (field_type_info);

        if (GI_IS_UNION_INFO (info)) {
            PyErr_SetString (PyExc_NotImplementedError,
                             "setting an union is not supported yet");
            goto out;
        } else if (GI_IS_STRUCT_INFO (info)) {
            gboolean is_simple;
            gsize offset;
            gssize size;

            is_simple = pygi_gi_struct_info_is_simple ((GIStructInfo *)info);

            if (!is_simple) {
                PyErr_SetString (PyExc_TypeError,
                                 "cannot set a structure which has no "
                                 "well-defined ownership transfer rules");
                gi_base_info_unref (info);
                goto out;
            }

            value = _pygi_argument_from_object (py_value, field_type_info,
                                                GI_TRANSFER_NOTHING);
            if (PyErr_Occurred ()) {
                gi_base_info_unref (info);
                goto out;
            }

            offset = gi_field_info_get_offset ((GIFieldInfo *)self->info);
            size = gi_struct_info_get_size ((GIStructInfo *)info);
            g_assert (size > 0);

            memmove ((char *)pointer + offset, value.v_pointer, size);

            gi_base_info_unref (info);

            retval = Py_None;
            goto out;
        } else {
            /* Fallback. */
        }

        gi_base_info_unref (info);
    } else if (gi_type_info_is_pointer (field_type_info)
               && (gi_type_info_get_tag (field_type_info) == GI_TYPE_TAG_VOID
                   || gi_type_info_get_tag (field_type_info)
                          == GI_TYPE_TAG_UTF8)) {
        int offset;
        value = _pygi_argument_from_object (py_value, field_type_info,
                                            GI_TRANSFER_NOTHING);
        if (PyErr_Occurred ()) {
            goto out;
        }

        offset = gi_field_info_get_offset ((GIFieldInfo *)self->info);
        G_STRUCT_MEMBER (gpointer, pointer, offset) =
            (gpointer)value.v_pointer;

        retval = Py_None;
        goto out;
    }

    value = _pygi_argument_from_object (py_value, field_type_info,
                                        GI_TRANSFER_EVERYTHING);
    if (PyErr_Occurred ()) {
        goto out;
    }

    if (!gi_field_info_set_field ((GIFieldInfo *)self->info, pointer,
                                  &value)) {
        _pygi_argument_release (&value, field_type_info, GI_TRANSFER_NOTHING,
                                GI_DIRECTION_IN);
        PyErr_SetString (PyExc_RuntimeError, "unable to set value for field");
        goto out;
    }

    retval = Py_None;

out:
    gi_base_info_unref ((GIBaseInfo *)field_type_info);

    Py_XINCREF (retval);
    return retval;
}

static PyObject *
_wrap_gi_field_info_get_flags (PyGIBaseInfo *self)
{
    return pygi_guint_to_py (
        gi_field_info_get_flags (GI_FIELD_INFO (self->info)));
}

static PyObject *
_wrap_gi_field_info_get_size (PyGIBaseInfo *self)
{
    return pygi_gint_to_py (
        gi_field_info_get_size (GI_FIELD_INFO (self->info)));
}

static PyObject *
_wrap_gi_field_info_get_offset (PyGIBaseInfo *self)
{
    return pygi_gint_to_py (
        gi_field_info_get_offset (GI_FIELD_INFO (self->info)));
}

static PyObject *
_wrap_gi_field_info_get_type_info (PyGIBaseInfo *self)
{
    return _get_child_info (self,
                            (GetChildInfoCallback)gi_field_info_get_type_info);
}

static PyMethodDef _PyGIFieldInfo_methods[] = {
    { "get_value", (PyCFunction)_wrap_gi_field_info_get_value, METH_VARARGS },
    { "set_value", (PyCFunction)_wrap_gi_field_info_set_value, METH_VARARGS },
    { "get_flags", (PyCFunction)_wrap_gi_field_info_get_flags, METH_VARARGS },
    { "get_size", (PyCFunction)_wrap_gi_field_info_get_size, METH_VARARGS },
    { "get_offset", (PyCFunction)_wrap_gi_field_info_get_offset,
      METH_VARARGS },
    { "get_type_info", (PyCFunction)_wrap_gi_field_info_get_type_info,
      METH_VARARGS },
    { NULL, NULL, 0 },
};


/* GIUnresolvedInfo */
PYGI_DEFINE_TYPE ("gi.UnresolvedInfo", PyGIUnresolvedInfo_Type, PyGIBaseInfo);

static PyMethodDef _PyGIUnresolvedInfo_methods[] = {
    { NULL, NULL, 0 },
};

/* GIVFuncInfo */
PYGI_DEFINE_TYPE ("gi.VFuncInfo", PyGIVFuncInfo_Type, PyGICallableInfo);

static PyObject *
_wrap_gi_vfunc_info_get_flags (PyGIBaseInfo *self)
{
    return pygi_guint_to_py (
        gi_vfunc_info_get_flags ((GIVFuncInfo *)self->info));
}

static PyObject *
_wrap_gi_vfunc_info_get_offset (PyGIBaseInfo *self)
{
    return pygi_gint_to_py (
        gi_vfunc_info_get_offset ((GIVFuncInfo *)self->info));
}

static PyObject *
_wrap_gi_vfunc_info_get_signal (PyGIBaseInfo *self)
{
    return _get_child_info (self,
                            (GetChildInfoCallback)gi_vfunc_info_get_signal);
}

static PyObject *
_wrap_gi_vfunc_info_get_invoker (PyGIBaseInfo *self)
{
    return _get_child_info (self,
                            (GetChildInfoCallback)gi_vfunc_info_get_invoker);
}

static PyMethodDef _PyGIVFuncInfo_methods[] = {
    { "get_flags", (PyCFunction)_wrap_gi_vfunc_info_get_flags, METH_NOARGS },
    { "get_offset", (PyCFunction)_wrap_gi_vfunc_info_get_offset, METH_NOARGS },
    { "get_signal", (PyCFunction)_wrap_gi_vfunc_info_get_signal, METH_NOARGS },
    { "get_invoker", (PyCFunction)_wrap_gi_vfunc_info_get_invoker,
      METH_NOARGS },
    { NULL, NULL, 0 },
};


/* GIUnionInfo */
PYGI_DEFINE_TYPE ("gi.UnionInfo", PyGIUnionInfo_Type, PyGIBaseInfo);

static PyObject *
_wrap_gi_union_info_get_fields (PyGIBaseInfo *self)
{
    return _make_infos_tuple (self,
                              (GetNInfosCallback)gi_union_info_get_n_fields,
                              (MakeInfosCallback)gi_union_info_get_field);
}

static PyObject *
_wrap_gi_union_info_get_methods (PyGIBaseInfo *self)
{
    return _make_infos_tuple (self,
                              (GetNInfosCallback)gi_union_info_get_n_methods,
                              (MakeInfosCallback)gi_union_info_get_method);
}

static PyObject *
_wrap_gi_union_info_get_size (PyGIBaseInfo *self)
{
    return pygi_gsize_to_py (
        gi_union_info_get_size (GI_UNION_INFO (self->info)));
}

static PyObject *
_wrap_gi_union_info_get_alignment (PyGIBaseInfo *self)
{
    return pygi_gsize_to_py (
        gi_union_info_get_alignment (GI_UNION_INFO (self->info)));
}

static PyMethodDef _PyGIUnionInfo_methods[] = {
    { "get_fields", (PyCFunction)_wrap_gi_union_info_get_fields, METH_NOARGS },
    { "get_methods", (PyCFunction)_wrap_gi_union_info_get_methods,
      METH_NOARGS },
    { "get_size", (PyCFunction)_wrap_gi_union_info_get_size, METH_NOARGS },
    { "get_alignment", (PyCFunction)_wrap_gi_union_info_get_alignment,
      METH_NOARGS },
    { NULL, NULL, 0 },
};

/* Private */

gchar *
_pygi_gi_base_info_get_fullname (GIBaseInfo *info)
{
    GIBaseInfo *container_info;
    gchar *fullname;

    container_info = gi_base_info_get_container (info);
    if (container_info != NULL) {
        fullname = g_strdup_printf (
            "%s.%s.%s", gi_base_info_get_namespace (container_info),
            _safe_base_info_get_name (container_info),
            _safe_base_info_get_name (info));
    } else {
        fullname = g_strdup_printf ("%s.%s", gi_base_info_get_namespace (info),
                                    _safe_base_info_get_name (info));
    }

    if (fullname == NULL) {
        PyErr_NoMemory ();
    }

    return fullname;
}


/**
 * Returns 0 on success, or -1 and sets an exception.
 */
int
pygi_info_register_types (PyObject *m)
{
#define _PyGI_REGISTER_TYPE(m, type, cname, base)                             \
    Py_SET_TYPE (&type, &PyType_Type);                                        \
    type.tp_flags |= (Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE);              \
    type.tp_weaklistoffset = offsetof (PyGIBaseInfo, inst_weakreflist);       \
    type.tp_methods = _PyGI##cname##_methods;                                 \
    type.tp_base = &base;                                                     \
    if (PyType_Ready (&type) < 0) return -1;                                  \
    Py_INCREF ((PyObject *)&type);                                            \
    if (PyModule_AddObject (m, #cname, (PyObject *)&type) < 0) {              \
        Py_DECREF ((PyObject *)&type);                                        \
        return -1;                                                            \
    };

    Py_SET_TYPE (&PyGIBaseInfo_Type, &PyType_Type);

    PyGIBaseInfo_Type.tp_dealloc = (destructor)_base_info_dealloc;
    PyGIBaseInfo_Type.tp_repr = (reprfunc)_base_info_repr;
    PyGIBaseInfo_Type.tp_flags = (Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE);
    PyGIBaseInfo_Type.tp_weaklistoffset =
        offsetof (PyGIBaseInfo, inst_weakreflist);
    PyGIBaseInfo_Type.tp_methods = _PyGIBaseInfo_methods;
    PyGIBaseInfo_Type.tp_richcompare = (richcmpfunc)_base_info_richcompare;
    PyGIBaseInfo_Type.tp_getset = _base_info_getsets;
    PyGIBaseInfo_Type.tp_getattro = (getattrofunc)_base_info_getattro;

    if (PyType_Ready (&PyGIBaseInfo_Type) < 0) return -1;
    Py_INCREF ((PyObject *)&PyGIBaseInfo_Type);
    if (PyModule_AddObject (m, "BaseInfo", (PyObject *)&PyGIBaseInfo_Type)
        < 0) {
        Py_DECREF ((PyObject *)&PyGIBaseInfo_Type);
        return -1;
    }

    PyGICallableInfo_Type.tp_dealloc = (destructor)_callable_info_dealloc;
    PyGICallableInfo_Type.tp_flags |= Py_TPFLAGS_HAVE_VECTORCALL;
    PyGICallableInfo_Type.tp_vectorcall_offset =
        offsetof (PyGICallableInfo, vectorcall);
    PyGICallableInfo_Type.tp_call = PyVectorcall_Call;
    PyGICallableInfo_Type.tp_repr = (reprfunc)_callable_info_repr;
    PyGICallableInfo_Type.tp_getset = _PyGICallableInfo_getsets;
    _PyGI_REGISTER_TYPE (m, PyGICallableInfo_Type, CallableInfo,
                         PyGIBaseInfo_Type);

    PyGIFunctionInfo_Type.tp_flags |= Py_TPFLAGS_METHOD_DESCRIPTOR;
    PyGIFunctionInfo_Type.tp_descr_get =
        (descrgetfunc)_function_info_descr_get;
    _PyGI_REGISTER_TYPE (m, PyGIFunctionInfo_Type, FunctionInfo,
                         PyGICallableInfo_Type);

    PyGIVFuncInfo_Type.tp_descr_get = (descrgetfunc)_vfunc_info_descr_get;
    _PyGI_REGISTER_TYPE (m, PyGIVFuncInfo_Type, VFuncInfo,
                         PyGICallableInfo_Type);

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
    _PyGI_REGISTER_TYPE (m, PyGIValueInfo_Type, ValueInfo, PyGIBaseInfo_Type);
    _PyGI_REGISTER_TYPE (m, PyGIFieldInfo_Type, FieldInfo, PyGIBaseInfo_Type);
    _PyGI_REGISTER_TYPE (m, PyGIUnionInfo_Type, UnionInfo,
                         PyGIRegisteredTypeInfo_Type);
    _PyGI_REGISTER_TYPE (m, PyGIErrorDomainInfo_Type, ErrorDomainInfo,
                         PyGIBaseInfo_Type);
    _PyGI_REGISTER_TYPE (m, PyGIPropertyInfo_Type, PropertyInfo,
                         PyGIBaseInfo_Type);
    _PyGI_REGISTER_TYPE (m, PyGIArgInfo_Type, ArgInfo, PyGIBaseInfo_Type);
    _PyGI_REGISTER_TYPE (m, PyGITypeInfo_Type, TypeInfo, PyGIBaseInfo_Type);

#undef _PyGI_REGISTER_TYPE

#define _PyGI_ENUM_BEGIN(name)                                                \
    {                                                                         \
        const char *__enum_name = #name;                                      \
        PyObject *__enum_value = NULL;                                        \
        PyObject *__new_enum_cls = NULL;                                      \
        PyObject *__enum_instance_dict = PyDict_New ();                       \
        PyObject *__module_name = PyObject_GetAttrString (m, "__name__");     \
        PyDict_SetItemString (__enum_instance_dict, "__module__",             \
                              __module_name);                                 \
        Py_DECREF (__module_name);

#define _PyGI_ENUM_ADD_VALUE(prefix, name)                                    \
    __enum_value = pygi_guint_to_py (prefix##_##name);                        \
    if (PyDict_SetItemString (__enum_instance_dict, #name, __enum_value)      \
        < 0) {                                                                \
        Py_DECREF (__enum_instance_dict);                                     \
        Py_DECREF (__enum_value);                                             \
        return -1;                                                            \
    }                                                                         \
    Py_DECREF (__enum_value);

#define _PyGI_ENUM_END                                                        \
    __new_enum_cls = PyObject_CallFunction (                                  \
        (PyObject *)&PyType_Type, "s(O)O", __enum_name,                       \
        (PyObject *)&PyType_Type, __enum_instance_dict);                      \
    Py_DECREF (__enum_instance_dict);                                         \
    PyModule_AddObject (m, __enum_name, __new_enum_cls); /* steals ref */     \
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

#undef _PyGI_ENUM_BEGIN
#undef _PyGI_ENUM_ADD_VALUE
#undef _PyGI_ENUM_END

    return 0;
}
