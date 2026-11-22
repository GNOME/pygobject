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

#pragma once

#include <pythoncapi_compat.h>
#include <glib-object.h>

G_BEGIN_DECLS

gboolean pyg_gpointer_from_py (PyObject *py_arg, gpointer *result);

PyObject *pyg_gint8_to_py (gint8 value);
PyObject *pyg_guint8_to_py (guint8 value);
PyObject *pyg_gint16_to_py (gint16 value);
PyObject *pyg_guint16_to_py (guint16 value);
PyObject *pyg_gint32_to_py (gint32 value);
PyObject *pyg_guint32_to_py (guint32 value);
PyObject *pyg_gint64_to_py (gint64 value);
PyObject *pyg_guint64_to_py (guint64 value);
PyObject *pyg_gfloat_to_py (gfloat value);
PyObject *pyg_gdouble_to_py (gdouble value);
PyObject *pyg_gboolean_to_py (gboolean value);
PyObject *pyg_utf8_to_py (gchar *value);
PyObject *pyg_gint_to_py (gint value);
PyObject *pyg_glong_to_py (glong value);
PyObject *pyg_guint_to_py (guint value);
PyObject *pyg_gulong_to_py (gulong value);
PyObject *pyg_filename_to_py (gchar *value);
PyObject *pyg_gsize_to_py (gsize value);
PyObject *pyg_gssize_to_py (gssize value);
PyObject *pyg_gunichar_to_py (gunichar value);

gboolean pyg_gboolean_from_py (PyObject *object, gboolean *result);
gboolean pyg_gint8_from_py (PyObject *object, gint8 *result);
gboolean pyg_guint8_from_py (PyObject *object, guint8 *result);
gboolean pyg_gint16_from_py (PyObject *object, gint16 *result);
gboolean pyg_guint16_from_py (PyObject *object, guint16 *result);
gboolean pyg_gint32_from_py (PyObject *object, gint32 *result);
gboolean pyg_guint32_from_py (PyObject *object, guint32 *result);
gboolean pyg_gint64_from_py (PyObject *object, gint64 *result);
gboolean pyg_guint64_from_py (PyObject *object, guint64 *result);
gboolean pyg_gfloat_from_py (PyObject *py_arg, gfloat *result);
gboolean pyg_gdouble_from_py (PyObject *py_arg, gdouble *result);
gboolean pyg_utf8_from_py (PyObject *py_arg, gchar **result);
gboolean pyg_gint_from_py (PyObject *object, gint *result);
gboolean pyg_glong_from_py (PyObject *object, glong *result);
gboolean pyg_guint_from_py (PyObject *object, guint *result);
gboolean pyg_gulong_from_py (PyObject *object, gulong *result);
gboolean pyg_filename_from_py (PyObject *py_arg, gchar **result);
gboolean pyg_gunichar_from_py (PyObject *py_arg, gunichar *result);
gboolean pyg_gschar_from_py (PyObject *object, gint8 *result);
gboolean pyg_guchar_from_py (PyObject *object, guchar *result);
gboolean pyg_gtype_from_py (PyObject *py_arg, GType *type);

G_END_DECLS
