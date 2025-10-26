#ifndef __PYGOBJECT_OBJECT_H__
#define __PYGOBJECT_OBJECT_H__

#include <glib-object.h>
#include <pythoncapi_compat.h>

#include "pygobject-types.h"

/* Data that belongs to the GObject instance, not the Python wrapper */
struct _PyGObjectData {
    PyTypeObject *type; /* wrapper type for this instance */
    GSList *closures;
};

extern GType PY_TYPE_OBJECT;
extern GQuark pygobject_instance_data_key;
extern GQuark pygobject_custom_key;
extern GQuark pygobject_wrapper_key;
extern GQuark pygobject_class_key;
extern GQuark pygobject_class_init_key;

extern PyTypeObject PyGObjectWeakRef_Type;
extern PyTypeObject PyGObject_Type;
extern PyTypeObject *PyGObject_MetaType;

static inline PyGObjectData *
pyg_object_peek_inst_data (GObject *obj)
{
    return ((PyGObjectData *)g_object_get_qdata (obj,
                                                 pygobject_instance_data_key));
}

void pygobject_register_class (PyObject *dict, const gchar *type_name,
                               GType gtype, PyTypeObject *type,
                               PyObject *bases);
void pygobject_register_wrapper (PyObject *self);
PyObject *pygobject_new (GObject *obj);
PyObject *pygobject_new_full (GObject *obj, gboolean steal, gpointer g_class);
PyTypeObject *pygobject_lookup_class (GType gtype);
void pygobject_watch_closure (PyObject *self, GClosure *closure);
int pyg_object_register_types (PyObject *d);
PyObject *pyg_object_new (PyGObject *self, PyObject *args, PyObject *kwargs);

void pygobject__g_instance_init (GTypeInstance *instance, gpointer g_class);

/* from pygobject-class.c */
void pygobject__g_class_init (GObjectClass *class, PyObject *py_class);

#endif /*__PYGOBJECT_OBJECT_H__*/
