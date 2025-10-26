#ifndef __PYGOBJECT_OBJECT_PROPS_H__
#define __PYGOBJECT_OBJECT_PROPS_H__

#include <pythoncapi_compat.h>


extern PyTypeObject PyGPropsIter_Type;
extern PyTypeObject PyGPropsDescr_Type;
extern PyTypeObject PyGProps_Type;

int pyg_object_props_register_types (PyObject *d);

#endif /*__PYGOBJECT_OBJECT_PROPS_H__*/
