#ifndef __PYGI_PRIVATE_H__
#define __PYGI_PRIVATE_H__

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <Python.h>

#include "pygi.h"

#include "pygirepository.h"
#include "pygiinfo.h"
#include "pygargument.h"

extern PyTypeObject PyGIRepository_Type;

extern PyTypeObject PyGIBaseInfo_Type;
extern PyTypeObject PyGICallableInfo_Type;
extern PyTypeObject PyGIFunctionInfo_Type;
extern PyTypeObject PyGICallbackInfo_Type;
extern PyTypeObject PyGIRegisteredTypeInfo_Type;
extern PyTypeObject PyGIStructInfo_Type;
extern PyTypeObject PyGIUnionInfo_Type;
extern PyTypeObject PyGIEnumInfo_Type;
extern PyTypeObject PyGIObjectInfo_Type;
extern PyTypeObject PyGIBoxedInfo_Type;
extern PyTypeObject PyGIInterfaceInfo_Type;
extern PyTypeObject PyGIConstantInfo_Type;
extern PyTypeObject PyGIValueInfo_Type;
extern PyTypeObject PyGISignalInfo_Type;
extern PyTypeObject PyGIVFuncInfo_Type;
extern PyTypeObject PyGIPropertyInfo_Type;
extern PyTypeObject PyGIFieldInfo_Type;
extern PyTypeObject PyGIArgInfo_Type;
extern PyTypeObject PyGITypeInfo_Type;
#if 0
extern PyTypeObject PyGIErrorDomainInfo_Type;
#endif
extern PyTypeObject PyGIUnresolvedInfo_Type;

#endif /* __PYGI_PRIVATE_H__ */
