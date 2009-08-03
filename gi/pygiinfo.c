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

static PyMethodDef _PyGIBaseInfo_methods[] = {
    { "get_name", (PyCFunction)_wrap_g_base_info_get_name, METH_NOARGS },
    { "get_namespace", (PyCFunction)_wrap_g_base_info_get_namespace, METH_NOARGS },
    { NULL, NULL, 0 }
};

PyObject *
pyg_info_new(GIBaseInfo *info)
{
    GIInfoType info_type;
    PyTypeObject *type;
    PyGIBaseInfo *self;

    info_type = g_base_info_get_type(info);

    switch (info_type)
    {
        case GI_INFO_TYPE_INVALID:
            PyErr_SetString(PyExc_RuntimeError, "Invalid info type");
            return NULL;
        case GI_INFO_TYPE_FUNCTION:
            type = &PyGIFunctionInfo_Type;
            break;
        case GI_INFO_TYPE_CALLBACK:
            PyErr_SetString(PyExc_NotImplementedError, "GICallbackInfo bindings not implemented");
            return NULL;
        case GI_INFO_TYPE_STRUCT:
            type = &PyGIStructInfo_Type;
            break;
        case GI_INFO_TYPE_BOXED:
            PyErr_SetString(PyExc_NotImplementedError, "GIBoxedInfo bindings not implemented");
            return NULL;
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
            PyErr_SetString(PyExc_NotImplementedError, "GIConstantInfo bindings not implemented");
            return NULL;
        case GI_INFO_TYPE_ERROR_DOMAIN:
            PyErr_SetString(PyExc_NotImplementedError, "GIErrorDomainInfo bindings not implemented");
            return NULL;
        case GI_INFO_TYPE_UNION:
            PyErr_SetString(PyExc_NotImplementedError, "GIUnionInfo bindings not implemented");
            return NULL;
        case GI_INFO_TYPE_VALUE:
            type = &PyGIValueInfo_Type;
            break;
        case GI_INFO_TYPE_SIGNAL:
            PyErr_SetString(PyExc_NotImplementedError, "GISignalInfo bindings not implemented");
            return NULL;
        case GI_INFO_TYPE_VFUNC:
            PyErr_SetString(PyExc_NotImplementedError, "GIVFuncInfo bindings not implemented");
            return NULL;
        case GI_INFO_TYPE_PROPERTY:
            PyErr_SetString(PyExc_NotImplementedError, "GIPropertyInfo bindings not implemented");
            return NULL;
        case GI_INFO_TYPE_FIELD:
            type = &PyGIFieldInfo_Type;
            break;
        case GI_INFO_TYPE_ARG:
            PyErr_SetString(PyExc_NotImplementedError, "GIArgInfo bindings not implemented");
            return NULL;
        case GI_INFO_TYPE_TYPE:
            PyErr_SetString(PyExc_NotImplementedError, "GITypeInfo bindings not implemented");
            return NULL;
        case GI_INFO_TYPE_UNRESOLVED:
            type = &PyGIUnresolvedInfo_Type;
            break;
    }

    self = (PyGIBaseInfo *)PyObject_GC_New(PyGIBaseInfo, type);
    if (self == NULL) {
        return NULL;
    }

    if (type->tp_flags & Py_TPFLAGS_HEAPTYPE) {
        Py_INCREF(type);
    }

    self->info = g_base_info_ref(info);

    self->instance_dict = NULL;
    self->weakreflist = NULL;

    PyObject_GC_Track((PyObject *)self);

    return (PyObject *)self;
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
        PyErr_NoMemory();
    }

    return fullname;
}


/* CallableInfo */
PYGIINFO_DEFINE_TYPE("CallableInfo", GICallableInfo, PyGIBaseInfo_Type);

static PyMethodDef _PyGICallableInfo_methods[] = {
    { NULL, NULL, 0 }
};


/* FunctionInfo */
PYGIINFO_DEFINE_TYPE("FunctionInfo", GIFunctionInfo, PyGICallableInfo_Type);

static PyObject *
_wrap_g_function_info_is_constructor(PyGIBaseInfo *self)
{
    GIFunctionInfoFlags flags;
    gboolean is_constructor;

    flags = g_function_info_get_flags((GIFunctionInfo*)self->info);
    is_constructor = flags & GI_FUNCTION_IS_CONSTRUCTOR;

    return PyBool_FromLong(is_constructor);
}

static PyObject *
_wrap_g_function_info_is_method(PyGIBaseInfo *self)
{
    GIFunctionInfoFlags flags;
    gboolean is_method;

    flags = g_function_info_get_flags((GIFunctionInfo*)self->info);
    is_method = flags & GI_FUNCTION_IS_METHOD;

    return PyBool_FromLong(is_method);
}

static
GArray *
pygi_g_array_from_array(gpointer array, GITypeInfo *type_info, GArgument *args[])
{
    /* Create a GArray. */
    GITypeInfo *item_type_info;
    gboolean is_zero_terminated;
    GITypeTag item_type_tag;
    gsize item_size;
    gssize length;
    GArray *g_array;

    is_zero_terminated = g_type_info_is_zero_terminated(type_info);
    item_type_info = g_type_info_get_param_type(type_info, 0);
    g_assert(item_type_info != NULL);

    item_type_tag = g_type_info_get_tag(item_type_info);
    item_size = pygi_gi_type_tag_get_size(item_type_tag);

    g_base_info_unref((GIBaseInfo *)item_type_info);

    if (is_zero_terminated) {
        length = g_strv_length(array);
    } else {
        length = g_type_info_get_array_fixed_size(type_info);
        if (length < 0) {
            gint length_arg_pos;

            length_arg_pos = g_type_info_get_array_length(type_info);
            g_assert(length_arg_pos >= 0);

            /* FIXME: Take into account the type of the argument. */
            length = args[length_arg_pos]->v_int;
        }
    }

    g_array = g_array_new(is_zero_terminated, FALSE, item_size);
    if (g_array == NULL) {
        PyErr_NoMemory();
        return NULL;
    }

    g_array->data = array;
    g_array->len = length;

    return g_array;
}

static PyObject *
_wrap_g_function_info_invoke(PyGIBaseInfo *self, PyObject *py_args)
{
    gboolean is_method;
    gboolean is_constructor;

    gsize n_args;
    gsize n_in_args;
    gsize n_out_args;
    gsize n_containers;
    Py_ssize_t n_py_args;
    gsize n_aux_in_args;
    gsize n_aux_out_args;
    gsize n_return_values;

    GIArgInfo **arg_infos;
    GITypeInfo **arg_type_infos;
    GITypeInfo *return_type_info;
    GITypeTag return_type_tag;

    GArgument **args;
    gboolean *args_is_auxiliary;

    GArgument *in_args;
    GArgument *out_args;
    GArgument *out_values;
    GArgument *containers;
    GArgument return_arg;

    PyObject *return_value = NULL;

    gsize i;

    {
        GIFunctionInfoFlags flags;

        flags = g_function_info_get_flags((GIFunctionInfo *)self->info);
        is_method = (flags & GI_FUNCTION_IS_METHOD) != 0;
        is_constructor = (flags & GI_FUNCTION_IS_CONSTRUCTOR) != 0;
    }

    /* Count arguments. */
    n_args = g_callable_info_get_n_args((GICallableInfo *)self->info);
    n_in_args = 0;
    n_out_args = 0;
    n_containers = 0;
    n_aux_in_args = 0;
    n_aux_out_args = 0;

    arg_infos = g_newa(GIArgInfo *, n_args);
    arg_type_infos = g_newa(GITypeInfo *, n_args);

    args_is_auxiliary = g_newa(gboolean, n_args);
    memset(args_is_auxiliary, 0, sizeof(args_is_auxiliary));

    if (is_method) {
        /* The first argument is the instance. */
        n_in_args += 1;
    }

    for (i = 0; i < n_args; i++) {
        GIDirection direction;
        GITransfer transfer;
        GITypeTag arg_type_tag;

        arg_infos[i] = g_callable_info_get_arg((GICallableInfo *)self->info, i);
        g_assert(arg_infos[i] != NULL);

        arg_type_infos[i] = g_arg_info_get_type(arg_infos[i]);
        g_assert(arg_type_infos[i] != NULL);

        direction = g_arg_info_get_direction(arg_infos[i]);
        transfer = g_arg_info_get_ownership_transfer(arg_infos[i]);
        arg_type_tag = g_type_info_get_tag(arg_type_infos[i]);

        if (direction == GI_DIRECTION_IN || direction == GI_DIRECTION_INOUT) {
            n_in_args += 1;
        }
        if (direction == GI_DIRECTION_OUT || direction == GI_DIRECTION_INOUT) {
            n_out_args += 1;
        }

        if ((direction == GI_DIRECTION_INOUT && transfer != GI_TRANSFER_EVERYTHING)
                || (direction == GI_DIRECTION_IN && transfer == GI_TRANSFER_CONTAINER)) {
            n_containers += 1;
        }

        if (arg_type_tag == GI_TYPE_TAG_ARRAY) {
            gint length_arg_pos;

            length_arg_pos = g_type_info_get_array_length(arg_type_infos[i]);
            if (length_arg_pos >= 0) {
                g_assert(length_arg_pos < n_args);
                args_is_auxiliary[length_arg_pos] = TRUE;

                if (direction == GI_DIRECTION_IN || direction == GI_DIRECTION_INOUT) {
                    n_aux_in_args += 1;
                }
                if (direction == GI_DIRECTION_OUT || direction == GI_DIRECTION_INOUT) {
                    n_aux_out_args += 1;
                }
            }
        }
    }

    return_type_info = g_callable_info_get_return_type((GICallableInfo *)self->info);
    g_assert(return_type_info != NULL);

    return_type_tag = g_type_info_get_tag(return_type_info);

    if (return_type_tag == GI_TYPE_TAG_ARRAY) {
        gint length_arg_pos;
        length_arg_pos = g_type_info_get_array_length(return_type_info);
        if (length_arg_pos >= 0) {
            g_assert(length_arg_pos < n_args);
            args_is_auxiliary[length_arg_pos] = TRUE;
            n_aux_out_args += 1;
        }
    }

    n_return_values = n_out_args - n_aux_out_args;
    if (return_type_tag != GI_TYPE_TAG_VOID) {
        n_return_values += 1;
    }

    {
        gsize n_py_args_expected;
        Py_ssize_t py_args_pos;

        /* Check the argument count. */
        n_py_args = PyTuple_Size(py_args);
        g_assert(n_py_args >= 0);

        n_py_args_expected = n_in_args + (is_constructor ? 1 : 0) - n_aux_in_args;

        if (n_py_args != n_py_args_expected) {
            gchar *fullname;
            fullname = pygi_gi_base_info_get_fullname(self->info);
            if (fullname != NULL) {
                PyErr_Format(PyExc_TypeError,
                    "%s() takes exactly %zd argument(s) (%zd given)",
                    fullname, n_py_args_expected, n_py_args);
                g_free(fullname);
            }
            goto return_;
        }

        /* Check argument types. */
        py_args_pos = 0;
        if (is_constructor || is_method) {
            GIBaseInfo *container_info;
            PyObject *py_arg;
            gint retval;

            container_info = g_base_info_get_container(self->info);
            g_assert(container_info != NULL);

            g_assert(py_args_pos < n_py_args);
            py_arg = PyTuple_GET_ITEM(py_args, py_args_pos);

            /* AFAIK, only registered types can have constructors or methods,
             * so the cast should be safe. */
            retval = pygi_gi_registered_type_info_check_py_object(
                    (GIRegisteredTypeInfo *)container_info, py_arg, is_method);

            if (retval < 0) {
                goto return_;
            } else if (!retval) {
                gchar *fullname;
                fullname = pygi_gi_base_info_get_fullname(self->info);
                if (fullname != NULL) {
                    PyErr_PREFIX_FROM_FORMAT("%s() argument %zd: ", fullname, py_args_pos);
                    g_free(fullname);
                }
                goto return_;
            }

            py_args_pos += 1;
        }

        for (i = 0; i < n_args; i++) {
            GIDirection direction;
            gboolean may_be_null;
            PyObject *py_arg;
            gint retval;

            direction = g_arg_info_get_direction(arg_infos[i]);

            if (direction == GI_DIRECTION_OUT || args_is_auxiliary[i]) {
                continue;
            }

            g_assert(py_args_pos < n_py_args);
            py_arg = PyTuple_GET_ITEM(py_args, py_args_pos);

            may_be_null = g_arg_info_may_be_null(arg_infos[i]);

            retval = pygi_gi_type_info_check_py_object(arg_type_infos[i],
                    may_be_null, py_arg);

            if (retval < 0) {
                goto return_;
            } else if (!retval) {
                gchar *fullname;
                fullname = pygi_gi_base_info_get_fullname(self->info);
                if (fullname != NULL) {
                    PyErr_PREFIX_FROM_FORMAT("%s() argument %zd: ",
                        pygi_gi_base_info_get_fullname(self->info),
                        py_args_pos);
                    g_free(fullname);
                }
                goto return_;
            }

            py_args_pos += 1;
        }

        g_assert(py_args_pos == n_py_args);
    }

    args = g_newa(GArgument *, n_args);
    in_args = g_newa(GArgument, n_in_args);
    out_args = g_newa(GArgument, n_out_args);
    out_values = g_newa(GArgument, n_out_args);
    containers = g_newa(GArgument, n_containers);

    /* Bind args so we can use an unique index. */
    {
        gsize in_args_pos;
        gsize out_args_pos;

        in_args_pos = is_method ? 1 : 0;
        out_args_pos = 0;

        for (i = 0; i < n_args; i++) {
            GIDirection direction;

            direction = g_arg_info_get_direction(arg_infos[i]);

            switch (direction) {
                case GI_DIRECTION_IN:
                    g_assert(in_args_pos < n_in_args);
                    args[i] = &in_args[in_args_pos];
                    in_args_pos += 1;
                    break;
                case GI_DIRECTION_INOUT:
                    g_assert(in_args_pos < n_in_args);
                    g_assert(out_args_pos < n_out_args);
                    in_args[in_args_pos].v_pointer = &out_values[out_args_pos];
                    in_args_pos += 1;
                case GI_DIRECTION_OUT:
                    g_assert(out_args_pos < n_out_args);
                    out_args[out_args_pos].v_pointer = &out_values[out_args_pos];
                    args[i] = &out_values[out_args_pos];
                    out_args_pos += 1;
            }
        }

        g_assert(in_args_pos == n_in_args);
        g_assert(out_args_pos == n_out_args);
    }

    /* Convert the input arguments. */
    {
        Py_ssize_t py_args_pos;
        gsize containers_pos;

        py_args_pos = 0;
        containers_pos = 0;

        if (is_constructor) {
            /* Skip the first argument. */
            py_args_pos += 1;
        } else if (is_method) {
            /* Get the instance. */
            GIBaseInfo *container_info;
            GIInfoType container_info_type;
            PyObject *py_arg;

            container_info = g_base_info_get_container(self->info);
            container_info_type = g_base_info_get_type(container_info);

            g_assert(py_args_pos < n_py_args);
            py_arg = PyTuple_GET_ITEM(py_args, py_args_pos);

            g_assert(n_in_args > 0);
            switch(container_info_type) {
                case GI_INFO_TYPE_UNION:
                    /* TODO */
                    g_assert_not_reached();
                    break;
                case GI_INFO_TYPE_STRUCT:
                {
                    gsize size;
                    in_args[0].v_pointer = pygi_py_object_get_buffer(py_arg, &size);
                    break;
                }
                case GI_INFO_TYPE_OBJECT:
                    in_args[0].v_pointer = pygobject_get(py_arg);
                    break;
                default:
                    /* Other types don't have methods. */
                    g_assert_not_reached();
            }

            py_args_pos += 1;
        }

        for (i = 0; i < n_args; i++) {
            GIDirection direction;

            if (args_is_auxiliary[i]) {
                continue;
            }

            direction = g_arg_info_get_direction(arg_infos[i]);

            if (direction == GI_DIRECTION_IN || direction == GI_DIRECTION_INOUT) {
                PyObject *py_arg;
                GITypeTag arg_type_tag;
                GITransfer transfer;

                transfer = g_arg_info_get_ownership_transfer(arg_infos[i]);

                g_assert(py_args_pos < n_py_args);
                py_arg = PyTuple_GET_ITEM(py_args, py_args_pos);

                *args[i] = pygi_g_argument_from_py_object(py_arg, arg_type_infos[i],
                        transfer);

                if (PyErr_Occurred()) {
                    /* TODO: Release ressources allocated for previous arguments. */
                    return NULL;
                }

                arg_type_tag = g_type_info_get_tag(arg_type_infos[i]);

                if ((direction == GI_DIRECTION_INOUT && transfer != GI_TRANSFER_EVERYTHING)
                        || (direction == GI_DIRECTION_IN && transfer == GI_TRANSFER_CONTAINER)) {
                    /* We need to keep a copy of the container to be able to
                     * release the items after the call. */
                    g_assert(containers_pos < n_containers);
                    switch(arg_type_tag) {
                        case GI_TYPE_TAG_FILENAME:
                        case GI_TYPE_TAG_UTF8:
                            containers[containers_pos].v_string = args[i]->v_pointer;
                            break;
                        case GI_TYPE_TAG_ARRAY:
                            containers[containers_pos].v_pointer = g_array_copy(args[i]->v_pointer);
                            break;
                        case GI_TYPE_TAG_INTERFACE:
                        {
                            GIBaseInfo *info;
                            GIInfoType info_type;

                            info = g_type_info_get_interface(arg_type_infos[i]);
                            g_assert(info != NULL);

                            info_type = g_base_info_get_type(info);

                            switch (info_type) {
                                case GI_INFO_TYPE_STRUCT:
                                {
                                    GType type;

                                    type = g_registered_type_info_get_g_type((GIRegisteredTypeInfo *)info);

                                    if (g_type_is_a(type, G_TYPE_VALUE)) {
                                        GValue *value;
                                        GValue *new_value;

                                        value = args[i]->v_pointer;
                                        new_value = g_slice_new0(GValue);

                                        g_value_init(new_value, G_VALUE_TYPE(value));
                                        g_value_copy(value, new_value);

                                        containers[containers_pos].v_pointer = new_value;
                                        break;
                                    }

                                    /* TODO */

                                    break;
                                }
                                default:
                                    break;
                            }

                            g_base_info_unref(info);
                            break;
                        }
                        case GI_TYPE_TAG_GLIST:
                            containers[containers_pos].v_pointer = g_list_copy(args[i]->v_pointer);
                            break;
                        case GI_TYPE_TAG_GSLIST:
                            containers[containers_pos].v_pointer = g_slist_copy(args[i]->v_pointer);
                            break;
                        case GI_TYPE_TAG_GHASH:
                            containers[containers_pos].v_pointer = g_hash_table_copy(args[i]->v_pointer);
                            break;
                        default:
                            break;
                    }
                    containers_pos += 1;
                }

                if (arg_type_tag == GI_TYPE_TAG_ARRAY) {
                    GArray *array;
                    gssize length_arg_pos;

                    array = args[i]->v_pointer;

                    length_arg_pos = g_type_info_get_array_length(arg_type_infos[i]);
                    if (length_arg_pos >= 0) {
                        /* Set the auxiliary argument holding the length. */
                        args[length_arg_pos]->v_size = array->len;
                    }

                    /* Get rid of the GArray. */
                    args[i]->v_pointer = g_array_free(array, FALSE);
                }

                py_args_pos += 1;
            }
        }

        g_assert(py_args_pos == n_py_args);
        g_assert(containers_pos == n_containers);
    }

    /* Invoke the callable. */
    {
        GError *error;
        gint retval;

        error = NULL;

        retval = g_function_info_invoke((GIFunctionInfo *)self->info,
                in_args, n_in_args, out_args, n_out_args, &return_arg, &error);
        if (!retval) {
            gchar *fullname;

            g_assert(error != NULL);

            fullname = pygi_gi_base_info_get_fullname(self->info);
            if (fullname != NULL) {
                PyErr_Format(PyExc_RuntimeError, "Error invoking %s(): %s",
                    fullname, error->message);
            }

            g_error_free(error);
            /* TODO */
            goto return_;
        }
    }

    /* Convert the return value. */
    if (is_constructor) {
        PyTypeObject *type;

        g_assert(n_py_args > 0);
        type = (PyTypeObject *)PyTuple_GET_ITEM(py_args, 0);

        return_value = pygobject_new_from_type(return_arg.v_pointer, TRUE, type);
    } else {
        GITransfer transfer;

        if (return_type_tag == GI_TYPE_TAG_ARRAY) {
            GArray *array;

            array = pygi_g_array_from_array(return_arg.v_pointer, return_type_info, args);
            if (array == NULL) {
                /* TODO */
                goto return_;
            }

            return_arg.v_pointer = array;
        }

        return_value = pygi_g_argument_to_py_object(&return_arg, return_type_info);
        if (return_value == NULL) {
            /* TODO */
            goto return_;
        }

        transfer = g_callable_info_get_caller_owns((GICallableInfo *)self->info);

        pygi_g_argument_release(&return_arg, return_type_info, transfer,
            GI_DIRECTION_OUT);

        if (return_type_tag == GI_TYPE_TAG_ARRAY
                && transfer == GI_TRANSFER_NOTHING) {
            /* We created a #GArray, so free it. */
            return_arg.v_pointer = g_array_free(return_arg.v_pointer, FALSE);
        }
    }

    /* Convert output arguments and release arguments. */
    {
        gsize containers_pos;
        gsize return_values_pos;

        containers_pos = 0;
        return_values_pos = 0;

        if (n_return_values > 1) {
            /* Return a tuple. */
            PyObject *return_values;

            return_values = PyTuple_New(n_return_values);
            if (return_values == NULL) {
                /* TODO */
                return NULL;
            }

            if (return_type_tag == GI_TYPE_TAG_VOID) {
                /* The current return value is None. */
                Py_DECREF(return_value);
            } else {
                /* Put the return value first. */
                int retval;
                g_assert(return_value != NULL);
                retval = PyTuple_SetItem(return_values, return_values_pos, return_value);
                g_assert(retval == 0);
                return_values_pos += 1;
            }

            return_value = return_values;
        }

        for (i = 0; i < n_args; i++) {
            GIDirection direction;
            GITypeTag type_tag;
            GITransfer transfer;

            if (args_is_auxiliary[i]) {
                /* Auxiliary arguments are handled at the same time as their
                 * relatives. */
                continue;
            }

            direction = g_arg_info_get_direction(arg_infos[i]);
            transfer = g_arg_info_get_ownership_transfer(arg_infos[i]);

            type_tag = g_type_info_get_tag(arg_type_infos[i]);

            if (type_tag == GI_TYPE_TAG_ARRAY
                    && (direction != GI_DIRECTION_IN || transfer == GI_TRANSFER_NOTHING)) {
                GArray *array;

                array = pygi_g_array_from_array(args[i]->v_pointer, arg_type_infos[i], args);
                if (array == NULL) {
                    /* TODO */
                    goto return_;
                }

                args[i]->v_pointer = array;
            }

            if (direction == GI_DIRECTION_INOUT || direction == GI_DIRECTION_OUT) {
                /* Convert the argument. */
                PyObject *obj;

                obj = pygi_g_argument_to_py_object(args[i], arg_type_infos[i]);
                if (obj == NULL) {
                    /* TODO */
                    goto return_;
                }

                g_assert(return_values_pos < n_return_values);

                if (n_return_values > 1) {
                    PyTuple_SET_ITEM(return_value, return_values_pos, obj);
                } else {
                    /* The current return value is None. */
                    Py_DECREF(return_value);
                    return_value = obj;
                }

                return_values_pos += 1;
            }

            /* Release the argument. */
            if (direction == GI_DIRECTION_INOUT) {
                if (transfer != GI_TRANSFER_EVERYTHING) {
                    g_assert(containers_pos < n_containers);
                    pygi_g_argument_release(&containers[containers_pos], arg_type_infos[i],
                        GI_TRANSFER_NOTHING, GI_DIRECTION_IN);
                    containers_pos += 1;
                }
                if (transfer != GI_TRANSFER_NOTHING) {
                    pygi_g_argument_release(args[i], arg_type_infos[i], transfer,
                        GI_DIRECTION_OUT);
                }
            } else if (direction == GI_DIRECTION_IN && transfer == GI_TRANSFER_CONTAINER) {
                g_assert(containers_pos < n_containers);
                pygi_g_argument_release(&containers[containers_pos], arg_type_infos[i],
                    GI_TRANSFER_NOTHING, direction);
                containers_pos += 1;
            } else {
                pygi_g_argument_release(args[i], arg_type_infos[i], transfer, direction);
            }

            if (type_tag == GI_TYPE_TAG_ARRAY
                    && (direction != GI_DIRECTION_IN && transfer == GI_TRANSFER_NOTHING)) {
                /* We created a #GArray and it has not been released above, so free it. */
                args[i]->v_pointer = g_array_free(args[i]->v_pointer, FALSE);
            }
        }

        g_assert(n_return_values <= 1 || return_values_pos == n_return_values);
        g_assert(containers_pos == n_containers);
    }

return_:
    g_base_info_unref((GIBaseInfo *)return_type_info);

    for (i = 0; i < n_args; i++) {
        g_base_info_unref((GIBaseInfo *)arg_type_infos[i]);
        g_base_info_unref((GIBaseInfo *)arg_infos[i]);
    }

    if (PyErr_Occurred()) {
        Py_CLEAR(return_value);
    }

    return return_value;
}

static PyMethodDef _PyGIFunctionInfo_methods[] = {
    { "is_constructor", (PyCFunction)_wrap_g_function_info_is_constructor, METH_NOARGS },
    { "is_method", (PyCFunction)_wrap_g_function_info_is_method, METH_NOARGS },
    { "invoke", (PyCFunction)_wrap_g_function_info_invoke, METH_VARARGS },
    { NULL, NULL, 0 }
};


/* RegisteredTypeInfo */
PYGIINFO_DEFINE_TYPE("RegisteredTypeInfo", GIRegisteredTypeInfo, PyGIBaseInfo_Type);

static PyObject *
_wrap_g_registered_type_info_get_g_type(PyGIBaseInfo *self)
{
    GType type;

    type = g_registered_type_info_get_g_type((GIRegisteredTypeInfo *)self->info);

    return pyg_type_wrapper_new(type);
}

static PyMethodDef _PyGIRegisteredTypeInfo_methods[] = {
    { "get_g_type", (PyCFunction)_wrap_g_registered_type_info_get_g_type, METH_NOARGS },
    { NULL, NULL, 0 }
};


/* GIStructInfo */
PYGIINFO_DEFINE_TYPE("StructInfo", GIStructInfo, PyGIRegisteredTypeInfo_Type);

static PyObject *
_wrap_g_struct_info_get_fields(PyGIBaseInfo *self)
{
    gssize n_infos;
    PyObject *infos;
    gssize i;

    n_infos = g_struct_info_get_n_fields((GIStructInfo *)self->info);

    infos = PyTuple_New(n_infos);
    if (infos == NULL) {
        return NULL;
    }

    for (i = 0; i < n_infos; i++) {
        GIBaseInfo *info;
        PyObject *py_info;

        info = (GIBaseInfo *)g_struct_info_get_field((GIStructInfo *)self->info, i);
        g_assert(info != NULL);

        py_info = pyg_info_new(info);

        g_base_info_unref(info);

        if (py_info == NULL) {
            Py_CLEAR(infos);
            break;
        }

        PyTuple_SET_ITEM(infos, i, py_info);
    }

    return infos;
}

static PyObject *
_wrap_g_struct_info_get_methods(PyGIBaseInfo *self)
{
    gssize n_infos;
    PyObject *infos;
    gssize i;

    n_infos = g_struct_info_get_n_methods((GIStructInfo *)self->info);

    infos = PyTuple_New(n_infos);
    if (infos == NULL) {
        return NULL;
    }

    for (i = 0; i < n_infos; i++) {
        GIBaseInfo *info;
        PyObject *py_info;

        info = (GIBaseInfo *)g_struct_info_get_method((GIStructInfo *)self->info, i);
        g_assert(info != NULL);

        py_info = pyg_info_new(info);

        g_base_info_unref(info);

        if (py_info == NULL) {
            Py_CLEAR(infos);
            break;
        }

        PyTuple_SET_ITEM(infos, i, py_info);
    }

    return infos;
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
    { "get_fields", (PyCFunction)_wrap_g_struct_info_get_fields, METH_NOARGS },
    { "get_methods", (PyCFunction)_wrap_g_struct_info_get_methods, METH_NOARGS },
    { "new_buffer", (PyCFunction)_wrap_g_struct_info_new_buffer, METH_NOARGS },
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
    { "get_values", (PyCFunction)_wrap_g_enum_info_get_values, METH_NOARGS },
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
    { "get_parent", (PyCFunction)_wrap_g_object_info_get_parent, METH_NOARGS },
    { "get_methods", (PyCFunction)_wrap_g_object_info_get_methods, METH_NOARGS },
    { "get_fields", (PyCFunction)_wrap_g_object_info_get_fields, METH_NOARGS },
    { "get_interfaces", (PyCFunction)_wrap_g_object_info_get_interfaces, METH_NOARGS },
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
    { "get_methods", (PyCFunction)_wrap_g_interface_info_get_methods, METH_NOARGS },
    { "register", (PyCFunction)_wrap_g_interface_info_register, METH_NOARGS },
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
    { "get_value", (PyCFunction)_wrap_g_value_info_get_value, METH_NOARGS },
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

    if (!PyArg_ParseTuple(args, "O:FieldInfo.get_value", &object)) {
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
    retval = pygi_g_argument_to_py_object(&value, field_type_info);

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

    if (!PyArg_ParseTuple(args, "OO:FieldInfo.set_value", &object, &py_value)) {
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

    value = pygi_g_argument_from_py_object(py_value, field_type_info, GI_TRANSFER_NOTHING);

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
    { "get_value", (PyCFunction)_wrap_g_field_info_get_value, METH_VARARGS },
    { "set_value", (PyCFunction)_wrap_g_field_info_set_value, METH_VARARGS },
    { NULL, NULL, 0 }
};


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
    REGISTER_TYPE(m, PyGIRegisteredTypeInfo_Type, "RegisteredTypeInfo");
    REGISTER_TYPE(m, PyGIStructInfo_Type, "StructInfo");
    REGISTER_TYPE(m, PyGIEnumInfo_Type, "EnumInfo");
    REGISTER_TYPE(m, PyGIObjectInfo_Type, "ObjectInfo");
    REGISTER_TYPE(m, PyGIInterfaceInfo_Type, "InterfaceInfo");
    REGISTER_TYPE(m, PyGIValueInfo_Type, "ValueInfo");
    REGISTER_TYPE(m, PyGIFieldInfo_Type, "FieldInfo");

#undef REGISTER_TYPE
}

#undef PYGIINFO_DEFINE_TYPE
