/* -*- Mode: C; c-basic-offset: 4 -*-
 * vim: tabstop=4 shiftwidth=4 expandtab
 *
 * Copyright (C) 2014 Simon Feltman <sfeltman@gnome.org>
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

#ifndef __PYGI_ARG_BASICTYPE_H__
#define __PYGI_ARG_BASICTYPE_H__

#include <girepository/girepository.h>
#include "pygi-cache.h"

G_BEGIN_DECLS

gboolean pygi_marshal_from_py_basic_type (PyObject *object, /* in */
                                          GIArgument *arg,  /* out */
                                          GITypeTag type_tag,
                                          GITransfer transfer,
                                          gpointer *cleanup_data);
gboolean pygi_marshal_from_py_basic_type_cache_adapter (
    PyGIInvokeState *state, PyGICallableCache *callable_cache,
    PyGIArgCache *arg_cache, PyObject *py_arg, GIArgument *arg,
    gpointer *cleanup_data);

PyObject *pygi_marshal_to_py_basic_type (GIArgument *arg, /* in */
                                         GITypeTag type_tag,
                                         GITransfer transfer);
PyObject *pygi_marshal_to_py_basic_type_cache_adapter (
    PyGIInvokeState *state, PyGICallableCache *callable_cache,
    PyGIArgCache *arg_cache, GIArgument *arg, gpointer *cleanup_data);

PyGIArgCache *pygi_arg_basic_type_new_from_info (
    GITypeInfo *type_info, GIArgInfo *arg_info, /* may be null */
    GITransfer transfer, PyGIDirection direction);

PyObject *pygi_gint64_to_py (gint64 value);
PyObject *pygi_guint64_to_py (guint64 value);
PyObject *pygi_gfloat_to_py (gfloat value);
PyObject *pygi_gdouble_to_py (gdouble value);
PyObject *pygi_gboolean_to_py (gboolean value);
PyObject *pygi_gint8_to_py (gint8 value);
PyObject *pygi_guint8_to_py (guint8 value);
PyObject *pygi_utf8_to_py (gchar *value);
PyObject *pygi_gint_to_py (gint value);
PyObject *pygi_glong_to_py (glong value);
PyObject *pygi_guint_to_py (guint value);
PyObject *pygi_gulong_to_py (gulong value);
PyObject *pygi_filename_to_py (gchar *value);
PyObject *pygi_gsize_to_py (gsize value);
PyObject *pygi_gssize_to_py (gssize value);
PyObject *pygi_guint32_to_py (guint32 value);

gboolean pygi_gboolean_from_py (PyObject *object, gboolean *result);
gboolean pygi_gint64_from_py (PyObject *object, gint64 *result);
gboolean pygi_guint64_from_py (PyObject *object, guint64 *result);
gboolean pygi_gfloat_from_py (PyObject *py_arg, gfloat *result);
gboolean pygi_gdouble_from_py (PyObject *py_arg, gdouble *result);
gboolean pygi_utf8_from_py (PyObject *py_arg, gchar **result);
gboolean pygi_glong_from_py (PyObject *object, glong *result);
gboolean pygi_gulong_from_py (PyObject *object, gulong *result);
gboolean pygi_gint_from_py (PyObject *object, gint *result);
gboolean pygi_guint_from_py (PyObject *object, guint *result);
gboolean pygi_gunichar_from_py (PyObject *py_arg, gunichar *result);
gboolean pygi_gint8_from_py (PyObject *object, gint8 *result);
gboolean pygi_gschar_from_py (PyObject *object, gint8 *result);
gboolean pygi_guint8_from_py (PyObject *object, guint8 *result);
gboolean pygi_guchar_from_py (PyObject *object, guchar *result);

G_END_DECLS

#endif /*__PYGI_ARG_BASICTYPE_H__*/
