/* -*- Mode: C; c-basic-offset: 4 -*-
 * vim: tabstop=4 shiftwidth=4 expandtab
 *
 * Copyright (C) 2015 Christoph Reiter <reiter.christoph@gmail.com>
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

#ifndef __PYGI_ASYNC_H__
#define __PYGI_ASYNC_H__

#include "Python.h"

#include "pygi-info.h"
#include "pygi-cache.h"

typedef struct {
    PyObject *func;
    PyObject *context;
} PyGIAsyncCallback;

typedef struct {
    PyObject_HEAD

    /* Everything for the instance, finish_func is kept in the class. */
    PyGICallableInfo *finish_func;
    PyObject *loop;
    PyObject *cancellable;
    char _asyncio_future_blocking;
    PyObject *result;
    PyObject *exception;

    gboolean log_tb;

    GArray *callbacks;
} PyGIAsync;


int pygi_async_register_types (PyObject *d);

void pygi_async_finish_cb (GObject *source_object, gpointer res,
                           PyGIAsync *async);

PyObject *pygi_async_new (PyObject *async_finish, PyObject *cancellable);

#endif /* __PYGI_ASYNCRESULT_H__ */
