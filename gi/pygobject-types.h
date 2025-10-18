/* -*- Mode: C; c-basic-offset: 4 -*-
 * vim: tabstop=4 shiftwidth=4 expandtab
 *
 * Copyright (C) 2000 James Henstridge <james@daa.com.au>
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

#ifndef __PYGOBJECT_TYPES_H__
#define __PYGOBJECT_TYPES_H__

#include <glib.h>
#include <glib-object.h>
#include <Python.h>

G_BEGIN_DECLS

/* PyGClosure is a _private_ structure */
typedef void (*PyClosureExceptionHandler) (GValue *ret, guint n_param_values,
                                           const GValue *params);
typedef struct _PyGClosure PyGClosure;
typedef struct _PyGObjectData PyGObjectData;

struct _PyGClosure {
    GClosure closure;
    PyObject *callback;
    PyObject *extra_args; /* tuple of extra args to pass to callback */
    PyObject *swap_data;  /* other object for gtk_signal_connect__object */
    PyClosureExceptionHandler exception_handler;
};

typedef enum {
    PYGOBJECT_USING_TOGGLE_REF = 1 << 0, /* No longer used DO_NOT_USE! */
} PyGObjectFlags;

/* closures is just an alias for what is found in the
   * PyGObjectData */
typedef struct {
    PyObject_HEAD
    GObject *obj;
    PyObject *inst_dict;   /* the instance dictionary -- must be last */
    PyObject *weakreflist; /* list of weak references */

                           /*< private >*/
    /* using union to preserve ABI compatibility (structure size
       * must not change) */
    union {
        GSList *closures;     /* stale field; no longer updated DO-NOT-USE! */
        PyGObjectFlags flags; /* stale field; no longer updated DO-NOT-USE! */
    } private_flags;

} PyGObject;

#define pygobject_get(v)         (((PyGObject *)(v))->obj)
#define pygobject_check(v, base) (PyObject_TypeCheck (v, base))

typedef struct {
    PyObject_HEAD
    gpointer boxed;
    GType gtype;
    gboolean free_on_dealloc;
} PyGBoxed;

#define pyg_boxed_get(v, t)     ((t *)((PyGBoxed *)(v))->boxed)
#define pyg_boxed_get_ptr(v)    (((PyGBoxed *)(v))->boxed)
#define pyg_boxed_set_ptr(v, p) (((PyGBoxed *)(v))->boxed = (gpointer)p)
#define pyg_boxed_check(v, typecode)                                          \
    (PyObject_TypeCheck (v, &PyGBoxed_Type)                                   \
     && ((PyGBoxed *)(v))->gtype == (typecode))

typedef struct {
    PyObject_HEAD
    gpointer pointer;
    GType gtype;
} PyGPointer;

#define pyg_pointer_get(v, t)     ((t *)((PyGPointer *)(v))->pointer)
#define pyg_pointer_get_ptr(v)    (((PyGPointer *)(v))->pointer)
#define pyg_pointer_set_ptr(v, p) (((PyGPointer *)(v))->pointer = (gpointer)p)
#define pyg_pointer_check(v, typecode)                                        \
    (PyObject_TypeCheck (v, &PyGPointer_Type)                                 \
     && ((PyGPointer *)(v))->gtype == (typecode))

typedef void (*PyGFatalExceptionFunc) (void);
typedef void (*PyGThreadBlockFunc) (void);

typedef int (*PyGClassInitFunc) (gpointer gclass, PyTypeObject *pyclass);
typedef PyTypeObject *(*PyGTypeRegistrationFunction) (const gchar *name,
                                                      gpointer data);

struct _PyGObject_Functions {
    /*
     * All field names in here are considered private,
     * use the macros below instead, which provides stability
     */
    void (*register_class) (PyObject *dict, const gchar *class_name,
                            GType gtype, PyTypeObject *type, PyObject *bases);
    void (*register_wrapper) (PyObject *self);
    PyTypeObject *(*lookup_class) (GType type);
    PyObject *(*newgobj) (GObject *obj);

    GClosure *(*closure_new) (PyObject *callback, PyObject *extra_args,
                              PyObject *swap_data);
    void (*object_watch_closure) (PyObject *self, GClosure *closure);
    GDestroyNotify destroy_notify;

    GType (*type_from_object) (PyObject *obj);
    PyObject *(*type_wrapper_new) (GType type);

    gint (*enum_get_value) (GType enum_type, PyObject *obj, gint *val);
    gint (*flags_get_value) (GType flag_type, PyObject *obj, guint *val);
    void (*register_gtype_custom) (
        GType gtype, PyObject *(*from_func) (const GValue *value),
        int (*to_func) (GValue *value, PyObject *obj));
    int (*value_from_pyobject) (GValue *value, PyObject *obj);
    PyObject *(*value_as_pyobject) (const GValue *value, gboolean copy_boxed);

    void (*register_interface) (PyObject *dict, const gchar *class_name,
                                GType gtype, PyTypeObject *type);

    PyTypeObject *boxed_type;
    void (*register_boxed) (PyObject *dict, const gchar *class_name,
                            GType boxed_type, PyTypeObject *type);
    PyObject *(*boxed_new) (GType boxed_type, gpointer boxed,
                            gboolean copy_boxed, gboolean own_ref);

    PyTypeObject *pointer_type;
    void (*register_pointer) (PyObject *dict, const gchar *class_name,
                              GType pointer_type, PyTypeObject *type);
    PyObject *(*pointer_new) (GType boxed_type, gpointer pointer);

    void (*enum_add_constants) (PyObject *module, GType enum_type,
                                const gchar *strip_prefix);
    void (*flags_add_constants) (PyObject *module, GType flags_type,
                                 const gchar *strip_prefix);

    const gchar *(*constant_strip_prefix) (const gchar *name,
                                           const gchar *strip_prefix);

    gboolean (*error_check) (GError **error);

    /* hooks to register handlers for getting GDK threads to cooperate
     * with python threading */
    void (*set_thread_block_funcs) (PyGThreadBlockFunc block_threads_func,
                                    PyGThreadBlockFunc unblock_threads_func);
    PyGThreadBlockFunc block_threads;
    PyGThreadBlockFunc unblock_threads;

    PyTypeObject *paramspec_type;
    PyObject *(*paramspec_new) (GParamSpec *spec);
    GParamSpec *(*paramspec_get) (PyObject *tuple);
    int (*pyobj_to_unichar_conv) (PyObject *pyobj, void *ptr);
    G_GNUC_BEGIN_IGNORE_DEPRECATIONS
    gboolean (*parse_constructor_args) (GType obj_type, char **arg_names,
                                        char **prop_names, GParameter *params,
                                        guint *nparams, PyObject **py_args);
    G_GNUC_END_IGNORE_DEPRECATIONS
    PyObject *(*param_gvalue_as_pyobject) (const GValue *gvalue,
                                           gboolean copy_boxed,
                                           const GParamSpec *pspec);
    int (*gvalue_from_param_pyobject) (GValue *value, PyObject *py_obj,
                                       const GParamSpec *pspec);
    PyTypeObject *enum_type;
    PyObject *(*enum_add) (PyObject *module, const char *type_name_,
                           const char *strip_prefix, GType gtype);
    PyObject *(*enum_from_gtype) (GType gtype, int value);

    PyTypeObject *flags_type;
    PyObject *(*flags_add) (PyObject *module, const char *type_name_,
                            const char *strip_prefix, GType gtype);
    PyObject *(*flags_from_gtype) (GType gtype, guint value);

    gboolean threads_enabled;
    int (*enable_threads) (void);

    int (*gil_state_ensure) (void);
    void (*gil_state_release) (int flag);

    void (*register_class_init) (GType gtype, PyGClassInitFunc class_init);
    void (*register_interface_info) (GType gtype, const GInterfaceInfo *info);
    void (*closure_set_exception_handler) (GClosure *closure,
                                           PyClosureExceptionHandler handler);

    void (*add_warning_redirection) (const char *domain, PyObject *warning);
    void (*disable_warning_redirections) (void);

    /* type_register_custom API is removed, but left a pointer to not break ABI. */
    void *_type_register_custom;

    gboolean (*gerror_exception_check) (GError **error);

    /* option_group_new API is removed, but left a pointer to not break ABI. */
    void *_option_group_new;

    GType (*type_from_object_strict) (PyObject *obj, gboolean strict);

    PyObject *(*newgobj_full) (GObject *obj, gboolean steal, gpointer g_class);
    PyTypeObject *object_type;
    int (*value_from_pyobject_with_error) (GValue *value, PyObject *obj);
};


/* If PyGObject is statically linked, append PyInit__gi to Python's inittab. */
PyMODINIT_FUNC PyInit__gi (void);
PyMODINIT_FUNC PyInit__gi_cairo (void);


G_END_DECLS

#endif /* __PYGOBJECT_TYPES_H__ */
