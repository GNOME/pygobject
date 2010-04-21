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

    g_base_info_unref(self->info);

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

static PyObject *
_wrap_g_base_info_get_container (PyGIBaseInfo *self)
{
    GIBaseInfo *info;

    info = g_base_info_get_container(self->info);

    if (info == NULL) {
        Py_RETURN_NONE;
    }

    return _pygi_info_new(info);
}


static PyMethodDef _PyGIBaseInfo_methods[] = {
    { "get_name", (PyCFunction)_wrap_g_base_info_get_name, METH_NOARGS },
    { "get_namespace", (PyCFunction)_wrap_g_base_info_get_namespace, METH_NOARGS },
    { "get_container", (PyCFunction)_wrap_g_base_info_get_container, METH_NOARGS },
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
            type = &PyGIConstantInfo_Type;
            break;
        case GI_INFO_TYPE_ERROR_DOMAIN:
            PyErr_SetString(PyExc_NotImplementedError, "GIErrorDomainInfo bindings not implemented");
            return NULL;
        case GI_INFO_TYPE_UNION:
            type = &PyGIUnionInfo_Type;
            break;
        case GI_INFO_TYPE_VALUE:
            type = &PyGIValueInfo_Type;
            break;
        case GI_INFO_TYPE_SIGNAL:
            PyErr_SetString(PyExc_NotImplementedError, "GISignalInfo bindings not implemented");
            return NULL;
        case GI_INFO_TYPE_VFUNC:
            type = &PyGIVFuncInfo_Type;
            break;
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

GIBaseInfo *
_pygi_object_get_gi_info (PyObject     *object,
                          PyTypeObject *type)
{
    PyObject *py_info;
    GIBaseInfo *info = NULL;

    py_info = PyObject_GetAttrString(object, "__info__");
    if (py_info == NULL) {
        return NULL;
    }
    if (!PyObject_TypeCheck(py_info, type)) {
        PyErr_Format(PyExc_TypeError, "attribute '__info__' must be %s, not %s",
            type->tp_name, py_info->ob_type->tp_name);
        goto out;
    }

    info = ((PyGIBaseInfo *)py_info)->info;
    g_base_info_ref(info);

out:
    Py_DECREF(py_info);

    return info;
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

gsize
_pygi_g_type_tag_size (GITypeTag type_tag)
{
    gsize size = 0;

    switch(type_tag) {
        case GI_TYPE_TAG_BOOLEAN:
            size = sizeof(gboolean);
            break;
        case GI_TYPE_TAG_INT8:
        case GI_TYPE_TAG_UINT8:
            size = sizeof(gint8);
            break;
        case GI_TYPE_TAG_INT16:
        case GI_TYPE_TAG_UINT16:
            size = sizeof(gint16);
            break;
        case GI_TYPE_TAG_INT32:
        case GI_TYPE_TAG_UINT32:
            size = sizeof(gint32);
            break;
        case GI_TYPE_TAG_INT64:
        case GI_TYPE_TAG_UINT64:
            size = sizeof(gint64);
            break;
        case GI_TYPE_TAG_SHORT:
        case GI_TYPE_TAG_USHORT:
            size = sizeof(gshort);
            break;
        case GI_TYPE_TAG_INT:
        case GI_TYPE_TAG_UINT:
            size = sizeof(gint);
            break;
        case GI_TYPE_TAG_LONG:
        case GI_TYPE_TAG_ULONG:
            size = sizeof(glong);
            break;
        case GI_TYPE_TAG_SIZE:
        case GI_TYPE_TAG_SSIZE:
            size = sizeof(gsize);
            break;
        case GI_TYPE_TAG_FLOAT:
            size = sizeof(gfloat);
            break;
        case GI_TYPE_TAG_DOUBLE:
            size = sizeof(gdouble);
            break;
        case GI_TYPE_TAG_TIME_T:
            size = sizeof(time_t);
            break;
        case GI_TYPE_TAG_GTYPE:
            size = sizeof(GType);
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
            PyErr_Format(PyExc_TypeError,
                "Unable to know the size (assuming %s is not a pointer)",
                g_type_tag_to_string(type_tag));
            break;
    }

    return size;
}

gsize
_pygi_g_type_info_size (GITypeInfo *type_info)
{
    gsize size = 0;

    GITypeTag type_tag;

    type_tag = g_type_info_get_tag(type_info);
    switch(type_tag) {
        case GI_TYPE_TAG_BOOLEAN:
        case GI_TYPE_TAG_INT8:
        case GI_TYPE_TAG_UINT8:
        case GI_TYPE_TAG_INT16:
        case GI_TYPE_TAG_UINT16:
        case GI_TYPE_TAG_INT32:
        case GI_TYPE_TAG_UINT32:
        case GI_TYPE_TAG_INT64:
        case GI_TYPE_TAG_UINT64:
        case GI_TYPE_TAG_SHORT:
        case GI_TYPE_TAG_USHORT:
        case GI_TYPE_TAG_INT:
        case GI_TYPE_TAG_UINT:
        case GI_TYPE_TAG_LONG:
        case GI_TYPE_TAG_ULONG:
        case GI_TYPE_TAG_SIZE:
        case GI_TYPE_TAG_SSIZE:
        case GI_TYPE_TAG_FLOAT:
        case GI_TYPE_TAG_DOUBLE:
        case GI_TYPE_TAG_TIME_T:
        case GI_TYPE_TAG_GTYPE:
            if (g_type_info_is_pointer(type_info)) {
                size = sizeof(gpointer);
            } else {
                size = _pygi_g_type_tag_size(type_tag);
                g_assert(size > 0);
            }
            break;
        case GI_TYPE_TAG_INTERFACE:
        {
            GIBaseInfo *info;
            GIInfoType info_type;

            info = g_type_info_get_interface(type_info);
            info_type = g_base_info_get_type(info);

            switch (info_type) {
                case GI_INFO_TYPE_STRUCT:
                    if (g_type_info_is_pointer(type_info)) {
                        size = sizeof(gpointer);
                    } else {
                        size = g_struct_info_get_size((GIStructInfo *)info);
                    }
                    break;
                case GI_INFO_TYPE_UNION:
                    if (g_type_info_is_pointer(type_info)) {
                        size = sizeof(gpointer);
                    } else {
                        size = g_union_info_get_size((GIUnionInfo *)info);
                    }
                    break;
                case GI_INFO_TYPE_ENUM:
                case GI_INFO_TYPE_FLAGS:
                    if (g_type_info_is_pointer(type_info)) {
                        size = sizeof(gpointer);
                    } else {
                        GITypeTag type_tag;

                        type_tag = g_enum_info_get_storage_type((GIEnumInfo *)info);
                        size = _pygi_g_type_tag_size(type_tag);
                    }
                    break;
                case GI_INFO_TYPE_BOXED:
                case GI_INFO_TYPE_OBJECT:
                case GI_INFO_TYPE_INTERFACE:
                case GI_INFO_TYPE_CALLBACK:
                    size = sizeof(gpointer);
                    break;
                case GI_INFO_TYPE_VFUNC:
                case GI_INFO_TYPE_INVALID:
                case GI_INFO_TYPE_FUNCTION:
                case GI_INFO_TYPE_CONSTANT:
                case GI_INFO_TYPE_ERROR_DOMAIN:
                case GI_INFO_TYPE_VALUE:
                case GI_INFO_TYPE_SIGNAL:
                case GI_INFO_TYPE_PROPERTY:
                case GI_INFO_TYPE_FIELD:
                case GI_INFO_TYPE_ARG:
                case GI_INFO_TYPE_TYPE:
                case GI_INFO_TYPE_UNRESOLVED:
                    g_assert_not_reached();
                    break;
            }

            g_base_info_unref(info);
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
            size = sizeof(gpointer);
            break;
    }

    return size;
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
    gsize n_backup_args;
    Py_ssize_t n_py_args;
    gsize n_aux_in_args;
    gsize n_aux_out_args;
    gsize n_return_values;

    guint8 callback_index;
    guint8 user_data_index;
    guint8 destroy_notify_index;
    PyGICClosure *closure = NULL;
    
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
    GArgument *backup_args;
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
    n_backup_args = 0;
    n_aux_in_args = 0;
    n_aux_out_args = 0;
    
    /* Check the argument count. */
    n_py_args = PyTuple_Size(py_args);
    g_assert(n_py_args >= 0);

    error_arg_pos = -1;

    arg_infos = g_newa(GIArgInfo *, n_args);
    arg_type_infos = g_newa(GITypeInfo *, n_args);

    args_is_auxiliary = g_newa(gboolean, n_args);
    memset(args_is_auxiliary, 0, sizeof(args_is_auxiliary) * n_args);

    if (!_pygi_scan_for_callbacks (self, is_method, &callback_index, &user_data_index,
                             &destroy_notify_index))
        return NULL;
        
    if (callback_index != G_MAXUINT8) {
        if (!_pygi_create_callback (self, is_method, 
                             n_args, n_py_args, py_args, callback_index,
                             user_data_index,
                             destroy_notify_index, &closure))
            return NULL;

        args_is_auxiliary[callback_index] = FALSE;
        if (destroy_notify_index != G_MAXUINT8) {
            args_is_auxiliary[destroy_notify_index] = TRUE;
            n_aux_in_args += 1;
        }
    }

    if (is_method) {
        /* The first argument is the instance. */
        n_in_args += 1;
    }

    /* We do a first (well, second) pass here over the function to scan for special cases.
     * This is currently array+length combinations and GError.
     */
    for (i = 0; i < n_args; i++) {
        GIDirection direction;
        GITransfer transfer;
        GITypeTag arg_type_tag;

        arg_infos[i] = g_callable_info_get_arg((GICallableInfo *)self->info, i);
        arg_type_infos[i] = g_arg_info_get_type(arg_infos[i]);
        
        direction = g_arg_info_get_direction(arg_infos[i]);
        transfer = g_arg_info_get_ownership_transfer(arg_infos[i]);
        arg_type_tag = g_type_info_get_tag(arg_type_infos[i]);

        if (direction == GI_DIRECTION_IN || direction == GI_DIRECTION_INOUT) {
            n_in_args += 1;
            if (transfer == GI_TRANSFER_CONTAINER) {
                n_backup_args += 1;
            }
        }
        if (direction == GI_DIRECTION_OUT || direction == GI_DIRECTION_INOUT) {
            n_out_args += 1;
        }

        if (direction == GI_DIRECTION_INOUT && transfer == GI_TRANSFER_NOTHING) {
            n_backup_args += 1;
        }

        switch (arg_type_tag) {
            case GI_TYPE_TAG_ARRAY:
            {
                gint length_arg_pos;

                length_arg_pos = g_type_info_get_array_length(arg_type_infos[i]);

                if (is_method)
                    length_arg_pos--; // length_arg_pos refers to C args

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
                g_warn_if_fail(error_arg_pos < 0);
                error_arg_pos = i;
                break;
            default:
                break;
        }
    }

    return_type_info = g_callable_info_get_return_type((GICallableInfo *)self->info);
    return_type_tag = g_type_info_get_tag(return_type_info);

    if (return_type_tag == GI_TYPE_TAG_ARRAY) {
        gint length_arg_pos;
        length_arg_pos = g_type_info_get_array_length(return_type_info);

        if (is_method)
            length_arg_pos--; // length_arg_pos refers to C args

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

        n_py_args_expected = n_in_args
            + (is_constructor ? 1 : 0)
            - n_aux_in_args
            - (error_arg_pos >= 0 ? 1 : 0);

        if (n_py_args != n_py_args_expected) {
            PyErr_Format(PyExc_TypeError,
                "takes exactly %zd argument(s) (%zd given)",
                n_py_args_expected, n_py_args);
            goto out;
        }

        /* Check argument types. */
        py_args_pos = 0;
        if (is_constructor || is_method) {
            py_args_pos += 1;
        }

        for (i = 0; i < n_args; i++) {
            GIDirection direction;
            GITypeTag type_tag;
            PyObject *py_arg;
            gint retval;
            gboolean allow_none;

            direction = g_arg_info_get_direction(arg_infos[i]);
            type_tag = g_type_info_get_tag(arg_type_infos[i]);

            if (direction == GI_DIRECTION_OUT
                    || args_is_auxiliary[i]
                    || type_tag == GI_TYPE_TAG_ERROR) {
                continue;
            }

            g_assert(py_args_pos < n_py_args);
            py_arg = PyTuple_GET_ITEM(py_args, py_args_pos);

            allow_none = g_arg_info_may_be_null(arg_infos[i]);

            retval = _pygi_g_type_info_check_object(arg_type_infos[i],
                                                    py_arg,
                                                    allow_none);

            if (retval < 0) {
                goto out;
            } else if (!retval) {
                _PyGI_ERROR_PREFIX("argument %zd: ", py_args_pos);
                goto out;
            }

            py_args_pos += 1;
        }

        g_assert(py_args_pos == n_py_args);
    }

    args = g_newa(GArgument *, n_args);
    in_args = g_newa(GArgument, n_in_args);
    out_args = g_newa(GArgument, n_out_args);
    out_values = g_newa(GArgument, n_out_args);
    backup_args = g_newa(GArgument, n_backup_args);

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
                    out_values[out_args_pos].v_pointer = NULL;
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
        gsize backup_args_pos;

        py_args_pos = 0;
        backup_args_pos = 0;

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

            switch(container_info_type) {
                case GI_INFO_TYPE_UNION:
                case GI_INFO_TYPE_STRUCT:
                {
                    GType type;

                    type = g_registered_type_info_get_g_type((GIRegisteredTypeInfo *)container_info);

                    if (g_type_is_a(type, G_TYPE_BOXED)) {
                        g_assert(n_in_args > 0);
                        in_args[0].v_pointer = pyg_boxed_get(py_arg, void);
                    } else if (g_type_is_a(type, G_TYPE_POINTER) || type == G_TYPE_NONE) {
                        g_assert(n_in_args > 0);
                        in_args[0].v_pointer = pyg_pointer_get(py_arg, void);
                    } else {
                        PyErr_Format(PyExc_TypeError, "unable to convert an instance of '%s'", g_type_name(type));
                        goto out;
                    }

                    break;
                }
                case GI_INFO_TYPE_OBJECT:
                case GI_INFO_TYPE_INTERFACE:
                    g_assert(n_in_args > 0);
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

            if (i == callback_index) {
                args[i]->v_pointer = closure->closure;
                py_args_pos++;
                continue;
            } else if (i == user_data_index) {
                args[i]->v_pointer = closure;
                py_args_pos++;
                continue;
            } else if (i == destroy_notify_index) {
                args[i]->v_pointer = _pygi_destroy_notify_create();
                continue;
            }
            
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

                *args[i] = _pygi_argument_from_object(py_arg, arg_type_infos[i], transfer);

                if (PyErr_Occurred()) {
                    /* TODO: release previous input arguments. */
                    goto out;
                }

                if (direction == GI_DIRECTION_INOUT && transfer == GI_TRANSFER_NOTHING) {
                    /* We need to keep a copy of the argument to be able to release it later. */
                    g_assert(backup_args_pos < n_backup_args);
                    backup_args[backup_args_pos] = *args[i];
                    backup_args_pos += 1;
                } else if (transfer == GI_TRANSFER_CONTAINER) {
                    /* We need to keep a copy of the items to be able to release them later. */
                    switch (arg_type_tag) {
                        case GI_TYPE_TAG_ARRAY:
                        {
                            GArray *array;
                            gsize item_size;
                            GArray *new_array;

                            array = args[i]->v_pointer;

                            item_size = g_array_get_element_size(array);

                            new_array = g_array_sized_new(FALSE, FALSE, item_size, array->len);
                            g_array_append_vals(new_array, array->data, array->len);

                            g_assert(backup_args_pos < n_backup_args);
                            backup_args[backup_args_pos].v_pointer = new_array;

                            break;
                        }
                        case GI_TYPE_TAG_GLIST:
                            g_assert(backup_args_pos < n_backup_args);
                            backup_args[backup_args_pos].v_pointer = g_list_copy(args[i]->v_pointer);
                            break;
                        case GI_TYPE_TAG_GSLIST:
                            g_assert(backup_args_pos < n_backup_args);
                            backup_args[backup_args_pos].v_pointer = g_slist_copy(args[i]->v_pointer);
                            break;
                        case GI_TYPE_TAG_GHASH:
                        {
                            GHashTable *hash_table;
                            GList *keys;
                            GList *values;

                            hash_table = args[i]->v_pointer;

                            keys = g_hash_table_get_keys(hash_table);
                            values = g_hash_table_get_values(hash_table);

                            g_assert(backup_args_pos < n_backup_args);
                            backup_args[backup_args_pos].v_pointer = g_list_concat(keys, values);

                            break;
                        }
                        default:
                            g_warn_if_reached();
                    }

                    backup_args_pos += 1;
                }

                if (arg_type_tag == GI_TYPE_TAG_ARRAY) {
                    GArray *array;
                    gssize length_arg_pos;

                    array = args[i]->v_pointer;

                    length_arg_pos = g_type_info_get_array_length(arg_type_infos[i]);
                    if (is_method)
                        length_arg_pos--; // length_arg_pos refers to C args
                    if (length_arg_pos >= 0) {
                        /* Set the auxiliary argument holding the length. */
                        args[length_arg_pos]->v_size = array->len;
                    }

                    /* Get rid of the GArray. */
                    if (array != NULL) {
                        args[i]->v_pointer = array->data;

                        if (direction != GI_DIRECTION_INOUT || transfer != GI_TRANSFER_NOTHING) {
                            /* The array hasn't been referenced anywhere, so free it to avoid losing memory. */
                            g_array_free(array, FALSE);
                        }
                    }
                }

                py_args_pos += 1;
            }
        }

        g_assert(py_args_pos == n_py_args);
        g_assert(backup_args_pos == n_backup_args);
    }

    /* Invoke the callable. */
    {
        GError *error;
        gint retval;

        error = NULL;

        retval = g_function_info_invoke((GIFunctionInfo *)self->info,
                in_args, n_in_args, out_args, n_out_args, &return_arg, &error);
        if (!retval) {
            g_assert(error != NULL);
            /* TODO: raise the right error, out of the error domain. */
            PyErr_SetString(PyExc_RuntimeError, error->message);
            g_error_free(error);

            /* TODO: release input arguments. */

            goto out;
        }
    }

    if (error_arg_pos >= 0) {
        GError **error;

        error = args[error_arg_pos]->v_pointer;

        if (*error != NULL) {
            /* TODO: raise the right error, out of the error domain, if applicable. */
            PyErr_SetString(PyExc_Exception, (*error)->message);
            g_error_free(*error);

            /* TODO: release input arguments. */

            goto out;
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

        transfer = g_callable_info_get_caller_owns((GICallableInfo *)self->info);

        switch (info_type) {
            case GI_INFO_TYPE_UNION:
                /* TODO */
                PyErr_SetString(PyExc_NotImplementedError, "creating unions is not supported yet");
                g_base_info_unref(info);
                goto out;
            case GI_INFO_TYPE_STRUCT:
            {
                GType type;

                type = g_registered_type_info_get_g_type((GIRegisteredTypeInfo *)info);

                if (g_type_is_a(type, G_TYPE_BOXED)) {
                    if (return_arg.v_pointer == NULL) {
                        PyErr_SetString(PyExc_TypeError, "constructor returned NULL");
                        break;
                    }
                    g_warn_if_fail(transfer == GI_TRANSFER_EVERYTHING);
                    return_value = _pygi_boxed_new(py_type, return_arg.v_pointer, transfer == GI_TRANSFER_EVERYTHING);
                } else if (g_type_is_a(type, G_TYPE_POINTER) || type == G_TYPE_NONE) {
                    if (return_arg.v_pointer == NULL) {
                        PyErr_SetString(PyExc_TypeError, "constructor returned NULL");
                        break;
                    }
                    g_warn_if_fail(transfer == GI_TRANSFER_NOTHING);
                    return_value = _pygi_struct_new(py_type, return_arg.v_pointer, transfer == GI_TRANSFER_EVERYTHING);
                } else {
                    PyErr_Format(PyExc_TypeError, "cannot create '%s' instances", py_type->tp_name);
                    g_base_info_unref(info);
                    goto out;
                }

                break;
            }
            case GI_INFO_TYPE_OBJECT:
                if (return_arg.v_pointer == NULL) {
                    PyErr_SetString(PyExc_TypeError, "constructor returned NULL");
                    break;
                }
                return_value = pygobject_new(return_arg.v_pointer);
                if (transfer == GI_TRANSFER_EVERYTHING) {
                    /* The new wrapper increased the reference count, so decrease it. */
                    g_object_unref (return_arg.v_pointer);
                }
                break;
            default:
                /* Other types don't have neither methods nor constructors. */
                g_assert_not_reached();
        }

        g_base_info_unref(info);

        if (return_value == NULL) {
            /* TODO: release arguments. */
            goto out;
        }
    } else {
        GITransfer transfer;

        if (return_type_tag == GI_TYPE_TAG_ARRAY) {
            /* Create a #GArray. */
            return_arg.v_pointer = _pygi_argument_to_array(&return_arg, args, return_type_info, is_method);
        }

        transfer = g_callable_info_get_caller_owns((GICallableInfo *)self->info);

        return_value = _pygi_argument_to_object(&return_arg, return_type_info, transfer);
        if (return_value == NULL) {
            /* TODO: release argument. */
            goto out;
        }

        _pygi_argument_release(&return_arg, return_type_info, transfer, GI_DIRECTION_OUT);

        if (return_type_tag == GI_TYPE_TAG_ARRAY
                && transfer == GI_TRANSFER_NOTHING) {
            /* We created a #GArray, so free it. */
            return_arg.v_pointer = g_array_free(return_arg.v_pointer, FALSE);
        }
    }

    /* Convert output arguments and release arguments. */
    {
        gsize backup_args_pos;
        gsize return_values_pos;

        backup_args_pos = 0;
        return_values_pos = 0;

        if (n_return_values > 1) {
            /* Return a tuple. */
            PyObject *return_values;

            return_values = PyTuple_New(n_return_values);
            if (return_values == NULL) {
                /* TODO: release arguments. */
                goto out;
            }

            if (return_type_tag == GI_TYPE_TAG_VOID) {
                /* The current return value is None. */
                Py_DECREF(return_value);
            } else {
                /* Put the return value first. */
                g_assert(return_value != NULL);
                PyTuple_SET_ITEM(return_values, return_values_pos, return_value);
                return_values_pos += 1;
            }

            return_value = return_values;
        }

        for (i = 0; i < n_args; i++) {
            GIDirection direction;
            GITypeTag type_tag;
            GITransfer transfer;

            if (args_is_auxiliary[i]) {
                /* Auxiliary arguments are handled at the same time as their relatives. */
                continue;
            }

            direction = g_arg_info_get_direction(arg_infos[i]);
            transfer = g_arg_info_get_ownership_transfer(arg_infos[i]);

            type_tag = g_type_info_get_tag(arg_type_infos[i]);

            if (type_tag == GI_TYPE_TAG_ARRAY
                    && (direction != GI_DIRECTION_IN || transfer == GI_TRANSFER_NOTHING)) {
                /* Create a #GArray. */
                args[i]->v_pointer = _pygi_argument_to_array(args[i], args, arg_type_infos[i], is_method);
            }

            if (direction == GI_DIRECTION_INOUT || direction == GI_DIRECTION_OUT) {
                /* Convert the argument. */
                PyObject *obj;

                obj = _pygi_argument_to_object(args[i], arg_type_infos[i], transfer);
                if (obj == NULL) {
                    /* TODO: release arguments. */
                    goto out;
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

            if ((direction == GI_DIRECTION_IN || direction == GI_DIRECTION_INOUT)
                    && transfer == GI_TRANSFER_CONTAINER) {
                /* Release the items we kept in another container. */
                switch (type_tag) {
                    case GI_TYPE_TAG_ARRAY:
                    case GI_TYPE_TAG_GLIST:
                    case GI_TYPE_TAG_GSLIST:
                        g_assert(backup_args_pos < n_backup_args);
                        _pygi_argument_release(&backup_args[backup_args_pos], arg_type_infos[i],
                            transfer, GI_DIRECTION_IN);
                        break;
                    case GI_TYPE_TAG_GHASH:
                    {
                        GITypeInfo *key_type_info;
                        GITypeInfo *value_type_info;
                        GList *item;
                        gsize length;
                        gsize j;

                        key_type_info = g_type_info_get_param_type(arg_type_infos[i], 0);
                        value_type_info = g_type_info_get_param_type(arg_type_infos[i], 1);

                        g_assert(backup_args_pos < n_backup_args);
                        item = backup_args[backup_args_pos].v_pointer;

                        length = g_list_length(item) / 2;

                        for (j = 0; j < length; j++, item = g_list_next(item)) {
                            _pygi_argument_release((GArgument *)&item->data, key_type_info,
                                GI_TRANSFER_NOTHING, GI_DIRECTION_IN);
                        }

                        for (j = 0; j < length; j++, item = g_list_next(item)) {
                            _pygi_argument_release((GArgument *)&item->data, value_type_info,
                                GI_TRANSFER_NOTHING, GI_DIRECTION_IN);
                        }

                        g_list_free(backup_args[backup_args_pos].v_pointer);

                        break;
                    }
                    default:
                        g_warn_if_reached();
                }

                if (direction == GI_DIRECTION_INOUT) {
                    /* Release the output argument. */
                    _pygi_argument_release(args[i], arg_type_infos[i], GI_TRANSFER_CONTAINER,
                        GI_DIRECTION_OUT);
                }

                backup_args_pos += 1;
            } else if (direction == GI_DIRECTION_INOUT) {
                if (transfer == GI_TRANSFER_NOTHING) {
                    g_assert(backup_args_pos < n_backup_args);
                    _pygi_argument_release(&backup_args[backup_args_pos], arg_type_infos[i],
                        GI_TRANSFER_NOTHING, GI_DIRECTION_IN);
                    backup_args_pos += 1;
                }

                _pygi_argument_release(args[i], arg_type_infos[i], transfer,
                    GI_DIRECTION_OUT);
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
        g_assert(backup_args_pos == n_backup_args);
    }

out:
    g_base_info_unref((GIBaseInfo *)return_type_info);

    if (closure != NULL) {
        if (closure->scope == GI_SCOPE_TYPE_CALL) 
            _pygi_invoke_closure_free(closure);
    }

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
_get_fields (PyGIBaseInfo *self, GIInfoType info_type)
{
    gssize n_infos;
    PyObject *infos;
    gssize i;

    switch (info_type) {
        case GI_INFO_TYPE_STRUCT:
            n_infos = g_struct_info_get_n_fields((GIStructInfo *)self->info);
            break;
        case GI_INFO_TYPE_OBJECT:
            n_infos = g_object_info_get_n_fields((GIObjectInfo *)self->info);
            break;
        default:
            g_assert_not_reached();
    }

    infos = PyTuple_New(n_infos);
    if (infos == NULL) {
        return NULL;
    }

    for (i = 0; i < n_infos; i++) {
        GIBaseInfo *info;
        PyObject *py_info;

        switch (info_type) {
            case GI_INFO_TYPE_STRUCT:
                info = (GIBaseInfo *)g_struct_info_get_field((GIStructInfo *)self->info, i);
                break;
            case GI_INFO_TYPE_OBJECT:
                info = (GIBaseInfo *)g_object_info_get_field((GIObjectInfo *)self->info, i);
                break;
            default:
                g_assert_not_reached();
        }
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
_get_methods (PyGIBaseInfo *self, GIInfoType info_type)
{
    gssize n_infos;
    PyObject *infos;
    gssize i;

    switch (info_type) {
        case GI_INFO_TYPE_STRUCT:
            n_infos = g_struct_info_get_n_methods((GIStructInfo *)self->info);
            break;
        case GI_INFO_TYPE_OBJECT:
            n_infos = g_object_info_get_n_methods((GIObjectInfo *)self->info);
            break;
        default:
            g_assert_not_reached();
    }

    infos = PyTuple_New(n_infos);
    if (infos == NULL) {
        return NULL;
    }

    for (i = 0; i < n_infos; i++) {
        GIBaseInfo *info;
        PyObject *py_info;

        switch (info_type) {
            case GI_INFO_TYPE_STRUCT:
                info = (GIBaseInfo *)g_struct_info_get_method((GIStructInfo *)self->info, i);
                break;
            case GI_INFO_TYPE_OBJECT:
                info = (GIBaseInfo *)g_object_info_get_method((GIObjectInfo *)self->info, i);
                break;
            default:
                g_assert_not_reached();
        }
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
_get_constants (PyGIBaseInfo *self, GIInfoType info_type)
{
    gssize n_infos;
    PyObject *infos;
    gssize i;

    switch (info_type) {
        case GI_INFO_TYPE_INTERFACE:
            n_infos = g_interface_info_get_n_constants((GIInterfaceInfo *)self->info);
            break;
        case GI_INFO_TYPE_OBJECT:
            n_infos = g_object_info_get_n_constants((GIObjectInfo *)self->info);
            break;
        default:
            g_assert_not_reached();
    }

    infos = PyTuple_New(n_infos);
    if (infos == NULL) {
        return NULL;
    }

    for (i = 0; i < n_infos; i++) {
        GIBaseInfo *info;
        PyObject *py_info;

        switch (info_type) {
            case GI_INFO_TYPE_INTERFACE:
                info = (GIBaseInfo *)g_interface_info_get_constant((GIInterfaceInfo *)self->info, i);
                break;
            case GI_INFO_TYPE_OBJECT:
                info = (GIBaseInfo *)g_object_info_get_constant((GIObjectInfo *)self->info, i);
                break;
            default:
                g_assert_not_reached();
        }
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
_get_vfuncs (PyGIBaseInfo *self, GIInfoType info_type)
{
    gssize n_infos;
    PyObject *infos;
    gssize i;

    switch (info_type) {
        case GI_INFO_TYPE_INTERFACE:
            n_infos = g_interface_info_get_n_vfuncs((GIInterfaceInfo *)self->info);
            break;
        case GI_INFO_TYPE_OBJECT:
            n_infos = g_object_info_get_n_vfuncs((GIObjectInfo *)self->info);
            break;
        default:
            g_assert_not_reached();
    }

    infos = PyTuple_New(n_infos);
    if (infos == NULL) {
        return NULL;
    }

    for (i = 0; i < n_infos; i++) {
        GIBaseInfo *info;
        PyObject *py_info;

        switch (info_type) {
            case GI_INFO_TYPE_INTERFACE:
                info = (GIBaseInfo *)g_interface_info_get_vfunc((GIInterfaceInfo *)self->info, i);
                break;
            case GI_INFO_TYPE_OBJECT:
                info = (GIBaseInfo *)g_object_info_get_vfunc((GIObjectInfo *)self->info, i);
                break;
            default:
                g_assert_not_reached();
        }
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
_wrap_g_struct_info_get_fields (PyGIBaseInfo *self)
{
    return _get_fields(self, GI_INFO_TYPE_STRUCT);
}

static PyObject *
_wrap_g_struct_info_get_methods (PyGIBaseInfo *self)
{
    return _get_methods(self, GI_INFO_TYPE_STRUCT);
}

static PyMethodDef _PyGIStructInfo_methods[] = {
    { "get_fields", (PyCFunction)_wrap_g_struct_info_get_fields, METH_NOARGS },
    { "get_methods", (PyCFunction)_wrap_g_struct_info_get_methods, METH_NOARGS },
    { NULL, NULL, 0 }
};

gboolean
pygi_g_struct_info_is_simple (GIStructInfo *struct_info)
{
    gboolean is_simple;
    gsize n_field_infos;
    gsize i;

    is_simple = TRUE;

    n_field_infos = g_struct_info_get_n_fields(struct_info);

    for (i = 0; i < n_field_infos && is_simple; i++) {
        GIFieldInfo *field_info;
        GITypeInfo *field_type_info;

        field_info = g_struct_info_get_field(struct_info, i);
        field_type_info = g_field_info_get_type(field_info);

        GITypeTag field_type_tag;

        field_type_tag = g_type_info_get_tag(field_type_info);

        switch (field_type_tag) {
            case GI_TYPE_TAG_BOOLEAN:
            case GI_TYPE_TAG_INT8:
            case GI_TYPE_TAG_UINT8:
            case GI_TYPE_TAG_INT16:
            case GI_TYPE_TAG_UINT16:
            case GI_TYPE_TAG_INT32:
            case GI_TYPE_TAG_UINT32:
            case GI_TYPE_TAG_SHORT:
            case GI_TYPE_TAG_USHORT:
            case GI_TYPE_TAG_INT:
            case GI_TYPE_TAG_UINT:
            case GI_TYPE_TAG_INT64:
            case GI_TYPE_TAG_UINT64:
            case GI_TYPE_TAG_LONG:
            case GI_TYPE_TAG_ULONG:
            case GI_TYPE_TAG_SSIZE:
            case GI_TYPE_TAG_SIZE:
            case GI_TYPE_TAG_FLOAT:
            case GI_TYPE_TAG_DOUBLE:
            case GI_TYPE_TAG_TIME_T:
                if (g_type_info_is_pointer(field_type_info)) {
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

                info = g_type_info_get_interface(field_type_info);
                info_type = g_base_info_get_type(info);

                switch (info_type) {
                    case GI_INFO_TYPE_STRUCT:
                        if (g_type_info_is_pointer(field_type_info)) {
                            is_simple = FALSE;
                        } else {
                            is_simple = pygi_g_struct_info_is_simple((GIStructInfo *)info);
                        }
                        break;
                    case GI_INFO_TYPE_UNION:
                        /* TODO */
                        is_simple = FALSE;
                        break;
                    case GI_INFO_TYPE_ENUM:
                    case GI_INFO_TYPE_FLAGS:
                        if (g_type_info_is_pointer(field_type_info)) {
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
                    case GI_INFO_TYPE_ERROR_DOMAIN:
                    case GI_INFO_TYPE_VALUE:
                    case GI_INFO_TYPE_SIGNAL:
                    case GI_INFO_TYPE_PROPERTY:
                    case GI_INFO_TYPE_FIELD:
                    case GI_INFO_TYPE_ARG:
                    case GI_INFO_TYPE_TYPE:
                    case GI_INFO_TYPE_UNRESOLVED:
                        g_assert_not_reached();
                }

                g_base_info_unref(info);
                break;
            }
        }

        g_base_info_unref((GIBaseInfo *)field_type_info);
        g_base_info_unref((GIBaseInfo *)field_info);
    }

    return is_simple;
}


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
    return _get_methods(self, GI_INFO_TYPE_OBJECT);
}

static PyObject *
_wrap_g_object_info_get_fields (PyGIBaseInfo *self)
{
    return _get_fields(self, GI_INFO_TYPE_OBJECT);
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

static PyObject *
_wrap_g_object_info_get_constants (PyGIBaseInfo *self)
{
    return _get_constants(self, GI_INFO_TYPE_OBJECT);
}

static PyObject *
_wrap_g_object_info_get_vfuncs (PyGIBaseInfo *self)
{
    return _get_vfuncs(self, GI_INFO_TYPE_OBJECT);
}

static PyMethodDef _PyGIObjectInfo_methods[] = {
    { "get_parent", (PyCFunction)_wrap_g_object_info_get_parent, METH_NOARGS },
    { "get_methods", (PyCFunction)_wrap_g_object_info_get_methods, METH_NOARGS },
    { "get_fields", (PyCFunction)_wrap_g_object_info_get_fields, METH_NOARGS },
    { "get_interfaces", (PyCFunction)_wrap_g_object_info_get_interfaces, METH_NOARGS },
    { "get_constants", (PyCFunction)_wrap_g_object_info_get_constants, METH_NOARGS },
    { "get_vfuncs", (PyCFunction)_wrap_g_object_info_get_vfuncs, METH_NOARGS },
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

static PyObject *
_wrap_g_interface_info_get_constants (PyGIBaseInfo *self)
{
    return _get_constants(self, GI_INFO_TYPE_INTERFACE);
}

static PyObject *
_wrap_g_interface_info_get_vfuncs (PyGIBaseInfo *self)
{
    return _get_vfuncs(self, GI_INFO_TYPE_INTERFACE);
}

static PyMethodDef _PyGIInterfaceInfo_methods[] = {
    { "get_methods", (PyCFunction)_wrap_g_interface_info_get_methods, METH_NOARGS },
    { "get_constants", (PyCFunction)_wrap_g_interface_info_get_constants, METH_NOARGS },
    { "get_vfuncs", (PyCFunction)_wrap_g_interface_info_get_vfuncs, METH_NOARGS },
    { NULL, NULL, 0 }
};

/* GIConstantInfo */
_PyGI_DEFINE_INFO_TYPE("ConstantInfo", GIConstantInfo, PyGIBaseInfo_Type);

static PyObject *
_wrap_g_constant_info_get_value (PyGIBaseInfo *self)
{
    GITypeInfo *type_info;
    GArgument value;
    PyObject *py_value;

    if (g_constant_info_get_value((GIConstantInfo *)self->info, &value) < 0) {
        PyErr_SetString(PyExc_RuntimeError, "unable to get value");
        return NULL;
    }

    type_info = g_constant_info_get_type((GIConstantInfo *)self->info);

    py_value = _pygi_argument_to_object(&value, type_info, GI_TRANSFER_NOTHING);

    g_base_info_unref((GIBaseInfo *)type_info);

    return py_value;
}

static PyMethodDef _PyGIConstantInfo_methods[] = {
    { "get_value", (PyCFunction)_wrap_g_constant_info_get_value, METH_NOARGS },
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

        retval = _pygi_g_type_info_check_object(field_type_info, py_value, TRUE);
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

                is_simple = pygi_g_struct_info_is_simple((GIStructInfo *)info);

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

/* GIVFuncInfo */
_PyGI_DEFINE_INFO_TYPE("VFuncInfo", GIVFuncInfo, PyGIBaseInfo_Type);

static PyMethodDef _PyGIVFuncInfo_methods[] = {
    { NULL, NULL, 0 }
};


/* GIUnionInfo */
_PyGI_DEFINE_INFO_TYPE("UnionInfo", GIUnionInfo, PyGIRegisteredTypeInfo_Type);

static PyObject *
_wrap_g_union_info_get_fields (PyGIBaseInfo *self)
{
    gssize n_infos;
    PyObject *infos;
    gssize i;

    n_infos = g_union_info_get_n_fields((GIUnionInfo *)self->info);

    infos = PyTuple_New(n_infos);
    if (infos == NULL) {
        return NULL;
    }

    for (i = 0; i < n_infos; i++) {
        GIBaseInfo *info;
        PyObject *py_info;

        info = (GIBaseInfo *)g_union_info_get_field((GIUnionInfo *)self->info, i);
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
_wrap_g_union_info_get_methods (PyGIBaseInfo *self)
{
    gssize n_infos;
    PyObject *infos;
    gssize i;

    n_infos = g_union_info_get_n_methods((GIUnionInfo *)self->info);

    infos = PyTuple_New(n_infos);
    if (infos == NULL) {
        return NULL;
    }

    for (i = 0; i < n_infos; i++) {
        GIBaseInfo *info;
        PyObject *py_info;

        info = (GIBaseInfo *)g_union_info_get_method((GIUnionInfo *)self->info, i);
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

static PyMethodDef _PyGIUnionInfo_methods[] = {
    { "get_fields", (PyCFunction)_wrap_g_union_info_get_fields, METH_NOARGS },
    { "get_methods", (PyCFunction)_wrap_g_union_info_get_methods, METH_NOARGS },
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
    _PyGI_REGISTER_TYPE(m, PyGIConstantInfo_Type, "ConstantInfo");
    _PyGI_REGISTER_TYPE(m, PyGIValueInfo_Type, "ValueInfo");
    _PyGI_REGISTER_TYPE(m, PyGIFieldInfo_Type, "FieldInfo");
    _PyGI_REGISTER_TYPE(m, PyGIVFuncInfo_Type, "VFuncInfo");
    _PyGI_REGISTER_TYPE(m, PyGIUnionInfo_Type, "UnionInfo");

#undef _PyGI_REGISTER_TYPE
}
