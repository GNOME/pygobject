/* -*- Mode: C; c-basic-offset: 4 -*- */
#ifndef _PYGOBJECT_H_
#define _PYGOBJECT_H_

#include <Python.h>
#include <ExtensionClass.h>

#include <glib.h>
#include <glib-object.h>

typedef struct {
    PyObject_HEAD
    GObject *obj;
    gboolean hasref;     /* the GObject owns this reference */
    PyObject *inst_dict; /* the instance dictionary -- must be last */
} PyGObject;

#define pygobject_get(v) (((PyGObject *)v)->obj)
#define pygobject_check(v,base) (ExtensionClassSubclassInstance_Check(v,base))

struct _PyGObject_Functions {
    void (* register_class)(PyObject *dict, const gchar *class_name,
			    PyExtensionClass *ec, PyExtensionClass *parent);
    void (* register_wrapper)(PyObject *self);
    PyExtensionClass *(* lookup_class)(GType type);
    PyObject *(* new)(GObject *obj);
    gint (* enum_get_value)(GType enum_type, PyObject *obj, gint *val);
    gint (* flags_get_value)(GType flag_type, PyObject *obj, gint *val);
    void (* boxed_register)(GType boxed_type,
			    PyObject *(* from_func)(const GValue *value),
			    int (* to_func)(GValue *value, PyObject *obj));
    int (* value_from_pyobject)(GValue *value, PyObject *obj);
    PyObject *(* value_as_pyobject)(const GValue *value);
};

#ifndef _INSIDE_PYGOBJECT_

#if defined(NO_IMPORT) || defined(NO_IMPORT_PYGOBJECT)
extern struct _PyGObject_Functions *_PyGObject_API;
#else
struct _PyGObject_Functions *_PyGObject_API;
#endif

#define pygobject_register_class   (_PyGObject_API->register_class)
#define pygobject_register_wrapper (_PyGObject_API->register_wrapper)
#define pygobject_lookup_class     (_PyGObject_API->lookup_class)
#define pygobject_new              (_PyGObject_API->new)
#define pyg_enum_get_value         (_PyGObject_API->enum_get_value)
#define pyg_flags_get_value        (_PyGObject_API->flags_get_value)
#define pyg_boxed_register         (_PyGObject_API->boxed_register)
#define pyg_value_from_pyobject    (_PyGObject_API->value_from_pyobject)
#define pyg_value_as_pyobject      (_PyGObject_API->value_as_pyobject)

#define init_pygobject() { \
    PyObject *gobject = PyImport_ImportModule("gobject"); \
    if (gobject != NULL) { \
        PyObject *mdict = PyModule_GetDict(gobject); \
        PyObject *cobject = PyDict_GetItemString(mdict, "_PyGObject_API"); \
        if (PyCObject_Check(cobject)) \
            _PyGObject_API = PyCObject_AsVoidPtr(cobject); \
        else { \
	    Py_FatalError("could not find _PyGObject_API object"); \
	    return; \
        } \
    } else { \
        Py_FatalError("could not import gobject"); \
        return; \
    } \
    ExtensionClassImported; \
}

#endif /* !_INSIDE_PYGOBJECT_ */

#endif /* !_PYGOBJECT_H_ */
