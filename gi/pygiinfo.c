/* -*- Mode: C; c-basic-offset: 4 -*-
 * vim: tabstop=4 shiftwidth=4 expandtab
 *
 * Copyright (C) 2005-2009 Johan Dahlin <johan@gnome.org>
 *
 *   pygiinfo.c: GI.*Info wrappers.
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

#include <pygobject.h>

#define PYGIINFO_DEFINE_TYPE(name, cname, base) \
static PyMethodDef _Py##cname##_methods[]; \
PyTypeObject Py##cname##_Type = { \
    PyObject_HEAD_INIT(NULL) \
    0, \
    "gi." name,                               /* tp_name */ \
    sizeof(PyGIBaseInfo),                     /* tp_basicsize */ \
    0,                                        /* tp_itemsize */ \
    (destructor)NULL,                         /* tp_dealloc */ \
    (printfunc)NULL,                          /* tp_print */ \
    (getattrfunc)NULL,                        /* tp_getattr */ \
    (setattrfunc)NULL,                        /* tp_setattr */ \
    (cmpfunc)NULL,                            /* tp_compare */ \
    (reprfunc)NULL,                           /* tp_repr */ \
    NULL,                                     /* tp_as_number */ \
    NULL,                                     /* tp_as_sequence */ \
    NULL,                                     /* tp_as_mapping */ \
    (hashfunc)NULL,                           /* tp_hash */ \
    (ternaryfunc)NULL,                        /* tp_call */ \
    (reprfunc)NULL,                           /* tp_str */ \
    (getattrofunc)NULL,                       /* tp_getattro */ \
    (setattrofunc)NULL,                       /* tp_setattro */ \
    NULL,                                     /* tp_as_buffer */ \
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE, /* tp_flags */ \
    NULL,                                     /* tp_doc */ \
    (traverseproc)NULL,                       /* tp_traverse */ \
    (inquiry)NULL,                            /* tp_clear */ \
    (richcmpfunc)NULL,                        /* tp_richcompare */ \
    offsetof(PyGIBaseInfo, weakreflist),      /* tp_weaklistoffset */ \
    (getiterfunc)NULL,                        /* tp_iter */ \
    (iternextfunc)NULL,                       /* tp_iternext */ \
    _Py##cname##_methods,                     /* tp_methods */ \
    NULL,                                     /* tp_members */ \
    NULL,                                     /* tp_getset */ \
    &base,                                    /* tp_base */ \
    NULL,                                     /* tp_dict */ \
    (descrgetfunc)NULL,                       /* tp_descr_get */ \
    (descrsetfunc)NULL,                       /* tp_descr_set */ \
    offsetof(PyGIBaseInfo, instance_dict),    /* tp_dictoffset */ \
}


/* BaseInfo */

static void
pygi_base_info_clear(PyGIBaseInfo *self)
{
    PyObject_GC_UnTrack((PyObject *)self);

    Py_CLEAR(self->instance_dict);

    if (self->info) {
        g_base_info_unref(self->info);
        self->info = NULL;
    }

    PyObject_GC_Del(self);
}

static void
pygi_base_info_dealloc(PyGIBaseInfo *self)
{
    PyObject_ClearWeakRefs((PyObject *)self);
    pygi_base_info_clear(self);
}

static int
pygi_base_info_traverse(PyGIBaseInfo *self, visitproc visit, void *data)
{
    if (self->instance_dict) {
        return visit(self->instance_dict, data);
    }
    return 0;
}

static void
pygi_base_info_free(PyObject *self)
{
    PyObject_GC_Del(self);
}

static PyObject *
pygi_base_info_repr(PyGIBaseInfo *self)
{
    gchar buf[256];

    g_snprintf(buf, sizeof(buf),
               "<%s object (%s) at 0x%lx>",
               self->ob_type->tp_name,
               g_base_info_get_name(self->info), (long)self);
    return PyString_FromString(buf);
}

static PyMethodDef _PyGIBaseInfo_methods[];
static PyGetSetDef _PyGIBaseInfo_getsets[];

PyTypeObject PyGIBaseInfo_Type = {
    PyObject_HEAD_INIT(NULL)
    0,
    "gi.BaseInfo",                             /* tp_name */
    sizeof(PyGIBaseInfo),                      /* tp_basicsize */
    0,                                         /* tp_itemsize */
    (destructor)pygi_base_info_dealloc,        /* tp_dealloc */
    (printfunc)NULL,                           /* tp_print */
    (getattrfunc)NULL,                         /* tp_getattr */
    (setattrfunc)NULL,                         /* tp_setattr */
    (cmpfunc)NULL,                             /* tp_compare */
    (reprfunc)pygi_base_info_repr,             /* tp_repr */
    NULL,                                      /* tp_as_number */
    NULL,                                      /* tp_as_sequence */
    NULL,                                      /* tp_as_mapping */
    (hashfunc)NULL,                            /* tp_hash */
    (ternaryfunc)NULL,                         /* tp_call */
    (reprfunc)NULL,                            /* tp_str */
    (getattrofunc)NULL,                        /* tp_getattro */
    (setattrofunc)NULL,                        /* tp_setattro */
    NULL,                                      /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE |
        Py_TPFLAGS_HAVE_GC,                    /* tp_flags */
    NULL,                                      /* tp_doc */
    (traverseproc)pygi_base_info_traverse,     /* tp_traverse */
    (inquiry)pygi_base_info_clear,             /* tp_clear */
    (richcmpfunc)NULL,                         /* tp_richcompare */
    offsetof(PyGIBaseInfo, weakreflist),       /* tp_weaklistoffset */
    (getiterfunc)NULL,                         /* tp_iter */
    (iternextfunc)NULL,                        /* tp_iternext */
    _PyGIBaseInfo_methods,                     /* tp_methods */
    NULL,                                      /* tp_members */
    _PyGIBaseInfo_getsets,                     /* tp_getset */
    NULL,                                      /* tp_base */
    NULL,                                      /* tp_dict */
    (descrgetfunc)NULL,                        /* tp_descr_get */
    (descrsetfunc)NULL,                        /* tp_descr_set */
    offsetof(PyGIBaseInfo, instance_dict),     /* tp_dictoffset */
    (initproc)NULL,                            /* tp_init */
    (allocfunc)NULL,                           /* tp_alloc */
    (newfunc)NULL,                             /* tp_new */
    (freefunc)pygi_base_info_free,             /* tp_free */
};

static PyObject *
pygi_base_info_get_dict(PyGIBaseInfo *self, void *closure)
{
    if (self->instance_dict == NULL) {
        self->instance_dict = PyDict_New();
        if (self->instance_dict == NULL) {
            return NULL;
        }
    }
    Py_INCREF(self->instance_dict);
    return self->instance_dict;
}

static PyGetSetDef _PyGIBaseInfo_getsets[] = {
    { "__dict__", (getter)pygi_base_info_get_dict, (setter)0 },
    { NULL, 0, 0 }
};


static PyObject *
_wrap_g_base_info_get_name(PyGIBaseInfo *self)
{
    return PyString_FromString(g_base_info_get_name(self->info));
}

static PyObject *
_wrap_g_base_info_get_namespace(PyGIBaseInfo *self)
{
    return PyString_FromString(g_base_info_get_namespace(self->info));
}

static PyObject *
_wrap_g_base_info_get_type(PyGIBaseInfo *self)
{
    return PyInt_FromLong(g_base_info_get_type(self->info));
}

static PyMethodDef _PyGIBaseInfo_methods[] = {
    { "getName", (PyCFunction)_wrap_g_base_info_get_name, METH_NOARGS },
    { "getType", (PyCFunction)_wrap_g_base_info_get_type, METH_NOARGS },
    { "getNamespace", (PyCFunction)_wrap_g_base_info_get_namespace, METH_NOARGS },
    { NULL, NULL, 0 }
};

PyObject *
pyg_info_new(void *info)
{
    PyGIBaseInfo *self;
    GIInfoType type_info;
    PyTypeObject *tp;

    if (info == NULL) {
        PyErr_SetString(PyExc_TypeError, "NULL value sent to pyg_info_new");
        return NULL;
    }

    type_info = g_base_info_get_type((GIBaseInfo*)info);

    switch (type_info)
        {
        case GI_INFO_TYPE_OBJECT:
            tp = &PyGIObjectInfo_Type;
            break;
        case GI_INFO_TYPE_BOXED:
            tp = &PyGIBoxedInfo_Type;
            break;
        case GI_INFO_TYPE_STRUCT:
            tp = &PyGIStructInfo_Type;
            break;
        case GI_INFO_TYPE_FUNCTION:
            tp = &PyGIFunctionInfo_Type;
            break;
        case GI_INFO_TYPE_ENUM:
        case GI_INFO_TYPE_FLAGS:
            tp = &PyGIEnumInfo_Type;
            break;
        case GI_INFO_TYPE_ARG:
            tp = &PyGIArgInfo_Type;
            break;
        case GI_INFO_TYPE_TYPE:
            tp = &PyGITypeInfo_Type;
            break;
        case GI_INFO_TYPE_INTERFACE:
            tp = &PyGIInterfaceInfo_Type;
            break;
        case GI_INFO_TYPE_UNRESOLVED:
            tp = &PyGIUnresolvedInfo_Type;
            break;
        case GI_INFO_TYPE_VALUE:
            tp = &PyGIValueInfo_Type;
            break;
        case GI_INFO_TYPE_FIELD:
            tp = &PyGIFieldInfo_Type;
            break;
        default:
            g_print ("Unhandled GIInfoType: %d\n", type_info);
            Py_INCREF(Py_None);
            return Py_None;
        }

    if (tp->tp_flags & Py_TPFLAGS_HEAPTYPE)
        Py_INCREF(tp);

    self = (PyGIBaseInfo*)PyObject_GC_New(PyGIBaseInfo, tp);
    if (self == NULL)
        return NULL;

    self->info = g_base_info_ref(info);

    self->instance_dict = NULL;
    self->weakreflist = NULL;

    PyObject_GC_Track((PyObject *)self);

    return (PyObject*)self;
}

GIBaseInfo *
pyg_base_info_from_object(PyObject *object)
{
    PyObject *py_info;
    GIBaseInfo *info;

    g_return_val_if_fail(object != NULL, NULL);

    py_info = PyObject_GetAttrString(object, "__info__");
    if (py_info == NULL) {
        PyErr_Clear();
        return NULL;
    }
    if (!PyObject_TypeCheck(py_info, (PyTypeObject *)&PyGIBaseInfo_Type)) {
        Py_DECREF(py_info);
        return NULL;
    }

    info = ((PyGIBaseInfo *)py_info)->info;
    g_base_info_ref(info);

    Py_DECREF(py_info);

    return info;
}

gchar *
pygi_gi_base_info_get_fullname(GIBaseInfo *info) {
    GIBaseInfo *container_info;
    gchar *fullname;

    container_info = g_base_info_get_container(info);
    if (container_info != NULL) {
        fullname = g_strdup_printf("%s.%s.%s",
                g_base_info_get_namespace(container_info),
                g_base_info_get_name(container_info),
                g_base_info_get_name(info));
    } else {
        fullname = g_strdup_printf("%s.%s",
                g_base_info_get_namespace(info),
                g_base_info_get_name(info));
    }

    if (fullname == NULL) {
        PyErr_SetString(PyExc_MemoryError, "Out of memory");
    }

    return fullname;
}

/* CallableInfo */
PYGIINFO_DEFINE_TYPE("CallableInfo", GICallableInfo, PyGIBaseInfo_Type);

static PyObject *
_wrap_g_callable_info_get_args(PyGIBaseInfo *self)
{
    int i, length;
    PyObject *retval;

    length = g_callable_info_get_n_args((GICallableInfo*)self->info);
    retval = PyTuple_New(length);

    for (i = 0; i < length; i++) {
        GIArgInfo *arg;
        arg = g_callable_info_get_arg((GICallableInfo*)self->info, i);
        PyTuple_SetItem(retval, i, pyg_info_new(arg));
        g_base_info_unref((GIBaseInfo*)arg);
    }

    return retval;
}

static PyObject *
_wrap_g_callable_info_get_return_type(PyGIBaseInfo *self)
{
    return pyg_info_new(g_callable_info_get_return_type((GICallableInfo*)self->info));
}

static PyMethodDef _PyGICallableInfo_methods[] = {
    { "getArgs", (PyCFunction)_wrap_g_callable_info_get_args, METH_NOARGS },
    { "getReturnType", (PyCFunction)_wrap_g_callable_info_get_return_type, METH_NOARGS },
    { NULL, NULL, 0 }
};

/* FunctionInfo */
PYGIINFO_DEFINE_TYPE("FunctionInfo", GIFunctionInfo, PyGICallableInfo_Type);

static PyObject *
_wrap_g_function_info_is_constructor(PyGIBaseInfo *self)
{
    return PyInt_FromLong(g_function_info_get_flags((GIFunctionInfo*)self->info) &
                          GI_FUNCTION_IS_CONSTRUCTOR);
}

static PyObject *
_wrap_g_function_info_is_method(PyGIBaseInfo *self)
{
    return PyInt_FromLong(g_function_info_get_flags((GIFunctionInfo*)self->info) &
                          GI_FUNCTION_IS_METHOD);
}

static PyObject *
_wrap_g_function_info_invoke(PyGIBaseInfo *self, PyObject *args)
{
    gboolean is_method;
    gboolean is_constructor;

    guint n_args;
    guint n_in_args;
    guint n_out_args;
    Py_ssize_t n_py_args;
    gsize n_aux_in_args;
    gsize n_aux_out_args;
    guint n_return_values;

    GICallableInfo *callable_info;
    GITypeInfo *return_info;
    GITypeTag return_tag;

    GArgument *in_args;
    GArgument *out_args;
    GArgument **aux_args;
    GArgument *out_values;
    GArgument return_arg;
    PyObject *return_value;

    guint i;

    callable_info = (GICallableInfo *)self->info;

    {
        GIFunctionInfoFlags flags;

        flags = g_function_info_get_flags((GIFunctionInfo *)callable_info);
        is_method = (flags & GI_FUNCTION_IS_METHOD) != 0;
        is_constructor = (flags & GI_FUNCTION_IS_CONSTRUCTOR) != 0;
    }

    /* Count arguments. */
    n_args = g_callable_info_get_n_args(callable_info);
    n_py_args = PyTuple_Size(args);
    n_in_args = is_method ? 1 : 0;  /* The first argument is the instance. */
    n_out_args = 0;
    n_aux_in_args = 0;
    n_aux_out_args = 0;
    aux_args = g_newa(GArgument *, n_args);

    for (i = 0; i < n_args; i++) {
        GIDirection direction;
        GIArgInfo *arg_info;
        GITypeInfo *arg_type_info;
        GITypeTag arg_type_tag;

        arg_info = g_callable_info_get_arg(callable_info, i);

        arg_type_info = g_arg_info_get_type(arg_info);
        direction = g_arg_info_get_direction(arg_info);

        arg_type_tag = g_type_info_get_tag(arg_type_info);

        if (direction == GI_DIRECTION_IN || direction == GI_DIRECTION_INOUT) {
            n_in_args += 1;
        }
        if (direction == GI_DIRECTION_OUT || direction == GI_DIRECTION_INOUT) {
            n_out_args += 1;
        }

        aux_args[i] = NULL;
        if (arg_type_tag == GI_TYPE_TAG_ARRAY) {
            gint length_arg_pos;

            length_arg_pos = g_type_info_get_array_length(arg_type_info);
            if (length_arg_pos != -1) {
                /* Tag the argument as auxiliary. Later, it'll be changed into a pointer to (in|out)_args[(in|out)_args_pos].
                 * We cannot do it now since we don't know how much space to allocate (n_(in|out)_args) yet.
                 */
                aux_args[length_arg_pos] = (GArgument *)!NULL;

                if (direction == GI_DIRECTION_IN || direction == GI_DIRECTION_INOUT) {
                    n_aux_in_args += 1;
                }
                if (direction == GI_DIRECTION_OUT || direction == GI_DIRECTION_INOUT) {
                    n_aux_out_args += 1;
                }
            }
        }

        g_base_info_unref((GIBaseInfo *)arg_type_info);
        g_base_info_unref((GIBaseInfo *)arg_info);
    }

    return_info = g_callable_info_get_return_type((GICallableInfo *)self->info);
    return_tag = g_type_info_get_tag(return_info);

    /* Tag the return value's auxiliary argument too, if applicable. */
    if (return_tag == GI_TYPE_TAG_ARRAY) {
        gint length_arg_pos;
        length_arg_pos = g_type_info_get_array_length(return_info);
        if (length_arg_pos != -1) {
            aux_args[length_arg_pos] = (GArgument *)!NULL;
            n_aux_out_args += 1;
        }
    }

    {
        Py_ssize_t py_args_pos;

        /* Check the argument count. */
        if (n_py_args != n_in_args + (is_constructor ? 1 : 0) - n_aux_in_args) {
            g_base_info_unref((GIBaseInfo *)return_info);
            PyErr_Format(PyExc_TypeError, "%s() takes exactly %zd argument(s) (%zd given)",
                    pygi_gi_base_info_get_fullname(self->info),
                    n_in_args + (is_constructor ? 1 : 0) - n_aux_in_args, n_py_args);
            return NULL;
        }

        /* Check argument types. */
        py_args_pos = 0;
        if (is_constructor) {
            PyObject *py_arg;

            g_assert(n_py_args > 0);
            py_arg = PyTuple_GetItem(args, py_args_pos);
            g_assert(py_arg != NULL);

            if (!PyType_Check(py_arg)) {
                PyErr_Format(PyExc_TypeError, "%s() argument %zd: Must be type, not %s",
                    pygi_gi_base_info_get_fullname(self->info), py_args_pos,
                    ((PyTypeObject *)py_arg)->tp_name);
            } else if (!PyType_IsSubtype((PyTypeObject *)py_arg, &PyGObject_Type)) {
                PyErr_Format(PyExc_TypeError, "%s() argument %zd: Must be a non-strict subclass of %s",
                    pygi_gi_base_info_get_fullname(self->info), py_args_pos,
                    PyGObject_Type.tp_name);
            } else {
                GIBaseInfo *interface_info;
                GType interface_g_type;
                GType arg_g_type;

                interface_info = g_type_info_get_interface(return_info);

                interface_g_type = g_registered_type_info_get_g_type((GIRegisteredTypeInfo *)interface_info);
                arg_g_type = pyg_type_from_object(py_arg);

                if (!g_type_is_a(arg_g_type, interface_g_type)) {
                    PyErr_Format(PyExc_TypeError, "%s() argument %zd: Must be a non-strict subclass of %s",
                        pygi_gi_base_info_get_fullname(self->info), py_args_pos,
                        g_type_name(interface_g_type));
                }

                g_base_info_unref(interface_info);
            }

            if (PyErr_Occurred()) {
                g_base_info_unref((GIBaseInfo *)return_info);
                return NULL;
            }

            py_args_pos += 1;
        } else if (is_method) {
            PyObject *py_arg;
            GIBaseInfo *container_info;
            GIInfoType container_info_type;

            g_assert(n_py_args > 0);
            py_arg = PyTuple_GetItem(args, py_args_pos);
            g_assert(py_arg != NULL);

            container_info = g_base_info_get_container(self->info);
            container_info_type = g_base_info_get_type(container_info);

            /* FIXME: this could take place in pygi_g_argument_from_py_object, but we need to create an
               independant function because it needs a GITypeInfo to be passed. */

            switch(container_info_type) {
                case GI_INFO_TYPE_OBJECT:
                {
                    PyObject *py_type;
                    GType container_g_type;
                    GType arg_g_type;

                    py_type = PyObject_Type(py_arg);
                    g_assert(py_type != NULL);

                    container_g_type = g_registered_type_info_get_g_type((GIRegisteredTypeInfo *)container_info);
                    arg_g_type = pyg_type_from_object(py_arg);

                    /* Note: If the first argument is not an instance, its type hasn't got a gtype. */
                    if (!g_type_is_a(arg_g_type, container_g_type)) {
                        PyErr_Format(PyExc_TypeError, "%s() argument %zd: Must be %s, not %s",
                            pygi_gi_base_info_get_fullname(self->info), py_args_pos,
                            g_base_info_get_name(container_info), ((PyTypeObject *)py_type)->tp_name);
                    }

                    Py_DECREF(py_type);

                    break;
                }
                case GI_INFO_TYPE_BOXED:
                case GI_INFO_TYPE_STRUCT:
                {
                    GIBaseInfo *info;

                    info = pyg_base_info_from_object(py_arg);
                    if (info == NULL || !g_base_info_equals(info, container_info)) {
                        PyObject *py_type;

                        py_type = PyObject_Type(py_arg);
                        g_assert(py_type != NULL);

                        PyErr_Format(PyExc_TypeError, "%s() argument %zd: Must be %s, not %s",
                            pygi_gi_base_info_get_fullname(self->info), py_args_pos,
                            g_base_info_get_name(container_info), ((PyTypeObject *)py_type)->tp_name);

                        Py_DECREF(py_type);
                    }

                    if (info != NULL) {
                        g_base_info_unref(info);
                    }

                    break;
                }
                case GI_INFO_TYPE_UNION:
                    /* TODO */
                default:
                    /* The other info types haven't got methods. */
                    g_assert_not_reached();
            }

            g_base_info_unref(container_info);

            if (PyErr_Occurred()) {
                return NULL;
            }

            py_args_pos += 1;
        }
        for (i = 0; i < n_args; i++) {
            GIArgInfo *arg_info;
            GITypeInfo *type_info;
            GIDirection direction;
            gboolean may_be_null;
            PyObject *py_arg;
            gint retval;

            if (aux_args[i] != NULL) {
                /* No check needed for auxiliary arguments. */
                continue;
            }

            arg_info = g_callable_info_get_arg(callable_info, i);
            direction = g_arg_info_get_direction(arg_info);
            if (!(direction == GI_DIRECTION_IN || direction == GI_DIRECTION_INOUT)) {
                g_base_info_unref((GIBaseInfo *)arg_info);
                continue;
            }

            g_assert(py_args_pos < n_py_args);

            py_arg = PyTuple_GetItem(args, py_args_pos);
            g_assert(py_arg != NULL);

            type_info = g_arg_info_get_type(arg_info);
            may_be_null = g_arg_info_may_be_null(arg_info);

            retval = pygi_gi_type_info_check_py_object(type_info, may_be_null, py_arg);

            g_base_info_unref((GIBaseInfo *)type_info);
            g_base_info_unref((GIBaseInfo *)arg_info);

            if (retval < 0) {
                g_base_info_unref((GIBaseInfo *)return_info);
                return NULL;
            }

            if (!retval) {
                g_base_info_unref((GIBaseInfo *)return_info);
                PyErr_PREFIX_FROM_FORMAT("%s() argument %zd: ",
                        pygi_gi_base_info_get_fullname(self->info),
                        py_args_pos);
                return NULL;
            }

            py_args_pos += 1;
        }

        g_assert(py_args_pos == n_py_args);
    }

    in_args = g_newa(GArgument, n_in_args);
    out_args = g_newa(GArgument, n_out_args);
    /* each out arg is a pointer, they point to these values */
    /* FIXME: This will break for caller-allocates funcs:
       http://bugzilla.gnome.org/show_bug.cgi?id=573314 */
    out_values = g_newa(GArgument, n_out_args);

    /* Link aux_args to in_args. */
    {
        gsize in_args_pos;
        gsize out_args_pos;

        in_args_pos = is_method ? 1 : 0;
        out_args_pos = 0;

        for (i = 0; i < n_args; i++) {
            GIArgInfo *arg_info;
            GIDirection direction;

            arg_info = g_callable_info_get_arg((GICallableInfo *)self->info, i);
            direction = g_arg_info_get_direction(arg_info);

            if (direction == GI_DIRECTION_IN) {
                if (aux_args[i] != NULL) {
                    g_assert(in_args_pos < n_in_args);
                    aux_args[i] = &in_args[in_args_pos];
                }
                in_args_pos += 1;
            } else {
                if (aux_args[i] != NULL) {
                    g_assert(out_args_pos < n_out_args);
                    aux_args[i] = &out_values[out_args_pos];
                }
                out_args_pos += 1;
                if (direction == GI_DIRECTION_INOUT) {
                    in_args_pos += 1;
                }
            }

            g_base_info_unref((GIBaseInfo *)arg_info);
        }

        g_assert(in_args_pos == n_in_args);
        g_assert(out_args_pos == n_out_args);
    }

    /* Get the arguments. */
    {
        guint in_args_pos = 0;
        guint out_args_pos = 0;
        Py_ssize_t py_args_pos = 0;

        if (is_method && !is_constructor) {
            /* Get the instance. */
            GIBaseInfo *container;
            GIInfoType container_info_type;
            PyObject *py_arg;

            container = g_base_info_get_container((GIBaseInfo *)callable_info);
            container_info_type = g_base_info_get_type(container);

            py_arg = PyTuple_GetItem(args, py_args_pos);
            g_assert(py_arg != NULL);

            if (py_arg == Py_None) {
                in_args[in_args_pos].v_pointer = NULL;
            } else if (container_info_type == GI_INFO_TYPE_STRUCT || container_info_type == GI_INFO_TYPE_BOXED) {
                PyObject *py_buffer;
                PyBufferProcs *py_buffer_procs;
                py_buffer = PyObject_GetAttrString((PyObject *)py_arg, "__buffer__");
                if (py_buffer == NULL) {
                    g_base_info_unref(container);
                    g_base_info_unref((GIBaseInfo *)return_info);
                    return NULL;
                }
                py_buffer_procs = py_buffer->ob_type->tp_as_buffer;
                (*py_buffer_procs->bf_getreadbuffer)(py_buffer, 0, &in_args[in_args_pos].v_pointer);
            } else { /* by fallback is always object */
                in_args[in_args_pos].v_pointer = pygobject_get(py_arg);
            }

            g_base_info_unref(container);

            in_args_pos += 1;
            py_args_pos += 1;
        } else if (is_constructor) {
            /* Skip the first argument. */
            py_args_pos += 1;
        }

        for (i = 0; i < n_args; i++) {
            GIDirection direction;
            GIArgInfo *arg_info;
            GArgument *out_value = NULL;

            arg_info = g_callable_info_get_arg(callable_info, i);
            direction = g_arg_info_get_direction(arg_info);

            if (direction == GI_DIRECTION_OUT || direction == GI_DIRECTION_INOUT) {
                g_assert(out_args_pos < n_out_args);

                out_value = &out_values[out_args_pos];
                out_args[out_args_pos].v_pointer = out_value;
                out_args_pos += 1;
            }

            if (direction == GI_DIRECTION_IN || direction == GI_DIRECTION_INOUT) {
                PyObject *py_arg;
                GITypeInfo *arg_type_info;
                GITypeTag arg_type_tag;

                g_assert(in_args_pos < n_in_args);
                if (direction == GI_DIRECTION_INOUT) {
                    in_args[in_args_pos].v_pointer = out_value;
                }

                if (aux_args[i] != NULL) {
                    /* Auxiliary argument has already been set or will be set later. */
                    in_args_pos += 1;
                    g_base_info_unref((GIBaseInfo *)arg_info);
                    continue;
                }

                arg_type_info = g_arg_info_get_type(arg_info);
                arg_type_tag = g_type_info_get_tag(arg_type_info);

                g_assert(py_args_pos < n_py_args);
                py_arg = PyTuple_GetItem(args, py_args_pos);
                g_assert(py_arg != NULL);

                GArgument in_value = pygi_g_argument_from_py_object(py_arg, g_arg_info_get_type(arg_info));

                if (PyErr_Occurred()) {
                    /* TODO: Release ressources allocated for previous arguments. */
                    g_base_info_unref((GIBaseInfo *)arg_type_info);
                    g_base_info_unref((GIBaseInfo *)arg_info);
                    g_base_info_unref((GIBaseInfo *)return_info);
                    return NULL;
                }

                if (arg_type_tag == GI_TYPE_TAG_ARRAY) {
                    GArray *array;
                    gint length_arg_pos;

                    array = in_value.v_pointer;

                    length_arg_pos = g_type_info_get_array_length(arg_type_info);
                    if (length_arg_pos != -1) {
                        GArgument *length_arg;

                        length_arg = aux_args[length_arg_pos];
                        g_assert(length_arg != NULL);
                        length_arg->v_size = array->len;
                    }

                    /* Get rid of the GArray. */
                    in_value.v_pointer = g_array_free(array, FALSE);
                    g_assert(in_value.v_pointer != NULL);
                }

                if (direction == GI_DIRECTION_IN) {
                    /* Pass the value. */
                    in_args[in_args_pos] = in_value;
                } else {
                    /* Pass a pointer to the value. */
                    g_assert(out_value != NULL);
                    *out_value = in_value;
                }

                g_base_info_unref((GIBaseInfo *)arg_type_info);

                in_args_pos += 1;
                py_args_pos += 1;
            }

            g_base_info_unref((GIBaseInfo *)arg_info);
        }

        g_assert(in_args_pos == n_in_args);
        g_assert(out_args_pos == n_out_args);
        g_assert(py_args_pos == n_py_args);
    }

    /* Invoke the callable. */
    {
        GError *error;

        error = NULL;

        if (!g_function_info_invoke((GIFunctionInfo *)callable_info,
                    in_args, n_in_args,
                    out_args, n_out_args,
                    &return_arg,
                    &error)) {
            g_base_info_unref((GIBaseInfo *)return_info);
            PyErr_Format(PyExc_RuntimeError, "Error invoking %s.%s(): %s",
                    g_base_info_get_namespace((GIBaseInfo *)callable_info),
                    g_base_info_get_name((GIBaseInfo *)callable_info),
                    error->message);
            g_error_free(error);
            return NULL;
        }
    }

    n_return_values = n_out_args - n_aux_out_args;
    if (return_tag != GI_TYPE_TAG_VOID) {
        n_return_values += 1;

        if (!is_constructor) {
            if (return_tag == GI_TYPE_TAG_ARRAY) {
                /* Create a GArray. */
                GITypeInfo *item_type_info;
                GITypeTag item_type_tag;
                gsize item_size;
                gssize length;
                gboolean is_zero_terminated;
                GArray *array;

                item_type_info = g_type_info_get_param_type(return_info, 0);
                item_type_tag = g_type_info_get_tag(item_type_info);
                item_size = pygi_gi_type_tag_get_size(item_type_tag);
                is_zero_terminated = g_type_info_is_zero_terminated(return_info);

                if (is_zero_terminated) {
                    length = g_strv_length(return_arg.v_pointer);
                } else {
                    length = g_type_info_get_array_fixed_size(return_info);
                    if (length < 0) {
                        gint length_arg_pos;
                        GArgument *length_arg;

                        length_arg_pos = g_type_info_get_array_length(return_info);
                        g_assert(length_arg_pos >= 0);

                        length_arg = aux_args[length_arg_pos];
                        g_assert(length_arg != NULL);

                        /* FIXME: Take into account the type of the argument. */
                        length = length_arg->v_int;
                    }
                }
                
                array = g_array_new(is_zero_terminated, FALSE, item_size);
                array->data = return_arg.v_pointer;
                array->len = length;
                
                return_arg.v_pointer = array;

                g_base_info_unref((GIBaseInfo *)item_type_info);
            }

            return_value = pygi_g_argument_to_py_object(return_arg, return_info);

            if (return_tag == GI_TYPE_TAG_ARRAY) {
                return_arg.v_pointer = g_array_free((GArray *)return_arg.v_pointer, FALSE);
            }

            g_assert(return_value != NULL);
        } else {
            /* Instanciate the class passed as first argument and attach the GObject instance. */
            PyTypeObject *py_type;
            PyGObject *self;

            g_assert(n_py_args > 0);
            py_type = (PyTypeObject *)PyTuple_GetItem(args, 0);
            g_assert(py_type != NULL);

            if (py_type->tp_flags & Py_TPFLAGS_HEAPTYPE) {
                Py_INCREF(py_type);
            }
            self = PyObject_GC_New(PyGObject, py_type);
            self->inst_dict = NULL;
            self->weakreflist = NULL;
            self->private_flags.flags = 0;

            self->obj = return_arg.v_pointer;

            if (g_object_is_floating(self->obj)) {
                g_object_ref_sink(self->obj);
            }
            pygobject_register_wrapper((PyObject *)self);

            PyObject_GC_Track((PyObject *)self);

            return_value = (PyObject *)self;
        }
    } else {
        return_value = NULL;
    }

    /* Get output arguments and release input arguments. */
    {
        guint in_args_pos;
        guint out_args_pos;
        guint return_values_pos;

        return_values_pos = 0;

        if (n_return_values > 1) {
            /* Return a tuple. */
            PyObject *return_values;

            return_values = PyTuple_New(n_return_values);
            if (return_values == NULL) {
                g_base_info_unref((GIBaseInfo *)return_info);
                return NULL;
            }

            if (return_tag != GI_TYPE_TAG_VOID) {
                /* Put the return value first. */
                int retval;
                g_assert(return_value != NULL);
                retval = PyTuple_SetItem(return_values, return_values_pos, return_value);
                g_assert(retval == 0);
                return_values_pos += 1;
            }

            return_value = return_values;
        }

        in_args_pos = is_method ? 1 : 0;
        out_args_pos = 0;

        for (i = 0; i < n_args; i++) {
            GIDirection direction;
            GIArgInfo *arg_info;
            GITypeInfo *arg_type_info;

            arg_info = g_callable_info_get_arg(callable_info, i);
            arg_type_info = g_arg_info_get_type(arg_info);

            direction = g_arg_info_get_direction(arg_info);

            if (direction == GI_DIRECTION_IN) {
                g_assert(in_args_pos < n_in_args);

                /* TODO: Release if allocated. */

                in_args_pos += 1;
            } else {
                PyObject *obj;
                GITypeTag type_tag;
                GArgument *arg;

                if (direction == GI_DIRECTION_INOUT) {
                    g_assert(in_args_pos < n_in_args);
                }
                g_assert(out_args_pos < n_out_args);

                if (aux_args[i] != NULL) {
                    if (direction == GI_DIRECTION_INOUT) {
                        in_args_pos += 1;
                    }
                    out_args_pos += 1;
                    g_base_info_unref((GIBaseInfo *)arg_type_info);
                    g_base_info_unref((GIBaseInfo *)arg_info);
                    continue;
                }

                arg = (GArgument *)out_args[out_args_pos].v_pointer;

                type_tag = g_type_info_get_tag(arg_type_info);

                if (type_tag == GI_TYPE_TAG_ARRAY) {
                    /* Create a GArray. */
                    GITypeInfo *item_type_info;
                    GITypeTag item_type_tag;
                    gsize item_size;
                    gssize length;
                    gboolean is_zero_terminated;
                    GArray *array;

                    item_type_info = g_type_info_get_param_type(arg_type_info, 0);
                    item_type_tag = g_type_info_get_tag(item_type_info);
                    item_size = pygi_gi_type_tag_get_size(item_type_tag);
                    is_zero_terminated = g_type_info_is_zero_terminated(arg_type_info);

                    if (is_zero_terminated) {
                        length = g_strv_length(arg->v_pointer);
                    } else {
                        length = g_type_info_get_array_fixed_size(arg_type_info);
                        if (length < 0) {
                            gint length_arg_pos;
                            GArgument *length_arg;

                            length_arg_pos = g_type_info_get_array_length(arg_type_info);
                            g_assert(length_arg_pos >= 0);

                            length_arg = aux_args[length_arg_pos];
                            g_assert(length_arg != NULL);

                            /* FIXME: Take into account the type of the argument. */
                            length = length_arg->v_int;
                        }
                    }

                    array = g_array_new(is_zero_terminated, FALSE, item_size);
                    array->data = arg->v_pointer;
                    array->len = length;

                    arg->v_pointer = array;

                    g_base_info_unref((GIBaseInfo *)item_type_info);
                }

                obj = pygi_g_argument_to_py_object(*arg, arg_type_info);

                if (type_tag == GI_TYPE_TAG_ARRAY) {
                    arg->v_pointer = g_array_free((GArray *)arg->v_pointer, FALSE);
                }

                g_assert(obj != NULL);
                g_assert(return_values_pos < n_return_values);

                if (n_return_values > 1) {
                    int retval;
                    g_assert(return_value != NULL);

                    retval = PyTuple_SetItem(return_value, return_values_pos, obj);
                    g_assert(retval == 0);
                } else {
                    g_assert(return_value == NULL);
                    return_value = obj;
                }

                if (direction == GI_DIRECTION_INOUT) {
                    in_args_pos += 1;
                }
                out_args_pos += 1;
                return_values_pos += 1;
            }

            g_base_info_unref((GIBaseInfo *)arg_type_info);
            g_base_info_unref((GIBaseInfo *)arg_info);
        }

        if (n_return_values > 1) {
            g_assert(return_values_pos == n_return_values);
        }
        g_assert(out_args_pos == n_out_args);
        g_assert(in_args_pos == n_in_args);
    }

    g_base_info_unref((GIBaseInfo *)return_info);

    if (return_value == NULL) {
        Py_INCREF(Py_None);
        return_value = Py_None;
    }

    return return_value;
}

static PyMethodDef _PyGIFunctionInfo_methods[] = {
    { "isConstructor", (PyCFunction)_wrap_g_function_info_is_constructor, METH_NOARGS },
    { "isMethod", (PyCFunction)_wrap_g_function_info_is_method, METH_NOARGS },
    { "invoke", (PyCFunction)_wrap_g_function_info_invoke, METH_VARARGS },
    { NULL, NULL, 0 }
};

/* GICallbackInfo */
PYGIINFO_DEFINE_TYPE("CallbackInfo", GICallbackInfo, PyGICallableInfo_Type);

static PyMethodDef _PyGICallbackInfo_methods[] = {
    { NULL, NULL, 0 }
};

/* GISignalInfo */
PYGIINFO_DEFINE_TYPE("SignalInfo", GISignalInfo, PyGICallableInfo_Type);

static PyMethodDef _PyGISignalInfo_methods[] = {
    { NULL, NULL, 0 }
};


/* GIVFuncInfo */
PYGIINFO_DEFINE_TYPE("VFuncInfo", GIVFuncInfo, PyGICallableInfo_Type);

static PyMethodDef _PyGIVFuncInfo_methods[] = {
    { NULL, NULL, 0 }
};

/* RegisteredTypeInfo */
PYGIINFO_DEFINE_TYPE("RegisteredTypeInfo", GIRegisteredTypeInfo, PyGIBaseInfo_Type);

static PyObject *
_wrap_g_registered_type_info_get_g_type (PyGIBaseInfo* self)
{
    int gtype;

    gtype = g_registered_type_info_get_g_type ((GIRegisteredTypeInfo*)self->info);
    return pyg_type_wrapper_new(gtype);
}

static PyMethodDef _PyGIRegisteredTypeInfo_methods[] = {
    { "getGType", (PyCFunction)_wrap_g_registered_type_info_get_g_type, METH_NOARGS },
    { NULL, NULL, 0 }
};

/* GIStructInfo */
PYGIINFO_DEFINE_TYPE("StructInfo", GIStructInfo, PyGIRegisteredTypeInfo_Type);

static PyObject *
_wrap_g_struct_info_get_fields(PyGIBaseInfo *self)
{
    int i, length;
    PyObject *retval;

    g_base_info_ref(self->info);
    length = g_struct_info_get_n_fields((GIStructInfo*)self->info);
    retval = PyTuple_New(length);

    for (i = 0; i < length; i++) {
        GIFieldInfo *field;
        field = g_struct_info_get_field((GIStructInfo*)self->info, i);
        PyTuple_SetItem(retval, i, pyg_info_new(field));
        g_base_info_unref((GIBaseInfo*)field);
    }
    g_base_info_unref(self->info);

    return retval;
}

static PyObject *
_wrap_g_struct_info_get_methods(PyGIBaseInfo *self)
{
    int i, length;
    PyObject *retval;

    g_base_info_ref(self->info);
    length = g_struct_info_get_n_methods((GIStructInfo*)self->info);
    retval = PyTuple_New(length);

    for (i = 0; i < length; i++) {
        GIFunctionInfo *function;
        function = g_struct_info_get_method((GIStructInfo*)self->info, i);
        PyTuple_SetItem(retval, i, pyg_info_new(function));
        g_base_info_unref((GIBaseInfo*)function);
    }
    g_base_info_unref(self->info);

    return retval;
}

static PyObject *
_wrap_g_struct_info_new_buffer(PyGIBaseInfo *self)
{
    gsize size = g_struct_info_get_size ((GIStructInfo*)self->info);
    PyObject *buffer = PyBuffer_New (size);
    Py_INCREF(buffer);
    return buffer;
}

static PyMethodDef _PyGIStructInfo_methods[] = {
    { "getFields", (PyCFunction)_wrap_g_struct_info_get_fields, METH_NOARGS },
    { "getMethods", (PyCFunction)_wrap_g_struct_info_get_methods, METH_NOARGS },
    { "newBuffer", (PyCFunction)_wrap_g_struct_info_new_buffer, METH_NOARGS },
    { NULL, NULL, 0 }
};

/* GIUnionInfo */
PYGIINFO_DEFINE_TYPE("UnionInfo", GIUnionInfo, PyGIRegisteredTypeInfo_Type);

static PyMethodDef _PyGIUnionInfo_methods[] = {
    { NULL, NULL, 0 }
};

/* EnumInfo */
PYGIINFO_DEFINE_TYPE("EnumInfo", GIEnumInfo, PyGIRegisteredTypeInfo_Type);

static PyObject *
_wrap_g_enum_info_get_values(PyGIBaseInfo *self)
{
    int n_values, i;
    GIValueInfo  *value;
    PyObject *list;

    g_base_info_ref(self->info);
    n_values = g_enum_info_get_n_values((GIEnumInfo*)self->info);
    list = PyList_New(n_values);
    for (i = 0; i < n_values; i++)
        {
            value = g_enum_info_get_value((GIEnumInfo*)self->info, i);
            PyList_SetItem(list, i, pyg_info_new(value));
            g_base_info_unref((GIBaseInfo*)value);
        }
    g_base_info_unref(self->info);

    return list;
}

static PyMethodDef _PyGIEnumInfo_methods[] = {
    { "getValues", (PyCFunction)_wrap_g_enum_info_get_values, METH_NOARGS },
    { NULL, NULL, 0 }
};

/* BoxedInfo */
PYGIINFO_DEFINE_TYPE("BoxedInfo", GIBoxedInfo, PyGIRegisteredTypeInfo_Type);

static PyMethodDef _PyGIBoxedInfo_methods[] = {
    { NULL, NULL, 0 }
};

/* ObjectInfo */
PYGIINFO_DEFINE_TYPE("ObjectInfo", GIObjectInfo, PyGIRegisteredTypeInfo_Type);

static PyObject *
_wrap_g_object_info_get_parent(PyGIBaseInfo *self)
{
    GIObjectInfo *parent_info;

    g_base_info_ref(self->info);
    parent_info = g_object_info_get_parent((GIObjectInfo*)self->info);
    g_base_info_unref(self->info);

    if (parent_info)
        return pyg_info_new(parent_info);

    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
_wrap_g_object_info_get_type_name(PyGIBaseInfo *self)
{
    const gchar *type_name;

    g_base_info_ref(self->info);
    type_name = g_object_info_get_type_name((GIObjectInfo*)self->info);
    g_base_info_unref(self->info);

    return PyString_FromString(type_name);
}

static PyObject *
_wrap_g_object_info_get_methods(PyGIBaseInfo *self)
{
    int i, length;
    PyObject *retval;

    g_base_info_ref(self->info);
    length = g_object_info_get_n_methods((GIObjectInfo*)self->info);
    retval = PyTuple_New(length);

    for (i = 0; i < length; i++) {
        GIFunctionInfo *function;
        function = g_object_info_get_method((GIObjectInfo*)self->info, i);
        PyTuple_SetItem(retval, i, pyg_info_new(function));
        g_base_info_unref((GIBaseInfo*)function);
    }
    g_base_info_unref(self->info);

    return retval;
}

static PyObject *
_wrap_g_object_info_get_fields(PyGIBaseInfo *self)
{
    int i, length;
    PyObject *retval;

    g_base_info_ref(self->info);
    length = g_object_info_get_n_fields((GIObjectInfo*)self->info);
    retval = PyTuple_New(length);

    for (i = 0; i < length; i++) {
        GIFieldInfo *field;
        field = g_object_info_get_field((GIObjectInfo*)self->info, i);
        PyTuple_SetItem(retval, i, pyg_info_new(field));
        g_base_info_unref((GIBaseInfo*)field);
    }
    g_base_info_unref(self->info);

    return retval;
}

static PyObject *
_wrap_g_object_info_get_interfaces(PyGIBaseInfo *self)
{
    int i, length;
    PyObject *retval;

    g_base_info_ref(self->info);
    length = g_object_info_get_n_interfaces((GIObjectInfo*)self->info);
    retval = PyTuple_New(length);

    for (i = 0; i < length; i++) {
        GIInterfaceInfo *interface;
        interface = g_object_info_get_interface((GIObjectInfo*)self->info, i);
        PyTuple_SetItem(retval, i, pyg_info_new(interface));
        g_base_info_unref((GIBaseInfo*)interface);
    }
    g_base_info_unref(self->info);

    return retval;
}

static PyMethodDef _PyGIObjectInfo_methods[] = {
    { "getParent", (PyCFunction)_wrap_g_object_info_get_parent, METH_NOARGS },
    { "getTypeName", (PyCFunction)_wrap_g_object_info_get_type_name, METH_NOARGS },
    { "getMethods", (PyCFunction)_wrap_g_object_info_get_methods, METH_NOARGS },
    { "getFields", (PyCFunction)_wrap_g_object_info_get_fields, METH_NOARGS },
    { "getInterfaces", (PyCFunction)_wrap_g_object_info_get_interfaces, METH_NOARGS },
    { NULL, NULL, 0 }
};



/* GIInterfaceInfo */
PYGIINFO_DEFINE_TYPE("InterfaceInfo", GIInterfaceInfo, PyGIRegisteredTypeInfo_Type);

static PyObject *
_wrap_g_interface_info_get_methods(PyGIBaseInfo *self)
{
    int i, length;
    PyObject *retval;

    g_base_info_ref(self->info);
    length = g_interface_info_get_n_methods((GIInterfaceInfo*)self->info);
    retval = PyTuple_New(length);

    for (i = 0; i < length; i++) {
        GIFunctionInfo *function;
        function = g_interface_info_get_method((GIInterfaceInfo*)self->info, i);
        PyTuple_SetItem(retval, i, pyg_info_new(function));
        g_base_info_unref((GIBaseInfo*)function);
    }
    g_base_info_unref(self->info);

    return retval;
}

static void
initialize_interface (GTypeInterface *iface, PyTypeObject *pytype)
{
    // TODO: Implement this when g-i adds supports for vfunc offsets:
    // http://bugzilla.gnome.org/show_bug.cgi?id=560281
    /*
    GIRepository *repo = g_irepository_get_default();
    GIBaseInfo *iface_info = g_irepository_find_by_gtype(repo, G_TYPE_FROM_INTERFACE(iface));
    int length, i;
    GTypeInterface *parent_iface = g_type_interface_peek_parent(iface);

    length = g_interface_info_get_n_methods((GIInterfaceInfo *) iface_info);

    for (i = 0; i < length; i++) {
        GIFunctionInfo *method = g_interface_info_get_method((GIInterfaceInfo *) iface_info, i);
        const gchar *method_name = g_base_info_get_name((GIBaseInfo *) method);
        gchar pymethod_name[250];
        PyObject *py_method;
        void *method_ptr = iface + i * sizeof(void*);

        printf("%s\n", method_name);

        g_snprintf(pymethod_name, sizeof(pymethod_name), "do_%s", pymethod_name);
        py_method = PyObject_GetAttrString((PyObject *) pytype, pymethod_name);
        if (py_method && !PyObject_TypeCheck(py_method, &PyCFunction_Type)) {
            method_ptr = interface_method;
        } else {
            PyErr_Clear();
            if (parent_iface) {
                method_ptr = parent_iface + i * sizeof(void*);
            }
            Py_XDECREF(py_method);
        }

        g_base_info_unref((GIBaseInfo *) method);
    }
    */
}

static PyObject *
_wrap_g_interface_info_register(PyGIBaseInfo *self)
{
    GType gtype;
    GInterfaceInfo *info_struct = g_new0(GInterfaceInfo, 1);

    info_struct->interface_init = (GInterfaceInitFunc) initialize_interface;
    info_struct->interface_finalize = NULL;
    info_struct->interface_data = NULL;

    gtype = g_registered_type_info_get_g_type((GIRegisteredTypeInfo *) self->info);
    pyg_register_interface_info(gtype, info_struct);

    Py_INCREF(Py_None);
    return Py_None;
}

static PyMethodDef _PyGIInterfaceInfo_methods[] = {
    { "getMethods", (PyCFunction)_wrap_g_interface_info_get_methods, METH_NOARGS },
    { "register", (PyCFunction)_wrap_g_interface_info_register, METH_NOARGS },
    { NULL, NULL, 0 }
};


/* GIConstantInfo */
PYGIINFO_DEFINE_TYPE("ConstantInfo", GIConstantInfo, PyGIBaseInfo_Type);

static PyMethodDef _PyGIConstantInfo_methods[] = {
    { NULL, NULL, 0 }
};


/* GIValueInfo */
PYGIINFO_DEFINE_TYPE("ValueInfo", GIValueInfo, PyGIBaseInfo_Type);

static PyObject *
_wrap_g_value_info_get_value(PyGIBaseInfo *self)
{
    glong value;

    g_base_info_ref(self->info);
    value = g_value_info_get_value((GIValueInfo*)self->info);
    g_base_info_unref(self->info);

    return PyLong_FromLong(value);
}


static PyMethodDef _PyGIValueInfo_methods[] = {
    { "getValue", (PyCFunction)_wrap_g_value_info_get_value, METH_NOARGS },
    { NULL, NULL, 0 }
};


/* GIPropertyInfo */
PYGIINFO_DEFINE_TYPE("PropertyInfo", GIPropertyInfo, PyGIBaseInfo_Type);

static PyMethodDef _PyGIPropertyInfo_methods[] = {
    { NULL, NULL, 0 }
};

/* GIFieldInfo */
PYGIINFO_DEFINE_TYPE("FieldInfo", GIFieldInfo, PyGIBaseInfo_Type);

static PyObject *
_wrap_g_field_info_get_value(PyGIBaseInfo *self, PyObject *args)
{
    PyObject *retval;
    GIBaseInfo *container_info;
    GIInfoType container_info_type;
    GITypeInfo *field_type_info;
    GArgument value;
    PyObject *object;
    gpointer buffer;

    retval = NULL;

    if (!PyArg_ParseTuple(args, "O:FieldInfo.getValue", &object)) {
        return NULL;
    }

    container_info = g_base_info_get_container(self->info);
    container_info_type = g_base_info_get_type(container_info);

    field_type_info = g_field_info_get_type((GIFieldInfo *)self->info);

    if (container_info_type == GI_INFO_TYPE_STRUCT
            || container_info_type == GI_INFO_TYPE_BOXED) {
        gsize size;
        buffer = pygi_py_object_get_buffer(object, &size);
        if (buffer == NULL) {
            goto return_;
        }
    } else {
        buffer = pygobject_get(object);
    }

    /* A few types are not handled by g_field_info_get_field, so do it here. */
    if (!g_type_info_is_pointer(field_type_info)
            && g_type_info_get_tag(field_type_info) == GI_TYPE_TAG_INTERFACE) {
        GIBaseInfo *info;

        if (!(g_field_info_get_flags((GIFieldInfo *)self->info) & GI_FIELD_IS_READABLE)) {
            PyErr_SetString(PyExc_RuntimeError, "Field is not readable");
            goto return_;
        }

        info = g_type_info_get_interface (field_type_info);
        switch(g_base_info_get_type(info))
        {
            case GI_INFO_TYPE_STRUCT:
            {
                gsize offset;
                gsize size;

                offset = g_field_info_get_offset((GIFieldInfo *)self->info);
                size = g_struct_info_get_size((GIStructInfo *)info);
                g_assert(size > 0);

                value.v_pointer = g_try_malloc(size);
                if (value.v_pointer == NULL) {
                    PyErr_SetString(PyExc_MemoryError, "Failed to allocate memory");
                    break;
                }
                g_memmove(value.v_pointer, buffer + offset, size);

                g_base_info_unref(info);
                goto g_argument_to_py_object;
            }
            case GI_INFO_TYPE_UNION:
            case GI_INFO_TYPE_BOXED:
                /* TODO */
                g_assert_not_reached();
                break;
            default:
                /* Fallback. */
                break;
        }

        g_base_info_unref(info);

        if (PyErr_Occurred()) {
            goto return_;
        }
    }

    if (!g_field_info_get_field((GIFieldInfo *)self->info, buffer, &value)) {
        PyErr_SetString(PyExc_RuntimeError, "Failed to get value for field");
        goto return_;
    }

g_argument_to_py_object:
    retval = pygi_g_argument_to_py_object(value, field_type_info);

return_:
    g_base_info_unref((GIBaseInfo *)field_type_info);

    Py_XINCREF(retval);
    return retval;
}

static PyObject *
_wrap_g_field_info_set_value(PyGIBaseInfo *self, PyObject *args)
{
    PyObject *object;
    PyObject *py_value;
    GArgument value;
    gpointer buffer;
    GIBaseInfo *container_info;
    GIInfoType container_info_type;
    GITypeInfo *field_type_info;
    PyObject *retval;
    gint check_retval;

    retval = NULL;

    if (!PyArg_ParseTuple(args, "OO:FieldInfo.setValue", &object, &py_value)) {
        return NULL;
    }

    container_info = g_base_info_get_container(self->info);
    container_info_type = g_base_info_get_type(container_info);

    field_type_info = g_field_info_get_type((GIFieldInfo *)self->info);

    if (container_info_type == GI_INFO_TYPE_STRUCT
            || container_info_type == GI_INFO_TYPE_BOXED) {
        gsize size;
        buffer = pygi_py_object_get_buffer(object, &size);
        if (buffer == NULL) {
            goto return_;
        }
    } else {
        buffer = pygobject_get(object);
    }

    /* Check the value. */
    check_retval = pygi_gi_type_info_check_py_object(field_type_info, TRUE, py_value);

    if (check_retval < 0) {
        goto return_;
    }

    if (!check_retval) {
        PyErr_PREFIX_FROM_FORMAT("%s.set_value() argument 1: ",
                g_base_info_get_namespace(self->info));
        goto return_;
    }

    value = pygi_g_argument_from_py_object(py_value, field_type_info);

    /* A few types are not handled by g_field_info_set_field, so do it here. */
    if (!g_type_info_is_pointer(field_type_info)
            && g_type_info_get_tag(field_type_info) == GI_TYPE_TAG_INTERFACE) {
        GIBaseInfo *info;

        if (!(g_field_info_get_flags((GIFieldInfo *)self->info) & GI_FIELD_IS_WRITABLE)) {
            PyErr_SetString(PyExc_RuntimeError, "Field is not writable");
            goto return_;
        }

        info = g_type_info_get_interface(field_type_info);
        switch (g_base_info_get_type(info))
        {
            case GI_INFO_TYPE_STRUCT:
            {
                gsize offset;
                gsize size;

                offset = g_field_info_get_offset((GIFieldInfo *)self->info);
                size = g_struct_info_get_size((GIStructInfo *)info);
                g_assert(size > 0);

                g_memmove(buffer + offset, value.v_pointer, size);

                retval = Py_None;
                g_base_info_unref(info);
                goto return_;
            }
            case GI_INFO_TYPE_UNION:
            case GI_INFO_TYPE_BOXED:
                /* TODO */
                g_assert_not_reached();
                break;
            default:
                /* Fallback. */
                break;
        }

        g_base_info_unref(info);
    }

    if (!g_field_info_set_field((GIFieldInfo *)self->info, buffer, &value)) {
        PyErr_SetString(PyExc_RuntimeError, "Failed to set value for field");
        goto return_;
    }

    retval = Py_None;

return_:
    g_base_info_unref((GIBaseInfo *)field_type_info);

    Py_XINCREF(retval);
    return retval;
}

static PyMethodDef _PyGIFieldInfo_methods[] = {
    { "getValue", (PyCFunction)_wrap_g_field_info_get_value, METH_VARARGS },
    { "setValue", (PyCFunction)_wrap_g_field_info_set_value, METH_VARARGS },
    { NULL, NULL, 0 }
};

/* ArgInfo */
PYGIINFO_DEFINE_TYPE("ArgInfo", GIArgInfo, PyGIBaseInfo_Type);

static PyObject *
_wrap_g_arg_info_get_type(PyGIBaseInfo *self)
{
    return pyg_info_new(g_arg_info_get_type((GIArgInfo*)self->info));
}

static PyObject *
_wrap_g_arg_info_get_direction(PyGIBaseInfo *self)
{
    return PyInt_FromLong(g_arg_info_get_direction((GIArgInfo*)self->info));
}

static PyMethodDef _PyGIArgInfo_methods[] = {
    { "getType", (PyCFunction)_wrap_g_arg_info_get_type, METH_NOARGS },
    { "getDirection", (PyCFunction)_wrap_g_arg_info_get_direction, METH_NOARGS },
    { NULL, NULL, 0 }
};

/* TypeInfo */
PYGIINFO_DEFINE_TYPE("TypeInfo", GITypeInfo, PyGIBaseInfo_Type);

static PyObject *
_wrap_g_type_info_get_tag(PyGIBaseInfo *self)
{
    return PyInt_FromLong(g_type_info_get_tag((GITypeInfo*)self->info));
}

static PyObject *
_wrap_g_type_info_get_param_type(PyGIBaseInfo *self, PyObject *args)
{
    int index;

    if (!PyArg_ParseTuple(args, "i:TypeInfo.getParamType",
                          &index))
        return NULL;

    return pyg_info_new(g_type_info_get_param_type((GITypeInfo*)self->info, index));
}

static PyObject *
_wrap_g_type_info_get_interface(PyGIBaseInfo *self)
{
    return pyg_info_new(g_type_info_get_interface((GITypeInfo*)self->info));
}

static PyMethodDef _PyGITypeInfo_methods[] = {
    { "getTag", (PyCFunction)_wrap_g_type_info_get_tag, METH_NOARGS },
    { "getParamType", (PyCFunction)_wrap_g_type_info_get_param_type, METH_VARARGS },
    { "getInterface", (PyCFunction)_wrap_g_type_info_get_interface, METH_NOARGS },
    { NULL, NULL, 0 }
};

#if 0
GIErrorDomainInfo
#endif

/* GIUnresolvedInfo */
PYGIINFO_DEFINE_TYPE("UnresolvedInfo", GIUnresolvedInfo, PyGIBaseInfo_Type);

static PyMethodDef _PyGIUnresolvedInfo_methods[] = {
    { NULL, NULL, 0 }
};

void
pygi_info_register_types(PyObject *m)
{
#define REGISTER_TYPE(m, type, name) \
    type.ob_type = &PyType_Type; \
    type.tp_alloc = PyType_GenericAlloc; \
    type.tp_new = PyType_GenericNew; \
    if (PyType_Ready(&type)) \
        return; \
    if (PyModule_AddObject(m, name, (PyObject *)&type)) \
        return; \
    Py_INCREF(&type)

    REGISTER_TYPE(m, PyGIBaseInfo_Type, "BaseInfo");
    REGISTER_TYPE(m, PyGIUnresolvedInfo_Type, "UnresolvedInfo");
    REGISTER_TYPE(m, PyGICallableInfo_Type, "CallableInfo");
    REGISTER_TYPE(m, PyGIFunctionInfo_Type, "FunctionInfo");
    REGISTER_TYPE(m, PyGICallbackInfo_Type, "CallbackInfo");
    REGISTER_TYPE(m, PyGIVFuncInfo_Type, "VFuncInfo");
    REGISTER_TYPE(m, PyGISignalInfo_Type, "SignalInfo");
    REGISTER_TYPE(m, PyGIRegisteredTypeInfo_Type, "RegisteredTypeInfo");
    REGISTER_TYPE(m, PyGIStructInfo_Type, "StructInfo");
    REGISTER_TYPE(m, PyGIUnionInfo_Type, "UnionInfo");
    REGISTER_TYPE(m, PyGIEnumInfo_Type, "EnumInfo");
    REGISTER_TYPE(m, PyGIBoxedInfo_Type, "BoxedInfo");
    REGISTER_TYPE(m, PyGIObjectInfo_Type, "ObjectInfo");
    REGISTER_TYPE(m, PyGIInterfaceInfo_Type, "InterfaceInfo");
    REGISTER_TYPE(m, PyGIConstantInfo_Type, "ConstantInfo");
    REGISTER_TYPE(m, PyGIValueInfo_Type, "ValueInfo");
    REGISTER_TYPE(m, PyGIPropertyInfo_Type, "PropertyInfo");
    REGISTER_TYPE(m, PyGIFieldInfo_Type, "FieldInfo");
    REGISTER_TYPE(m, PyGIArgInfo_Type, "ArgInfo");
    REGISTER_TYPE(m, PyGITypeInfo_Type, "TypeInfo");

#undef REGISTER_TYPE
}

#undef PYGIINFO_DEFINE_TYPE
