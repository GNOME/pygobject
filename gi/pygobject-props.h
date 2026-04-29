#pragma once

#include <glib.h>
#include <pythoncapi_compat.h>

G_BEGIN_DECLS

extern PyTypeObject PyGPropsIter_Type;
extern PyTypeObject PyGPropsDescr_Type;
extern PyTypeObject PyGProps_Type;

int pyg_object_props_register_types (PyObject *d);

G_END_DECLS
