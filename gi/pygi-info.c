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
#include <pyglib-python-compat.h>

/* BaseInfo */

static void
_base_info_dealloc (PyGIBaseInfo *self)
{
    PyObject_GC_UnTrack ( (PyObject *) self);

    PyObject_ClearWeakRefs ( (PyObject *) self);

    g_base_info_unref (self->info);

    Py_TYPE( (PyObject *) self)->tp_free ( (PyObject *) self);
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
    return PYGLIB_PyUnicode_FromFormat ("<%s object (%s) at 0x%p>",
                                        Py_TYPE( (PyObject *) self)->tp_name, 
                                        g_base_info_get_name (self->info), 
                                        (void *) self);
}

static PyObject *
_base_info_richcompare (PyGIBaseInfo *self, PyObject *other, int op)
{
    PyObject *res;
    GIBaseInfo *other_info;

    if (!PyObject_TypeCheck(other, &PyGIBaseInfo_Type)) {
        Py_INCREF(Py_NotImplemented);
        return Py_NotImplemented;
    }

    other_info = ((PyGIBaseInfo *)other)->info;

    switch (op) {
        case Py_EQ:
            res = g_base_info_equal (self->info, other_info) ? Py_True : Py_False;
            break;
        case Py_NE:
            res = g_base_info_equal (self->info, other_info) ? Py_False : Py_True;
            break;
        default:
            res = Py_NotImplemented;
            break;
    }
    Py_INCREF(res);
    return res;
}

static PyMethodDef _PyGIBaseInfo_methods[];

PYGLIB_DEFINE_TYPE("gi.BaseInfo", PyGIBaseInfo_Type, PyGIBaseInfo);

static PyObject *
_wrap_g_base_info_get_name (PyGIBaseInfo *self)
{
    return PYGLIB_PyUnicode_FromString (g_base_info_get_name (self->info));
}

static PyObject *
_wrap_g_base_info_get_namespace (PyGIBaseInfo *self)
{
    return PYGLIB_PyUnicode_FromString (g_base_info_get_namespace (self->info));
}

static PyObject *
_wrap_g_base_info_get_container (PyGIBaseInfo *self)
{
    GIBaseInfo *info;

    info = g_base_info_get_container (self->info);

    if (info == NULL) {
        Py_RETURN_NONE;
    }

    return _pygi_info_new (info);
}


static PyMethodDef _PyGIBaseInfo_methods[] = {
    { "get_name", (PyCFunction) _wrap_g_base_info_get_name, METH_NOARGS },
    { "get_namespace", (PyCFunction) _wrap_g_base_info_get_namespace, METH_NOARGS },
    { "get_container", (PyCFunction) _wrap_g_base_info_get_container, METH_NOARGS },
    { NULL, NULL, 0 }
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
            type = &PyGIStructInfo_Type;
            break;
        case GI_INFO_TYPE_BOXED:
            type = &PyGIBoxedInfo_Type;
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
        case GI_INFO_TYPE_ERROR_DOMAIN:
            type = &PyGIErrorDomainInfo_Type;
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
    }

    self = (PyGIBaseInfo *) type->tp_alloc (type, 0);
    if (self == NULL) {
        return NULL;
    }

    self->info = g_base_info_ref (info);

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
                      type->tp_name, Py_TYPE(&py_info)->tp_name);
        goto out;
    }

    info = ( (PyGIBaseInfo *) py_info)->info;
    g_base_info_ref (info);

out:
    Py_DECREF (py_info);

    return info;
}


/* CallableInfo */
PYGLIB_DEFINE_TYPE ("gi.CallableInfo", PyGICallableInfo_Type, PyGIBaseInfo);

static PyObject *
_wrap_g_callable_info_get_arguments (PyGIBaseInfo *self)
{
    gssize n_infos;
    PyObject *infos;
    gssize i;

    n_infos = g_callable_info_get_n_args ( (GICallableInfo *) self->info);

    infos = PyTuple_New (n_infos);
    if (infos == NULL) {
        return NULL;
    }

    for (i = 0; i < n_infos; i++) {
        GIBaseInfo *info;
        PyObject *py_info;

        info = (GIBaseInfo *) g_callable_info_get_arg ( (GICallableInfo *) self->info, i);
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

static PyMethodDef _PyGICallableInfo_methods[] = {
    { "invoke", (PyCFunction) _wrap_g_callable_info_invoke, METH_VARARGS | METH_KEYWORDS },
    { "get_arguments", (PyCFunction) _wrap_g_callable_info_get_arguments, METH_NOARGS },
    { NULL, NULL, 0 }
};

/* CallbackInfo */
PYGLIB_DEFINE_TYPE ("gi.CallbackInfo", PyGICallbackInfo_Type, PyGIBaseInfo);

static PyMethodDef _PyGICallbackInfo_methods[] = {
    { NULL, NULL, 0 }
};

/* BoxedInfo */
PYGLIB_DEFINE_TYPE ("gi.BoxedInfo", PyGIBoxedInfo_Type, PyGIBaseInfo);

static PyMethodDef _PyGIBoxedInfo_methods[] = {
    { NULL, NULL, 0 }
};

/* ErrorDomainInfo */
PYGLIB_DEFINE_TYPE ("gi.ErrorDomainInfo", PyGIErrorDomainInfo_Type, PyGIBaseInfo);

static PyMethodDef _PyGIErrorDomainInfo_methods[] = {
    { NULL, NULL, 0 }
};

/* SignalInfo */
PYGLIB_DEFINE_TYPE ("gi.SignalInfo", PyGISignalInfo_Type, PyGIBaseInfo);

static PyMethodDef _PyGISignalInfo_methods[] = {
    { NULL, NULL, 0 }
};

/* PropertyInfo */
PYGLIB_DEFINE_TYPE ("gi.PropertyInfo", PyGIPropertyInfo_Type, PyGIBaseInfo);

static PyMethodDef _PyGIPropertyInfo_methods[] = {
    { NULL, NULL, 0 }
};

/* ArgInfo */
PYGLIB_DEFINE_TYPE ("gi.ArgInfo", PyGIArgInfo_Type, PyGIBaseInfo);

static PyMethodDef _PyGIArgInfo_methods[] = {
    { NULL, NULL, 0 }
};

/* TypeInfo */
PYGLIB_DEFINE_TYPE ("gi.TypeInfo", PyGITypeInfo_Type, PyGIBaseInfo);

static PyMethodDef _PyGITypeInfo_methods[] = {
    { NULL, NULL, 0 }
};


/* FunctionInfo */
PYGLIB_DEFINE_TYPE ("gi.FunctionInfo", PyGIFunctionInfo_Type, PyGIBaseInfo);

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
            if (g_type_info_is_pointer (type_info)) {
                size = sizeof (gpointer);
            } else {
                size = _pygi_g_type_tag_size (type_tag);
                g_assert (size > 0);
            }
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

static PyMethodDef _PyGIFunctionInfo_methods[] = {
    { "is_constructor", (PyCFunction) _wrap_g_function_info_is_constructor, METH_NOARGS },
    { "is_method", (PyCFunction) _wrap_g_function_info_is_method, METH_NOARGS },
    { NULL, NULL, 0 }
};


/* RegisteredTypeInfo */
PYGLIB_DEFINE_TYPE ("gi.RegisteredTypeInfo", PyGIRegisteredTypeInfo_Type, PyGIBaseInfo);

static PyObject *
_wrap_g_registered_type_info_get_g_type (PyGIBaseInfo *self)
{
    GType type;

    type = g_registered_type_info_get_g_type ( (GIRegisteredTypeInfo *) self->info);

    return pyg_type_wrapper_new (type);
}

static PyMethodDef _PyGIRegisteredTypeInfo_methods[] = {
    { "get_g_type", (PyCFunction) _wrap_g_registered_type_info_get_g_type, METH_NOARGS },
    { NULL, NULL, 0 }
};


/* GIStructInfo */
PYGLIB_DEFINE_TYPE ("StructInfo", PyGIStructInfo_Type, PyGIBaseInfo);

static PyObject *
_get_fields (PyGIBaseInfo *self, GIInfoType info_type)
{
    gssize n_infos;
    PyObject *infos;
    gssize i;

    switch (info_type) {
        case GI_INFO_TYPE_STRUCT:
            n_infos = g_struct_info_get_n_fields ( (GIStructInfo *) self->info);
            break;
        case GI_INFO_TYPE_OBJECT:
            n_infos = g_object_info_get_n_fields ( (GIObjectInfo *) self->info);
            break;
        default:
            g_assert_not_reached();
    }

    infos = PyTuple_New (n_infos);
    if (infos == NULL) {
        return NULL;
    }

    for (i = 0; i < n_infos; i++) {
        GIBaseInfo *info;
        PyObject *py_info;

        switch (info_type) {
            case GI_INFO_TYPE_STRUCT:
                info = (GIBaseInfo *) g_struct_info_get_field ( (GIStructInfo *) self->info, i);
                break;
            case GI_INFO_TYPE_OBJECT:
                info = (GIBaseInfo *) g_object_info_get_field ( (GIObjectInfo *) self->info, i);
                break;
            default:
                g_assert_not_reached();
        }
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
_get_methods (PyGIBaseInfo *self, GIInfoType info_type)
{
    gssize n_infos;
    PyObject *infos;
    gssize i;

    switch (info_type) {
        case GI_INFO_TYPE_STRUCT:
            n_infos = g_struct_info_get_n_methods ( (GIStructInfo *) self->info);
            break;
        case GI_INFO_TYPE_OBJECT:
            n_infos = g_object_info_get_n_methods ( (GIObjectInfo *) self->info);
            break;
        default:
            g_assert_not_reached();
    }

    infos = PyTuple_New (n_infos);
    if (infos == NULL) {
        return NULL;
    }

    for (i = 0; i < n_infos; i++) {
        GIBaseInfo *info;
        PyObject *py_info;

        switch (info_type) {
            case GI_INFO_TYPE_STRUCT:
                info = (GIBaseInfo *) g_struct_info_get_method ( (GIStructInfo *) self->info, i);
                break;
            case GI_INFO_TYPE_OBJECT:
                info = (GIBaseInfo *) g_object_info_get_method ( (GIObjectInfo *) self->info, i);
                break;
            default:
                g_assert_not_reached();
        }
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
_get_constants (PyGIBaseInfo *self, GIInfoType info_type)
{
    gssize n_infos;
    PyObject *infos;
    gssize i;

    switch (info_type) {
        case GI_INFO_TYPE_INTERFACE:
            n_infos = g_interface_info_get_n_constants ( (GIInterfaceInfo *) self->info);
            break;
        case GI_INFO_TYPE_OBJECT:
            n_infos = g_object_info_get_n_constants ( (GIObjectInfo *) self->info);
            break;
        default:
            g_assert_not_reached();
    }

    infos = PyTuple_New (n_infos);
    if (infos == NULL) {
        return NULL;
    }

    for (i = 0; i < n_infos; i++) {
        GIBaseInfo *info;
        PyObject *py_info;

        switch (info_type) {
            case GI_INFO_TYPE_INTERFACE:
                info = (GIBaseInfo *) g_interface_info_get_constant ( (GIInterfaceInfo *) self->info, i);
                break;
            case GI_INFO_TYPE_OBJECT:
                info = (GIBaseInfo *) g_object_info_get_constant ( (GIObjectInfo *) self->info, i);
                break;
            default:
                g_assert_not_reached();
        }
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
_get_vfuncs (PyGIBaseInfo *self, GIInfoType info_type)
{
    gssize n_infos;
    PyObject *infos;
    gssize i;

    switch (info_type) {
        case GI_INFO_TYPE_INTERFACE:
            n_infos = g_interface_info_get_n_vfuncs ( (GIInterfaceInfo *) self->info);
            break;
        case GI_INFO_TYPE_OBJECT:
            n_infos = g_object_info_get_n_vfuncs ( (GIObjectInfo *) self->info);
            break;
        default:
            g_assert_not_reached();
    }

    infos = PyTuple_New (n_infos);
    if (infos == NULL) {
        return NULL;
    }

    for (i = 0; i < n_infos; i++) {
        GIBaseInfo *info;
        PyObject *py_info;

        switch (info_type) {
            case GI_INFO_TYPE_INTERFACE:
                info = (GIBaseInfo *) g_interface_info_get_vfunc ( (GIInterfaceInfo *) self->info, i);
                break;
            case GI_INFO_TYPE_OBJECT:
                info = (GIBaseInfo *) g_object_info_get_vfunc ( (GIObjectInfo *) self->info, i);
                break;
            default:
                g_assert_not_reached();
        }
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
_wrap_g_struct_info_get_fields (PyGIBaseInfo *self)
{
    return _get_fields (self, GI_INFO_TYPE_STRUCT);
}

static PyObject *
_wrap_g_struct_info_get_methods (PyGIBaseInfo *self)
{
    return _get_methods (self, GI_INFO_TYPE_STRUCT);
}

static PyMethodDef _PyGIStructInfo_methods[] = {
    { "get_fields", (PyCFunction) _wrap_g_struct_info_get_fields, METH_NOARGS },
    { "get_methods", (PyCFunction) _wrap_g_struct_info_get_methods, METH_NOARGS },
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

        field_info = g_struct_info_get_field (struct_info, i);
        field_type_info = g_field_info_get_type (field_info);

        GITypeTag field_type_tag;

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
    gssize n_infos;
    PyObject *infos;
    gssize i;

    n_infos = g_enum_info_get_n_values ( (GIEnumInfo *) self->info);

    infos = PyTuple_New (n_infos);
    if (infos == NULL) {
        return NULL;
    }

    for (i = 0; i < n_infos; i++) {
        GIBaseInfo *info;
        PyObject *py_info;

        info = (GIBaseInfo *) g_enum_info_get_value ( (GIEnumInfo *) self->info, i);
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

static PyMethodDef _PyGIEnumInfo_methods[] = {
    { "get_values", (PyCFunction) _wrap_g_enum_info_get_values, METH_NOARGS },
    { "is_flags", (PyCFunction) _wrap_g_enum_info_is_flags, METH_NOARGS },
    { NULL, NULL, 0 }
};


/* ObjectInfo */
PYGLIB_DEFINE_TYPE ("ObjectInfo", PyGIObjectInfo_Type, PyGIBaseInfo);

static PyObject *
_wrap_g_object_info_get_parent (PyGIBaseInfo *self)
{
    GIBaseInfo *info;
    PyObject *py_info;

    info = (GIBaseInfo *) g_object_info_get_parent ( (GIObjectInfo*) self->info);

    if (info == NULL) {
        Py_RETURN_NONE;
    }

    py_info = _pygi_info_new (info);

    g_base_info_unref (info);

    return py_info;
}

static PyObject *
_wrap_g_object_info_get_methods (PyGIBaseInfo *self)
{
    return _get_methods (self, GI_INFO_TYPE_OBJECT);
}

static PyObject *
_wrap_g_object_info_get_fields (PyGIBaseInfo *self)
{
    return _get_fields (self, GI_INFO_TYPE_OBJECT);
}

static PyObject *
_wrap_g_object_info_get_interfaces (PyGIBaseInfo *self)
{
    gssize n_infos;
    PyObject *infos;
    gssize i;

    n_infos = g_object_info_get_n_interfaces ( (GIObjectInfo *) self->info);

    infos = PyTuple_New (n_infos);
    if (infos == NULL) {
        return NULL;
    }

    for (i = 0; i < n_infos; i++) {
        GIBaseInfo *info;
        PyObject *py_info;

        info = (GIBaseInfo *) g_object_info_get_interface ( (GIObjectInfo *) self->info, i);
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
_wrap_g_object_info_get_constants (PyGIBaseInfo *self)
{
    return _get_constants (self, GI_INFO_TYPE_OBJECT);
}

static PyObject *
_wrap_g_object_info_get_vfuncs (PyGIBaseInfo *self)
{
    return _get_vfuncs (self, GI_INFO_TYPE_OBJECT);
}

static PyMethodDef _PyGIObjectInfo_methods[] = {
    { "get_parent", (PyCFunction) _wrap_g_object_info_get_parent, METH_NOARGS },
    { "get_methods", (PyCFunction) _wrap_g_object_info_get_methods, METH_NOARGS },
    { "get_fields", (PyCFunction) _wrap_g_object_info_get_fields, METH_NOARGS },
    { "get_interfaces", (PyCFunction) _wrap_g_object_info_get_interfaces, METH_NOARGS },
    { "get_constants", (PyCFunction) _wrap_g_object_info_get_constants, METH_NOARGS },
    { "get_vfuncs", (PyCFunction) _wrap_g_object_info_get_vfuncs, METH_NOARGS },
    { NULL, NULL, 0 }
};


/* GIInterfaceInfo */
PYGLIB_DEFINE_TYPE ("InterfaceInfo", PyGIInterfaceInfo_Type, PyGIBaseInfo);

static PyObject *
_wrap_g_interface_info_get_methods (PyGIBaseInfo *self)
{
    gssize n_infos;
    PyObject *infos;
    gssize i;

    n_infos = g_interface_info_get_n_methods ( (GIInterfaceInfo *) self->info);

    infos = PyTuple_New (n_infos);
    if (infos == NULL) {
        return NULL;
    }

    for (i = 0; i < n_infos; i++) {
        GIBaseInfo *info;
        PyObject *py_info;

        info = (GIBaseInfo *) g_interface_info_get_method ( (GIInterfaceInfo *) self->info, i);
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
_wrap_g_interface_info_get_constants (PyGIBaseInfo *self)
{
    return _get_constants (self, GI_INFO_TYPE_INTERFACE);
}

static PyObject *
_wrap_g_interface_info_get_vfuncs (PyGIBaseInfo *self)
{
    return _get_vfuncs (self, GI_INFO_TYPE_INTERFACE);
}

static PyMethodDef _PyGIInterfaceInfo_methods[] = {
    { "get_methods", (PyCFunction) _wrap_g_interface_info_get_methods, METH_NOARGS },
    { "get_constants", (PyCFunction) _wrap_g_interface_info_get_constants, METH_NOARGS },
    { "get_vfuncs", (PyCFunction) _wrap_g_interface_info_get_vfuncs, METH_NOARGS },
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

    if (g_constant_info_get_value ( (GIConstantInfo *) self->info, &value) < 0) {
        PyErr_SetString (PyExc_RuntimeError, "unable to get value");
        return NULL;
    }

    type_info = g_constant_info_get_type ( (GIConstantInfo *) self->info);

    py_value = _pygi_argument_to_object (&value, type_info, GI_TRANSFER_NOTHING);

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

                value.v_pointer = pointer + offset;

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

    if ( (g_type_info_get_tag (field_type_info) == GI_TYPE_TAG_ARRAY) &&
            (g_type_info_get_array_type (field_type_info) == GI_ARRAY_TYPE_C)) {
        value.v_pointer = _pygi_argument_to_array (&value, NULL,
                                                   field_type_info, FALSE);
    }

argument_to_object:
    py_value = _pygi_argument_to_object (&value, field_type_info, GI_TRANSFER_NOTHING);

    if ( (value.v_pointer != NULL) &&
            (g_type_info_get_tag (field_type_info) == GI_TYPE_TAG_ARRAY) &&
               (g_type_info_get_array_type (field_type_info) == GI_ARRAY_TYPE_C)) {
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

                g_memmove (pointer + offset, value.v_pointer, size);

                g_base_info_unref (info);

                retval = Py_None;
                goto out;
            }
            default:
                /* Fallback. */
                break;
        }

        g_base_info_unref (info);
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

static PyMethodDef _PyGIFieldInfo_methods[] = {
    { "get_value", (PyCFunction) _wrap_g_field_info_get_value, METH_VARARGS },
    { "set_value", (PyCFunction) _wrap_g_field_info_set_value, METH_VARARGS },
    { NULL, NULL, 0 }
};


/* GIUnresolvedInfo */
PYGLIB_DEFINE_TYPE ("gi.UnresolvedInfo", PyGIUnresolvedInfo_Type, PyGIBaseInfo);

static PyMethodDef _PyGIUnresolvedInfo_methods[] = {
    { NULL, NULL, 0 }
};

/* GIVFuncInfo */
PYGLIB_DEFINE_TYPE ("gi.VFuncInfo", PyGIVFuncInfo_Type, PyGIBaseInfo);

static PyObject *
_wrap_g_vfunc_info_get_invoker (PyGIBaseInfo *self)
{
    PyObject *result = Py_None;
    GIBaseInfo *info;

    info = (GIBaseInfo *) g_vfunc_info_get_invoker ( (GIVFuncInfo *) self->info );
    if (info)
        result = _pygi_info_new(info);
    else
        Py_INCREF(Py_None);

    return result;
}

static PyMethodDef _PyGIVFuncInfo_methods[] = {
    { "get_invoker", (PyCFunction) _wrap_g_vfunc_info_get_invoker, METH_NOARGS },
    { NULL, NULL, 0 }
};


/* GIUnionInfo */
PYGLIB_DEFINE_TYPE ("gi.UnionInfo", PyGIUnionInfo_Type, PyGIBaseInfo);

static PyObject *
_wrap_g_union_info_get_fields (PyGIBaseInfo *self)
{
    gssize n_infos;
    PyObject *infos;
    gssize i;

    n_infos = g_union_info_get_n_fields ( (GIUnionInfo *) self->info);

    infos = PyTuple_New (n_infos);
    if (infos == NULL) {
        return NULL;
    }

    for (i = 0; i < n_infos; i++) {
        GIBaseInfo *info;
        PyObject *py_info;

        info = (GIBaseInfo *) g_union_info_get_field ( (GIUnionInfo *) self->info, i);
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
_wrap_g_union_info_get_methods (PyGIBaseInfo *self)
{
    gssize n_infos;
    PyObject *infos;
    gssize i;

    n_infos = g_union_info_get_n_methods ( (GIUnionInfo *) self->info);

    infos = PyTuple_New (n_infos);
    if (infos == NULL) {
        return NULL;
    }

    for (i = 0; i < n_infos; i++) {
        GIBaseInfo *info;
        PyObject *py_info;

        info = (GIBaseInfo *) g_union_info_get_method ( (GIUnionInfo *) self->info, i);
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
                                    g_base_info_get_name (container_info),
                                    g_base_info_get_name (info));
    } else {
        fullname = g_strdup_printf ("%s.%s",
                                    g_base_info_get_namespace (info),
                                    g_base_info_get_name (info));
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
    PyGIBaseInfo_Type.tp_flags = (Py_TPFLAGS_DEFAULT | 
                                   Py_TPFLAGS_BASETYPE  | 
                                   Py_TPFLAGS_HAVE_GC);
    PyGIBaseInfo_Type.tp_traverse = (traverseproc) _base_info_traverse;
    PyGIBaseInfo_Type.tp_weaklistoffset = offsetof(PyGIBaseInfo, inst_weakreflist);
    PyGIBaseInfo_Type.tp_methods = _PyGIBaseInfo_methods; 
    PyGIBaseInfo_Type.tp_richcompare = (richcmpfunc)_base_info_richcompare;

    if (PyType_Ready(&PyGIBaseInfo_Type))
        return;   
 
    if (PyModule_AddObject(m, "BaseInfo", (PyObject *)&PyGIBaseInfo_Type))
        return;

    _PyGI_REGISTER_TYPE (m, PyGIUnresolvedInfo_Type, UnresolvedInfo,
                         PyGIBaseInfo_Type);
    _PyGI_REGISTER_TYPE (m, PyGICallableInfo_Type, CallableInfo, 
                         PyGIBaseInfo_Type);
    _PyGI_REGISTER_TYPE (m, PyGICallbackInfo_Type, CallbackInfo,
                         PyGIBaseInfo_Type);
    _PyGI_REGISTER_TYPE (m, PyGIFunctionInfo_Type, FunctionInfo, 
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
    _PyGI_REGISTER_TYPE (m, PyGIVFuncInfo_Type, VFuncInfo,
                         PyGICallableInfo_Type);
    _PyGI_REGISTER_TYPE (m, PyGIUnionInfo_Type, UnionInfo,
                         PyGIRegisteredTypeInfo_Type);
    _PyGI_REGISTER_TYPE (m, PyGIBoxedInfo_Type, BoxedInfo,
                         PyGIBaseInfo_Type);
    _PyGI_REGISTER_TYPE (m, PyGIErrorDomainInfo_Type, ErrorDomainInfo,
                         PyGIBaseInfo_Type);
    _PyGI_REGISTER_TYPE (m, PyGISignalInfo_Type, SignalInfo,
                         PyGIBaseInfo_Type);
    _PyGI_REGISTER_TYPE (m, PyGIPropertyInfo_Type, PropertyInfo,
                         PyGIBaseInfo_Type);
    _PyGI_REGISTER_TYPE (m, PyGIArgInfo_Type, ArgInfo,
                         PyGIBaseInfo_Type);
    _PyGI_REGISTER_TYPE (m, PyGITypeInfo_Type, TypeInfo,
                         PyGIBaseInfo_Type);


#undef _PyGI_REGISTER_TYPE
}
