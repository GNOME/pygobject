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

#include <Python.h>
#include <glib-object.h>

#include "config.h"
#include "pyglib.h"
#include "pyginterface.h"
#include "pygi-repository.h"
#include "pyglib.h"
#include "pygtype.h"
#include "pygenum.h"
#include "pygboxed.h"
#include "pygflags.h"
#include "pygi-error.h"
#include "pygi-foreign.h"
#include "pygi-resulttuple.h"
#include "pygi-source.h"
#include "pygi-ccallback.h"
#include "pygi-closure.h"
#include "pygi-type.h"
#include "pygi-boxed.h"
#include "pygi-info.h"
#include "pygi-struct.h"
#include "pygobject-object.h"
#include "pygoptioncontext.h"
#include "pygoptiongroup.h"
#include "pygspawn.h"
#include "gobjectmodule.h"
#include "pygparamspec.h"
#include "pygpointer.h"

#include <pyglib-python-compat.h>

PyObject *PyGIWarning;
PyObject *PyGIDeprecationWarning;
PyObject *_PyGIDefaultArgPlaceholder;


/* Defined by PYGLIB_MODULE_START */
extern PyObject *pyglib__gobject_module_create (void);

/* Returns a new flag/enum type or %NULL */
static PyObject *
flags_enum_from_gtype (GType g_type,
                       PyObject * (add_func) (PyObject *, const char *,
                                              const char *, GType))
{
    PyObject *new_type;
    GIRepository *repository;
    GIBaseInfo *info;
    const gchar *type_name;

    repository = g_irepository_get_default ();
    info = g_irepository_find_by_gtype (repository, g_type);
    if (info != NULL) {
        type_name = g_base_info_get_name (info);
        new_type = add_func (NULL, type_name, NULL, g_type);
        g_base_info_unref (info);
    } else {
        type_name = g_type_name (g_type);
        new_type = add_func (NULL, type_name, NULL, g_type);
    }

    return new_type;
}


static PyObject *
_wrap_pyg_enum_add (PyObject *self,
                    PyObject *args,
                    PyObject *kwargs)
{
    static char *kwlist[] = { "g_type", NULL };
    PyObject *py_g_type;
    GType g_type;

    if (!PyArg_ParseTupleAndKeywords (args, kwargs,
                                      "O!:enum_add",
                                      kwlist, &PyGTypeWrapper_Type, &py_g_type)) {
        return NULL;
    }

    g_type = pyg_type_from_object (py_g_type);
    if (g_type == G_TYPE_INVALID) {
        return NULL;
    }

    return flags_enum_from_gtype (g_type, pyg_enum_add);
}

static PyObject *
_wrap_pyg_enum_register_new_gtype_and_add (PyObject *self,
                                           PyObject *args,
                                           PyObject *kwargs)
{
    static char *kwlist[] = { "info", NULL };
    PyGIBaseInfo *py_info;
    GIEnumInfo *info;
    gint n_values;
    GEnumValue *g_enum_values;
    int i;
    const gchar *namespace;
    const gchar *type_name;
    gchar *full_name;
    GType g_type;

    if (!PyArg_ParseTupleAndKeywords (args, kwargs,
                                      "O:enum_add_make_new_gtype",
                                      kwlist, (PyObject *)&py_info)) {
        return NULL;
    }

    if (!GI_IS_ENUM_INFO (py_info->info) ||
            g_base_info_get_type ((GIBaseInfo *) py_info->info) != GI_INFO_TYPE_ENUM) {
        PyErr_SetString (PyExc_TypeError, "info must be an EnumInfo with info type GI_INFO_TYPE_ENUM");
        return NULL;
    }

    info = (GIEnumInfo *)py_info->info;
    n_values = g_enum_info_get_n_values (info);

    /* The new memory is zero filled which fulfills the registration
     * function requirement that the last item is zeroed out as a terminator.
     */
    g_enum_values = g_new0 (GEnumValue, n_values + 1);

    for (i = 0; i < n_values; i++) {
        GIValueInfo *value_info;
        GEnumValue *enum_value;
        const gchar *name;
        const gchar *c_identifier;

        value_info = g_enum_info_get_value (info, i);
        name = g_base_info_get_name ((GIBaseInfo *) value_info);
        c_identifier = g_base_info_get_attribute ((GIBaseInfo *) value_info,
                                                  "c:identifier");

        enum_value = &g_enum_values[i];
        enum_value->value_nick = g_strdup (name);
        enum_value->value = g_value_info_get_value (value_info);

        if (c_identifier == NULL) {
            enum_value->value_name = enum_value->value_nick;
        } else {
            enum_value->value_name = g_strdup (c_identifier);
        }

        g_base_info_unref ((GIBaseInfo *) value_info);
    }

    /* Obfuscate the full_name by prefixing it with "Py" to avoid conflicts
     * with real GTypes. See: https://bugzilla.gnome.org/show_bug.cgi?id=692515
     */
    namespace = g_base_info_get_namespace ((GIBaseInfo *) info);
    type_name = g_base_info_get_name ((GIBaseInfo *) info);
    full_name = g_strconcat ("Py", namespace, type_name, NULL);

    /* If enum registration fails, free all the memory allocated
     * for the values array. This needs to leak when successful
     * as GObject keeps a reference to the data as specified in the docs.
     */
    g_type = g_enum_register_static (full_name, g_enum_values);
    if (g_type == G_TYPE_INVALID) {
        for (i = 0; i < n_values; i++) {
            GEnumValue *enum_value = &g_enum_values[i];

            /* Only free value_name if it is different from value_nick to avoid
             * a double free. The pointer might have been is re-used in the case
             * c_identifier was NULL in the above loop.
             */
            if (enum_value->value_name != enum_value->value_nick)
                g_free ((gchar *) enum_value->value_name);
            g_free ((gchar *) enum_value->value_nick);
        }

        PyErr_Format (PyExc_RuntimeError, "Unable to register enum '%s'", full_name);

        g_free (g_enum_values);
        g_free (full_name);
        return NULL;
    }

    g_free (full_name);
    return pyg_enum_add (NULL, type_name, NULL, g_type);
}

static PyObject *
_wrap_pyg_flags_add (PyObject *self,
                     PyObject *args,
                     PyObject *kwargs)
{
    static char *kwlist[] = { "g_type", NULL };
    PyObject *py_g_type;
    GType g_type;

    if (!PyArg_ParseTupleAndKeywords (args, kwargs,
                                      "O!:flags_add",
                                      kwlist, &PyGTypeWrapper_Type, &py_g_type)) {
        return NULL;
    }

    g_type = pyg_type_from_object (py_g_type);
    if (g_type == G_TYPE_INVALID) {
        return NULL;
    }

    return flags_enum_from_gtype (g_type, pyg_flags_add);
}

static PyObject *
_wrap_pyg_flags_register_new_gtype_and_add (PyObject *self,
                                            PyObject *args,
                                            PyObject *kwargs)
{
    static char *kwlist[] = { "info", NULL };
    PyGIBaseInfo *py_info;
    GIEnumInfo *info;
    gint n_values;
    GFlagsValue *g_flags_values;
    int i;
    const gchar *namespace;
    const gchar *type_name;
    gchar *full_name;
    GType g_type;

    if (!PyArg_ParseTupleAndKeywords (args, kwargs,
                                      "O:flags_add_make_new_gtype",
                                      kwlist, (PyObject *)&py_info)) {
        return NULL;
    }

    if (!GI_IS_ENUM_INFO (py_info->info) ||
            g_base_info_get_type ((GIBaseInfo *) py_info->info) != GI_INFO_TYPE_FLAGS) {
        PyErr_SetString (PyExc_TypeError, "info must be an EnumInfo with info type GI_INFO_TYPE_FLAGS");
        return NULL;
    }

    info = (GIEnumInfo *)py_info->info;
    n_values = g_enum_info_get_n_values (info);

    /* The new memory is zero filled which fulfills the registration
     * function requirement that the last item is zeroed out as a terminator.
     */
    g_flags_values = g_new0 (GFlagsValue, n_values + 1);

    for (i = 0; i < n_values; i++) {
        GIValueInfo *value_info;
        GFlagsValue *flags_value;
        const gchar *name;
        const gchar *c_identifier;

        value_info = g_enum_info_get_value (info, i);
        name = g_base_info_get_name ((GIBaseInfo *) value_info);
        c_identifier = g_base_info_get_attribute ((GIBaseInfo *) value_info,
                                                  "c:identifier");

        flags_value = &g_flags_values[i];
        flags_value->value_nick = g_strdup (name);
        flags_value->value = g_value_info_get_value (value_info);

        if (c_identifier == NULL) {
            flags_value->value_name = flags_value->value_nick;
        } else {
            flags_value->value_name = g_strdup (c_identifier);
        }

        g_base_info_unref ((GIBaseInfo *) value_info);
    }

    /* Obfuscate the full_name by prefixing it with "Py" to avoid conflicts
     * with real GTypes. See: https://bugzilla.gnome.org/show_bug.cgi?id=692515
     */
    namespace = g_base_info_get_namespace ((GIBaseInfo *) info);
    type_name = g_base_info_get_name ((GIBaseInfo *) info);
    full_name = g_strconcat ("Py", namespace, type_name, NULL);

    /* If enum registration fails, free all the memory allocated
     * for the values array. This needs to leak when successful
     * as GObject keeps a reference to the data as specified in the docs.
     */
    g_type = g_flags_register_static (full_name, g_flags_values);
    if (g_type == G_TYPE_INVALID) {
        for (i = 0; i < n_values; i++) {
            GFlagsValue *flags_value = &g_flags_values[i];

            /* Only free value_name if it is different from value_nick to avoid
             * a double free. The pointer might have been is re-used in the case
             * c_identifier was NULL in the above loop.
             */
            if (flags_value->value_name != flags_value->value_nick)
                g_free ((gchar *) flags_value->value_name);
            g_free ((gchar *) flags_value->value_nick);
        }

        PyErr_Format (PyExc_RuntimeError, "Unable to register flags '%s'", full_name);

        g_free (g_flags_values);
        g_free (full_name);
        return NULL;
    }

    g_free (full_name);
    return pyg_flags_add (NULL, type_name, NULL, g_type);
}

static void
initialize_interface (GTypeInterface *iface, PyTypeObject *pytype)
{
    /* pygobject prints a warning if interface_init is NULL */
}

static PyObject *
_wrap_pyg_register_interface_info (PyObject *self, PyObject *args)
{
    PyObject *py_g_type;
    GType g_type;
    GInterfaceInfo *info;

    if (!PyArg_ParseTuple (args, "O!:register_interface_info",
                           &PyGTypeWrapper_Type, &py_g_type)) {
        return NULL;
    }

    g_type = pyg_type_from_object (py_g_type);
    if (!g_type_is_a (g_type, G_TYPE_INTERFACE)) {
        PyErr_SetString (PyExc_TypeError, "must be an interface");
        return NULL;
    }

    info = g_new0 (GInterfaceInfo, 1);
    info->interface_init = (GInterfaceInitFunc) initialize_interface;

    pyg_register_interface_info (g_type, info);

    Py_RETURN_NONE;
}

static void
find_vfunc_info (GIBaseInfo *vfunc_info,
                 GType implementor_gtype,
                 gpointer *implementor_class_ret,
                 gpointer *implementor_vtable_ret,
                 GIFieldInfo **field_info_ret)
{
    GType ancestor_g_type = 0;
    int length, i;
    GIBaseInfo *ancestor_info;
    GIStructInfo *struct_info;
    gpointer implementor_class = NULL;
    gboolean is_interface = FALSE;

    ancestor_info = g_base_info_get_container (vfunc_info);
    is_interface = g_base_info_get_type (ancestor_info) == GI_INFO_TYPE_INTERFACE;

    ancestor_g_type = g_registered_type_info_get_g_type (
                          (GIRegisteredTypeInfo *) ancestor_info);
    implementor_class = g_type_class_ref (implementor_gtype);
    if (is_interface) {
        GTypeInstance *implementor_iface_class;
        implementor_iface_class = g_type_interface_peek (implementor_class,
                                                         ancestor_g_type);
        if (implementor_iface_class == NULL) {
            g_type_class_unref (implementor_class);
            PyErr_Format (PyExc_RuntimeError,
                          "Couldn't find GType of implementor of interface %s. "
                          "Forgot to set __gtype_name__?",
                          g_type_name (ancestor_g_type));
            return;
        }

        *implementor_vtable_ret = implementor_iface_class;

        struct_info = g_interface_info_get_iface_struct ( (GIInterfaceInfo*) ancestor_info);
    } else {
        struct_info = g_object_info_get_class_struct ( (GIObjectInfo*) ancestor_info);
        *implementor_vtable_ret = implementor_class;
    }

    *implementor_class_ret = implementor_class;

    length = g_struct_info_get_n_fields (struct_info);
    for (i = 0; i < length; i++) {
        GIFieldInfo *field_info;
        GITypeInfo *type_info;

        field_info = g_struct_info_get_field (struct_info, i);

        if (strcmp (g_base_info_get_name ( (GIBaseInfo*) field_info),
                    g_base_info_get_name ( (GIBaseInfo*) vfunc_info)) != 0) {
            g_base_info_unref (field_info);
            continue;
        }

        type_info = g_field_info_get_type (field_info);
        if (g_type_info_get_tag (type_info) == GI_TYPE_TAG_INTERFACE) {
            g_base_info_unref (type_info);
            *field_info_ret = field_info;
            break;
        }

        g_base_info_unref (type_info);
        g_base_info_unref (field_info);
    }

    g_base_info_unref (struct_info);
}

static PyObject *
_wrap_pyg_hook_up_vfunc_implementation (PyObject *self, PyObject *args)
{
    PyGIBaseInfo *py_info;
    PyObject *py_type;
    PyObject *py_function;
    GType implementor_gtype = 0;
    gpointer implementor_class = NULL;
    gpointer implementor_vtable = NULL;
    GIFieldInfo *field_info = NULL;
    gpointer *method_ptr = NULL;
    PyGICClosure *closure = NULL;

    if (!PyArg_ParseTuple (args, "O!O!O:hook_up_vfunc_implementation",
                           &PyGIBaseInfo_Type, &py_info,
                           &PyGTypeWrapper_Type, &py_type,
                           &py_function))
        return NULL;

    implementor_gtype = pyg_type_from_object (py_type);
    g_assert (G_TYPE_IS_CLASSED (implementor_gtype));

    find_vfunc_info (py_info->info, implementor_gtype, &implementor_class, &implementor_vtable, &field_info);
    if (field_info != NULL) {
        GITypeInfo *type_info;
        GIBaseInfo *interface_info;
        GICallbackInfo *callback_info;
        gint offset;

        type_info = g_field_info_get_type (field_info);

        interface_info = g_type_info_get_interface (type_info);
        g_assert (g_base_info_get_type (interface_info) == GI_INFO_TYPE_CALLBACK);

        callback_info = (GICallbackInfo*) interface_info;
        offset = g_field_info_get_offset (field_info);
        method_ptr = G_STRUCT_MEMBER_P (implementor_vtable, offset);

        closure = _pygi_make_native_closure ( (GICallableInfo*) callback_info,
                                              GI_SCOPE_TYPE_NOTIFIED, py_function, NULL);

        *method_ptr = closure->closure;

        g_base_info_unref (interface_info);
        g_base_info_unref (type_info);
        g_base_info_unref (field_info);
    }
    g_type_class_unref (implementor_class);

    Py_RETURN_NONE;
}

#if 0
/* Not used, left around for future reference */
static PyObject *
_wrap_pyg_has_vfunc_implementation (PyObject *self, PyObject *args)
{
    PyGIBaseInfo *py_info;
    PyObject *py_type;
    PyObject *py_ret;
    gpointer implementor_class = NULL;
    gpointer implementor_vtable = NULL;
    GType implementor_gtype = 0;
    GIFieldInfo *field_info = NULL;

    if (!PyArg_ParseTuple (args, "O!O!:has_vfunc_implementation",
                           &PyGIBaseInfo_Type, &py_info,
                           &PyGTypeWrapper_Type, &py_type))
        return NULL;

    implementor_gtype = pyg_type_from_object (py_type);
    g_assert (G_TYPE_IS_CLASSED (implementor_gtype));

    py_ret = Py_False;
    find_vfunc_info (py_info->info, implementor_gtype, &implementor_class, &implementor_vtable, &field_info);
    if (field_info != NULL) {
        gpointer *method_ptr;
        gint offset;

        offset = g_field_info_get_offset (field_info);
        method_ptr = G_STRUCT_MEMBER_P (implementor_vtable, offset);
        if (*method_ptr != NULL) {
            py_ret = Py_True;
        }

        g_base_info_unref (field_info);
    }
    g_type_class_unref (implementor_class);

    Py_INCREF(py_ret);
    return py_ret;
}
#endif

static PyObject *
_wrap_pyg_variant_type_from_string (PyObject *self, PyObject *args)
{
    char *type_string;
    PyObject *py_type;
    PyObject *py_variant = NULL;

    if (!PyArg_ParseTuple (args, "s:variant_type_from_string",
                           &type_string)) {
        return NULL;
    }

    py_type = _pygi_type_import_by_name ("GLib", "VariantType");

    py_variant = _pygi_boxed_new ( (PyTypeObject *) py_type, type_string, FALSE, 0);

    return py_variant;
}

static PyObject *
_wrap_pyg_source_new (PyObject *self, PyObject *args)
{
    return pyg_source_new ();
}

#define CHUNK_SIZE 8192

static PyObject*
pyg_channel_read(PyObject* self, PyObject *args, PyObject *kwargs)
{
    int max_count = -1;
    PyObject *py_iochannel, *ret_obj = NULL;
    gsize total_read = 0;
    GError* error = NULL;
    GIOStatus status = G_IO_STATUS_NORMAL;
    GIOChannel *iochannel = NULL;

    if (!PyArg_ParseTuple (args, "Oi:pyg_channel_read", &py_iochannel, &max_count)) {
        return NULL;
    }
    if (!pyg_boxed_check (py_iochannel, G_TYPE_IO_CHANNEL)) {
        PyErr_SetString(PyExc_TypeError, "first argument is not a GLib.IOChannel");
        return NULL;
    }
	
    if (max_count == 0)
        return PYGLIB_PyBytes_FromString("");

    iochannel = pyg_boxed_get (py_iochannel, GIOChannel);

    while (status == G_IO_STATUS_NORMAL
	   && (max_count == -1 || total_read < (gsize)max_count)) {
	gsize single_read;
	char* buf;
	gsize buf_size;
	
	if (max_count == -1) 
	    buf_size = CHUNK_SIZE;
	else {
	    buf_size = max_count - total_read;
	    if (buf_size > CHUNK_SIZE)
		buf_size = CHUNK_SIZE;
        }
	
	if ( ret_obj == NULL ) {
	    ret_obj = PYGLIB_PyBytes_FromStringAndSize((char *)NULL, buf_size);
	    if (ret_obj == NULL)
		goto failure;
	}
	else if (buf_size + total_read > (gsize)PYGLIB_PyBytes_Size(ret_obj)) {
	    if (PYGLIB_PyBytes_Resize(&ret_obj, buf_size + total_read) == -1)
		goto failure;
	}
       
        buf = PYGLIB_PyBytes_AsString(ret_obj) + total_read;

        Py_BEGIN_ALLOW_THREADS;
        status = g_io_channel_read_chars (iochannel, buf, buf_size, &single_read, &error);
        Py_END_ALLOW_THREADS;

        if (pygi_error_check (&error))
	    goto failure;
	
	total_read += single_read;
    }
	
    if ( total_read != (gsize)PYGLIB_PyBytes_Size(ret_obj) ) {
	if (PYGLIB_PyBytes_Resize(&ret_obj, total_read) == -1)
	    goto failure;
    }

    return ret_obj;

  failure:
    Py_XDECREF(ret_obj);
    return NULL;
}

static PyMethodDef _gi_functions[] = {
    { "enum_add", (PyCFunction) _wrap_pyg_enum_add, METH_VARARGS | METH_KEYWORDS },
    { "enum_register_new_gtype_and_add", (PyCFunction) _wrap_pyg_enum_register_new_gtype_and_add, METH_VARARGS | METH_KEYWORDS },
    { "flags_add", (PyCFunction) _wrap_pyg_flags_add, METH_VARARGS | METH_KEYWORDS },
    { "flags_register_new_gtype_and_add", (PyCFunction) _wrap_pyg_flags_register_new_gtype_and_add, METH_VARARGS | METH_KEYWORDS },

    { "register_interface_info", (PyCFunction) _wrap_pyg_register_interface_info, METH_VARARGS },
    { "hook_up_vfunc_implementation", (PyCFunction) _wrap_pyg_hook_up_vfunc_implementation, METH_VARARGS },
    { "variant_type_from_string", (PyCFunction) _wrap_pyg_variant_type_from_string, METH_VARARGS },
    { "source_new", (PyCFunction) _wrap_pyg_source_new, METH_NOARGS },
    { "source_set_callback", (PyCFunction) pyg_source_set_callback, METH_VARARGS },
    { "io_channel_read", (PyCFunction) pyg_channel_read, METH_VARARGS },
    { "require_foreign", (PyCFunction) pygi_require_foreign, METH_VARARGS | METH_KEYWORDS },
    { "spawn_async",
      (PyCFunction)pyglib_spawn_async, METH_VARARGS|METH_KEYWORDS,
      "spawn_async(argv, envp=None, working_directory=None,\n"
      "            flags=0, child_setup=None, user_data=None,\n"
      "            standard_input=None, standard_output=None,\n"
      "            standard_error=None) -> (pid, stdin, stdout, stderr)\n"
      "\n"
      "Execute a child program asynchronously within a glib.MainLoop()\n"
      "See the reference manual for a complete reference.\n" },
    { "type_name", pyg_type_name, METH_VARARGS },
    { "type_from_name", pyg_type_from_name, METH_VARARGS },
    { "type_is_a", pyg_type_is_a, METH_VARARGS },
    { "type_register", _wrap_pyg_type_register, METH_VARARGS },
    { "signal_new", pyg_signal_new, METH_VARARGS },
    { "list_properties",
      pyg_object_class_list_properties, METH_VARARGS },
    { "new",
      (PyCFunction)pyg_object_new, METH_VARARGS|METH_KEYWORDS },
    { "signal_accumulator_true_handled",
      (PyCFunction)pyg_signal_accumulator_true_handled, METH_VARARGS },
    { "add_emission_hook",
      (PyCFunction)pyg_add_emission_hook, METH_VARARGS },
    { "_install_metaclass",
      (PyCFunction)pyg__install_metaclass, METH_O },
    { "_gvalue_get",
      (PyCFunction)pyg__gvalue_get, METH_O },
    { "_gvalue_set",
      (PyCFunction)pyg__gvalue_set, METH_VARARGS },
    { NULL, NULL, 0 }
};

static struct PyGI_API CAPI = {
  pygi_register_foreign_struct,
};

PYGLIB_MODULE_START(_gi, "_gi")
{
    PyObject *api;
    PyObject *module_dict = PyModule_GetDict (module);

    /* Always enable Python threads since we cannot predict which GI repositories
     * might accept Python callbacks run within non-Python threads or might trigger
     * toggle ref notifications.
     * See: https://bugzilla.gnome.org/show_bug.cgi?id=709223
     */
    PyEval_InitThreads ();

    PyModule_AddStringConstant(module, "__package__", "gi._gi");

    pygi_foreign_init ();
    pygi_error_register_types (module);
    _pygi_repository_register_types (module);
    _pygi_info_register_types (module);
    _pygi_struct_register_types (module);
    _pygi_boxed_register_types (module);
    _pygi_ccallback_register_types (module);
    pygi_resulttuple_register_types (module);

    pyglib_spawn_register_types (module_dict);
    pyglib_option_context_register_types (module_dict);
    pyglib_option_group_register_types (module_dict);

    pygobject_register_api (module_dict);
    pygobject_register_constants (module);
    pygobject_register_features (module_dict);
    pygobject_register_version_tuples (module_dict);
    pygobject_register_warnings (module_dict);
    pygobject_type_register_types (module_dict);
    pygobject_object_register_types (module_dict);
    pygobject_interface_register_types (module_dict);
    pygobject_paramspec_register_types (module_dict);
    pygobject_boxed_register_types (module_dict);
    pygobject_pointer_register_types (module_dict);
    pygobject_enum_register_types (module_dict);
    pygobject_flags_register_types (module_dict);

    PyGIWarning = PyErr_NewException ("gi.PyGIWarning", PyExc_Warning, NULL);

    /* Use RuntimeWarning as the base class of PyGIDeprecationWarning
     * for unstable (odd minor version) and use DeprecationWarning for
     * stable (even minor version). This is so PyGObject deprecations
     * behave the same as regular Python deprecations in stable releases.
     */
#if PYGOBJECT_MINOR_VERSION % 2
    PyGIDeprecationWarning = PyErr_NewException("gi.PyGIDeprecationWarning",
                                                PyExc_RuntimeWarning, NULL);
#else
    PyGIDeprecationWarning = PyErr_NewException("gi.PyGIDeprecationWarning",
                                                PyExc_DeprecationWarning, NULL);
#endif

    /* Place holder object used to fill in "from Python" argument lists
     * for values not supplied by the caller but support a GI default.
     */
    _PyGIDefaultArgPlaceholder = PyList_New(0);

    Py_INCREF (PyGIWarning);
    PyModule_AddObject (module, "PyGIWarning", PyGIWarning);

    Py_INCREF(PyGIDeprecationWarning);
    PyModule_AddObject(module, "PyGIDeprecationWarning", PyGIDeprecationWarning);

    api = PYGLIB_CPointer_WrapPointer ( (void *) &CAPI, "gi._API");
    if (api == NULL) {
        return PYGLIB_MODULE_ERROR_RETURN;
    }
    PyModule_AddObject (module, "_API", api);
}
PYGLIB_MODULE_END
