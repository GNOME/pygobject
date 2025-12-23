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

#include "pygenum.h"
#include "pygflags.h"
#include "pygboxed.h"
#include "pygi-basictype.h"
#include "pygi-error.h"
#include "pygi-type.h"
#include "pygi-util.h"
#include "pygi-value.h"
#include "pyginterface.h"
#include "pygobject-object.h"
#include "pygoptiongroup.h"
#include "pygpointer.h"
#include "pygi-capi.h"


/**
 * pyg_register_interface:
 * @dict: a module dictionary.
 * @class_name: the class name for the wrapper class.
 * @gtype: the GType of the interface.
 * @type: the wrapper class for the interface.
 *
 * Registers a Python class as the wrapper for a GInterface.  As a
 * convenience it will also place a reference to the wrapper class in
 * the provided module dictionary.
 */
static void
pyg_register_interface (PyObject *dict, const gchar *class_name, GType gtype,
                        PyTypeObject *type)
{
    PyObject *o;

    Py_SET_TYPE (type, &PyType_Type);
    g_assert (Py_TYPE (&PyGInterface_Type) != NULL);
    type->tp_base = &PyGInterface_Type;

    if (PyType_Ready (type) < 0) {
        g_warning ("could not ready `%s'", type->tp_name);
        return;
    }

    if (gtype) {
        o = pyg_type_wrapper_new (gtype);
        PyDict_SetItemString (type->tp_dict, "__gtype__", o);
        Py_DECREF (o);
    }

    g_type_set_qdata (gtype, pyginterface_type_key, type);

    PyDict_SetItemString (dict, (char *)class_name, (PyObject *)type);
}

static void pyg_flags_add_constants (PyObject *module, GType flags_type,
                                     const gchar *strip_prefix);

/**
 * pyg_enum_add_constants:
 * @module: a Python module
 * @enum_type: the GType of the enumeration.
 * @strip_prefix: the prefix to strip from the constant names.
 *
 * Adds constants to the given Python module for each value name of
 * the enumeration.  A prefix will be stripped from each enum name.
 */
static void
pyg_enum_add_constants (PyObject *module, GType enum_type,
                        const gchar *strip_prefix)
{
    GEnumClass *eclass;
    guint i;

    if (!G_TYPE_IS_ENUM (enum_type)) {
        if (G_TYPE_IS_FLAGS (enum_type)) /* See bug #136204 */
            pyg_flags_add_constants (module, enum_type, strip_prefix);
        else
            g_warning ("`%s' is not an enum type", g_type_name (enum_type));
        return;
    }
    g_return_if_fail (strip_prefix != NULL);

    eclass = G_ENUM_CLASS (g_type_class_ref (enum_type));

    for (i = 0; i < eclass->n_values; i++) {
        const gchar *name = eclass->values[i].value_name;
        gint value = eclass->values[i].value;

        PyModule_AddIntConstant (
            module, (char *)pyg_constant_strip_prefix (name, strip_prefix),
            (long)value);
    }

    g_type_class_unref (eclass);
}

/**
 * pyg_flags_add_constants:
 * @module: a Python module
 * @flags_type: the GType of the flags type.
 * @strip_prefix: the prefix to strip from the constant names.
 *
 * Adds constants to the given Python module for each value name of
 * the flags set.  A prefix will be stripped from each flag name.
 */
static void
pyg_flags_add_constants (PyObject *module, GType flags_type,
                         const gchar *strip_prefix)
{
    GFlagsClass *fclass;
    guint i;

    if (!G_TYPE_IS_FLAGS (flags_type)) {
        if (G_TYPE_IS_ENUM (flags_type)) /* See bug #136204 */
            pyg_enum_add_constants (module, flags_type, strip_prefix);
        else
            g_warning ("`%s' is not an flags type", g_type_name (flags_type));
        return;
    }
    g_return_if_fail (strip_prefix != NULL);

    fclass = G_FLAGS_CLASS (g_type_class_ref (flags_type));

    for (i = 0; i < fclass->n_values; i++) {
        const gchar *name = fclass->values[i].value_name;
        guint value = fclass->values[i].value;

        PyModule_AddIntConstant (
            module, (char *)pyg_constant_strip_prefix (name, strip_prefix),
            (long)value);
    }

    g_type_class_unref (fclass);
}

G_GNUC_BEGIN_IGNORE_DEPRECATIONS

/**
 * pyg_parse_constructor_args: helper function for PyGObject constructors
 * @obj_type: GType of the GObject, for parameter introspection
 * @arg_names: %NULL-terminated array of constructor argument names
 * @prop_names: %NULL-terminated array of property names, with direct
 * correspondence to @arg_names
 * @params: GParameter array where parameters will be placed; length
 * of this array must be at least equal to the number of
 * arguments/properties
 * @nparams: output parameter to contain actual number of arguments found
 * @py_args: array of PyObject* containing the actual constructor arguments
 *
 * Parses an array of PyObject's and creates a GParameter array
 *
 * Return value: %TRUE if all is successful, otherwise %FALSE and
 * python exception set.
 **/
static gboolean
pyg_parse_constructor_args (GType obj_type, char **arg_names,
                            char **prop_names, GParameter *params,
                            guint *nparams, PyObject **py_args)
{
    guint arg_i, param_i;
    GObjectClass *oclass;

    oclass = g_type_class_ref (obj_type);
    g_return_val_if_fail (oclass, FALSE);

    for (param_i = arg_i = 0; arg_names[arg_i]; ++arg_i) {
        GParamSpec *spec;
        if (!py_args[arg_i]) continue;
        spec = g_object_class_find_property (oclass, prop_names[arg_i]);
        params[param_i].name = prop_names[arg_i];
        g_value_init (&params[param_i].value, spec->value_type);
        if (pyg_value_from_pyobject (&params[param_i].value, py_args[arg_i])
            == -1) {
            guint i;
            PyErr_Format (PyExc_TypeError,
                          "could not convert parameter '%s' of type '%s'",
                          arg_names[arg_i], g_type_name (spec->value_type));
            g_type_class_unref (oclass);
            for (i = 0; i < param_i; ++i) g_value_unset (&params[i].value);
            return FALSE;
        }
        ++param_i;
    }
    g_type_class_unref (oclass);
    *nparams = param_i;
    return TRUE;
}

G_GNUC_END_IGNORE_DEPRECATIONS


static PyObject *
pyg_param_gvalue_as_pyobject (const GValue *gvalue, gboolean copy_boxed,
                              const GParamSpec *pspec)
{
    if (G_IS_PARAM_SPEC_UNICHAR (pspec)) {
        return pygi_gunichar_to_py (g_value_get_uint (gvalue));
    } else {
        return pyg_value_to_pyobject (gvalue, copy_boxed);
    }
}

/* Only for backwards compatibility */
static int
pygobject_enable_threads (void)
{
    return 0;
}

static int
pygobject_gil_state_ensure (void)
{
    return (int)PyGILState_Ensure ();
}

static void
pygobject_gil_state_release (int flag)
{
    PyGILState_Release ((PyGILState_STATE)flag);
}


static void
pyg_register_class_init (GType gtype, PyGClassInitFunc class_init)
{
    GSList *list;

    list = g_type_get_qdata (gtype, pygobject_class_init_key);
    list = g_slist_prepend (list, class_init);
    g_type_set_qdata (gtype, pygobject_class_init_key, list);
}


static GHashTable *log_handlers = NULL;
static gboolean log_handlers_disabled = FALSE;


static void
remove_handler (gpointer domain, gpointer handler, gpointer unused)
{
    g_log_remove_handler (domain, GPOINTER_TO_UINT (handler));
}

static void
_log_func (const gchar *log_domain, GLogLevelFlags log_level,
           const gchar *message, gpointer user_data)
{
    if (G_LIKELY (Py_IsInitialized ())) {
        PyGILState_STATE state;
        PyObject *warning = user_data;

        state = PyGILState_Ensure ();
        PyErr_Warn (warning, (char *)message);
        PyGILState_Release (state);
    } else
        g_log_default_handler (log_domain, log_level, message, user_data);
}

static void
add_warning_redirection (const char *domain, PyObject *warning)
{
    g_return_if_fail (domain != NULL);
    g_return_if_fail (warning != NULL);

    if (!log_handlers_disabled) {
        guint handler;
        gpointer old_handler;

        if (!log_handlers)
            log_handlers =
                g_hash_table_new_full (g_str_hash, g_str_equal, g_free, NULL);

        if ((old_handler = g_hash_table_lookup (log_handlers, domain)))
            g_log_remove_handler (domain, GPOINTER_TO_UINT (old_handler));

        handler = g_log_set_handler (
            domain, G_LOG_LEVEL_CRITICAL | G_LOG_LEVEL_WARNING, _log_func,
            warning);
        g_hash_table_insert (log_handlers, g_strdup (domain),
                             GUINT_TO_POINTER (handler));
    }
}

static void
disable_warning_redirections (void)
{
    log_handlers_disabled = TRUE;

    if (log_handlers) {
        g_hash_table_foreach (log_handlers, remove_handler, NULL);
        g_hash_table_destroy (log_handlers);
        log_handlers = NULL;
    }
}

/**
 * pyg_set_thread_block_funcs:
 * Deprecated, only available for ABI compatibility.
 */
static void
_pyg_set_thread_block_funcs (PyGThreadBlockFunc block_threads_func,
                             PyGThreadBlockFunc unblock_threads_func)
{
    PyGILState_STATE state = PyGILState_Ensure ();
    PyErr_Warn (PyExc_DeprecationWarning,
                "Using pyg_set_thread_block_funcs is not longer needed. "
                "PyGObject always uses Py_BLOCK/UNBLOCK_THREADS.");
    PyGILState_Release (state);
}

static PyObject *
pyg_param_spec_new (GParamSpec *pspec)
{
    PyErr_SetString (PyExc_NotImplementedError,
                     "Creating ParamSpecs through pyg_param_spec_new is no "
                     "longer supported");
    return NULL;
}

static GParamSpec *
pyg_param_spec_from_object (PyObject *tuple)
{
    PyErr_SetString (PyExc_NotImplementedError,
                     "Creating ParamSpecs through pyg_param_spec_from_object "
                     "is no longer supported");
    return NULL;
}


/**
 * pyg_pyobj_to_unichar_conv:
 *
 * Converts PyObject value to a unichar and write result to memory
 * pointed to by ptr.  Follows the calling convention of a ParseArgs
 * converter (O& format specifier) so it may be used to convert function
 * arguments.
 *
 * Returns: 1 if the conversion succeeds and 0 otherwise.  If the conversion
 *          did not succeesd, a Python exception is raised
 */
static int
pyg_pyobj_to_unichar_conv (PyObject *py_obj, void *ptr)
{
    if (!pygi_gunichar_from_py (py_obj, ptr)) return 0;
    return 1;
}

/**
 * pyg_closure_set_exception_handler:
 * @closure: a closure created with pyg_closure_new()
 * @handler: the handler to call when an exception occurs or NULL for none
 *
 * Sets the handler to call when an exception occurs during closure invocation.
 * The handler is responsible for providing a proper return value to the
 * closure invocation. If @handler is %NULL, the default handler will be used.
 * The default handler prints the exception to stderr and doesn't touch the
 * closure's return value.
 */
static void
pyg_closure_set_exception_handler (GClosure *closure,
                                   PyClosureExceptionHandler handler)
{
    PyGClosure *pygclosure;

    g_return_if_fail (closure != NULL);

    pygclosure = (PyGClosure *)closure;
    pygclosure->exception_handler = handler;
}
const gpointer PyGParamSpec_Type_Stub = NULL;

struct _PyGObject_Functions pygobject_api_functions = {
    pygobject_register_class,
    pygobject_register_wrapper,
    pygobject_lookup_class,
    pygobject_new,

    pyg_closure_new,
    pygobject_watch_closure,
    pyg_destroy_notify,

    pyg_type_from_object,
    pyg_type_wrapper_new,
    pyg_enum_get_value,
    pyg_flags_get_value,
    pyg_register_gtype_custom,
    pyg_value_from_pyobject,
    pyg_value_to_pyobject,

    pyg_register_interface,

    &PyGBoxed_Type,
    pygi_register_gboxed,
    pygi_gboxed_new,

    &PyGPointer_Type,
    pyg_register_pointer,
    pyg_pointer_new,

    pyg_enum_add_constants,
    pyg_flags_add_constants,

    pyg_constant_strip_prefix,

    pygi_error_check,

    _pyg_set_thread_block_funcs,
    (PyGThreadBlockFunc)0, /* block_threads */
    (PyGThreadBlockFunc)0, /* unblock_threads */

    (PyTypeObject *)&PyGParamSpec_Type_Stub,
    pyg_param_spec_new,
    pyg_param_spec_from_object,

    pyg_pyobj_to_unichar_conv,
    pyg_parse_constructor_args,
    pyg_param_gvalue_as_pyobject,
    pyg_param_gvalue_from_pyobject,

    NULL /* PyGEnum_Type */,
    pyg_enum_add,
    pyg_enum_from_gtype,

    NULL /* PyGFlags_Type */,
    pyg_flags_add,
    pyg_flags_from_gtype,

    TRUE, /* threads_enabled */

    pygobject_enable_threads,
    pygobject_gil_state_ensure,
    pygobject_gil_state_release,
    pyg_register_class_init,
    pyg_register_interface_info,

    pyg_closure_set_exception_handler,

    add_warning_redirection,
    disable_warning_redirections,

    NULL, /* previously type_register_custom */

    pygi_gerror_exception_check,

    pyg_option_group_new,
    pyg_type_from_object_strict,

    pygobject_new_full,
    &PyGObject_Type,

    pyg_value_from_pyobject_with_error
};

/**
 * Returns 0 on success, or -1 and sets an exception.
 */
int
pygi_register_warnings (PyObject *d)
{
    PyObject *warning;

    warning = PyErr_NewException ("gobject.Warning", PyExc_Warning, NULL);
    if (warning == NULL) return -1;
    PyDict_SetItemString (d, "Warning", warning);
    add_warning_redirection ("GLib", warning);
    add_warning_redirection ("GLib-GObject", warning);
    add_warning_redirection ("GThread", warning);

    return 0;
}

/**
 * Returns 0 on success, or -1 and sets an exception.
 */
int
pygi_register_api (PyObject *d)
{
    PyObject *api;

    pygobject_api_functions.enum_type = PyGEnum_Type;
    pygobject_api_functions.flags_type = PyGFlags_Type;
    api = PyCapsule_New (&pygobject_api_functions, "gobject._PyGObject_API",
                         NULL);
    if (api == NULL) return -1;
    PyDict_SetItemString (d, "_PyGObject_API", api);
    Py_DECREF (api);
    return 0;
}
