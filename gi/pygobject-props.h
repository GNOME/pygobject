#ifndef _PYGOBJECT_OBJECT_PROPS_H_
#define _PYGOBJECT_OBJECT_PROPS_H_

#include <pythoncapi_compat.h>


extern PyTypeObject PyGPropsIter_Type;
extern PyTypeObject PyGPropsDescr_Type;
extern PyTypeObject PyGProps_Type;

int pyg_object_props_register_types (PyObject *d);

#endif /*_PYGOBJECT_OBJECT_PROPS_H_*/
