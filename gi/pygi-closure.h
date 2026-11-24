/* -*- Mode: C; c-basic-offset: 4 -*-
 * vim: tabstop=4 shiftwidth=4 expandtab
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

GClosure *pyg_closure_new (PyObject *callback, PyObject *extra_args,
                           PyObject *swap_data);

GClosure *pygi_signal_closure_new (GType g_type, const gchar *sig_name,
                                   PyObject *callback, PyObject *extra_args,
                                   PyObject *swap_data);

GClosure *pyg_signal_class_closure_get (void);

G_END_DECLS
