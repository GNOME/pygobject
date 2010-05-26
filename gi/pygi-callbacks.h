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
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301
 * USA
 */

#ifndef __PYGI_CALLBACKS_H__
#define __PYGI_CALLBACKS_H__

G_BEGIN_DECLS

void _pygi_callback_notify_info_free (gpointer user_data);

PyGICClosure*_pygi_destroy_notify_create (void);

gboolean _pygi_scan_for_callbacks (GIFunctionInfo *self,
                                   gboolean       is_method,
                                   guint8        *callback_index,
                                   guint8        *user_data_index,
                                   guint8        *destroy_notify_index);

gboolean _pygi_create_callback (GIFunctionInfo *self,
                                gboolean       is_method,
                                gboolean       is_constructor,
                                int            n_args,
                                Py_ssize_t     py_argc,
                                PyObject      *py_argv,
                                guint8         callback_index,
                                guint8         user_data_index,
                                guint8         destroy_notify_index,
                                PyGICClosure **closure_out);

G_END_DECLS

#endif
