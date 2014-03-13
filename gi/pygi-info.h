/* -*- Mode: C; c-basic-offset: 4 -*-
 * vim: tabstop=4 shiftwidth=4 expandtab
 *
 * Copyright (C) 2005-2009 Johan Dahlin <johan@gnome.org>
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

#ifndef __PYGI_INFO_H__
#define __PYGI_INFO_H__

#include <Python.h>

#include <girepository.h>

G_BEGIN_DECLS

gboolean pygi_g_struct_info_is_simple (GIStructInfo *struct_info);


/* Private */

extern PyTypeObject PyGIBaseInfo_Type;
extern PyTypeObject PyGICallableInfo_Type;
extern PyTypeObject PyGICallbackInfo_Type;
extern PyTypeObject PyGIFunctionInfo_Type;
extern PyTypeObject PyGIRegisteredTypeInfo_Type;
extern PyTypeObject PyGIStructInfo_Type;
extern PyTypeObject PyGIEnumInfo_Type;
extern PyTypeObject PyGIObjectInfo_Type;
extern PyTypeObject PyGIInterfaceInfo_Type;
extern PyTypeObject PyGIConstantInfo_Type;
extern PyTypeObject PyGIValueInfo_Type;
extern PyTypeObject PyGIFieldInfo_Type;
extern PyTypeObject PyGIUnresolvedInfo_Type;
extern PyTypeObject PyGIVFuncInfo_Type;
extern PyTypeObject PyGIUnionInfo_Type;
extern PyTypeObject PyGIBoxedInfo_Type;
extern PyTypeObject PyGIErrorDomainInfo_Type;
extern PyTypeObject PyGISignalInfo_Type;
extern PyTypeObject PyGIPropertyInfo_Type;
extern PyTypeObject PyGIArgInfo_Type;
extern PyTypeObject PyGITypeInfo_Type;

#define PyGIBaseInfo_GET_GI_INFO(object) g_base_info_ref(((PyGIBaseInfo *)object)->info)

PyObject* _pygi_info_new (GIBaseInfo *info);
GIBaseInfo* _pygi_object_get_gi_info (PyObject     *object,
                                      PyTypeObject *type);

gchar* _pygi_g_base_info_get_fullname (GIBaseInfo *info);

gsize _pygi_g_type_tag_size (GITypeTag type_tag);
gsize _pygi_g_type_info_size (GITypeInfo *type_info);

void _pygi_info_register_types (PyObject *m);

gboolean _pygi_is_python_keyword (const gchar *name);

G_END_DECLS

#endif /* __PYGI_INFO_H__ */
