/* -*- Mode: C; c-basic-offset: 4 -*-
 * vim: tabstop=4 shiftwidth=4 expandtab
 *
 * Copyright (C) 2005-2009 Johan Dahlin <johan@gnome.org>
 *
 *   gimodule.c: wrapper for the gobject-introspection library.
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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301
 * USA
 */

#include "pygi-private.h"

#include <pygobject.h>

# include <pycairo.h>
Pycairo_CAPI_t *Pycairo_CAPI;

static PyObject *
_wrap_pyg_enum_add (PyObject *self,
                    PyObject *args,
                    PyObject *kwargs)
{
    static char *kwlist[] = { "g_type", NULL };
    PyObject *py_g_type;
    GType g_type;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs,
                "O!:enum_add",
                kwlist, &PyGTypeWrapper_Type, &py_g_type)) {
        return NULL;
    }

    g_type = pyg_type_from_object(py_g_type);
    if (g_type == G_TYPE_INVALID) {
        return NULL;
    }

    return pyg_enum_add(NULL, g_type_name(g_type), NULL, g_type);
}

static PyObject *
_wrap_pyg_flags_add (PyObject *self,
                     PyObject *args,
                     PyObject *kwargs)
{
    static char *kwlist[] = { "g_type", NULL };
    PyObject *py_g_type;
    GType g_type;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs,
                "O!:flags_add",
                kwlist, &PyGTypeWrapper_Type, &py_g_type)) {
        return NULL;
    }

    g_type = pyg_type_from_object(py_g_type);
    if (g_type == G_TYPE_INVALID) {
        return NULL;
    }

    return pyg_flags_add(NULL, g_type_name(g_type), NULL, g_type);
}

static PyObject *
_wrap_pyg_set_object_has_new_constructor (PyObject *self,
                                          PyObject *args,
                                          PyObject *kwargs)
{
    static char *kwlist[] = { "g_type", NULL };
    PyObject *py_g_type;
    GType g_type;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs,
                "O!:set_object_has_new_constructor",
                kwlist, &PyGTypeWrapper_Type, &py_g_type)) {
        return NULL;
    }

    g_type = pyg_type_from_object(py_g_type);
    if (!g_type_is_a(g_type, G_TYPE_OBJECT)) {
        PyErr_SetString(PyExc_TypeError, "must be a subtype of GObject");
        return NULL;
    }

    pyg_set_object_has_new_constructor(g_type);

    Py_RETURN_NONE;
}

static void
initialize_interface (GTypeInterface *iface, PyTypeObject *pytype)
{
    // pygobject prints a warning if interface_init is NULL
}

static PyObject *
_wrap_pyg_register_interface_info(PyObject *self, PyObject *args)
{
    PyObject *py_g_type;
    GType g_type;
    GInterfaceInfo *info;

    if (!PyArg_ParseTuple(args, "O!:register_interface_info",
                          &PyGTypeWrapper_Type, &py_g_type)) {
        return NULL;
    }

    g_type = pyg_type_from_object(py_g_type);
    if (!g_type_is_a(g_type, G_TYPE_INTERFACE)) {
        PyErr_SetString(PyExc_TypeError, "must be an interface");
        return NULL;
    }

    info = g_new0(GInterfaceInfo, 1);
    info->interface_init = (GInterfaceInitFunc) initialize_interface;

    pyg_register_interface_info(g_type, info);

    Py_RETURN_NONE;
}

static PyObject *
_wrap_pyg_hook_up_vfunc_implementation (PyObject *self, PyObject *args)
{
    PyGIBaseInfo *py_info;
    PyObject *py_type;
    PyObject *py_function;
    gpointer implementor_class = NULL;
    GType ancestor_g_type = 0;
    GType implementor_gtype = 0;
    gpointer *method_ptr = NULL;
    int length, i;
    GIBaseInfo *vfunc_info;
    GIBaseInfo *ancestor_info;
    GIStructInfo *struct_info;
    gboolean is_interface = FALSE;
    PyGICClosure *closure = NULL;

    if (!PyArg_ParseTuple(args, "O!O!O:hook_up_vfunc_implementation",
                          &PyGIBaseInfo_Type, &py_info,
                          &PyGTypeWrapper_Type, &py_type,
                          &py_function))
        return NULL;

    implementor_gtype = pyg_type_from_object(py_type);
    g_assert(G_TYPE_IS_CLASSED(implementor_gtype));

    vfunc_info = py_info->info;
    ancestor_info = g_base_info_get_container(vfunc_info);
    is_interface = g_base_info_get_type(ancestor_info) == GI_INFO_TYPE_INTERFACE;

    ancestor_g_type = g_registered_type_info_get_g_type(
            (GIRegisteredTypeInfo *)ancestor_info);

    implementor_class = g_type_class_ref(implementor_gtype);
    if (is_interface) {
        GTypeInstance *implementor_iface_class;
        implementor_iface_class = g_type_interface_peek(implementor_class,
                                                        ancestor_g_type);
        g_type_class_unref (implementor_class);
        implementor_class = implementor_iface_class;

        struct_info = g_interface_info_get_iface_struct ((GIInterfaceInfo*)ancestor_info);
    } else
        struct_info = g_object_info_get_class_struct ((GIObjectInfo*)ancestor_info);

    length = g_struct_info_get_n_fields(struct_info);
    for (i = 0; i < length; i++) {
        GIFieldInfo *field_info;
        GITypeInfo *type_info;
        GIBaseInfo *interface_info;
        GICallbackInfo *callback_info;
        gint offset;

        field_info = g_struct_info_get_field (struct_info, i);

        if (strcmp(g_base_info_get_name((GIBaseInfo*) field_info),
                   g_base_info_get_name((GIBaseInfo*) vfunc_info)) != 0)
            continue;

        type_info = g_field_info_get_type (field_info);
        if (g_type_info_get_tag (type_info) != GI_TYPE_TAG_INTERFACE)
            continue;

        interface_info = g_type_info_get_interface (type_info);
        g_assert(g_base_info_get_type(interface_info) == GI_INFO_TYPE_CALLBACK);

        callback_info = (GICallbackInfo*) interface_info;
        offset = g_field_info_get_offset(field_info);
        method_ptr = G_STRUCT_MEMBER_P(implementor_class, offset);

        closure = _pygi_make_native_closure((GICallableInfo*)callback_info,
            GI_SCOPE_TYPE_NOTIFIED, py_function, NULL);

        *method_ptr = closure->closure;

        g_base_info_unref (interface_info);
        g_base_info_unref (type_info);
        g_base_info_unref (field_info);

        break;
    }

    g_base_info_unref (struct_info);

    if (!is_interface)
        g_type_class_unref (implementor_class);

    Py_RETURN_NONE;
}

static void
_sink_gobject(GObject *obj)
{
    if (G_IS_INITIALLY_UNOWNED(obj) || g_object_is_floating(obj)) {
            g_object_ref_sink(obj);
    }
}

static PyMethodDef _pygi_functions[] = {
    { "enum_add", (PyCFunction)_wrap_pyg_enum_add, METH_VARARGS | METH_KEYWORDS },
    { "flags_add", (PyCFunction)_wrap_pyg_flags_add, METH_VARARGS | METH_KEYWORDS },

    { "set_object_has_new_constructor", (PyCFunction)_wrap_pyg_set_object_has_new_constructor, METH_VARARGS | METH_KEYWORDS },
    { "register_interface_info", (PyCFunction)_wrap_pyg_register_interface_info, METH_VARARGS },
    { "hook_up_vfunc_implementation", (PyCFunction)_wrap_pyg_hook_up_vfunc_implementation, METH_VARARGS },
    { NULL, NULL, 0 }
};

struct PyGI_API PyGI_API = {
    pygi_type_import_by_g_type
};


PyMODINIT_FUNC
init_gi(void)
{
    PyObject *m;
    PyObject *api;

    m = Py_InitModule("_gi", _pygi_functions);
    if (m == NULL) {
        return;
    }

    if (pygobject_init(-1, -1, -1) == NULL) {
        return;
    }

    if (_pygobject_import() < 0) {
        return;
    }

    Pycairo_IMPORT;
    if (Pycairo_CAPI == NULL)
        return;

    // register our floating object sink while we wait for the pygobject
    // patch to be approved.  Once the patch is in this becomes a noop
    // For more details - https://bugzilla.gnome.org/show_bug.cgi?id=583909
    pygobject_register_sinkfunc(G_TYPE_OBJECT, _sink_gobject);

    _pygi_repository_register_types(m);
    _pygi_info_register_types(m);
    _pygi_struct_register_types(m);
    _pygi_boxed_register_types(m);
    _pygi_argument_init();

    api = PyCObject_FromVoidPtr((void *)&PyGI_API, NULL);
    if (api == NULL) {
        return;
    }
    PyModule_AddObject(m, "_API", api);
}

