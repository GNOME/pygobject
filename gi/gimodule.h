#ifndef _PYGOBJECT_GIMODULE_H_
#define _PYGOBJECT_GIMODULE_H_

#include "pygobject-internal.h"

int           pygobject_constructv (PyGObject   *self,
                                    guint        n_parameters,
                                    GParameter  *parameters);

#endif /*_PYGOBJECT_GIMODULE_H_*/
