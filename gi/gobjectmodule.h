#ifndef _PYGOBJECT_GOBJECTMODULE_H_
#define _PYGOBJECT_GOBJECTMODULE_H_


#include "pygobject-internal.h"

int           pygobject_constructv (PyGObject   *self,
                                    guint        n_parameters,
                                    GParameter  *parameters);

void        pygobject_register_api              (PyObject *d);
void        pygobject_register_constants        (PyObject *m);
void        pygobject_register_features         (PyObject *d);
void        pygobject_register_version_tuples   (PyObject *d);
void        pygobject_register_warnings         (PyObject *d);

PyObject *  pyg_type_name                       (PyObject *self, PyObject *args);
PyObject *  pyg_type_from_name                  (PyObject *self, PyObject *args);
PyObject *  pyg_type_is_a                       (PyObject *self, PyObject *args);
PyObject *  _wrap_pyg_type_register             (PyObject *self, PyObject *args);
PyObject *  pyg_signal_new                      (PyObject *self, PyObject *args);
PyObject *  pyg_object_class_list_properties    (PyObject *self, PyObject *args);
PyObject *  pyg_object_new                      (PyGObject *self, PyObject *args,
                                                 PyObject *kwargs);
PyObject *  pyg_signal_accumulator_true_handled (PyObject *unused, PyObject *args);
PyObject *  pyg_add_emission_hook               (PyGObject *self, PyObject *args);
PyObject *  pyg__install_metaclass              (PyObject *dummy,
                                                 PyTypeObject *metaclass);
PyObject *  pyg__gvalue_get                     (PyObject *module, PyObject *pygvalue);
PyObject *  pyg__gvalue_set                     (PyObject *module, PyObject *args);

#endif /*_PYGOBJECT_GOBJECTMODULE_H_*/
