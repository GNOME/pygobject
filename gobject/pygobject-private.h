#ifndef _PYGOBJECT_PRIVATE_H_
#define _PYGOBJECT_PRIVATE_H_

#ifdef _PYGOBJECT_H_
#  error "include pygobject.h or pygobject-private.h, but not both"
#endif

#define _INSIDE_PYGOBJECT_
#include "pygobject.h"


/* from gobjectmodule.c */
extern struct _PyGObject_Functions pygobject_api_functions;
#define pyg_block_threads()   G_STMT_START { \
    if (pygobject_api_functions.block_threads != NULL)    \
      (* pygobject_api_functions.block_threads)();        \
  } G_STMT_END
#define pyg_unblock_threads() G_STMT_START { \
    if (pygobject_api_functions.unblock_threads != NULL)  \
      (* pygobject_api_functions.unblock_threads)();      \
  } G_STMT_END

GType PY_TYPE_OBJECT;

void  pyg_destroy_notify (gpointer user_data);

/* from pygtype.h */
extern PyTypeObject PyGTypeWrapper_Type;

PyObject *pyg_type_wrapper_new (GType type);
GType     pyg_type_from_object (PyObject *obj);

gint pyg_enum_get_value  (GType enum_type, PyObject *obj, gint *val);
gint pyg_flags_get_value (GType flag_type, PyObject *obj, gint *val);

typedef PyObject *(* fromvaluefunc)(const GValue *value);
typedef int (*tovaluefunc)(GValue *value, PyObject *obj);

void      pyg_register_boxed_custom(GType boxed_type,
				    fromvaluefunc from_func,
				    tovaluefunc to_func);
int       pyg_value_from_pyobject(GValue *value, PyObject *obj);
PyObject *pyg_value_as_pyobject(const GValue *value);

GClosure *pyg_closure_new(PyObject *callback, PyObject *extra_args, PyObject *swap_data);
GClosure *pyg_signal_class_closure_get(void);

PyObject *pyg_object_descr_doc_get(void);


/* from pygobject.h */
extern PyTypeObject PyGObject_Type;

void          pygobject_register_class   (PyObject *dict,
					  const gchar *type_name,
					  GType gtype, PyTypeObject *type,
					  PyObject *bases);
void          pygobject_register_wrapper (PyObject *self);
PyObject *    pygobject_new              (GObject *obj);
PyTypeObject *pygobject_lookup_class     (GType gtype);


/* from pygboxed.c */
extern PyTypeObject PyGBoxed_Type;

void       pyg_register_boxed (PyObject *dict, const gchar *class_name,
			       GType boxed_type, PyTypeObject *type);
PyObject * pyg_boxed_new      (GType boxed_type, gpointer boxed,
			       gboolean copy_boxed, gboolean own_ref);


#endif
