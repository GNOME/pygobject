/* -*- Mode: C; c-basic-offset: 4 -*-
 * vim: tabstop=4 shiftwidth=4 expandtab
 *
 * Copyright (C) 2005-2009 Johan Dahlin <johan@gnome.org>
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
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301
 * USA
 */

#include "pygi-private.h"

#include <pygobject.h>

#define _PyGI_DEFINE_INFO_TYPE(name, cname, base) \
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
    offsetof(PyGIBaseInfo, inst_weakreflist), /* tp_weaklistoffset */ \
    (getiterfunc)NULL,                        /* tp_iter */ \
    (iternextfunc)NULL,                       /* tp_iternext */ \
    _Py##cname##_methods,                     /* tp_methods */ \
    NULL,                                     /* tp_members */ \
    NULL,                                     /* tp_getset */ \
    &base                                     /* tp_base */ \
}


/* BaseInfo */

static void
_base_info_dealloc (PyGIBaseInfo *self)
{
    PyObject_GC_UnTrack((PyObject *)self);

    PyObject_ClearWeakRefs((PyObject *)self);

    if (self->info) {
        g_base_info_unref(self->info);
        self->info = NULL;
    }

    self->ob_type->tp_free((PyObject *)self);
}

static int
_base_info_traverse (PyGIBaseInfo *self,
                     visitproc     visit,
                     void         *arg)
{
    return 0;
}

static PyObject *
_base_info_repr (PyGIBaseInfo *self)
{
    return PyString_FromFormat("<%s object (%s) at 0x%p>",
            self->ob_type->tp_name, g_base_info_get_name(self->info), (void *)self);
}

static PyMethodDef _PyGIBaseInfo_methods[];

PyTypeObject PyGIBaseInfo_Type = {
    PyObject_HEAD_INIT(NULL)
    0,
    "gi.BaseInfo",                             /* tp_name */
    sizeof(PyGIBaseInfo),                      /* tp_basicsize */
    0,                                         /* tp_itemsize */
    (destructor)_base_info_dealloc,        /* tp_dealloc */
    (printfunc)NULL,                           /* tp_print */
    (getattrfunc)NULL,                         /* tp_getattr */
    (setattrfunc)NULL,                         /* tp_setattr */
    (cmpfunc)NULL,                             /* tp_compare */
    (reprfunc)_base_info_repr,             /* tp_repr */
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
    (traverseproc)_base_info_traverse,     /* tp_traverse */
    (inquiry)NULL,                             /* tp_clear */
    (richcmpfunc)NULL,                         /* tp_richcompare */
    offsetof(PyGIBaseInfo, inst_weakreflist),  /* tp_weaklistoffset */
    (getiterfunc)NULL,                         /* tp_iter */
    (iternextfunc)NULL,                        /* tp_iternext */
    _PyGIBaseInfo_methods,                     /* tp_methods */
};

static PyObject *
_wrap_g_base_info_get_name (PyGIBaseInfo *self)
{
    return PyString_FromString(g_base_info_get_name(self->info));
}

static PyObject *
_wrap_g_base_info_get_namespace (PyGIBaseInfo *self)
{
    return PyString_FromString(g_base_info_get_namespace(self->info));
}

static PyMethodDef _PyGIBaseInfo_methods[] = {
    { "get_name", (PyCFunction)_wrap_g_base_info_get_name, METH_NOARGS },
    { "get_namespace", (PyCFunction)_wrap_g_base_info_get_namespace, METH_NOARGS },
    { NULL, NULL, 0 }
};

PyObject *
_pygi_info_new (GIBaseInfo *info)
{
    GIInfoType info_type;
    PyTypeObject *type = NULL;
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

    self = (PyGIBaseInfo *)type->tp_alloc(type, 0);
    if (self == NULL) {
        return NULL;
    }

    self->info = g_base_info_ref(info);

    return (PyObject *)self;
}


/* CallableInfo */
_PyGI_DEFINE_INFO_TYPE("CallableInfo", GICallableInfo, PyGIBaseInfo_Type);

static PyMethodDef _PyGICallableInfo_methods[] = {
    { NULL, NULL, 0 }
};


/* FunctionInfo */
_PyGI_DEFINE_INFO_TYPE("FunctionInfo", GIFunctionInfo, PyGICallableInfo_Type);

static PyObject *
_wrap_g_function_info_is_constructor (PyGIBaseInfo *self)
{
    GIFunctionInfoFlags flags;
    gboolean is_constructor;

    flags = g_function_info_get_flags((GIFunctionInfo*)self->info);
    is_constructor = flags & GI_FUNCTION_IS_CONSTRUCTOR;

    return PyBool_FromLong(is_constructor);
}

static PyObject *
_wrap_g_function_info_is_method (PyGIBaseInfo *self)
{
    GIFunctionInfoFlags flags;
    gboolean is_method;

    flags = g_function_info_get_flags((GIFunctionInfo*)self->info);
    is_method = flags & GI_FUNCTION_IS_METHOD;

    return PyBool_FromLong(is_method);
}

static PyObject *
_wrap_g_function_info_invoke (PyGIBaseInfo *self,
                              PyObject     *py_args)
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

    glong error_arg_pos;

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

    error_arg_pos = -1;

    arg_infos = g_newa(GIArgInfo *, n_args);
    arg_type_infos = g_newa(GITypeInfo *, n_args);

    args_is_auxiliary = g_newa(gboolean, n_args);
    memset(args_is_auxiliary, 0, sizeof(args_is_auxiliary) * n_args);

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

        switch (arg_type_tag) {
            case GI_TYPE_TAG_ARRAY:
            {
                gint length_arg_pos;

                length_arg_pos = g_type_info_get_array_length(arg_type_infos[i]);
                if (length_arg_pos < 0) {
                    break;
                }

                g_assert(length_arg_pos < n_args);
                args_is_auxiliary[length_arg_pos] = TRUE;

                if (direction == GI_DIRECTION_IN || direction == GI_DIRECTION_INOUT) {
                    n_aux_in_args += 1;
                }
                if (direction == GI_DIRECTION_OUT || direction == GI_DIRECTION_INOUT) {
                    n_aux_out_args += 1;
                }

                break;
            }
            case GI_TYPE_TAG_ERROR:
                if (error_arg_pos >= 0) {
                    PyErr_WarnEx(NULL, "Two or more error arguments; taking the last one", 1);
                }
                error_arg_pos = i;
                break;
            default:
                break;
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

        n_py_args_expected = n_in_args
            + (is_constructor ? 1 : 0)
            - n_aux_in_args
            - (error_arg_pos >= 0 ? 1 : 0);

        if (n_py_args != n_py_args_expected) {
            gchar *fullname;
            fullname = _pygi_g_base_info_get_fullname(self->info);
            if (fullname != NULL) {
                PyErr_Format(PyExc_TypeError,
                    "%s() takes exactly %zd argument(s) (%zd given)",
                    fullname, n_py_args_expected, n_py_args);
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
            retval = _pygi_g_registered_type_info_check_object(
                    (GIRegisteredTypeInfo *)container_info, is_method, py_arg);

            if (retval < 0) {
                goto return_;
            } else if (!retval) {
                gchar *fullname;
                fullname = _pygi_g_base_info_get_fullname(self->info);
                if (fullname != NULL) {
                    _PyGI_ERROR_PREFIX("%s() argument %zd: ", fullname, py_args_pos);
                    g_free(fullname);
                }
                goto return_;
            }

            py_args_pos += 1;
        }

        for (i = 0; i < n_args; i++) {
            GIDirection direction;
            GITypeTag type_tag;
            gboolean may_be_null;
            PyObject *py_arg;
            gint retval;

            direction = g_arg_info_get_direction(arg_infos[i]);
            type_tag = g_type_info_get_tag(arg_type_infos[i]);

            if (direction == GI_DIRECTION_OUT
                    || args_is_auxiliary[i]
                    || type_tag == GI_TYPE_TAG_ERROR) {
                continue;
            }

            g_assert(py_args_pos < n_py_args);
            py_arg = PyTuple_GET_ITEM(py_args, py_args_pos);

            may_be_null = g_arg_info_may_be_null(arg_infos[i]);

            retval = _pygi_g_type_info_check_object(arg_type_infos[i],
                    may_be_null, py_arg);

            if (retval < 0) {
                goto return_;
            } else if (!retval) {
                gchar *fullname;
                fullname = _pygi_g_base_info_get_fullname(self->info);
                if (fullname != NULL) {
                    _PyGI_ERROR_PREFIX("%s() argument %zd: ",
                        _pygi_g_base_info_get_fullname(self->info),
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
                    in_args[0].v_pointer = pyg_boxed_get(py_arg, void);
                    break;
                case GI_INFO_TYPE_OBJECT:
                case GI_INFO_TYPE_INTERFACE:
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

                arg_type_tag = g_type_info_get_tag(arg_type_infos[i]);

                if (arg_type_tag == GI_TYPE_TAG_ERROR) {
                    GError **error;

                    error = g_slice_new(GError *);
                    *error = NULL;

                    args[i]->v_pointer = error;
                    continue;
                }

                transfer = g_arg_info_get_ownership_transfer(arg_infos[i]);

                g_assert(py_args_pos < n_py_args);
                py_arg = PyTuple_GET_ITEM(py_args, py_args_pos);

                *args[i] = _pygi_argument_from_object(py_arg, arg_type_infos[i],
                        transfer);

                if (PyErr_Occurred()) {
                    /* TODO: Release ressources allocated for previous arguments. */
                    return NULL;
                }

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

            fullname = _pygi_g_base_info_get_fullname(self->info);
            if (fullname != NULL) {
                /* FIXME: Raise the right error, out of the error domain. */
                PyErr_Format(PyExc_RuntimeError, "Error invoking %s(): %s",
                    fullname, error->message);
                g_free(fullname);
            }

            g_error_free(error);

            /* TODO: Release input arguments. */

            goto return_;
        }
    }

    if (error_arg_pos >= 0) {
        GError **error;

        error = args[error_arg_pos]->v_pointer;

        if (*error != NULL) {
            /* TODO: Raises the right error, out of the error domain, if
             * applicable. */
            PyErr_SetString(PyExc_Exception, (*error)->message);

            /* TODO: Release input arguments. */

            goto return_;
        }
    }

    /* Convert the return value. */
    if (is_constructor) {
        PyTypeObject *py_type;
        GIBaseInfo *info;
        GIInfoType info_type;
        GITransfer transfer;

        g_assert(n_py_args > 0);
        py_type = (PyTypeObject *)PyTuple_GET_ITEM(py_args, 0);

        info = g_type_info_get_interface(return_type_info);
        g_assert(info != NULL);

        info_type = g_base_info_get_type(info);

        g_base_info_unref(info);

        transfer = g_callable_info_get_caller_owns((GICallableInfo *)self->info);

        switch (info_type) {
            case GI_INFO_TYPE_UNION:
                PyErr_SetString(PyExc_NotImplementedError, "creating unions is not supported yet");
                /* TODO */
                goto return_;
            case GI_INFO_TYPE_STRUCT:
            {
                GType type;

                type = g_registered_type_info_get_g_type((GIRegisteredTypeInfo *)info);

                if (transfer == GI_TRANSFER_EVERYTHING && !g_type_is_a(type, G_TYPE_BOXED)) {
                    gboolean is_simple;

                    is_simple = _pygi_g_struct_info_is_simple((GIStructInfo *)info);

                    if (is_simple) {
                        PyErr_Format(PyExc_TypeError,
                                "cannot create '%s' instances; non-boxed simple structures do not accept specific constructors",
                                py_type->tp_name);
                        /* TODO */
                        goto return_;
                    }
                }
                return_value = pygi_boxed_new_from_type(py_type, return_arg.v_pointer, transfer == GI_TRANSFER_EVERYTHING);
                break;
            }
            case GI_INFO_TYPE_OBJECT:
                return_value = pygobject_new_from_type(py_type, return_arg.v_pointer, TRUE);
                break;
            case GI_INFO_TYPE_INTERFACE:
                /* Isn't instantiable. */
            default:
                /* Other types don't have methods. */
                g_assert_not_reached();
        }

        if (return_value == NULL) {
            /* TODO */
            goto return_;
        }
    } else {
        GITransfer transfer;

        if (return_type_tag == GI_TYPE_TAG_ARRAY) {
            GArray *array;

            array = _pygi_argument_to_array(&return_arg, args, return_type_info);
            if (array == NULL) {
                /* TODO */
                goto return_;
            }

            return_arg.v_pointer = array;
        }

        transfer = g_callable_info_get_caller_owns((GICallableInfo *)self->info);

        return_value = _pygi_argument_to_object(&return_arg, return_type_info, transfer);
        if (return_value == NULL) {
            /* TODO */
            goto return_;
        }

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

                array = _pygi_argument_to_array(args[i], args, arg_type_infos[i]);
                if (array == NULL) {
                    /* TODO */
                    goto return_;
                }

                args[i]->v_pointer = array;
            }

            if (direction == GI_DIRECTION_INOUT || direction == GI_DIRECTION_OUT) {
                /* Convert the argument. */
                PyObject *obj;

                obj = _pygi_argument_to_object(args[i], arg_type_infos[i], transfer);
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
                    _pygi_argument_release(&containers[containers_pos], arg_type_infos[i],
                        GI_TRANSFER_NOTHING, GI_DIRECTION_IN);
                    containers_pos += 1;
                }
                if (transfer != GI_TRANSFER_NOTHING) {
                    _pygi_argument_release(args[i], arg_type_infos[i], transfer,
                        GI_DIRECTION_OUT);
                }
            } else if (direction == GI_DIRECTION_IN && transfer == GI_TRANSFER_CONTAINER) {
                g_assert(containers_pos < n_containers);
                _pygi_argument_release(&containers[containers_pos], arg_type_infos[i],
                    GI_TRANSFER_NOTHING, direction);
                containers_pos += 1;
            } else {
                _pygi_argument_release(args[i], arg_type_infos[i], transfer, direction);
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
_PyGI_DEFINE_INFO_TYPE("RegisteredTypeInfo", GIRegisteredTypeInfo, PyGIBaseInfo_Type);

static PyObject *
_wrap_g_registered_type_info_get_g_type (PyGIBaseInfo *self)
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
_PyGI_DEFINE_INFO_TYPE("StructInfo", GIStructInfo, PyGIRegisteredTypeInfo_Type);

static PyObject *
_wrap_g_struct_info_get_fields (PyGIBaseInfo *self)
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

        py_info = _pygi_info_new(info);

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
_wrap_g_struct_info_get_methods (PyGIBaseInfo *self)
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

        py_info = _pygi_info_new(info);

        g_base_info_unref(info);

        if (py_info == NULL) {
            Py_CLEAR(infos);
            break;
        }

        PyTuple_SET_ITEM(infos, i, py_info);
    }

    return infos;
}

static PyMethodDef _PyGIStructInfo_methods[] = {
    { "get_fields", (PyCFunction)_wrap_g_struct_info_get_fields, METH_NOARGS },
    { "get_methods", (PyCFunction)_wrap_g_struct_info_get_methods, METH_NOARGS },
    { NULL, NULL, 0 }
};


/* EnumInfo */
_PyGI_DEFINE_INFO_TYPE("EnumInfo", GIEnumInfo, PyGIRegisteredTypeInfo_Type);

static PyObject *
_wrap_g_enum_info_get_values (PyGIBaseInfo *self)
{
    gssize n_infos;
    PyObject *infos;
    gssize i;

    n_infos = g_enum_info_get_n_values((GIEnumInfo *)self->info);

    infos = PyTuple_New(n_infos);
    if (infos == NULL) {
        return NULL;
    }

    for (i = 0; i < n_infos; i++) {
        GIBaseInfo *info;
        PyObject *py_info;

        info = (GIBaseInfo *)g_enum_info_get_value((GIEnumInfo *)self->info, i);
        g_assert(info != NULL);

        py_info = _pygi_info_new(info);

        g_base_info_unref(info);

        if (py_info == NULL) {
            Py_CLEAR(infos);
            break;
        }

        PyTuple_SET_ITEM(infos, i, py_info);
    }

    return infos;
}

static PyMethodDef _PyGIEnumInfo_methods[] = {
    { "get_values", (PyCFunction)_wrap_g_enum_info_get_values, METH_NOARGS },
    { NULL, NULL, 0 }
};


/* ObjectInfo */
_PyGI_DEFINE_INFO_TYPE("ObjectInfo", GIObjectInfo, PyGIRegisteredTypeInfo_Type);

static PyObject *
_wrap_g_object_info_get_parent (PyGIBaseInfo *self)
{
    GIBaseInfo *info;
    PyObject *py_info;

    info = (GIBaseInfo *)g_object_info_get_parent((GIObjectInfo*)self->info);

    if (info == NULL) {
        Py_RETURN_NONE;
    }

    py_info = _pygi_info_new(info);

    g_base_info_unref(info);

    return py_info;
}

static PyObject *
_wrap_g_object_info_get_methods (PyGIBaseInfo *self)
{
    gssize n_infos;
    PyObject *infos;
    gssize i;

    n_infos = g_object_info_get_n_methods((GIObjectInfo *)self->info);

    infos = PyTuple_New(n_infos);
    if (infos == NULL) {
        return NULL;
    }

    for (i = 0; i < n_infos; i++) {
        GIBaseInfo *info;
        PyObject *py_info;

        info = (GIBaseInfo *)g_object_info_get_method((GIObjectInfo *)self->info, i);
        g_assert(info != NULL);

        py_info = _pygi_info_new(info);

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
_wrap_g_object_info_get_fields (PyGIBaseInfo *self)
{
    gssize n_infos;
    PyObject *infos;
    gssize i;

    n_infos = g_object_info_get_n_fields((GIObjectInfo *)self->info);

    infos = PyTuple_New(n_infos);
    if (infos == NULL) {
        return NULL;
    }

    for (i = 0; i < n_infos; i++) {
        GIBaseInfo *info;
        PyObject *py_info;

        info = (GIBaseInfo *)g_object_info_get_field((GIObjectInfo *)self->info, i);
        g_assert(info != NULL);

        py_info = _pygi_info_new(info);

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
_wrap_g_object_info_get_interfaces (PyGIBaseInfo *self)
{
    gssize n_infos;
    PyObject *infos;
    gssize i;

    n_infos = g_object_info_get_n_interfaces((GIObjectInfo *)self->info);

    infos = PyTuple_New(n_infos);
    if (infos == NULL) {
        return NULL;
    }

    for (i = 0; i < n_infos; i++) {
        GIBaseInfo *info;
        PyObject *py_info;

        info = (GIBaseInfo *)g_object_info_get_interface((GIObjectInfo *)self->info, i);
        g_assert(info != NULL);

        py_info = _pygi_info_new(info);

        g_base_info_unref(info);

        if (py_info == NULL) {
            Py_CLEAR(infos);
            break;
        }

        PyTuple_SET_ITEM(infos, i, py_info);
    }

    return infos;
}

static PyMethodDef _PyGIObjectInfo_methods[] = {
    { "get_parent", (PyCFunction)_wrap_g_object_info_get_parent, METH_NOARGS },
    { "get_methods", (PyCFunction)_wrap_g_object_info_get_methods, METH_NOARGS },
    { "get_fields", (PyCFunction)_wrap_g_object_info_get_fields, METH_NOARGS },
    { "get_interfaces", (PyCFunction)_wrap_g_object_info_get_interfaces, METH_NOARGS },
    { NULL, NULL, 0 }
};


/* GIInterfaceInfo */
_PyGI_DEFINE_INFO_TYPE("InterfaceInfo", GIInterfaceInfo, PyGIRegisteredTypeInfo_Type);

static PyObject *
_wrap_g_interface_info_get_methods (PyGIBaseInfo *self)
{
    gssize n_infos;
    PyObject *infos;
    gssize i;

    n_infos = g_interface_info_get_n_methods((GIInterfaceInfo *)self->info);

    infos = PyTuple_New(n_infos);
    if (infos == NULL) {
        return NULL;
    }

    for (i = 0; i < n_infos; i++) {
        GIBaseInfo *info;
        PyObject *py_info;

        info = (GIBaseInfo *)g_interface_info_get_method((GIInterfaceInfo *)self->info, i);
        g_assert(info != NULL);

        py_info = _pygi_info_new(info);

        g_base_info_unref(info);

        if (py_info == NULL) {
            Py_CLEAR(infos);
            break;
        }

        PyTuple_SET_ITEM(infos, i, py_info);
    }

    return infos;
}

static void
initialize_interface (GTypeInterface *iface,
                      PyTypeObject   *pytype)
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
_wrap_g_interface_info_register (PyGIBaseInfo *self)
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
_PyGI_DEFINE_INFO_TYPE("ValueInfo", GIValueInfo, PyGIBaseInfo_Type);

static PyObject *
_wrap_g_value_info_get_value (PyGIBaseInfo *self)
{
    glong value;

    value = g_value_info_get_value((GIValueInfo *)self->info);

    return PyInt_FromLong(value);
}


static PyMethodDef _PyGIValueInfo_methods[] = {
    { "get_value", (PyCFunction)_wrap_g_value_info_get_value, METH_NOARGS },
    { NULL, NULL, 0 }
};


/* GIFieldInfo */
_PyGI_DEFINE_INFO_TYPE("FieldInfo", GIFieldInfo, PyGIBaseInfo_Type);

static PyObject *
_wrap_g_field_info_get_value (PyGIBaseInfo *self,
                              PyObject     *args)
{
    PyObject *instance;
    GIBaseInfo *container_info;
    GIInfoType container_info_type;
    gpointer pointer;
    GITypeInfo *field_type_info;
    GArgument value;
    PyObject *py_value = NULL;

    if (!PyArg_ParseTuple(args, "O:FieldInfo.get_value", &instance)) {
        return NULL;
    }

    container_info = g_base_info_get_container(self->info);
    g_assert(container_info != NULL);

    /* Check the instance. */
    if (!_pygi_g_registered_type_info_check_object((GIRegisteredTypeInfo *)container_info, TRUE, instance)) {
        _PyGI_ERROR_PREFIX("argument 1: ");
        return NULL;
    }

    /* Get the pointer to the container. */
    container_info_type = g_base_info_get_type(container_info);
    switch (container_info_type) {
        case GI_INFO_TYPE_UNION:
            PyErr_SetString(PyExc_NotImplementedError, "getting a field from an union is not supported yet");
            return NULL;
        case GI_INFO_TYPE_STRUCT:
            pointer = pyg_boxed_get(instance, void);
            break;
        case GI_INFO_TYPE_OBJECT:
            pointer = pygobject_get(instance);
            break;
        default:
            /* Other types don't have fields. */
            g_assert_not_reached();
    }

    /* Get the field's value. */
    field_type_info = g_field_info_get_type((GIFieldInfo *)self->info);

    /* A few types are not handled by g_field_info_get_field, so do it here. */
    if (!g_type_info_is_pointer(field_type_info)
            && g_type_info_get_tag(field_type_info) == GI_TYPE_TAG_INTERFACE) {
        GIBaseInfo *info;
        GIInfoType info_type;

        if (!(g_field_info_get_flags((GIFieldInfo *)self->info) & GI_FIELD_IS_READABLE)) {
            PyErr_SetString(PyExc_RuntimeError, "field is not readable");
            goto out;
        }

        info = g_type_info_get_interface(field_type_info);

        info_type = g_base_info_get_type(info);

        g_base_info_unref(info);

        switch(info_type) {
            case GI_INFO_TYPE_UNION:
                PyErr_SetString(PyExc_NotImplementedError, "getting an union is not supported yet");
                goto out;
            case GI_INFO_TYPE_STRUCT:
            {
                gsize offset;

                offset = g_field_info_get_offset((GIFieldInfo *)self->info);

                value.v_pointer = pointer + offset;

                goto argument_to_object;
            }
            default:
                /* Fallback. */
                break;
        }
    }

    if (!g_field_info_get_field((GIFieldInfo *)self->info, pointer, &value)) {
        PyErr_SetString(PyExc_RuntimeError, "unable to get the value");
        goto out;
    }

argument_to_object:
    py_value = _pygi_argument_to_object(&value, field_type_info, GI_TRANSFER_NOTHING);

out:
    g_base_info_unref((GIBaseInfo *)field_type_info);

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
    GArgument value;
    PyObject *retval = NULL;

    if (!PyArg_ParseTuple(args, "OO:FieldInfo.set_value", &instance, &py_value)) {
        return NULL;
    }

    container_info = g_base_info_get_container(self->info);
    g_assert(container_info != NULL);

    /* Check the instance. */
    if (!_pygi_g_registered_type_info_check_object((GIRegisteredTypeInfo *)container_info, TRUE, instance)) {
        _PyGI_ERROR_PREFIX("argument 1: ");
        return NULL;
    }

    /* Get the pointer to the container. */
    container_info_type = g_base_info_get_type(container_info);
    switch (container_info_type) {
        case GI_INFO_TYPE_UNION:
            PyErr_SetString(PyExc_NotImplementedError, "setting a field in an union is not supported yet");
            return NULL;
        case GI_INFO_TYPE_STRUCT:
            pointer = pyg_boxed_get(instance, void);
            break;
        case GI_INFO_TYPE_OBJECT:
            pointer = pygobject_get(instance);
            break;
        default:
            /* Other types don't have fields. */
            g_assert_not_reached();
    }

    field_type_info = g_field_info_get_type((GIFieldInfo *)self->info);

    /* Check the value. */
    {
        gboolean retval;

        retval = _pygi_g_type_info_check_object(field_type_info, TRUE, py_value);
        if (retval < 0) {
            goto out;
        }

        if (!retval) {
            _PyGI_ERROR_PREFIX("argument 2: ");
            goto out;
        }
    }

    /* Set the field's value. */
    /* A few types are not handled by g_field_info_set_field, so do it here. */
    if (!g_type_info_is_pointer(field_type_info)
            && g_type_info_get_tag(field_type_info) == GI_TYPE_TAG_INTERFACE) {
        GIBaseInfo *info;
        GIInfoType info_type;

        if (!(g_field_info_get_flags((GIFieldInfo *)self->info) & GI_FIELD_IS_WRITABLE)) {
            PyErr_SetString(PyExc_RuntimeError, "field is not writable");
            goto out;
        }

        info = g_type_info_get_interface(field_type_info);

        info_type = g_base_info_get_type(info);

        switch (info_type) {
            case GI_INFO_TYPE_UNION:
                PyErr_SetString(PyExc_NotImplementedError, "setting an union is not supported yet");
                goto out;
            case GI_INFO_TYPE_STRUCT:
            {
                gboolean is_simple;
                gsize offset;
                gssize size;

                is_simple = _pygi_g_struct_info_is_simple((GIStructInfo *)info);

                if (!is_simple) {
                    PyErr_SetString(PyExc_TypeError,
                            "cannot set a structure which has no well-defined ownership transfer rules");
                    g_base_info_unref(info);
                    goto out;
                }

                value = _pygi_argument_from_object(py_value, field_type_info, GI_TRANSFER_NOTHING);
                if (PyErr_Occurred()) {
                    g_base_info_unref(info);
                    goto out;
                }

                offset = g_field_info_get_offset((GIFieldInfo *)self->info);
                size = g_struct_info_get_size((GIStructInfo *)info);
                g_assert(size > 0);

                g_memmove(pointer + offset, value.v_pointer, size);

                g_base_info_unref(info);

                retval = Py_None;
                goto out;
            }
            default:
                /* Fallback. */
                break;
        }

        g_base_info_unref(info);
    }

    value = _pygi_argument_from_object(py_value, field_type_info, GI_TRANSFER_EVERYTHING);
    if (PyErr_Occurred()) {
        goto out;
    }

    if (!g_field_info_set_field((GIFieldInfo *)self->info, pointer, &value)) {
        _pygi_argument_release(&value, field_type_info, GI_TRANSFER_NOTHING, GI_DIRECTION_IN);
        PyErr_SetString(PyExc_RuntimeError, "unable to set value for field");
        goto out;
    }

    retval = Py_None;

out:
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
_PyGI_DEFINE_INFO_TYPE("UnresolvedInfo", GIUnresolvedInfo, PyGIBaseInfo_Type);

static PyMethodDef _PyGIUnresolvedInfo_methods[] = {
    { NULL, NULL, 0 }
};

/* Private */

gchar *
_pygi_g_base_info_get_fullname (GIBaseInfo *info)
{
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

void
_pygi_info_register_types (PyObject *m)
{
#define _PyGI_REGISTER_TYPE(m, type, name) \
    type.ob_type = &PyType_Type; \
    if (PyType_Ready(&type)) \
        return; \
    if (PyModule_AddObject(m, name, (PyObject *)&type)) \
        return

    _PyGI_REGISTER_TYPE(m, PyGIBaseInfo_Type, "BaseInfo");
    _PyGI_REGISTER_TYPE(m, PyGIUnresolvedInfo_Type, "UnresolvedInfo");
    _PyGI_REGISTER_TYPE(m, PyGICallableInfo_Type, "CallableInfo");
    _PyGI_REGISTER_TYPE(m, PyGIFunctionInfo_Type, "FunctionInfo");
    _PyGI_REGISTER_TYPE(m, PyGIRegisteredTypeInfo_Type, "RegisteredTypeInfo");
    _PyGI_REGISTER_TYPE(m, PyGIStructInfo_Type, "StructInfo");
    _PyGI_REGISTER_TYPE(m, PyGIEnumInfo_Type, "EnumInfo");
    _PyGI_REGISTER_TYPE(m, PyGIObjectInfo_Type, "ObjectInfo");
    _PyGI_REGISTER_TYPE(m, PyGIInterfaceInfo_Type, "InterfaceInfo");
    _PyGI_REGISTER_TYPE(m, PyGIValueInfo_Type, "ValueInfo");
    _PyGI_REGISTER_TYPE(m, PyGIFieldInfo_Type, "FieldInfo");

#undef _PyGI_REGISTER_TYPE
}
