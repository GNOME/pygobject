/* -*- Mode: C; c-basic-offset: 4 -*-
 * vim: set ts=8 sts=4 sw=4 et ai cindent :
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
#include <pygobject.h>

static void      pyg_base_info_dealloc(PyGIBaseInfo *self);
static void      pyg_base_info_free(PyObject *op);
static PyObject* pyg_base_info_repr(PyGIBaseInfo *self);
static int       pyg_base_info_traverse(PyGIBaseInfo *self,
                                        visitproc visit,
                                        void *arg);
static void      pyg_base_info_clear(PyGIBaseInfo *self);

static PyObject *
_wrap_g_object_info_get_methods(PyGIBaseInfo *self);

#define NEW_CLASS(name, cname) \
static PyMethodDef _Py##cname##_methods[];    \
PyTypeObject Py##cname##_Type = {             \
    PyObject_HEAD_INIT(NULL)                  \
    0,                                        \
    "bank." name,                             \
    sizeof(PyGIBaseInfo),                     \
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0,             \
    0, 0, 0, 0, 0, 0,                         \
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE, \
    NULL, 0, 0, 0,                            \
    offsetof(PyGIBaseInfo, weakreflist),      \
    0, 0,                                     \
    _Py##cname##_methods,                     \
    0, 0, NULL, NULL, 0, 0,                   \
    offsetof(PyGIBaseInfo, instance_dict)     \
}

static PyMethodDef _PyGIBaseInfo_methods[];
static PyGetSetDef _PyGIBaseInfo_getsets[];

PyTypeObject PyGIBaseInfo_Type = {
    PyObject_HEAD_INIT(NULL)
    0,
    "bank.BaseInfo",
    sizeof(PyGIBaseInfo),
    0,
    /* methods */
    (destructor)pyg_base_info_dealloc,
    (printfunc)0,
    (getattrfunc)0,
    (setattrfunc)0,
    (cmpfunc)0,
    (reprfunc)pyg_base_info_repr,
    0,
    0,
    0,
    (hashfunc)0,
    (ternaryfunc)0,
    (reprfunc)0,
    (getattrofunc)0,
    (setattrofunc)0,
    0,
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE |
    Py_TPFLAGS_HAVE_GC,
    NULL,
    (traverseproc)pyg_base_info_traverse,
    (inquiry)pyg_base_info_clear,
    (richcmpfunc)0,
    offsetof(PyGIBaseInfo, weakreflist),
    (getiterfunc)0,
    (iternextfunc)0,
    _PyGIBaseInfo_methods,
    0,
    _PyGIBaseInfo_getsets,
    NULL,
    NULL,
    (descrgetfunc)0,
    (descrsetfunc)0,
    offsetof(PyGIBaseInfo, instance_dict),
    (initproc)0,
    (allocfunc)0,                       /* tp_alloc */
    (newfunc)0,                         /* tp_new */
    (freefunc)pyg_base_info_free,       /* tp_free */
    (inquiry)0,                         /* tp_is_gc */
    (PyObject *)0,                      /* tp_bases */
};

static PyObject *
pyg_base_info_repr(PyGIBaseInfo *self)
{
    gchar buf[256];

    g_snprintf(buf, sizeof(buf),
               "<%s object (%s) at 0x%lx>",
               self->ob_type->tp_name,
               g_base_info_get_name(self->info), (long)self);
    return PyString_FromString(buf);
}

static void
pyg_base_info_dealloc(PyGIBaseInfo *self)
{
    PyObject_ClearWeakRefs((PyObject *)self);
    pyg_base_info_clear(self);
}

static int
pyg_base_info_traverse(PyGIBaseInfo *self,
                       visitproc visit,
                       void *arg)
{
    int ret = 0;

    if (self->instance_dict)
        ret = visit(self->instance_dict, arg);

    if (ret != 0)
        return ret;

    return 0;

}

static void
pyg_base_info_clear(PyGIBaseInfo *self)
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
pyg_base_info_free(PyObject *op)
{
    PyObject_GC_Del(op);
}


static PyObject *
pyg_base_info_get_dict(PyGIBaseInfo *self, void *closure)
{
    if (self->instance_dict == NULL) {
        self->instance_dict = PyDict_New();
        if (self->instance_dict == NULL)
            return NULL;
    }
    Py_INCREF(self->instance_dict);
    return self->instance_dict;
}

static PyGetSetDef _PyGIBaseInfo_getsets[] = {
    { "__dict__", (getter)pyg_base_info_get_dict, (setter)0 },
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

static PyMethodDef _PyGIBaseInfo_methods[] = {
    { "getName", (PyCFunction)_wrap_g_base_info_get_name, METH_NOARGS },
    { "getType", (PyCFunction)_wrap_g_base_info_get_type, METH_NOARGS },
    { "getNamespace", (PyCFunction)_wrap_g_base_info_get_namespace, METH_NOARGS },
    { NULL, NULL, 0 }
};


/* CallableInfo */
NEW_CLASS("CallableInfo", GICallableInfo);

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
NEW_CLASS("FunctionInfo", GIFunctionInfo);

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
    GArgument *in_args;
    GArgument *out_args;
    GArgument *out_values;
    GArgument return_arg;
    int n_args;
    int expected_in_argc;
    int expected_out_argc;
    int i;
    int argv_pos;
    int in_args_pos;
    int out_args_pos;
    GError *error;
    gboolean failed;
    GIFunctionInfoFlags flags;
    gboolean is_method;
    gboolean is_constructor;
    gboolean invoke_ok;
    GITypeInfo *return_info;
    GITypeTag return_tag;
    PyObject **return_values;
    int n_return_values;
    int next_rval;
    PyObject *retval;
    PyObject *py_arg;

    flags = g_function_info_get_flags((GIFunctionInfo*)self->info);
    is_method = (flags & GI_FUNCTION_IS_METHOD) != 0;
    is_constructor = (flags & GI_FUNCTION_IS_CONSTRUCTOR) != 0;

    expected_in_argc = 0;
    expected_out_argc = 0;

    n_args = g_callable_info_get_n_args( (GICallableInfo*) self->info);
    for (i = 0; i < n_args; i++) {
        GIDirection direction;
        GIArgInfo *arg_info;

        arg_info = g_callable_info_get_arg( (GICallableInfo*) self->info, i);
        direction = g_arg_info_get_direction(arg_info);
        if (direction == GI_DIRECTION_IN || direction == GI_DIRECTION_INOUT)
            expected_in_argc += 1;
        if (direction == GI_DIRECTION_OUT || direction == GI_DIRECTION_INOUT)
            expected_out_argc += 1;
        g_base_info_unref( (GIBaseInfo*) arg_info);
    }
    /*
    g_debug("Call is to %s %s.%s with expected: %d in args, %d out args, %d total args",
                      is_method ? "method" : "function",
                      g_base_info_get_namespace( (GIBaseInfo*) self->info),
                      g_base_info_get_name( (GIBaseInfo*) self->info),
                      expected_in_argc,
                      expected_out_argc,
                      n_args);
    */
    if (is_method)
        expected_in_argc += 1;

    in_args = g_newa(GArgument, expected_in_argc);
    out_args = g_newa(GArgument, expected_out_argc);
    /* each out arg is a pointer, they point to these values */
    /* FIXME: This will break for caller-allocates funcs:
       http://bugzilla.gnome.org/show_bug.cgi?id=573314 */
    out_values = g_newa(GArgument, expected_out_argc);

    failed = FALSE;
    in_args_pos = 0; /* index into in_args */
    out_args_pos = 0; /* into out_args */
    argv_pos = 0; /* index into argv */

    if (is_method && !is_constructor) {
        GIBaseInfo *container = g_base_info_get_container((GIBaseInfo *) self->info);
        GIInfoType type = g_base_info_get_type(container);

        py_arg = PyTuple_GetItem(args, 0);
        if (!py_arg) {
            PyErr_SetString(PyExc_ValueError, "Calling a method without passing an instance");
            return NULL;
        }
        if (py_arg == Py_None) {
            in_args[0].v_pointer = NULL;
        } else if (type == GI_INFO_TYPE_STRUCT || type == GI_INFO_TYPE_BOXED) {
            PyObject *pybuffer = PyObject_GetAttrString((PyObject *)py_arg,
                                                        "__buffer__");
            PyBufferProcs *buffer_procs = pybuffer->ob_type->tp_as_buffer;
            (*buffer_procs->bf_getreadbuffer)(pybuffer, 0, &in_args[0].v_pointer);
        } else { /* by fallback is always object */
            in_args[0].v_pointer = pygobject_get(py_arg);
        }
        ++in_args_pos;
    }

    for (i = 0; i < n_args; i++) {
        GIDirection direction;
        GIArgInfo *arg_info;
        GArgument *out_value;

        arg_info = g_callable_info_get_arg( (GICallableInfo*) self->info, i);
        direction = g_arg_info_get_direction(arg_info);

        out_value = NULL;
        if (direction == GI_DIRECTION_OUT || direction == GI_DIRECTION_INOUT) {
            g_assert(out_args_pos < expected_out_argc);

            out_value = &out_values[out_args_pos];
            out_args[out_args_pos].v_pointer = out_value;
            ++out_args_pos;
        }

        if (direction == GI_DIRECTION_IN || direction == GI_DIRECTION_INOUT) {
            if (is_method || is_constructor)
                py_arg = PyTuple_GetItem(args, i + 1);
            else
                py_arg = PyTuple_GetItem(args, i);

            GArgument in_value = pyg_argument_from_pyobject(py_arg, g_arg_info_get_type(arg_info));

            ++argv_pos;

            if (direction == GI_DIRECTION_IN) {
                in_args[in_args_pos] = in_value;
            } else {
                /* INOUT means we pass a pointer */
                g_assert(out_value != NULL);
                *out_value = in_value;
                in_args[in_args_pos].v_pointer = out_value;
            }

            ++in_args_pos;
        }

        g_base_info_unref( (GIBaseInfo*) arg_info);

        if (failed)
            break;
    }

    if (failed) {
        PyErr_SetString(PyExc_ValueError, "Failed to convert all args.");
        return NULL;
    }

    g_assert(in_args_pos == expected_in_argc);
    g_assert(out_args_pos == expected_out_argc);

    error = NULL;
    invoke_ok = g_function_info_invoke( (GIFunctionInfo*) self->info,
                                        in_args, expected_in_argc,
                                        out_args, expected_out_argc,
                                        &return_arg,
                                        &error);

    return_info = g_callable_info_get_return_type( (GICallableInfo*) self->info);
    g_assert(return_info != NULL);

    if (!invoke_ok) {
        char buf[256];
        snprintf(buf, sizeof(buf), "Error invoking %s.%s: %s",
                 g_base_info_get_namespace( (GIBaseInfo*) self->info),
                 g_base_info_get_name( (GIBaseInfo*) self->info),
                 error->message);

        g_assert(error != NULL);
        PyErr_SetString(PyExc_RuntimeError, buf);
        g_error_free(error);

        return NULL;
    }

    return_tag = g_type_info_get_tag(return_info);

    if (is_constructor) {
        py_arg = PyTuple_GetItem(args, 0);

        if (return_tag == GI_TYPE_TAG_INTERFACE) {
            GIBaseInfo *interface_info = g_type_info_get_interface(return_info);
            GIInfoType interface_type = g_base_info_get_type(interface_info);

            if (interface_type == GI_INFO_TYPE_STRUCT || interface_type == GI_INFO_TYPE_BOXED) {
                // FIXME: We should reuse this. Perhaps by separating the
                // wrapper creation from the binding to the wrapper.
                gsize size = g_struct_info_get_size ((GIStructInfo*)return_info);
                PyObject *buffer = PyBuffer_FromReadWriteMemory(return_arg.v_pointer, size);

                //PyObject *dict = PyObject_GetDict(py_arg);
                PyObject_SetAttrString(py_arg, "__buffer__", buffer);

                Py_INCREF(py_arg);
                return py_arg;
            } else {
                PyGObject *self = (PyGObject *) py_arg;
                if (self->obj != NULL) {
                    PyErr_SetString(PyExc_ValueError, "Calling constructor on an instance that isn't a GObject");
                    return NULL;
                }
                self->obj = return_arg.v_pointer;
                g_object_ref(return_arg.v_pointer);
                pygobject_register_wrapper(py_arg);
                Py_INCREF(py_arg);
                return py_arg;
            }
        } else {
            PyErr_SetString(PyExc_NotImplementedError, "");
            return NULL;
        }
    }

    retval = NULL;

    next_rval = 0; /* index into return_values */

    n_return_values = expected_out_argc;
    if (return_tag != GI_TYPE_TAG_VOID)
        n_return_values += 1;

    return_values = g_newa(PyObject*, n_return_values);
    if (!is_constructor && n_return_values > 0) {
        if (return_tag != GI_TYPE_TAG_VOID) {
            PyObject *obj = pyg_argument_to_pyobject(&return_arg, return_info);
            if (obj == NULL) {
                return NULL;
            }
            return_values[next_rval] = obj;

            ++next_rval;
        }
    }

    /* We walk over all args, release in args (if allocated) and convert
     * all out args
     */
    in_args_pos = is_method ? 1 : 0; /* index into in_args */
    out_args_pos = 0; /* into out_args */

    for (i = 0; i < n_args; i++) {
        GIDirection direction;
        GIArgInfo *arg_info;
        GITypeInfo *arg_type_info;

        arg_info = g_callable_info_get_arg( (GICallableInfo*) self->info, i);
        direction = g_arg_info_get_direction(arg_info);

        arg_type_info = g_arg_info_get_type(arg_info);

        if (direction == GI_DIRECTION_IN) {
            g_assert(in_args_pos < expected_in_argc);

            ++in_args_pos;
        } else {
            /* INOUT or OUT */
            if (direction == GI_DIRECTION_INOUT)
                g_assert(in_args_pos < expected_in_argc);
            g_assert(next_rval < n_return_values);
            g_assert(out_args_pos < expected_out_argc);

            PyObject *obj;
            GITypeTag type_tag = g_type_info_get_tag(arg_type_info);

            if (type_tag == GI_TYPE_TAG_ARRAY) {
                GArgument *arg = out_args[out_args_pos].v_pointer;
                gint length_arg_index = g_type_info_get_array_length(arg_type_info);
                GArgument *length_arg;

                if (is_method)
                    length_arg_index--;

                if (length_arg_index == -1) {
                    PyErr_SetString(PyExc_NotImplementedError, "Need a field to specify the array length");
                    return NULL;
                }

                length_arg = out_args[length_arg_index].v_pointer;

                if (length_arg == NULL) {
                    PyErr_SetString(PyExc_RuntimeError, "Failed to get the length of the array");
                    return NULL;
                }

                obj = pyarray_to_pyobject(arg->v_pointer, length_arg->v_int, arg_type_info);
            } else
                obj = pyg_argument_to_pyobject(out_args[out_args_pos].v_pointer, arg_type_info);
            if (obj == NULL) {
                return NULL;
            }
            return_values[next_rval] = obj;

            if (direction == GI_DIRECTION_INOUT)
                ++in_args_pos;

            ++out_args_pos;

            ++next_rval;
        }

        g_base_info_unref( (GIBaseInfo*) arg_type_info);
        g_base_info_unref( (GIBaseInfo*) arg_info);
    }

    g_assert(next_rval == n_return_values);
    g_assert(out_args_pos == expected_out_argc);
    g_assert(in_args_pos == expected_in_argc);

    if (n_return_values > 0) {
        if (n_return_values == 0) {
            retval = Py_None;
            Py_INCREF(retval);
        } else if (n_return_values == 1) {
            retval = return_values[0];
        } else {
            retval = PyTuple_New(n_return_values);
            for (i = 0; i < n_return_values; i++) {
                PyTuple_SetItem(retval, i, return_values[i]);
            }
        }
    }

    g_base_info_unref( (GIBaseInfo*) return_info);

    if (retval == NULL) {
        Py_INCREF(Py_None);
        retval = Py_None;
    }

    return retval;
}

static PyMethodDef _PyGIFunctionInfo_methods[] = {
    { "isConstructor", (PyCFunction)_wrap_g_function_info_is_constructor, METH_NOARGS },
    { "isMethod", (PyCFunction)_wrap_g_function_info_is_method, METH_NOARGS },
    { "invoke", (PyCFunction)_wrap_g_function_info_invoke, METH_VARARGS },
    { NULL, NULL, 0 }
};

/* GICallbackInfo */
NEW_CLASS("CallbackInfo", GICallbackInfo);

static PyMethodDef _PyGICallbackInfo_methods[] = {
    { NULL, NULL, 0 }
};

/* RegisteredTypeInfo */
NEW_CLASS("RegisteredTypeInfo", GIRegisteredTypeInfo);

static PyObject *
_wrap_g_registered_type_info_get_g_type (PyGIBaseInfo* self)
{
    int gtype;

    gtype = g_registered_type_info_get_g_type ((GIRegisteredTypeInfo*)self->info);
    return PyInt_FromLong(gtype);
}

static PyMethodDef _PyGIRegisteredTypeInfo_methods[] = {
    { "getGType", (PyCFunction)_wrap_g_registered_type_info_get_g_type, METH_NOARGS },
    { NULL, NULL, 0 }
};

/* GIStructInfo */
NEW_CLASS("StructInfo", GIStructInfo);

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
NEW_CLASS("UnionInfo", GIUnionInfo);

static PyMethodDef _PyGIUnionInfo_methods[] = {
    { NULL, NULL, 0 }
};

/* EnumInfo */
NEW_CLASS("EnumInfo", GIEnumInfo);

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
NEW_CLASS("BoxedInfo", GIBoxedInfo);

static PyMethodDef _PyGIBoxedInfo_methods[] = {
    { NULL, NULL, 0 }
};

/* ObjectInfo */
NEW_CLASS("ObjectInfo", GIObjectInfo);

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
NEW_CLASS("InterfaceInfo", GIInterfaceInfo);

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
NEW_CLASS("ConstantInfo", GIConstantInfo);

static PyMethodDef _PyGIConstantInfo_methods[] = {
    { NULL, NULL, 0 }
};


/* GIValueInfo */
NEW_CLASS("ValueInfo", GIValueInfo);

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


/* GISignalInfo */
NEW_CLASS("SignalInfo", GISignalInfo);

static PyMethodDef _PyGISignalInfo_methods[] = {
    { NULL, NULL, 0 }
};


/* GIVFuncInfo */
NEW_CLASS("VFuncInfo", GIVFuncInfo);

static PyMethodDef _PyGIVFuncInfo_methods[] = {
    { NULL, NULL, 0 }
};

/* GIPropertyInfo */
NEW_CLASS("PropertyInfo", GIPropertyInfo);

static PyMethodDef _PyGIPropertyInfo_methods[] = {
    { NULL, NULL, 0 }
};

/* GIFieldInfo */
NEW_CLASS("FieldInfo", GIFieldInfo);

static PyObject *
_wrap_g_field_info_get_value(PyGIBaseInfo *self, PyObject *args)
{
    PyObject *obj;
    gpointer buffer;
    GArgument value;
    GIBaseInfo *container;
    GIInfoType container_type;
    GIFieldInfo *field_info;
    GITypeInfo *field_type_info;
    PyObject *retval;

    retval = NULL;

    field_info = (GIFieldInfo *)self->info;

    if (!PyArg_ParseTuple(args, "O:TypeInfo.getValue", &obj))
        return NULL;

    container = g_base_info_get_container((GIBaseInfo *)field_info);
    container_type = g_base_info_get_type(container);

    field_type_info = g_field_info_get_type(field_info);

    if (container_type == GI_INFO_TYPE_STRUCT || container_type == GI_INFO_TYPE_BOXED) {
        PyBufferProcs *py_buffer_procs;
        PyObject *py_buffer;

        py_buffer = PyObject_GetAttrString(obj, "__buffer__");
        if (py_buffer == NULL) {
            goto field_info_get_value_return;
        }

        py_buffer_procs = py_buffer->ob_type->tp_as_buffer;
        if (py_buffer_procs == NULL || py_buffer_procs->bf_getreadbuffer == 0) {
            Py_DECREF(py_buffer);
            PyErr_SetString(PyExc_RuntimeError, "Failed to get buffer for struct");
            goto field_info_get_value_return;
        }

        (*py_buffer_procs->bf_getreadbuffer)(py_buffer, 0, &buffer);
    } else {
        buffer = ((PyGObject *) obj)->obj;
    }

    /* A few types are not handled by g_field_info_get_field, so do it here. */
    if (!g_type_info_is_pointer(field_type_info) && g_type_info_get_tag(field_type_info) == GI_TYPE_TAG_INTERFACE) {
        GIBaseInfo *interface_info;

        if (!g_field_info_get_flags (field_info) & GI_FIELD_IS_READABLE) {
            PyErr_SetString(PyExc_RuntimeError, "Field is not readable");
            goto field_info_get_value_return;
        }

        interface_info = g_type_info_get_interface (field_type_info);
        switch(g_base_info_get_type(interface_info))
        {
            case GI_INFO_TYPE_STRUCT:
            {
                gsize offset;
                gsize size;

                offset = g_field_info_get_offset(field_info);
                size = g_struct_info_get_size((GIStructInfo *)interface_info);
                g_assert(size > 0);

                value.v_pointer = g_try_malloc(size);
                if (value.v_pointer == NULL) {
                    PyErr_SetString(PyExc_MemoryError, "Failed to allocate memory");
                    break;
                }
                g_memmove(value.v_pointer, buffer + offset, size);
                break;
            }
            case GI_INFO_TYPE_UNION:
            case GI_INFO_TYPE_BOXED:
                PyErr_SetString(PyExc_NotImplementedError, "Interface type not handled yet");
                break;
            default:
                if (!g_field_info_get_field(field_info, buffer, &value)) {
                    PyErr_SetString(PyExc_RuntimeError, "Failed to get value for field");
                }
        }

        g_base_info_unref(interface_info);

        if (PyErr_Occurred()) {
            goto field_info_get_value_return;
        }
    } else if (!g_field_info_get_field (field_info, buffer, &value)) {
        PyErr_SetString(PyExc_RuntimeError, "Failed to get value for field");
        goto field_info_get_value_return;
    }

    retval = pyg_argument_to_pyobject (&value, g_field_info_get_type (field_info));

field_info_get_value_return:
    g_base_info_unref((GIBaseInfo *)field_type_info);

    Py_XINCREF(retval);
    return retval;
}

static PyObject *
_wrap_g_field_info_set_value(PyGIBaseInfo *self, PyObject *args)
{
    PyObject *obj;
    PyObject *value;
    gpointer buffer;
    GArgument arg;
    GIFieldInfo *field_info;
    GIBaseInfo *container;
    GIInfoType container_type;
    GITypeInfo *field_type_info;
    PyObject *retval;

    retval = Py_None;

    field_info = (GIFieldInfo *)self->info;

    if (!PyArg_ParseTuple(args, "OO:TypeInfo.setValue", &obj, &value))
        return NULL;

    container = g_base_info_get_container((GIBaseInfo *) self->info);
    container_type = g_base_info_get_type(container);

    field_type_info = g_field_info_get_type(field_info);

    if (container_type == GI_INFO_TYPE_STRUCT || container_type == GI_INFO_TYPE_BOXED) {
        PyObject *py_buffer;
        PyBufferProcs *py_buffer_procs;

        py_buffer = PyObject_GetAttrString(obj, "__buffer__");
        if (py_buffer == NULL) {
            retval = NULL;
            goto field_info_set_value_return;
        }

        py_buffer_procs = py_buffer->ob_type->tp_as_buffer;
        if (py_buffer_procs == NULL || py_buffer_procs->bf_getreadbuffer == 0) {
            Py_DECREF(py_buffer);
            PyErr_SetString(PyExc_RuntimeError, "Failed to get buffer for struct");
            retval = NULL;
            goto field_info_set_value_return;
        }

        (*py_buffer_procs->bf_getreadbuffer)(py_buffer, 0, &buffer);
    } else {
        buffer = ((PyGObject *) obj)->obj;
    }

    arg = pyg_argument_from_pyobject(value, field_type_info);

    /* A few types are not handled by g_field_info_set_field, so do it here. */
    if (!g_type_info_is_pointer(field_type_info) && g_type_info_get_tag(field_type_info) == GI_TYPE_TAG_INTERFACE) {
        GIBaseInfo *interface_info;

        if (!g_field_info_get_flags (field_info) & GI_FIELD_IS_WRITABLE) {
            PyErr_SetString(PyExc_RuntimeError, "Field is not writable");
            retval = NULL;
            goto field_info_set_value_return;
        }

        interface_info = g_type_info_get_interface (field_type_info);
        switch (g_base_info_get_type(interface_info))
        {
            case GI_INFO_TYPE_STRUCT:
            {
                gsize offset;
                gsize size;

                offset = g_field_info_get_offset(field_info);
                size = g_struct_info_get_size((GIStructInfo *)interface_info);
                g_assert(size > 0);

                g_memmove(buffer + offset, arg.v_pointer, size);
                break;
            }
            case GI_INFO_TYPE_UNION:
            case GI_INFO_TYPE_BOXED:
                PyErr_SetString(PyExc_NotImplementedError, "Interface type not handled yet");
                retval = NULL;
                break;
            default:
                if (!g_field_info_set_field(field_info, buffer, &arg)) {
                    PyErr_SetString(PyExc_RuntimeError, "Failed to set value for field");
                    retval = NULL;
                }
        }

        g_base_info_unref(interface_info);

    } else if (!g_field_info_set_field(field_info, buffer, &arg)) {
        PyErr_SetString(PyExc_RuntimeError, "Failed to set value for field");
        retval = NULL;
    }

field_info_set_value_return:
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
NEW_CLASS("ArgInfo", GIArgInfo);

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
NEW_CLASS("TypeInfo", GITypeInfo);

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
NEW_CLASS("UnresolvedInfo", GIUnresolvedInfo);

static PyMethodDef _PyGIUnresolvedInfo_methods[] = {
    { NULL, NULL, 0 }
};


